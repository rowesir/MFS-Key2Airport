#ifndef INPUTLISTENER_H
#define INPUTLISTENER_H

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#include <QObject>
#include <QString>

class InputListener : public QObject
{
    Q_OBJECT

public:
    explicit InputListener(QObject *parent = nullptr);
    ~InputListener();

    void start();

signals:
    void keyPressed(const QString &name);

private:
    enum Modifier {
        ModLCtrl = 0x01,
        ModRCtrl = 0x02,
        ModLShift = 0x04,
        ModRShift = 0x08,
        ModLAlt = 0x10,
        ModRAlt = 0x20,
        ModLWin = 0x40,
        ModRWin = 0x80
    };

    static LRESULT CALLBACK hookProc(int nCode, WPARAM wParam, LPARAM lParam);
    static bool isModifier(WPARAM vk);
    static unsigned int modifierBit(WPARAM vk);
    static QString keyText(WPARAM vk);

    void handleKey(WPARAM vk, bool down);
    QString textFor(WPARAM vk) const;

    HHOOK hook = nullptr;
    unsigned int modifiers = 0;
};

#endif // INPUTLISTENER_H
