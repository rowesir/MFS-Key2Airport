#include "configmanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpression>

namespace {

const QRegularExpression kIdentifier(QStringLiteral("^[A-Za-z_][A-Za-z0-9_]*$"));

bool isIdentifier(const QString &value)
{
    return kIdentifier.match(value).hasMatch();
}

}

QString ConfigManager::configDirectoryPath()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("config"));
}

bool ConfigManager::ensureConfigDirectory()
{
    QDir directory;
    return directory.mkpath(configDirectoryPath());
}

QStringList ConfigManager::configurationNames()
{
    QDir directory(configDirectoryPath());
    const QFileInfoList files = directory.entryInfoList({QStringLiteral("*.json")},
                                                         QDir::Files | QDir::Readable,
                                                         QDir::Name | QDir::IgnoreCase);

    QStringList names;
    for (const QFileInfo &file : files)
        names.append(file.completeBaseName());
    return names;
}

QString ConfigManager::configurationPath(const QString &name)
{
    return QDir(configDirectoryPath()).filePath(name + QStringLiteral(".json"));
}

bool ConfigManager::createDefaultConfiguration(const QString &aircraft, const QString &path)
{
    QJsonObject root;
    root.insert(QStringLiteral("AIRCRAFT"), aircraft);
    root.insert(QStringLiteral("RA_CHECKBOX"), true);
    root.insert(QStringLiteral("LR_CHECKBOX"), true);
    root.insert(QStringLiteral("PAGESWITCH"), QStringLiteral("-"));
    root.insert(QStringLiteral("VARIABLES"), QJsonArray{});
    root.insert(QStringLiteral("PAGE_1"), QJsonObject{});
    root.insert(QStringLiteral("PAGE_2"), QJsonObject{});

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;

    const QByteArray data = QJsonDocument(root).toJson(QJsonDocument::Indented);
    return file.write(data) == data.size();
}

bool ConfigManager::loadConfiguration(const QString &path, AircraftConfiguration &configuration)
{
    configuration = AircraftConfiguration();

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
        return false;

    const QJsonObject root = document.object();
    const QStringList requiredFields = {
        QStringLiteral("AIRCRAFT"), QStringLiteral("RA_CHECKBOX"),
        QStringLiteral("LR_CHECKBOX"), QStringLiteral("PAGESWITCH"),
        QStringLiteral("VARIABLES"),
        QStringLiteral("PAGE_1"), QStringLiteral("PAGE_2")};
    for (const QString &field : requiredFields) {
        if (!root.contains(field))
            return false;
    }

    if (!root.value(QStringLiteral("AIRCRAFT")).isString() ||
        !root.value(QStringLiteral("RA_CHECKBOX")).isBool() ||
        !root.value(QStringLiteral("LR_CHECKBOX")).isBool() ||
        !root.value(QStringLiteral("PAGESWITCH")).isString() ||
        !root.value(QStringLiteral("VARIABLES")).isArray() ||
        !root.value(QStringLiteral("PAGE_1")).isObject() ||
        !root.value(QStringLiteral("PAGE_2")).isObject()) {
        return false;
    }

    QHash<QString, QString> variables;
    if (!parseVariables(root.value(QStringLiteral("VARIABLES")).toArray(), variables))
        return false;

    ConfigPage page1;
    ConfigPage page2;
    if (!parsePage(root.value(QStringLiteral("PAGE_1")).toObject(), page1) ||
        !parsePage(root.value(QStringLiteral("PAGE_2")).toObject(), page2)) {
        return false;
    }

    configuration.aircraft = root.value(QStringLiteral("AIRCRAFT")).toString();
    configuration.radioHeight = root.value(QStringLiteral("RA_CHECKBOX")).toBool();
    configuration.landingRate = root.value(QStringLiteral("LR_CHECKBOX")).toBool();
    configuration.pageSwitch = root.value(QStringLiteral("PAGESWITCH")).toString();
    configuration.variables = variables;
    configuration.pages[0] = page1;
    configuration.pages[1] = page2;
    configuration.valid = true;
    return true;
}

bool ConfigManager::parsePage(const QJsonObject &object, ConfigPage &page)
{
    page = ConfigPage();
    if (object.isEmpty())
        return true;

    for (auto it = object.constBegin(); it != object.constEnd(); ++it) {
        if (it.key() == QStringLiteral("VARIABLES"))
            return false;
        if (!it.value().isArray())
            continue;

        QList<ConfigRule> rules;
        if (parseRules(it.value().toArray(), rules) && !rules.isEmpty())
            page.bindings.insert(it.key(), rules);
    }

    return true;
}

bool ConfigManager::parseVariables(const QJsonArray &array, QHash<QString, QString> &variables)
{
    variables.clear();
    for (const QJsonValue &value : array) {
        if (!value.isObject())
            return false;

        const QJsonObject variable = value.toObject();
        if (variable.size() != 1)
            return false;

        const QString name = variable.constBegin().key();
        const QJsonValue hashValue = variable.constBegin().value();
        if (!isIdentifier(name) || !hashValue.isString())
            return false;
        if (variables.contains(name))
            continue;

        variables.insert(name, hashValue.toString());
    }
    return true;
}

bool ConfigManager::parseRules(const QJsonArray &array, QList<ConfigRule> &rules)
{
    rules.clear();

    for (const QJsonValue &value : array) {
        if (!value.isObject())
            continue;

        const QJsonObject object = value.toObject();
        const QJsonValue thenValue = object.value(QStringLiteral("THEN"));
        if (!thenValue.isString())
            continue;

        ConfigRule rule;
        rule.thenExpression = thenValue.toString();

        if (object.contains(QStringLiteral("IF"))) {
            const QJsonValue ifValue = object.value(QStringLiteral("IF"));
            if (!ifValue.isString() || ifValue.toString().trimmed().isEmpty())
                continue;
            rule.hasIf = true;
            rule.ifExpression = ifValue.toString();
        }

        if (object.contains(QStringLiteral("ELSE"))) {
            if (!rule.hasIf || !object.value(QStringLiteral("ELSE")).isString())
                continue;
            rule.hasElse = true;
            rule.elseExpression = object.value(QStringLiteral("ELSE")).toString();
        }

        rules.append(rule);
    }

    return true;
}
