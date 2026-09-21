#ifndef DIRECTINPUTLISTENER_H
#define DIRECTINPUTLISTENER_H

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#include <QList>
#include <QObject>
#include <QString>

class QTimer;
struct DirectInputDevice;
struct DirectInputMouse;

class DirectInputListener : public QObject
{
    Q_OBJECT

public:
    explicit DirectInputListener(QObject *parent = nullptr);
    ~DirectInputListener();

    void start(HWND window);

public slots:
    void requestRefresh();
    void refresh();

signals:
    void keyPressed(const QString &name);

private:
    void assignDisplayNames();
    void pollDevices();
    void readDevice(DirectInputDevice *dev);
    void readMouse();

    void *dinput = nullptr;
    HWND window = nullptr;
    QTimer *timer = nullptr;
    QTimer *refreshTimer = nullptr;
    QList<DirectInputDevice *> devices;
    DirectInputMouse *mouse = nullptr;
};

#endif // DIRECTINPUTLISTENER_H
