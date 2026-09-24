#include "inputlistener.h"

#include <QStringList>

namespace {

InputListener *g_listener = nullptr;

}

InputListener::InputListener(QObject *parent)
    : QObject(parent)
{
}

InputListener::~InputListener()
{
    if (hook) {
        UnhookWindowsHookEx(hook);
        hook = nullptr;
    }
    if (g_listener == this)
        g_listener = nullptr;
}

void InputListener::start()
{
    if (hook)
        return;

    g_listener = this;
    hook = SetWindowsHookExW(WH_KEYBOARD_LL, hookProc, GetModuleHandleW(nullptr), 0);
}

unsigned int InputListener::modifierBit(WPARAM vk)
{
    switch (vk) {
    case VK_LCONTROL: return ModLCtrl;
    case VK_RCONTROL: return ModRCtrl;
    case VK_LSHIFT:   return ModLShift;
    case VK_RSHIFT:   return ModRShift;
    case VK_LMENU:    return ModLAlt;
    case VK_RMENU:    return ModRAlt;
    case VK_LWIN:     return ModLWin;
    case VK_RWIN:     return ModRWin;
    default:          return 0;
    }
}

bool InputListener::isModifier(WPARAM vk)
{
    return modifierBit(vk) != 0;
}

QString InputListener::keyText(WPARAM vk)
{
    if (vk >= 'A' && vk <= 'Z')
        return QString(QChar(char16_t(vk)));
    if (vk >= '0' && vk <= '9')
        return QString(QChar(char16_t(vk)));
    if (vk >= VK_F1 && vk <= VK_F24)
        return QStringLiteral("F%1").arg(int(vk - VK_F1 + 1));
    if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9)
        return QStringLiteral("NUM %1").arg(int(vk - VK_NUMPAD0));

    switch (vk) {
    case VK_LCONTROL: return QStringLiteral("L CTRL");
    case VK_RCONTROL: return QStringLiteral("R CTRL");
    case VK_CONTROL:  return QStringLiteral("CTRL");
    case VK_LSHIFT:   return QStringLiteral("L SHIFT");
    case VK_RSHIFT:   return QStringLiteral("R SHIFT");
    case VK_SHIFT:    return QStringLiteral("SHIFT");
    case VK_LMENU:    return QStringLiteral("L ALT");
    case VK_RMENU:    return QStringLiteral("R ALT");
    case VK_MENU:     return QStringLiteral("ALT");
    case VK_LWIN:     return QStringLiteral("L WIN");
    case VK_RWIN:     return QStringLiteral("R WIN");
    case VK_SPACE:    return QStringLiteral("SPACE");
    case VK_TAB:      return QStringLiteral("TAB");
    case VK_RETURN:   return QStringLiteral("ENTER");
    case VK_ESCAPE:   return QStringLiteral("ESC");
    case VK_BACK:     return QStringLiteral("BACKSPACE");
    case VK_DELETE:   return QStringLiteral("DEL");
    case VK_INSERT:   return QStringLiteral("INS");
    case VK_HOME:     return QStringLiteral("HOME");
    case VK_END:      return QStringLiteral("END");
    case VK_PRIOR:    return QStringLiteral("PGUP");
    case VK_NEXT:     return QStringLiteral("PGDN");
    case VK_UP:       return QStringLiteral("UP");
    case VK_DOWN:     return QStringLiteral("DOWN");
    case VK_LEFT:     return QStringLiteral("LEFT");
    case VK_RIGHT:    return QStringLiteral("RIGHT");
    case VK_CAPITAL:  return QStringLiteral("CAPS");
    case VK_NUMLOCK:  return QStringLiteral("NUMLOCK");
    case VK_SCROLL:   return QStringLiteral("SCROLL");
    case VK_SNAPSHOT: return QStringLiteral("PRTSC");
    case VK_PAUSE:    return QStringLiteral("PAUSE");
    case VK_APPS:     return QStringLiteral("MENU");
    case VK_MULTIPLY: return QStringLiteral("NUM *");
    case VK_ADD:      return QStringLiteral("NUM +");
    case VK_SUBTRACT: return QStringLiteral("NUM -");
    case VK_DECIMAL:  return QStringLiteral("NUM .");
    case VK_DIVIDE:   return QStringLiteral("NUM /");
    case VK_OEM_1:      return QStringLiteral(";");
    case VK_OEM_PLUS:   return QStringLiteral("=");
    case VK_OEM_COMMA:  return QStringLiteral(",");
    case VK_OEM_MINUS:  return QStringLiteral("-");
    case VK_OEM_PERIOD: return QStringLiteral(".");
    case VK_OEM_2:      return QStringLiteral("/");
    case VK_OEM_3:      return QStringLiteral("`");
    case VK_OEM_4:      return QStringLiteral("[");
    case VK_OEM_5:      return QStringLiteral("\\");
    case VK_OEM_6:      return QStringLiteral("]");
    case VK_OEM_7:      return QStringLiteral("'");
    default: break;
    }

    const UINT scan = MapVirtualKeyW(UINT(vk), MAPVK_VK_TO_VSC);
    if (scan) {
        wchar_t buf[64] = {};
        if (GetKeyNameTextW(LONG(scan << 16), buf, 64) > 0)
            return QString::fromWCharArray(buf);
    }
    return QStringLiteral("VK %1").arg(int(vk));
}

QString InputListener::textFor(WPARAM vk) const
{
    if (isModifier(vk))
        return keyText(vk);

    QStringList parts;
    if (modifiers & ModLCtrl)  parts << keyText(VK_LCONTROL);
    if (modifiers & ModRCtrl)  parts << keyText(VK_RCONTROL);
    if (modifiers & ModLShift) parts << keyText(VK_LSHIFT);
    if (modifiers & ModRShift) parts << keyText(VK_RSHIFT);
    if (modifiers & ModLAlt)   parts << keyText(VK_LMENU);
    if (modifiers & ModRAlt)   parts << keyText(VK_RMENU);
    if (modifiers & ModLWin)   parts << keyText(VK_LWIN);
    if (modifiers & ModRWin)   parts << keyText(VK_RWIN);
    parts << keyText(vk);
    return parts.join(QLatin1Char('+'));
}

void InputListener::handleKey(WPARAM vk, bool down)
{
    const unsigned int bit = modifierBit(vk);

    if (bit) {
        if (down)
            modifiers |= bit;
        else
            modifiers &= ~bit;
    }

    if (!down)
        return;

    emit keyPressed(textFor(vk));
}

LRESULT CALLBACK InputListener::hookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && g_listener) {
        const KBDLLHOOKSTRUCT *kb = reinterpret_cast<const KBDLLHOOKSTRUCT *>(lParam);
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
            g_listener->handleKey(WPARAM(kb->vkCode), true);
        else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
            g_listener->handleKey(WPARAM(kb->vkCode), false);
    }
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}
