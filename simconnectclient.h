#ifndef SIMCONNECTCLIENT_H
#define SIMCONNECTCLIENT_H

#include <QObject>
#include <QSet>
#include <QString>
#include <QHash>

#include <windows.h>

#include "SimConnect.h"

class QTimer;
class QWinEventNotifier;

class SimConnectClient : public QObject
{
    Q_OBJECT

public:
    explicit SimConnectClient(QObject *parent = nullptr);
    ~SimConnectClient();

    bool isConnected() const;

public slots:
    void connectToSim();
    void disconnectFromSim();
    void enumerateInputEvents();
    void stopInputEventListening();

signals:
    void connected();
    void disconnected();
    void connectionLost();
    void flightStarted();
    void aircraftLoaded(const QString &file);
    void flightEnded();
    void inputEventEnumerated(const QString &name, quint64 hash, const QString &eType);
    void inputEventParamsEnumerated(quint64 hash, const QString &param, int size);
    void inputEventReceived(quint64 hash, const QString &eType, const QString &value,
                            const QString &param, int size);
    void simError(quint32 code);

private:
    void tryOpen();
    void closeConnection();
    void processMessages();
    void handleQuit();
    void handleException(DWORD code);
    void handleFlowEvent(const SIMCONNECT_RECV_FLOW_EVENT *event);
    void notifyAircraftLoaded(const char *file);
    static int inputEventParamSize(const QString &param);
    static void CALLBACK dispatchProc(SIMCONNECT_RECV *data, DWORD cbData, void *context);

    HANDLE handle = nullptr;
    HANDLE event = nullptr;
    QWinEventNotifier *notifier = nullptr;
    QTimer *retryTimer = nullptr;
    bool connecting = false;
    bool flightActive = false;
    bool inputEventListening = false;
    QHash<quint64, SIMCONNECT_INPUT_EVENT_TYPE> inputEventTypes;
    QHash<quint64, QPair<QString, int>> inputEventParams;
    QSet<quint64> inputEventParamRequests;
    QSet<quint64> subscribedInputEvents;
};

#endif // SIMCONNECTCLIENT_H
