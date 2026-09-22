#include "widget.h"
#include "ui_widget.h"

#include <QAbstractButton>
#include <QCloseEvent>
#include <QComboBox>
#include <QIcon>
#include <QMessageBox>
#include <QThread>

Widget::Widget(QWidget *parent) : QWidget(parent), ui(new Ui::Widget), dialogEnum(this)
{
    ui->setupUi(this);
    setWindowFlag(Qt::WindowStaysOnTopHint, true);

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
    connect(simClient, &SimConnectClient::inputEventEnumerated, &dialogEnum, &DialogEnum::addEnumAll);
    connect(simClient, &SimConnectClient::inputEventParamsEnumerated, &dialogEnum, &DialogEnum::setEnumParam);
    connect(simClient, &SimConnectClient::inputEventReceived, &dialogEnum, &DialogEnum::addListen);
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

    event->accept();
}

void Widget::initUI()
{
    ui->cbConfig->clear();
    ui->cbConfig->setEnabled(false);

    ui->ckbAuto->setChecked(true);

    setInfoText(QStringLiteral("Standby..."), QStringLiteral("#000000"));

    ui->pbtnEnum->setEnabled(false);

    ui->ckbRA->setChecked(true);
    ui->ckbLR->setChecked(true);

    ui->lcdRA->display(QStringLiteral("----"));
    ui->lcdLR->display(QStringLiteral("-----"));

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

        setInfoText(QStringLiteral("Waiting MFS..."), QStringLiteral("#E68A00"));
        ui->pbtnConnect->setText(QStringLiteral("Disconn"));
        ui->pbtnConnect->setIcon(QIcon(QStringLiteral(":/Resoure/CoilRed.png")));

        QMetaObject::invokeMethod(simClient, &SimConnectClient::connectToSim, Qt::QueuedConnection);
    }
    else
    {
        if (flightActive && !confirmFlightExit(QStringLiteral("disconnect"))) return;

        guard.keepDisabled(ui->pbtnEnum);
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
        QMetaObject::invokeMethod(simClient, &SimConnectClient::stopInputEventListening, Qt::QueuedConnection);
    }

    ui->pbtnEnum->setEnabled(flightActive);
}

void Widget::resetConnectionUi()
{
    ui->ckbAuto->setEnabled(true);
    ui->pbtnEnum->setEnabled(false);

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
    ui->pbtnEnum->setEnabled(false);
    setInfoText(QStringLiteral("Connected"), QStringLiteral("#006400"));
}

void Widget::onSimDisconnected()
{
    connected    = false;
    flightActive = false;
    resetConnectionUi();
}

void Widget::onSimConnectionLost()
{
    flightActive = false;
    ui->pbtnEnum->setEnabled(false);
    setInfoText(QStringLiteral("Waiting MFS..."), QStringLiteral("#E68A00"));
}

void Widget::onFlightStarted()
{
    flightActive = true;
    ui->pbtnEnum->setEnabled(true);
    setInfoText(QStringLiteral("Aircraft Loaded"), QStringLiteral("#006400"));
}

void Widget::onAircraftLoaded(const QString &file)
{
    Q_UNUSED(file)

    setInfoText(QStringLiteral("Aircraft Loaded"), QStringLiteral("#006400"));
}

void Widget::onFlightEnded()
{
    if (dialogEnum.isVisible())
        dialogEnum.reject();

    flightActive = false;
    ui->pbtnEnum->setEnabled(false);
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

    ui->pbtnEnum->setEnabled(false);

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
}

void Widget::on_ckbLR_clicked(bool checked)
{
    ControlGuard guard(this);

    ui->lcdLR->setEnabled(checked);
}
