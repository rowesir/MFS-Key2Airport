#include "simconnectclient.h"

#include <QTimer>
#include <QWinEventNotifier>
#include <QRegularExpression>
#include <QStringList>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace {

const int kRetryIntervalMs = 1000;
const SIMCONNECT_CLIENT_EVENT_ID kEventAircraftLoaded = 1;
const SIMCONNECT_DATA_REQUEST_ID kRequestEnumerateInputEvents = 1;
const SIMCONNECT_DATA_DEFINITION_ID kDefinitionRadioHeight = 1;
const SIMCONNECT_DATA_REQUEST_ID kRequestRadioHeight = 100;
const SIMCONNECT_DATA_DEFINITION_ID kDefinitionLandingRate = 2;
const SIMCONNECT_DATA_REQUEST_ID kRequestLandingRate = 101;
const DWORD kRadioHeightIntervalFrames = 5;
const double kLandingRateAirborneHeightFeet = 100.0;

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
    const HRESULT flowResult = SimConnect_SubscribeToFlowEvent(handle);
    if (FAILED(flowResult))
    {
        closeConnection();
        emit disconnected();
        emit simError(quint32(flowResult));
        return;
    }

    flightActive = false;

    emit connected();
}

void SimConnectClient::disconnectFromSim()
{
    closeConnection();

    emit disconnected();
}

void SimConnectClient::enumerateInputEvents()
{
    if (!handle)
        return;

    inputEventListening = true;
    for (const quint64 hash : subscribedInputEvents)
        SimConnect_UnsubscribeInputEvent(handle, hash);
    inputEventTypes.clear();
    inputEventParams.clear();
    inputEventParamRequests.clear();
    subscribedInputEvents.clear();
    inputEventGetRequests.clear();

    const HRESULT result = SimConnect_EnumerateInputEvents(handle, kRequestEnumerateInputEvents);
    if (FAILED(result)) {
        inputEventListening = false;
        emit simError(quint32(result));
    }
}

void SimConnectClient::stopInputEventListening()
{
    inputEventListening = false;

    if (handle) {
        for (const quint64 hash : subscribedInputEvents)
            SimConnect_UnsubscribeInputEvent(handle, hash);
    }

    inputEventTypes.clear();
    inputEventParams.clear();
    inputEventParamRequests.clear();
    subscribedInputEvents.clear();
    inputEventGetRequests.clear();
}

void SimConnectClient::getInputEvent(quint64 hash)
{
    if (!handle || !flightActive)
        return;

    SIMCONNECT_DATA_REQUEST_ID requestId = nextInputEventRequestId++;
    if (nextInputEventRequestId == 0)
        nextInputEventRequestId = 2;

    inputEventGetRequests.insert(requestId, hash);

    const HRESULT result = SimConnect_GetInputEvent(handle, requestId, UINT64(hash));
    if (FAILED(result)) {
        inputEventGetRequests.remove(requestId);
        emit simError(quint32(result));
    }
}

void SimConnectClient::sendInputEvent(quint64 hash, double value)
{
    if (!handle || !flightActive)
        return;

    const HRESULT result =
        SimConnect_SetInputEvent(handle, UINT64(hash), DWORD(sizeof(value)), &value);
    if (FAILED(result))
        emit simError(quint32(result));
}

void SimConnectClient::startRadioHeightReading()
{
    radioHeightRequested = true;
    if (!handle || !flightActive || radioHeightReading)
        return;

    if (!radioHeightDefinitionAdded) {
        const HRESULT definitionResult =
            SimConnect_AddToDataDefinition(handle, kDefinitionRadioHeight, "RADIO HEIGHT", "feet",
                                           SIMCONNECT_DATATYPE_FLOAT64);
        if (FAILED(definitionResult)) {
            emit radioHeightUnavailable();
            return;
        }

        radioHeightDefinitionAdded = true;
    }

    const HRESULT requestResult =
        SimConnect_RequestDataOnSimObject(handle, kRequestRadioHeight, kDefinitionRadioHeight,
                                          SIMCONNECT_OBJECT_ID_USER_AIRCRAFT,
                                          SIMCONNECT_PERIOD_SIM_FRAME,
                                          SIMCONNECT_DATA_REQUEST_FLAG_DEFAULT, 0,
                                          kRadioHeightIntervalFrames, 0);
    if (FAILED(requestResult)) {
        emit radioHeightUnavailable();
        return;
    }

    radioHeightReading = true;
    radioHeightAwaitingFirstData = true;
}

void SimConnectClient::stopRadioHeightReading()
{
    radioHeightRequested = false;
    stopRadioHeightRequest();
}

void SimConnectClient::startLandingRateReading()
{
    landingRateRequested = true;
    resetLandingRateState();
    if (!handle || !flightActive || landingRateReading)
        return;

    if (!landingRateDefinitionAdded) {
        HRESULT definitionResult =
            SimConnect_AddToDataDefinition(handle, kDefinitionLandingRate,
                                           "PLANE ALT ABOVE GROUND", "feet",
                                           SIMCONNECT_DATATYPE_FLOAT64);
        if (FAILED(definitionResult)) {
            emit landingRateUnavailable();
            return;
        }

        definitionResult =
            SimConnect_AddToDataDefinition(handle, kDefinitionLandingRate,
                                           "PLANE TOUCHDOWN NORMAL VELOCITY", "feet per second",
                                           SIMCONNECT_DATATYPE_FLOAT64);
        if (FAILED(definitionResult)) {
            SimConnect_ClearDataDefinition(handle, kDefinitionLandingRate);
            emit landingRateUnavailable();
            return;
        }

        definitionResult =
            SimConnect_AddToDataDefinition(handle, kDefinitionLandingRate, "SIM ON GROUND", "Bool",
                                           SIMCONNECT_DATATYPE_INT32);
        if (FAILED(definitionResult)) {
            SimConnect_ClearDataDefinition(handle, kDefinitionLandingRate);
            emit landingRateUnavailable();
            return;
        }

        landingRateDefinitionAdded = true;
    }

    const HRESULT requestResult =
        SimConnect_RequestDataOnSimObject(handle, kRequestLandingRate, kDefinitionLandingRate,
                                          SIMCONNECT_OBJECT_ID_USER_AIRCRAFT,
                                          SIMCONNECT_PERIOD_SIM_FRAME,
                                          SIMCONNECT_DATA_REQUEST_FLAG_DEFAULT, 0,
                                          kRadioHeightIntervalFrames, 0);
    if (FAILED(requestResult)) {
        emit landingRateUnavailable();
        return;
    }

    landingRateReading = true;
    landingRateAwaitingFirstData = true;
}

void SimConnectClient::stopLandingRateReading()
{
    landingRateRequested = false;
    resetLandingRateState();
    stopLandingRateRequest();
}

void SimConnectClient::stopRadioHeightRequest()
{
    if (handle && radioHeightReading) {
        SimConnect_RequestDataOnSimObject(handle, kRequestRadioHeight, kDefinitionRadioHeight,
                                          SIMCONNECT_OBJECT_ID_USER_AIRCRAFT,
                                          SIMCONNECT_PERIOD_NEVER,
                                          SIMCONNECT_DATA_REQUEST_FLAG_DEFAULT, 0, 0, 0);
    }

    radioHeightReading = false;
    radioHeightAwaitingFirstData = false;
}

void SimConnectClient::stopLandingRateRequest()
{
    if (handle && landingRateReading) {
        SimConnect_RequestDataOnSimObject(handle, kRequestLandingRate, kDefinitionLandingRate,
                                          SIMCONNECT_OBJECT_ID_USER_AIRCRAFT,
                                          SIMCONNECT_PERIOD_NEVER,
                                          SIMCONNECT_DATA_REQUEST_FLAG_DEFAULT, 0, 0, 0);
    }

    landingRateReading = false;
    landingRateAwaitingFirstData = false;
}

void SimConnectClient::resetLandingRateState()
{
    landingRateAirborne = false;
    landingRatePreviousOnGround = true;
    landingRateHasResult = false;
}

void SimConnectClient::closeConnection()
{
    stopRadioHeightReading();
    stopLandingRateReading();

    connecting = false;
    flightActive = false;
    inputEventListening = false;
    inputEventTypes.clear();
    inputEventParams.clear();
    inputEventParamRequests.clear();
    subscribedInputEvents.clear();
    inputEventGetRequests.clear();
    radioHeightRequested = false;
    landingRateRequested = false;
    radioHeightDefinitionAdded = false;
    landingRateDefinitionAdded = false;
    resetLandingRateState();

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
    radioHeightRequested = false;
    landingRateRequested = false;
    radioHeightReading = false;
    radioHeightAwaitingFirstData = false;
    radioHeightDefinitionAdded = false;
    landingRateReading = false;
    landingRateAwaitingFirstData = false;
    landingRateDefinitionAdded = false;
    resetLandingRateState();

    flightActive = false;
    inputEventListening = false;
    inputEventTypes.clear();
    inputEventParams.clear();
    inputEventParamRequests.clear();
    subscribedInputEvents.clear();
    inputEventGetRequests.clear();

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
    bool handled = false;

    if (radioHeightReading && radioHeightAwaitingFirstData && isRadioHeightException(code)) {
        stopRadioHeightRequest();
        if (handle)
            SimConnect_ClearDataDefinition(handle, kDefinitionRadioHeight);
        radioHeightDefinitionAdded = false;
        if (radioHeightRequested)
            emit radioHeightUnavailable();
        handled = true;
    }

    if (landingRateReading && landingRateAwaitingFirstData && isRadioHeightException(code)) {
        stopLandingRateRequest();
        if (handle)
            SimConnect_ClearDataDefinition(handle, kDefinitionLandingRate);
        landingRateDefinitionAdded = false;
        resetLandingRateState();
        if (landingRateRequested)
            emit landingRateUnavailable();
        handled = true;
    }

    if (handled)
        return;

    emit simError(quint32(code));
}

void SimConnectClient::handleFlowEvent(const SIMCONNECT_RECV_FLOW_EVENT *event)
{
    switch (event->FlowEvent)
    {
    case SIMCONNECT_FLOW_EVENT_FLIGHT_START:
        if (!flightActive)
        {
            flightActive = true;
            emit flightStarted();
        }
        break;

    case SIMCONNECT_FLOW_EVENT_FLIGHT_END:
    case SIMCONNECT_FLOW_EVENT_BACK_TO_MAIN_MENU:
        if (flightActive)
        {
            stopRadioHeightReading();
            stopLandingRateReading();
            flightActive = false;
            emit flightEnded();
        }
        break;

    default:
        break;
    }
}

void SimConnectClient::notifyAircraftLoaded(const char *file)
{
    // AircraftLoaded and FLIGHT_START are not guaranteed to arrive in a
    // fixed order.  The UI keeps the event until the flight state is ready.
    emit aircraftLoaded(QString::fromUtf8(file));
}

bool SimConnectClient::isRadioHeightException(DWORD code) const
{
    return code == SIMCONNECT_EXCEPTION_NAME_UNRECOGNIZED ||
           code == SIMCONNECT_EXCEPTION_INVALID_DATA_TYPE ||
           code == SIMCONNECT_EXCEPTION_INVALID_DATA_SIZE ||
           code == SIMCONNECT_EXCEPTION_DATA_ERROR ||
           code == SIMCONNECT_EXCEPTION_DEFINITION_ERROR ||
           code == SIMCONNECT_EXCEPTION_DATUM_ID;
}

int SimConnectClient::inputEventParamSize(const QString &param)
{
    static const QRegularExpression charParam(QStringLiteral("^char\\[(\\d+)\\]$"));

    int totalSize = 0;
    const QStringList parameters = param.split(QLatin1Char(';'), Qt::SkipEmptyParts);
    for (const QString &rawParameter : parameters) {
        const QString parameter = rawParameter.trimmed();
        int parameterSize = 0;

        if (parameter == QStringLiteral("FLOAT64") || parameter == QStringLiteral("DOUBLE")) {
            parameterSize = int(sizeof(double));
        } else {
            const QRegularExpressionMatch match = charParam.match(parameter);
            if (!match.hasMatch())
                return -1;

            bool ok = false;
            parameterSize = match.captured(1).toInt(&ok);
            if (!ok || parameterSize < 0)
                return -1;
        }

        if (parameterSize > std::numeric_limits<int>::max() - totalSize)
            return -1;
        totalSize += parameterSize;
    }

    return totalSize;
}

void CALLBACK SimConnectClient::dispatchProc(SIMCONNECT_RECV *data, DWORD cbData, void *context)
{
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
    case SIMCONNECT_RECV_ID_ENUMERATE_INPUT_EVENTS:
    {
        if (!client->inputEventListening)
            break;

        const SIMCONNECT_RECV_ENUMERATE_INPUT_EVENTS *list =
            static_cast<const SIMCONNECT_RECV_ENUMERATE_INPUT_EVENTS *>(data);
        if (list->dwRequestID != kRequestEnumerateInputEvents)
            break;

        const SIMCONNECT_INPUT_EVENT_DESCRIPTOR *descriptors =
            reinterpret_cast<const SIMCONNECT_INPUT_EVENT_DESCRIPTOR *>(&list->rgData);
        for (DWORD i = 0; i < list->dwArraySize; ++i)
        {
            const SIMCONNECT_INPUT_EVENT_DESCRIPTOR &descriptor = descriptors[i];
            const QString eType = descriptor.eType == SIMCONNECT_INPUT_EVENT_TYPE_DOUBLE
                                      ? QStringLiteral("DOUBLE")
                                      : QStringLiteral("STRING");

            if (!client->subscribedInputEvents.contains(quint64(descriptor.Hash))) {
                const quint64 hash = quint64(descriptor.Hash);
                client->inputEventTypes.insert(hash, descriptor.eType);
                client->inputEventParamRequests.insert(hash);
                const HRESULT paramsResult = SimConnect_EnumerateInputEventParams(client->handle, descriptor.Hash);
                if (FAILED(paramsResult)) {
                    client->inputEventParamRequests.remove(hash);
                    emit client->simError(quint32(paramsResult));
                }

                const HRESULT result = SimConnect_SubscribeInputEvent(client->handle, descriptor.Hash);
                if (FAILED(result)) {
                    emit client->simError(quint32(result));
                    continue;
                }
                client->subscribedInputEvents.insert(hash);
            }

            emit client->inputEventEnumerated(QString::fromUtf8(descriptor.Name), quint64(descriptor.Hash), eType);
        }
        break;
    }
    case SIMCONNECT_RECV_ID_ENUMERATE_INPUT_EVENT_PARAMS:
    {
        if (!client->inputEventListening)
            break;

        const SIMCONNECT_RECV_ENUMERATE_INPUT_EVENT_PARAMS *params =
            static_cast<const SIMCONNECT_RECV_ENUMERATE_INPUT_EVENT_PARAMS *>(data);
        const quint64 hash = quint64(params->Hash);
        if (!client->inputEventParamRequests.contains(hash))
            break;

        client->inputEventParamRequests.remove(hash);
        const QString param = QString::fromUtf8(params->Value);
        int size = inputEventParamSize(param);
        if (param.isEmpty() && size == 0) {
            const auto type = client->inputEventTypes.constFind(hash);
            if (type != client->inputEventTypes.constEnd() &&
                type.value() == SIMCONNECT_INPUT_EVENT_TYPE_DOUBLE) {
                size = int(sizeof(double));
            }
        }
        client->inputEventParams.insert(hash, qMakePair(param, size));
        emit client->inputEventParamsEnumerated(hash, param, size);
        break;
    }
    case SIMCONNECT_RECV_ID_SIMOBJECT_DATA:
    {
        const SIMCONNECT_RECV_SIMOBJECT_DATA *simObjectData =
            static_cast<const SIMCONNECT_RECV_SIMOBJECT_DATA *>(data);
        const char *rawValue = reinterpret_cast<const char *>(&simObjectData->dwData);
        const size_t valueOffset = rawValue - reinterpret_cast<const char *>(simObjectData);

        if (simObjectData->dwRequestID == kRequestRadioHeight &&
            simObjectData->dwDefineID == kDefinitionRadioHeight) {
            if (!client->radioHeightReading || simObjectData->dwDefineCount < 1)
                break;

            if (cbData < valueOffset + sizeof(double)) {
                if (client->radioHeightRequested)
                    emit client->radioHeightUnavailable();
                break;
            }

            double radioHeight = 0.0;
            std::memcpy(&radioHeight, rawValue, sizeof(radioHeight));
            client->radioHeightAwaitingFirstData = false;
            if (!std::isfinite(radioHeight)) {
                if (client->radioHeightRequested)
                    emit client->radioHeightUnavailable();
                break;
            }

            if (client->radioHeightRequested)
                emit client->radioHeightReceived(radioHeight);
            break;
        }

        if (simObjectData->dwRequestID != kRequestLandingRate ||
            simObjectData->dwDefineID != kDefinitionLandingRate ||
            !client->landingRateReading || simObjectData->dwDefineCount < 3)
            break;

        constexpr size_t dataSize = sizeof(double) + sizeof(double) + sizeof(LONG);
        if (cbData < valueOffset + dataSize) {
            if (client->landingRateRequested)
                emit client->landingRateUnavailable();
            break;
        }

        double aboveGroundHeight = 0.0;
        double touchdownNormalVelocity = 0.0;
        LONG onGroundValue = 0;
        std::memcpy(&aboveGroundHeight, rawValue, sizeof(aboveGroundHeight));
        std::memcpy(&touchdownNormalVelocity, rawValue + sizeof(aboveGroundHeight),
                    sizeof(touchdownNormalVelocity));
        std::memcpy(&onGroundValue, rawValue + sizeof(aboveGroundHeight) +
                                             sizeof(touchdownNormalVelocity),
                    sizeof(onGroundValue));
        client->landingRateAwaitingFirstData = false;
        if (!std::isfinite(aboveGroundHeight)) {
            if (client->landingRateRequested)
                emit client->landingRateUnavailable();
            break;
        }

        const bool onGround = onGroundValue != 0;
        if (aboveGroundHeight >= kLandingRateAirborneHeightFeet) {
            const bool newlyAirborne = !client->landingRateAirborne;
            client->landingRateHasResult = false;
            client->landingRateAirborne = true;
            if (newlyAirborne)
                emit client->landingRateCleared();
        }

        if (client->landingRateAirborne && !client->landingRatePreviousOnGround && onGround &&
            !client->landingRateHasResult) {
            if (std::isfinite(touchdownNormalVelocity)) {
                client->landingRateHasResult = true;
                client->landingRateAirborne = false;
                emit client->landingRateReceived(touchdownNormalVelocity * 60.0);
            } else {
                emit client->landingRateUnavailable();
            }
        }

        client->landingRatePreviousOnGround = onGround;
        break;
    }
    case SIMCONNECT_RECV_ID_GET_INPUT_EVENT:
    {
        const SIMCONNECT_RECV_GET_INPUT_EVENT *inputEvent =
            static_cast<const SIMCONNECT_RECV_GET_INPUT_EVENT *>(data);
        const auto request = client->inputEventGetRequests.constFind(inputEvent->dwRequestID);
        if (request == client->inputEventGetRequests.constEnd())
            break;

        const quint64 hash = request.value();
        client->inputEventGetRequests.remove(inputEvent->dwRequestID);

        if (inputEvent->eType != SIMCONNECT_INPUT_EVENT_TYPE_DOUBLE) {
            emit client->inputEventValueUnavailable(hash);
            break;
        }

        const char *rawValue = reinterpret_cast<const char *>(&inputEvent->Value);
        const size_t valueOffset = reinterpret_cast<const char *>(&inputEvent->Value) -
                                   reinterpret_cast<const char *>(inputEvent);
        const size_t rawSize = cbData > valueOffset ? cbData - valueOffset : 0;
        if (rawSize < sizeof(double)) {
            emit client->inputEventValueUnavailable(hash);
            break;
        }

        double value = 0.0;
        std::memcpy(&value, rawValue, sizeof(value));
        emit client->inputEventValueReceived(hash, value);
        break;
    }
    case SIMCONNECT_RECV_ID_SUBSCRIBE_INPUT_EVENT:
    {
        if (!client->inputEventListening)
            break;

        const SIMCONNECT_RECV_SUBSCRIBE_INPUT_EVENT *inputEvent =
            static_cast<const SIMCONNECT_RECV_SUBSCRIBE_INPUT_EVENT *>(data);
        const QString eType = inputEvent->eType == SIMCONNECT_INPUT_EVENT_TYPE_DOUBLE
                                  ? QStringLiteral("DOUBLE")
                                  : QStringLiteral("STRING");

        QString value;
        const char *rawValue = reinterpret_cast<const char *>(&inputEvent->Value);
        const size_t valueOffset = reinterpret_cast<const char *>(&inputEvent->Value) -
                                   reinterpret_cast<const char *>(inputEvent);
        const size_t rawSize = cbData > valueOffset ? cbData - valueOffset : 0;
        if (inputEvent->eType == SIMCONNECT_INPUT_EVENT_TYPE_DOUBLE && rawSize >= sizeof(double)) {
            double numericValue = 0.0;
            std::memcpy(&numericValue, rawValue, sizeof(numericValue));
            value = QString::number(numericValue, 'g', 15);
        } else if (inputEvent->eType == SIMCONNECT_INPUT_EVENT_TYPE_STRING && rawSize > 0) {
            const size_t stringSize = std::min(rawSize, size_t(256));
            size_t stringLength = 0;
            while (stringLength < stringSize && rawValue[stringLength] != '\0')
                ++stringLength;
            value = QString::fromUtf8(rawValue, int(stringLength));
        }

        const quint64 hash = quint64(inputEvent->Hash);
        const auto params = client->inputEventParams.constFind(hash);
        const QString param = params == client->inputEventParams.constEnd() ? QString() : params->first;
        const int size = params == client->inputEventParams.constEnd() ? -1 : params->second;
        emit client->inputEventReceived(hash, eType, value, param, size);
        break;
    }
    case SIMCONNECT_RECV_ID_FLOW_EVENT:
    {
        const SIMCONNECT_RECV_FLOW_EVENT *event = static_cast<SIMCONNECT_RECV_FLOW_EVENT *>(data);
        client->handleFlowEvent(event);
        break;
    }
    default:
        break;
    }
}
