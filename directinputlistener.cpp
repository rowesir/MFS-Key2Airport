#include "directinputlistener.h"

#include <QHash>
#include <QList>
#include <QTimer>
#include <QWinEventNotifier>

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

struct DirectInputDevice
{
    IDirectInputDevice8W *device = nullptr;
    GUID guid = {};
    QString name;
    QString displayName;
    QList<QString> buttonNames;
    int povCount = 0;
    int previousPov[4] = {-1, -1, -1, -1};
    DIJOYSTATE2 state = {};
    BYTE previous[128] = {};
    HANDLE event = nullptr;
    QWinEventNotifier *notifier = nullptr;
};

struct DirectInputMouse
{
    IDirectInputDevice8W *device = nullptr;
    DIMOUSESTATE2 state = {};
    BYTE previous[8] = {};
};

namespace {

const int kPollIntervalMs = 10;
const int kRefreshDelayMs = 500;

struct DeviceEnumContext
{
    IDirectInput8W *dinput = nullptr;
    HWND window = nullptr;
    QList<DirectInputDevice *> *devices = nullptr;
};

bool sameGuid(const GUID &a, const GUID &b)
{
    return IsEqualGUID(a, b) != 0;
}

QString modifierPrefix()
{
    QStringList parts;
    if (GetAsyncKeyState(VK_LCONTROL) & 0x8000) parts << QStringLiteral("L CTRL");
    if (GetAsyncKeyState(VK_RCONTROL) & 0x8000) parts << QStringLiteral("R CTRL");
    if (GetAsyncKeyState(VK_LSHIFT) & 0x8000) parts << QStringLiteral("L SHIFT");
    if (GetAsyncKeyState(VK_RSHIFT) & 0x8000) parts << QStringLiteral("R SHIFT");
    if (GetAsyncKeyState(VK_LMENU) & 0x8000) parts << QStringLiteral("L ALT");
    if (GetAsyncKeyState(VK_RMENU) & 0x8000) parts << QStringLiteral("R ALT");
    if (GetAsyncKeyState(VK_LWIN) & 0x8000) parts << QStringLiteral("L WIN");
    if (GetAsyncKeyState(VK_RWIN) & 0x8000) parts << QStringLiteral("R WIN");

    if (parts.isEmpty())
        return QString();
    return parts.join(QLatin1Char('+')) + QLatin1Char('+');
}

int povDirection(DWORD pov)
{
    if (pov == 0xFFFFFFFF || pov >= 36000)
        return -1;
    return int((pov + 2250) / 4500) % 8;
}

QString povDirectionName(int direction)
{
    switch (direction) {
    case 0: return QStringLiteral("Up");
    case 1: return QStringLiteral("Up-Right");
    case 2: return QStringLiteral("Right");
    case 3: return QStringLiteral("Down-Right");
    case 4: return QStringLiteral("Down");
    case 5: return QStringLiteral("Down-Left");
    case 6: return QStringLiteral("Left");
    case 7: return QStringLiteral("Up-Left");
    default: return QString();
    }
}

void releaseDevice(DirectInputDevice *dev)
{
    delete dev->notifier;
    dev->notifier = nullptr;

    if (dev->device) {
        dev->device->SetEventNotification(nullptr);
        dev->device->Unacquire();
        dev->device->Release();
        dev->device = nullptr;
    }

    if (dev->event) {
        CloseHandle(dev->event);
        dev->event = nullptr;
    }

    delete dev;
}

BOOL CALLBACK enumObjectProc(LPCDIDEVICEOBJECTINSTANCEW object, LPVOID context)
{
    DirectInputDevice *dev = static_cast<DirectInputDevice *>(context);
    if ((object->dwType & DIDFT_BUTTON) && dev->buttonNames.size() < 128)
        dev->buttonNames.append(QStringLiteral("Button %1").arg(dev->buttonNames.size()));
    else if ((object->dwType & DIDFT_POV) && dev->povCount < 4)
        ++dev->povCount;
    return DIENUM_CONTINUE;
}

BOOL CALLBACK enumDeviceProc(LPCDIDEVICEINSTANCEW instance, LPVOID context)
{
    DeviceEnumContext *ctx = static_cast<DeviceEnumContext *>(context);

    IDirectInputDevice8W *device = nullptr;
    if (FAILED(ctx->dinput->CreateDevice(instance->guidInstance, &device, nullptr)))
        return DIENUM_CONTINUE;

    if (FAILED(device->SetDataFormat(&c_dfDIJoystick2))
        || FAILED(device->SetCooperativeLevel(ctx->window, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE))) {
        device->Release();
        return DIENUM_CONTINUE;
    }

    DirectInputDevice *dev = new DirectInputDevice;
    dev->device = device;
    dev->guid = instance->guidInstance;
    dev->name = QString::fromWCharArray(instance->tszInstanceName);

    device->EnumObjects(enumObjectProc, dev, DIDFT_BUTTON | DIDFT_POV);
    device->Acquire();
    dev->event = CreateEventW(nullptr, FALSE, FALSE, nullptr);

    ctx->devices->append(dev);
    return DIENUM_CONTINUE;
}

}

DirectInputListener::DirectInputListener(QObject *parent)
    : QObject(parent)
{
}

DirectInputListener::~DirectInputListener()
{
    for (DirectInputDevice *dev : devices)
        releaseDevice(dev);
    devices.clear();

    if (dinput) {
        static_cast<IDirectInput8W *>(dinput)->Release();
        dinput = nullptr;
    }

    if (mouse) {
        mouse->device->Unacquire();
        mouse->device->Release();
        delete mouse;
        mouse = nullptr;
    }
}

void DirectInputListener::start(HWND window)
{
    if (dinput)
        return;

    IDirectInput8W *di = nullptr;
    if (FAILED(DirectInput8Create(GetModuleHandleW(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8W,
                                  reinterpret_cast<void **>(&di), nullptr)))
        return;

    dinput = di;
    this->window = window;

    IDirectInputDevice8W *mouseDevice = nullptr;
    if (SUCCEEDED(di->CreateDevice(GUID_SysMouse, &mouseDevice, nullptr))
        && SUCCEEDED(mouseDevice->SetDataFormat(&c_dfDIMouse2))
        && SUCCEEDED(mouseDevice->SetCooperativeLevel(window, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE)))
    {
        mouseDevice->Acquire();
        mouse = new DirectInputMouse;
        mouse->device = mouseDevice;
    }
    else if (mouseDevice)
    {
        mouseDevice->Release();
    }

    refresh();

    timer = new QTimer(this);
    timer->setInterval(kPollIntervalMs);
    connect(timer, &QTimer::timeout, this, &DirectInputListener::pollDevices);
    timer->start();

    refreshTimer = new QTimer(this);
    refreshTimer->setSingleShot(true);
    refreshTimer->setInterval(kRefreshDelayMs);
    connect(refreshTimer, &QTimer::timeout, this, &DirectInputListener::refresh);
}

void DirectInputListener::requestRefresh()
{
    if (refreshTimer)
        refreshTimer->start();
}

void DirectInputListener::refresh()
{
    IDirectInput8W *di = static_cast<IDirectInput8W *>(dinput);
    if (!di)
        return;

    QList<DirectInputDevice *> found;
    DeviceEnumContext ctx;
    ctx.dinput = di;
    ctx.window = window;
    ctx.devices = &found;
    di->EnumDevices(DI8DEVCLASS_GAMECTRL, enumDeviceProc, &ctx, DIEDFL_ATTACHEDONLY);

    for (int i = devices.size() - 1; i >= 0; --i) {
        bool present = false;
        for (DirectInputDevice *dev : found) {
            if (sameGuid(dev->guid, devices.at(i)->guid)) {
                present = true;
                break;
            }
        }
        if (!present)
            releaseDevice(devices.takeAt(i));
    }

    for (DirectInputDevice *dev : found) {
        bool known = false;
        for (DirectInputDevice *current : devices) {
            if (sameGuid(dev->guid, current->guid)) {
                known = true;
                break;
            }
        }
        if (known) {
            releaseDevice(dev);
            continue;
        }

        if (dev->event && SUCCEEDED(dev->device->SetEventNotification(dev->event))) {
            dev->notifier = new QWinEventNotifier(dev->event);
            connect(dev->notifier, &QWinEventNotifier::activated, this, [this, dev]() { readDevice(dev); });
        }

        devices.append(dev);
    }

    assignDisplayNames();
}

void DirectInputListener::assignDisplayNames()
{
    QHash<QString, int> counts;
    for (DirectInputDevice *dev : devices)
        counts[dev->name] += 1;

    QHash<QString, int> seen;
    for (DirectInputDevice *dev : devices)
    {
        if (counts.value(dev->name) > 1)
            dev->displayName = QStringLiteral("%1 #%2").arg(dev->name).arg(++seen[dev->name]);
        else
            dev->displayName = dev->name;
    }
}

void DirectInputListener::pollDevices()
{
    for (DirectInputDevice *dev : devices) {
        if (!dev->notifier)
            readDevice(dev);
    }

    readMouse();
}

void DirectInputListener::readMouse()
{
    if (!mouse)
        return;

    if (FAILED(mouse->device->GetDeviceState(sizeof(DIMOUSESTATE2), &mouse->state))) {
        mouse->device->Acquire();
        return;
    }

    const struct
    {
        int index;
        const char *name;
    } sideButtons[] = {{3, "Mouse Side 1"}, {4, "Mouse Side 2"}};

    for (const auto &side : sideButtons) {
        const BYTE now = mouse->state.rgbButtons[side.index];
        const BYTE before = mouse->previous[side.index];
        mouse->previous[side.index] = now;
        if ((now & 0x80) && !(before & 0x80))
            emit keyPressed(modifierPrefix() + QString::fromLatin1(side.name));
    }

    if (mouse->state.lZ > 0)
        emit keyPressed(modifierPrefix() + QStringLiteral("Mouse Wheel Up"));
    else if (mouse->state.lZ < 0)
        emit keyPressed(modifierPrefix() + QStringLiteral("Mouse Wheel Down"));
}

void DirectInputListener::readDevice(DirectInputDevice *dev)
{
    if (FAILED(dev->device->GetDeviceState(sizeof(DIJOYSTATE2), &dev->state))) {
        dev->device->Acquire();
        if (dev->event)
            dev->device->SetEventNotification(dev->event);
        return;
    }

    for (int i = 0; i < dev->buttonNames.size(); ++i) {
        const BYTE now = dev->state.rgbButtons[i];
        const BYTE before = dev->previous[i];
        dev->previous[i] = now;
        if ((now & 0x80) && !(before & 0x80))
            emit keyPressed(dev->displayName + QLatin1Char(' ') + dev->buttonNames.at(i));
    }

    for (int i = 0; i < dev->povCount; ++i) {
        const int direction = povDirection(dev->state.rgdwPOV[i]);
        if (direction >= 0 && direction != dev->previousPov[i]) {
            emit keyPressed(dev->displayName + QLatin1Char(' ')
                            + QStringLiteral("POV %1 %2").arg(i).arg(povDirectionName(direction)));
        }
        dev->previousPov[i] = direction;
    }
}
