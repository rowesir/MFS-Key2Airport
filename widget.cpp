#include "widget.h"
#include "ui_widget.h"

#include <QAbstractButton>
#include <QCloseEvent>
#include <QComboBox>
#include <QIcon>
#include <QMessageBox>
#include <QSoundEffect>
#include <QThread>
#include <QUrl>

#include <algorithm>
#include <cmath>

namespace {

const QList<int> kRadioHeightCalloutThresholds = {
    2500, 1000, 500, 300, 100, 50, 40, 30, 20, 10
};
const double kRadioHeightCalloutRearmHysteresisFeet = 5.0;

}

Widget::Widget(QWidget *parent)
    : QWidget(parent), ui(new Ui::Widget), dialogEnum(this), dialogTest(&dialogEnum)
{
    ui->setupUi(this);
    setWindowFlag(Qt::WindowStaysOnTopHint, true);

    for (const int threshold : kRadioHeightCalloutThresholds) {
        // Keep one independent effect per callout so every threshold can
        // start playing without interrupting any other threshold.
        auto *effect = new QSoundEffect(this);
        effect->setSource(QUrl(QStringLiteral("qrc:/sound/Resoure/altitude-%1.wav").arg(threshold)));
        effect->setLoopCount(1);
        effect->setVolume(1.0);
        radioHeightCalloutSounds.insert(threshold, effect);
        radioHeightCalloutPlayed.insert(threshold, false);
    }

    const HWND window = reinterpret_cast<HWND>(winId());

    inputThread         = new QThread(this);
    simThread           = new QThread(this);
    inputListener       = new InputListener;
    directInputListener = new DirectInputListener;
    simClient           = new SimConnectClient;
    inputListener->moveToThread(inputThread);
    directInputListener->moveToThread(inputThread);
    simClient->moveToThread(simThread);

    connect(inputThread, &QThread::finished, inputListener, &QObject::deleteLater);
    connect(inputThread, &QThread::finished, directInputListener, &QObject::deleteLater);
    connect(simThread, &QThread::finished, simClient, &QObject::deleteLater);

    connect(inputThread, &QThread::started, inputListener, [listener = inputListener]() { listener->start(); });
    connect(inputThread, &QThread::started, directInputListener,
            [listener = directInputListener, window]() { listener->start(window); });

    timerDetail = new QTimer(this);
    timerDetail->setSingleShot(true);
    timerDetail->setInterval(10000);
    connect(timerDetail, &QTimer::timeout, this, &Widget::onDetailTimeout);
    connect(inputListener, &InputListener::keyPressed, this, &Widget::onKeyPressed);
    connect(directInputListener, &DirectInputListener::keyPressed, this, &Widget::onKeyPressed);
    connect(simClient, &SimConnectClient::connected, this, &Widget::onSimConnected);
    connect(simClient, &SimConnectClient::disconnected, this, &Widget::onSimDisconnected);
    connect(simClient, &SimConnectClient::connectionLost, this, &Widget::onSimConnectionLost);
    connect(simClient, &SimConnectClient::flightStarted, this, &Widget::onFlightStarted);
    connect(simClient, &SimConnectClient::aircraftLoaded, this, &Widget::onAircraftLoaded);
    connect(simClient, &SimConnectClient::flightEnded, this, &Widget::onFlightEnded);
    connect(simClient, &SimConnectClient::radioHeightReceived,
            this, &Widget::onRadioHeightReceived);
    connect(simClient, &SimConnectClient::radioHeightUnavailable,
            this, &Widget::onRadioHeightUnavailable);
    connect(simClient, &SimConnectClient::landingRateReceived,
            this, &Widget::onLandingRateReceived);
    connect(simClient, &SimConnectClient::landingRateCleared,
            this, &Widget::onLandingRateCleared);
    connect(simClient, &SimConnectClient::landingRateUnavailable,
            this, &Widget::onLandingRateUnavailable);
    connect(simClient, &SimConnectClient::inputEventEnumerated, &dialogEnum, &DialogEnum::addEnumAll);
    connect(simClient, &SimConnectClient::inputEventParamsEnumerated, &dialogEnum, &DialogEnum::setEnumParam);
    connect(simClient, &SimConnectClient::inputEventReceived, &dialogEnum, &DialogEnum::addListen);
    connect(&dialogEnum, &DialogEnum::testRequested, this, &Widget::onDialogEnumTestRequested);
    connect(&dialogTest, &DialogTest::getRequested, simClient, &SimConnectClient::getInputEvent);
    connect(&dialogTest, &DialogTest::sendRequested, simClient, &SimConnectClient::sendInputEvent);
    connect(simClient, &SimConnectClient::inputEventValueReceived,
            &dialogTest, &DialogTest::setInputEventValue);
    connect(simClient, &SimConnectClient::inputEventValueUnavailable,
            &dialogTest, &DialogTest::showInputEventValueUnavailable);
    connect(simClient, &SimConnectClient::simError, this, &Widget::onSimError);

    initUI();
}

Widget::~Widget()
{
    if (inputThread)
    {
        inputThread->quit();
        inputThread->wait();
    }

    if (simThread)
    {
        if (simClient && simThread->isRunning()) {
            QMetaObject::invokeMethod(simClient, &SimConnectClient::disconnectFromSim,
                                      Qt::BlockingQueuedConnection);
        }
        simThread->quit();
        simThread->wait();
    }
    delete ui;
}

bool Widget::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(eventType)
    Q_UNUSED(result)

    const MSG *msg = static_cast<const MSG *>(message);
    if (msg->message == WM_DEVICECHANGE && directInputListener)
        QMetaObject::invokeMethod(directInputListener, &DirectInputListener::requestRefresh,
                                  Qt::QueuedConnection);

    return false;
}

void Widget::closeEvent(QCloseEvent *event)
{
    if (connected && flightActive && !confirmFlightExit(QStringLiteral("close the application")))
    {
        event->ignore();
        return;
    }

    resetRadioHeight();
    resetLandingRate();
    event->accept();
}

void Widget::initUI()
{
    ui->cbConfig->clear();
    ui->cbConfig->setEnabled(false);

    ui->ckbAuto->setChecked(true);

    setInfoText(QStringLiteral("Standby..."), QStringLiteral("#000000"));

    ui->pbtnEnum->setEnabled(false);
    ui->pbtnTest->setEnabled(false);

    ui->ckbRA->setChecked(true);
    ui->ckbLR->setChecked(true);

    ui->lcdRA->display(QStringLiteral("----"));
    ui->lcdLR->display(QStringLiteral("-----"));
    aircraftLoaded = false;

    ui->lbPage->clear();
    ui->lbDetail->clear();
    ui->lbDetail->setTextInteractionFlags(Qt::TextSelectableByMouse);

    inputThread->start();
    simThread->start();
}

void Widget::on_pbtnConnect_clicked()
{
    ControlGuard guard(this);

    if (!connected)
    {
        connected = true;

        guard.keepDisabled(ui->cbConfig);
        guard.keepDisabled(ui->ckbAuto);
        ui->pbtnEnum->setEnabled(false);
        ui->pbtnTest->setEnabled(false);

        setInfoText(QStringLiteral("Waiting MFS..."), QStringLiteral("#E68A00"));
        ui->pbtnConnect->setText(QStringLiteral("Disconn"));
        ui->pbtnConnect->setIcon(QIcon(QStringLiteral(":/Resoure/CoilRed.png")));

        QMetaObject::invokeMethod(simClient, &SimConnectClient::connectToSim, Qt::QueuedConnection);
    }
    else
    {
        if (flightActive && !confirmFlightExit(QStringLiteral("disconnect"))) return;

        guard.keepDisabled(ui->pbtnEnum);
        guard.keepDisabled(ui->pbtnTest);
        connected    = false;
        flightActive = false;

        resetConnectionUi();

        QMetaObject::invokeMethod(simClient, &SimConnectClient::disconnectFromSim, Qt::QueuedConnection);
    }
}

void Widget::on_pbtnEnum_clicked()
{
    {
        ControlGuard guard(this);

        dialogEnum.initUI();
        QMetaObject::invokeMethod(simClient, &SimConnectClient::enumerateInputEvents, Qt::QueuedConnection);
        dialogEnum.exec();
        if (dialogTest.isVisible())
            dialogTest.hide();
        QMetaObject::invokeMethod(simClient, &SimConnectClient::stopInputEventListening, Qt::QueuedConnection);
    }

    ui->pbtnEnum->setEnabled(flightActive);
}

void Widget::on_pbtnTest_clicked()
{
    {
        ControlGuard guard(this);
        dialogTest.exec();
    }

    ui->pbtnTest->setEnabled(flightActive);
}

void Widget::onDialogEnumTestRequested()
{
    if (!flightActive)
        return;

    dialogTest.setModal(false);
    dialogTest.setWindowModality(Qt::NonModal);
    dialogTest.show();
    dialogTest.raise();
    dialogTest.activateWindow();
}

void Widget::onRadioHeightReceived(double value)
{
    if (!flightActive || !aircraftLoaded || !ui->ckbRA->isChecked())
        return;

    if (!std::isfinite(value)) {
        resetRadioHeightCallouts();
        ui->lcdRA->display(QStringLiteral("----"));
        return;
    }

    updateRadioHeightCallouts(value);

    if (value > 2500.0) {
        ui->lcdRA->display(QStringLiteral("++++"));
        return;
    }

    // Simulate a radio altimeter display: integer feet, 5-foot resolution,
    // and round upward to the next 5-foot indication.
    const double displayValue = std::ceil(std::max(0.0, value) / 5.0) * 5.0;
    ui->lcdRA->display(displayValue);
}

void Widget::onRadioHeightUnavailable()
{
    resetRadioHeightCallouts();
    ui->lcdRA->display(QStringLiteral("----"));
}

void Widget::resetRadioHeightCallouts()
{
    hasPreviousRadioHeight = false;
    previousRadioHeight = 0.0;

    for (auto it = radioHeightCalloutPlayed.begin(); it != radioHeightCalloutPlayed.end(); ++it)
        it.value() = false;

    for (QSoundEffect *effect : radioHeightCalloutSounds) {
        if (effect)
            effect->stop();
    }
}

void Widget::updateRadioHeightCallouts(double value)
{
    if (!std::isfinite(value) || value < 0.0)
        return;

    if (!hasPreviousRadioHeight) {
        previousRadioHeight = value;
        hasPreviousRadioHeight = true;
        return;
    }

    for (const int threshold : kRadioHeightCalloutThresholds) {
        if (value > threshold + kRadioHeightCalloutRearmHysteresisFeet)
            radioHeightCalloutPlayed[threshold] = false;
    }

    if (value < previousRadioHeight) {
        for (const int threshold : kRadioHeightCalloutThresholds) {
            if (previousRadioHeight <= threshold || value > threshold ||
                radioHeightCalloutPlayed.value(threshold))
                continue;

            QSoundEffect *effect = radioHeightCalloutSounds.value(threshold, nullptr);
            if (effect)
                effect->play();
            radioHeightCalloutPlayed[threshold] = true;
        }
    }

    previousRadioHeight = value;
}

void Widget::onLandingRateReceived(double feetPerMinute)
{
    if (!flightActive || !aircraftLoaded || !ui->ckbLR->isChecked())
        return;

    if (!std::isfinite(feetPerMinute)) {
        ui->lcdLR->display(QStringLiteral("-----"));
        return;
    }

    // The SimVar is in feet per second.  Display the conventional integer
    // landing rate in feet per minute with a negative sign for descent,
    // regardless of the sign used by the aircraft/SimVar implementation.
    const qint64 landingRate = static_cast<qint64>(std::llround(std::fabs(feetPerMinute)));
    ui->lcdLR->display(QStringLiteral("-%1").arg(landingRate));
}

void Widget::onLandingRateCleared()
{
    if (ui->ckbLR->isChecked())
        ui->lcdLR->display(QStringLiteral("-----"));
}

void Widget::onLandingRateUnavailable()
{
    if (ui->ckbLR->isChecked())
        ui->lcdLR->display(QStringLiteral("-----"));
}

void Widget::resetRadioHeight()
{
    aircraftLoaded = false;
    resetRadioHeightCallouts();
    ui->lcdRA->display(QStringLiteral("----"));

    if (simClient && simThread && simThread->isRunning())
        QMetaObject::invokeMethod(simClient, &SimConnectClient::stopRadioHeightReading,
                                  Qt::QueuedConnection);
}

void Widget::resetLandingRate()
{
    ui->lcdLR->display(QStringLiteral("-----"));

    if (simClient && simThread && simThread->isRunning())
        QMetaObject::invokeMethod(simClient, &SimConnectClient::stopLandingRateReading,
                                  Qt::QueuedConnection);
}

void Widget::resetConnectionUi()
{
    resetRadioHeight();
    resetLandingRate();

    ui->ckbAuto->setEnabled(true);
    ui->pbtnEnum->setEnabled(false);
    ui->pbtnTest->setEnabled(false);

    setInfoText(QStringLiteral("Standby..."), QStringLiteral("#000000"));
    ui->pbtnConnect->setText(QStringLiteral("Connect"));
    ui->pbtnConnect->setIcon(QIcon(QStringLiteral(":/Resoure/CoilBalck.png")));
}

void Widget::setInfoText(const QString &text, const QString &color)
{
    ui->lbInfo->setText(text);
    ui->lbInfo->setStyleSheet(QStringLiteral("color: %1;").arg(color));
}

void Widget::onSimConnected()
{
    resetRadioHeight();
    resetLandingRate();

    ui->pbtnEnum->setEnabled(false);
    ui->pbtnTest->setEnabled(false);
    setInfoText(QStringLiteral("Connected"), QStringLiteral("#006400"));
}

void Widget::onSimDisconnected()
{
    if (dialogTest.isVisible())
        dialogTest.reject();

    connected    = false;
    flightActive = false;
    resetConnectionUi();
}

void Widget::onSimConnectionLost()
{
    if (dialogEnum.isVisible())
        dialogEnum.reject();
    if (dialogTest.isVisible())
        dialogTest.reject();

    resetRadioHeight();
    resetLandingRate();
    flightActive = false;
    ui->pbtnEnum->setEnabled(false);
    ui->pbtnTest->setEnabled(false);
    setInfoText(QStringLiteral("Waiting MFS..."), QStringLiteral("#E68A00"));
}

void Widget::onFlightStarted()
{
    const bool loadedBeforeFlightStart = aircraftLoaded;
    resetRadioHeight();
    resetLandingRate();
    flightActive = true;
    // AircraftLoaded may arrive before or after FLIGHT_START.  Preserve an
    // earlier load event, but otherwise wait for AircraftLoaded before
    // starting either radio-height or landing-rate monitoring.
    aircraftLoaded = loadedBeforeFlightStart;
    if (aircraftLoaded) {
        if (ui->ckbRA->isChecked())
            QMetaObject::invokeMethod(simClient, &SimConnectClient::startRadioHeightReading,
                                      Qt::QueuedConnection);
        if (ui->ckbLR->isChecked())
            QMetaObject::invokeMethod(simClient, &SimConnectClient::startLandingRateReading,
                                      Qt::QueuedConnection);
    }
    ui->pbtnEnum->setEnabled(true);
    ui->pbtnTest->setEnabled(true);
    setInfoText(QStringLiteral("Aircraft Loaded"), QStringLiteral("#006400"));
}

void Widget::onAircraftLoaded(const QString &file)
{
    Q_UNUSED(file)

    resetRadioHeight();
    resetLandingRate();
    aircraftLoaded = true;
    if (ui->ckbRA->isChecked())
        QMetaObject::invokeMethod(simClient, &SimConnectClient::startRadioHeightReading,
                                  Qt::QueuedConnection);
    if (ui->ckbLR->isChecked())
        QMetaObject::invokeMethod(simClient, &SimConnectClient::startLandingRateReading,
                                  Qt::QueuedConnection);

    setInfoText(QStringLiteral("Aircraft Loaded"), QStringLiteral("#006400"));
}

void Widget::onFlightEnded()
{
    if (dialogEnum.isVisible())
        dialogEnum.reject();
    if (dialogTest.isVisible())
        dialogTest.reject();

    resetRadioHeight();
    resetLandingRate();
    flightActive = false;
    ui->pbtnEnum->setEnabled(false);
    ui->pbtnTest->setEnabled(false);
    setInfoText(QStringLiteral("Connected"), QStringLiteral("#006400"));
}

bool Widget::confirmFlightExit(const QString &action)
{
    const QString message =
        QStringLiteral("You are currently in a flight.\n"
                       "If you %1 now, the next connection must be made before entering a flight.\n\n"
                       "Do you want to continue?")
            .arg(action);

    return QMessageBox::question(this, QStringLiteral("Confirm"), message, QMessageBox::Yes | QMessageBox::No,
                                 QMessageBox::No) == QMessageBox::Yes;
}

void Widget::onSimError(quint32 code)
{
    if (dialogEnum.isVisible())
        dialogEnum.reject();
    if (dialogTest.isVisible())
        dialogTest.reject();

    resetRadioHeight();
    resetLandingRate();
    ui->pbtnEnum->setEnabled(false);
    ui->pbtnTest->setEnabled(false);

    const QString hexCode =
        QStringLiteral("0x%1").arg(QString::number(code, 16).toUpper().rightJustified(8, QLatin1Char('0')));
    QMessageBox::critical(this, QStringLiteral("SimConnect Error"),
                          QStringLiteral("SimConnect reported an error.\nError code: %1 (%2).")
                              .arg(QString::number(code), hexCode));

    if (connected)
    {
        // The error handler performs an automatic disconnect; do not show the manual-exit confirmation.
        flightActive = false;
        on_pbtnConnect_clicked();
    }
    else
    {
        QMetaObject::invokeMethod(simClient, &SimConnectClient::disconnectFromSim, Qt::QueuedConnection);
    }
}

Widget::ControlGuard::ControlGuard(Widget *widget) : m_widget(widget)
{
    const QList<QWidget *> controls = m_widget->guardedControls();
    for (QWidget *control : controls)
    {
        m_states.append(qMakePair(control, control->isEnabled()));
        control->setEnabled(false);
    }
}

Widget::ControlGuard::~ControlGuard()
{
    for (const QPair<QWidget *, bool> &state : m_states)
    {
        if (m_keepDisabled.contains(state.first)) continue;
        if (state.first->isEnabled()) continue;
        state.first->setEnabled(state.second);
    }
}

void Widget::ControlGuard::keepDisabled(QWidget *control)
{
    m_keepDisabled.append(control);
    control->setEnabled(false);
}

QList<QWidget *> Widget::guardedControls() const
{
    QList<QWidget *> controls;

    const QList<QWidget *> children = findChildren<QWidget *>();
    for (QWidget *child : children)
    {
        if (child->window() != this) continue;
        if (qobject_cast<QAbstractButton *>(child) || qobject_cast<QComboBox *>(child)) controls.append(child);
    }

    return controls;
}

void Widget::onKeyPressed(const QString &name)
{
    ui->lbDetail->setText(name);
    timerDetail->start();
}

void Widget::onDetailTimeout() { ui->lbDetail->clear(); }

void Widget::on_ckbRA_clicked(bool checked)
{
    ControlGuard guard(this);

    ui->lcdRA->setEnabled(checked);
    resetRadioHeightCallouts();
    ui->lcdRA->display(QStringLiteral("----"));

    if (checked && aircraftLoaded && flightActive) {
        QMetaObject::invokeMethod(simClient, &SimConnectClient::startRadioHeightReading,
                                  Qt::QueuedConnection);
    } else {
        QMetaObject::invokeMethod(simClient, &SimConnectClient::stopRadioHeightReading,
                                  Qt::QueuedConnection);
    }
}

void Widget::on_ckbLR_clicked(bool checked)
{
    ControlGuard guard(this);

    ui->lcdLR->setEnabled(checked);
    ui->lcdLR->display(QStringLiteral("-----"));

    if (checked && aircraftLoaded && flightActive) {
        QMetaObject::invokeMethod(simClient, &SimConnectClient::startLandingRateReading,
                                  Qt::QueuedConnection);
    } else {
        QMetaObject::invokeMethod(simClient, &SimConnectClient::stopLandingRateReading,
                                  Qt::QueuedConnection);
    }
}
