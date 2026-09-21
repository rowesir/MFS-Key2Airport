#include "simconnectclient.h"

#include <QTimer>
#include <QWinEventNotifier>

namespace {

const int kRetryIntervalMs = 1000;
const SIMCONNECT_CLIENT_EVENT_ID kEventAircraftLoaded = 1;
const SIMCONNECT_CLIENT_EVENT_ID kEventSimRunning = 2;

}

SimConnectClient::SimConnectClient(QObject *parent) : QObject(parent)
{
}

SimConnectClient::~SimConnectClient()
{
    closeConnection();
}

bool SimConnectClient::isConnected() const
{
    return handle != nullptr;
}

void SimConnectClient::connectToSim()
{
    if (connecting)
        return;

    connecting = true;

    if (!event)
        event = CreateEventW(nullptr, FALSE, FALSE, nullptr);

    if (!retryTimer)
    {
        retryTimer = new QTimer(this);
        retryTimer->setInterval(kRetryIntervalMs);
        connect(retryTimer, &QTimer::timeout, this, &SimConnectClient::tryOpen);
    }

    if (!notifier && event)
    {
        notifier = new QWinEventNotifier(event, this);
        connect(notifier, &QWinEventNotifier::activated, this, &SimConnectClient::processMessages);
    }

    tryOpen();

    if (!handle)
        retryTimer->start();
}

void SimConnectClient::tryOpen()
{
    if (handle)
    {
        retryTimer->stop();
        return;
    }

    HANDLE opened = nullptr;
    const HRESULT result =
        SimConnect_Open(&opened, "Key2Airport", nullptr, 0, event, SIMCONNECT_OPEN_CONFIGINDEX_LOCAL);
    if (FAILED(result))
    {
        if (result != E_FAIL)
        {
            closeConnection();
            emit disconnected();
            emit simError(quint32(result));
        }
        return;
    }

    handle = opened;
    retryTimer->stop();

    SimConnect_SubscribeToSystemEvent(handle, kEventAircraftLoaded, "AircraftLoaded");
    SimConnect_SubscribeToSystemEvent(handle, kEventSimRunning, "Sim");

    emit connected();
}

void SimConnectClient::disconnectFromSim()
{
    closeConnection();

    emit disconnected();
}

void SimConnectClient::closeConnection()
{
    connecting = false;

    if (retryTimer)
        retryTimer->stop();

    if (handle)
    {
        SimConnect_Close(handle);
        handle = nullptr;
    }

    if (notifier)
    {
        delete notifier;
        notifier = nullptr;
    }

    if (event)
    {
        CloseHandle(event);
        event = nullptr;
    }
}

void SimConnectClient::processMessages()
{
    if (handle)
        SimConnect_CallDispatch(handle, dispatchProc, this);
}

void SimConnectClient::handleQuit()
{
    if (handle)
    {
        SimConnect_Close(handle);
        handle = nullptr;
    }

    if (event)
        ResetEvent(event);

    emit connectionLost();

    if (connecting && retryTimer)
        retryTimer->start();
}

void SimConnectClient::handleException(DWORD code)
{
    emit simError(quint32(code));
}

void SimConnectClient::notifyAircraftLoaded(const char *file)
{
    emit aircraftLoaded(QString::fromUtf8(file));
}

void CALLBACK SimConnectClient::dispatchProc(SIMCONNECT_RECV *data, DWORD cbData, void *context)
{
    Q_UNUSED(cbData)

    SimConnectClient *client = static_cast<SimConnectClient *>(context);

    switch (data->dwID)
    {
    case SIMCONNECT_RECV_ID_QUIT:
        client->handleQuit();
        break;
    case SIMCONNECT_RECV_ID_EXCEPTION:
        client->handleException(static_cast<SIMCONNECT_RECV_EXCEPTION *>(data)->dwException);
        break;
    case SIMCONNECT_RECV_ID_EVENT_FILENAME:
    {
        SIMCONNECT_RECV_EVENT_FILENAME *event = static_cast<SIMCONNECT_RECV_EVENT_FILENAME *>(data);
        if (event->uEventID == kEventAircraftLoaded)
            client->notifyAircraftLoaded(event->szFileName);
        break;
    }
    case SIMCONNECT_RECV_ID_EVENT:
    {
        SIMCONNECT_RECV_EVENT *event = static_cast<SIMCONNECT_RECV_EVENT *>(data);
        if (event->uEventID == kEventSimRunning && event->dwData == 0)
            emit client->flightEnded();
        break;
    }
    default:
        break;
    }
}
