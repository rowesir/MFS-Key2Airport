#ifndef SIMCONNECTCLIENT_H
#define SIMCONNECTCLIENT_H

#include <QObject>
#include <QString>

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

signals:
    void connected();
    void disconnected();
    void connectionLost();
    void aircraftLoaded(const QString &file);
    void flightEnded();
    void simError(quint32 code);

private:
    void tryOpen();
    void closeConnection();
    void processMessages();
    void handleQuit();
    void handleException(DWORD code);
    void notifyAircraftLoaded(const char *file);
    static void CALLBACK dispatchProc(SIMCONNECT_RECV *data, DWORD cbData, void *context);

    HANDLE handle = nullptr;
    HANDLE event = nullptr;
    QWinEventNotifier *notifier = nullptr;
    QTimer *retryTimer = nullptr;
    bool connecting = false;
};

#endif // SIMCONNECTCLIENT_H
