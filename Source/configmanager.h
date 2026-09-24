#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QString>

struct ConfigRule
{
    bool hasIf = false;
    QString ifExpression;
    QString thenExpression;
    bool hasElse = false;
    QString elseExpression;
};

struct ConfigPage
{
    QHash<QString, QList<ConfigRule>> bindings;
};

struct AircraftConfiguration
{
    QString aircraft;
    bool radioHeight = true;
    bool landingRate = true;
    QString pageSwitch;
    QHash<QString, QString> variables;
    ConfigPage pages[2];
    bool valid = false;
};

class ConfigManager
{
public:
    static QString configDirectoryPath();
    static bool ensureConfigDirectory();
    static QStringList configurationNames();
    static QString configurationPath(const QString &name);
    static bool createDefaultConfiguration(const QString &aircraft, const QString &path);
    static bool loadConfiguration(const QString &path, AircraftConfiguration &configuration);

private:
    static bool parsePage(const QJsonObject &object, ConfigPage &page);
    static bool parseVariables(const QJsonArray &array, QHash<QString, QString> &variables);
    static bool parseRules(const QJsonArray &array, QList<ConfigRule> &rules);
};

#endif // CONFIGMANAGER_H
