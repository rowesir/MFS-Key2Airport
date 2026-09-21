#include "widget.h"
#include "ui_widget.h"

#include <QAbstractButton>
#include <QComboBox>
#include <QIcon>
#include <QThread>

Widget::Widget(QWidget *parent) : QWidget(parent), ui(new Ui::Widget), dialogEnum(this)
{
    ui->setupUi(this);

    const HWND window = reinterpret_cast<HWND>(winId());

    inputThread         = new QThread(this);
    simThread           = new QThread(this);
    inputListener       = new InputListener;
    directInputListener = new DirectInputListener;
    simClient           = new SimConnectClient;
    inputListener->moveToThread(inputThread);
    directInputListener->moveToThread(inputThread);
    simClient->moveToThread(simThread);

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
    connect(simClient, &SimConnectClient::aircraftLoaded, this, &Widget::onAircraftLoaded);
    connect(simClient, &SimConnectClient::flightEnded, this, &Widget::onFlightEnded);
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
        simThread->quit();
        simThread->wait();
    }

    delete inputListener;
    delete directInputListener;
    delete simClient;
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

void Widget::initUI()
{
    ui->cbConfig->clear();
    ui->cbConfig->setEnabled(false);

    ui->ckbAuto->setChecked(true);

    ui->lbInfo->setText(QStringLiteral("Standby..."));

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

        ui->lbInfo->setText(QStringLiteral("Waiting MFS..."));
        ui->pbtnConnect->setText(QStringLiteral("Disconn"));
        ui->pbtnConnect->setIcon(QIcon(QStringLiteral(":/Resoure/CoilRed.png")));

        QMetaObject::invokeMethod(simClient, &SimConnectClient::connectToSim, Qt::QueuedConnection);
    }
    else
    {
        connected = false;

        resetConnectionUi();

        QMetaObject::invokeMethod(simClient, &SimConnectClient::disconnectFromSim, Qt::QueuedConnection);
    }
}

void Widget::resetConnectionUi()
{
    ui->ckbAuto->setEnabled(true);

    ui->lbInfo->setText(QStringLiteral("Standby..."));
    ui->pbtnConnect->setText(QStringLiteral("Connect"));
    ui->pbtnConnect->setIcon(QIcon(QStringLiteral(":/Resoure/CoilBalck.png")));
}

void Widget::onSimConnected() { ui->lbInfo->setText(QStringLiteral("Connected")); }

void Widget::onSimDisconnected()
{
    connected = false;
    resetConnectionUi();
}

void Widget::onSimConnectionLost() { ui->lbInfo->setText(QStringLiteral("Waiting MFS...")); }

void Widget::onAircraftLoaded(const QString &file)
{
    Q_UNUSED(file)

    ui->lbInfo->setText(QStringLiteral("Aircraft Loaded"));
}

void Widget::onFlightEnded() { ui->lbInfo->setText(QStringLiteral("Connected")); }

void Widget::onSimError(quint32 code)
{
    Q_UNUSED(code)

    ui->lbInfo->setText(QStringLiteral("Sim Error"));
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
