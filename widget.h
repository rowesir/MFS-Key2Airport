#ifndef WIDGET_H
#define WIDGET_H

#include <QList>
#include <QPair>
#include <QTimer>
#include <QWidget>

#include "dialogenum.h"
#include "directinputlistener.h"
#include "inputlistener.h"
#include "simconnectclient.h"

QT_BEGIN_NAMESPACE
class QThread;
QT_END_NAMESPACE

class QCloseEvent;

QT_BEGIN_NAMESPACE
namespace Ui
{
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

  public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

  protected:
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
    void closeEvent(QCloseEvent *event) override;

  private slots:
    void on_pbtnConnect_clicked();
    void on_pbtnEnum_clicked();

    void onKeyPressed(const QString &name);

    void onDetailTimeout();

    void on_ckbRA_clicked(bool checked);

    void on_ckbLR_clicked(bool checked);

    void onSimConnected();

    void onSimDisconnected();

    void onSimConnectionLost();

    void onFlightStarted();

    void onAircraftLoaded(const QString &file);

    void onFlightEnded();

    void onSimError(quint32 code);

  private:
    class ControlGuard
    {
      public:
        explicit ControlGuard(Widget *widget);
        ~ControlGuard();

        void keepDisabled(QWidget *control);

      private:
        Widget *m_widget = nullptr;
        QList<QPair<QWidget *, bool>> m_states;
        QList<QWidget *> m_keepDisabled;
    };

    void initUI();
    void setInfoText(const QString &text, const QString &color);
    void resetConnectionUi();
    bool confirmFlightExit(const QString &action);
    QList<QWidget *> guardedControls() const;

    Ui::Widget *ui;
    DialogEnum dialogEnum;
    QThread *inputThread                     = nullptr;
    QThread *simThread                       = nullptr;
    InputListener *inputListener             = nullptr;
    DirectInputListener *directInputListener = nullptr;
    SimConnectClient *simClient              = nullptr;
    QTimer *timerDetail                      = nullptr;
    bool connected                           = false;
    bool flightActive                        = false;
};
#endif // WIDGET_H
