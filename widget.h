#ifndef WIDGET_H
#define WIDGET_H

#include <QList>
#include <QPair>
#include <QHash>
#include <QTimer>
#include <QStringList>
#include <QWidget>

#include "dialogenum.h"
#include "dialogtest.h"
#include "configexecutor.h"
#include "directinputlistener.h"
#include "inputlistener.h"
#include "simconnectclient.h"

QT_BEGIN_NAMESPACE
class QThread;
class QSoundEffect;
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
    bool eventFilter(QObject *watched, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

  private slots:
    void on_pbtnConnect_clicked();
    void on_pbtnFolder_clicked();
    void on_pbtnReload_clicked();
    void on_pbtnEnum_clicked();
    void on_pbtnTest_clicked();
    void onDialogEnumTestRequested();

    void onKeyPressed(const QString &name);

    void onDetailTimeout();

    void on_ckbRA_clicked(bool checked);

    void on_ckbLR_clicked(bool checked);

    void on_ckbAuto_clicked(bool checked);

    void onSimConnected();

    void onSimDisconnected();

    void onSimConnectionLost();

    void onFlightStarted();

    void onAircraftLoaded(const QString &file);

    void onAircraftModelReceived(const QString &model);

    void onAircraftModelUnavailable();

    void onFlightEnded();

    void onSimError(quint32 code);

    void onRadioHeightReceived(double value);

    void onRadioHeightUnavailable();

    void onLandingRateReceived(double feetPerMinute);

    void onLandingRateCleared();

    void onLandingRateUnavailable();

    void onConfigPageChanged(int page);

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
    void resetRadioHeight();
    void resetRadioHeightCallouts();
    void updateRadioHeightCallouts(double value);
    void resetLandingRate();
    void initializeConfigurations();
    void refreshConfigurationList();
    void beginFlightConfiguration();
    void finishFlightConfiguration(const AircraftConfiguration &configuration);
    void finishFlightConfigurationUnavailable();
    void clearFlightConfiguration();
    void activateFlightFeatures();
    bool confirmFlightExit(const QString &action);
    QList<QWidget *> guardedControls() const;

    Ui::Widget *ui;
    DialogEnum dialogEnum;
    DialogTest dialogTest;
    QThread *inputThread                     = nullptr;
    QThread *simThread                       = nullptr;
    InputListener *inputListener             = nullptr;
    DirectInputListener *directInputListener = nullptr;
    SimConnectClient *simClient              = nullptr;
    ConfigExecutor *configExecutor           = nullptr;
    QTimer *timerDetail                      = nullptr;
    bool connected                           = false;
    bool flightActive                        = false;
    bool aircraftLoaded                      = false;
    bool configurationLoading                = false;
    bool flightFeaturesStarted               = false;
    QStringList availableConfigurationNames;
    QHash<int, QSoundEffect *> radioHeightCalloutSounds;
    QHash<int, bool> radioHeightCalloutPlayed;
    double previousRadioHeight               = 0.0;
    bool hasPreviousRadioHeight               = false;
};
#endif // WIDGET_H
