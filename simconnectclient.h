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
    void getInputEvent(quint64 hash);
    void sendInputEvent(quint64 hash, double value);
    void getInputEventForConfig(quint64 executionId, quint64 hash);
    void sendInputEventForConfig(quint64 executionId, quint64 hash, double value);
    void requestAircraftModel();
    void startRadioHeightReading();
    void stopRadioHeightReading();
    void startLandingRateReading();
    void stopLandingRateReading();

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
    void inputEventValueReceived(quint64 hash, double value);
    void inputEventValueUnavailable(quint64 hash);
    void configInputEventValueReceived(quint64 executionId, bool success, double value);
    void configInputEventSetFinished(quint64 executionId, bool success);
    void aircraftModelReceived(const QString &model);
    void aircraftModelUnavailable();
    void radioHeightReceived(double value);
    void radioHeightUnavailable();
    void landingRateReceived(double feetPerMinute);
    void landingRateCleared();
    void landingRateUnavailable();
    void simError(quint32 code);

private:
    void tryOpen();
    void closeConnection();
    void processMessages();
    void handleQuit();
    void handleException(DWORD code);
    void handleFlowEvent(const SIMCONNECT_RECV_FLOW_EVENT *event);
    void notifyAircraftLoaded(const char *file);
    bool isRadioHeightException(DWORD code) const;
    bool isInputEventException(DWORD code) const;
    void stopRadioHeightRequest();
    void stopLandingRateRequest();
    void resetLandingRateState();
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
    QHash<SIMCONNECT_DATA_REQUEST_ID, quint64> inputEventGetRequests;
    QHash<SIMCONNECT_DATA_REQUEST_ID, QPair<quint64, quint64>> configInputEventGetRequests;
    SIMCONNECT_DATA_REQUEST_ID nextInputEventRequestId = 2;
    quint64 configInputEventSetExecutionId = 0;
    bool configInputEventSetPending = false;
    bool aircraftModelRequested = false;
    bool aircraftModelDefinitionAdded = false;
    bool radioHeightRequested = false;
    bool landingRateRequested = false;
    bool radioHeightReading = false;
    bool radioHeightDefinitionAdded = false;
    bool radioHeightAwaitingFirstData = false;
    bool landingRateReading = false;
    bool landingRateDefinitionAdded = false;
    bool landingRateAwaitingFirstData = false;
    bool landingRateAirborne = false;
    bool landingRatePreviousOnGround = true;
    bool landingRateHasResult = false;
};

#endif // SIMCONNECTCLIENT_H
