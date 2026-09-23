#include "configexecutor.h"

#include "simconnectclient.h"

#include <QMetaObject>
#include <QRegularExpression>
#include <QStringList>

#include <cmath>

namespace {

enum class TokenType { Identifier, Number, Operator, LeftParen, RightParen, End, Invalid };

struct Token
{
    TokenType type = TokenType::Invalid;
    QString text;
    double number = 0.0;
};

bool isIdentifierStart(QChar character)
{
    return (character.isLetter() && character.unicode() < 128) || character == QLatin1Char('_');
}

bool isIdentifierPart(QChar character)
{
    return (character.isLetterOrNumber() && character.unicode() < 128) ||
           character == QLatin1Char('_');
}

bool tokenize(const QString &text, QList<Token> &tokens)
{
    tokens.clear();
    int position = 0;

    while (position < text.size()) {
        const QChar character = text.at(position);
        if (character.isSpace()) {
            ++position;
            continue;
        }

        if (isIdentifierStart(character)) {
            const int start = position++;
            while (position < text.size() && isIdentifierPart(text.at(position)))
                ++position;
            tokens.append({TokenType::Identifier, text.mid(start, position - start), 0.0});
            continue;
        }

        const bool signedNumber = (character == QLatin1Char('-') || character == QLatin1Char('+')) &&
                                  position + 1 < text.size() &&
                                  (text.at(position + 1).isDigit() ||
                                   text.at(position + 1) == QLatin1Char('.'));
        if (character.isDigit() || character == QLatin1Char('.') || signedNumber) {
            const int start = position;
            if (signedNumber)
                ++position;
            bool hasDigits = false;
            while (position < text.size() && text.at(position).isDigit()) {
                hasDigits = true;
                ++position;
            }
            if (position < text.size() && text.at(position) == QLatin1Char('.')) {
                ++position;
                while (position < text.size() && text.at(position).isDigit()) {
                    hasDigits = true;
                    ++position;
                }
            }
            if (!hasDigits)
                return false;
            if (position < text.size() && (text.at(position) == QLatin1Char('e') ||
                                           text.at(position) == QLatin1Char('E'))) {
                ++position;
                if (position < text.size() && (text.at(position) == QLatin1Char('-') ||
                                               text.at(position) == QLatin1Char('+')))
                    ++position;
                const int exponentStart = position;
                while (position < text.size() && text.at(position).isDigit())
                    ++position;
                if (exponentStart == position)
                    return false;
            }

            bool ok = false;
            const QString numberText = text.mid(start, position - start);
            const double number = numberText.toDouble(&ok);
            if (!ok || !std::isfinite(number))
                return false;
            tokens.append({TokenType::Number, numberText, number});
            continue;
        }

        if (character == QLatin1Char('(')) {
            tokens.append({TokenType::LeftParen, QString(), 0.0});
            ++position;
            continue;
        }
        if (character == QLatin1Char(')')) {
            tokens.append({TokenType::RightParen, QString(), 0.0});
            ++position;
            continue;
        }

        const QString twoCharacters = text.mid(position, 2);
        if (twoCharacters == QStringLiteral("==") || twoCharacters == QStringLiteral(">=") ||
            twoCharacters == QStringLiteral("<=") || twoCharacters == QStringLiteral("&&") ||
            twoCharacters == QStringLiteral("||")) {
            tokens.append({TokenType::Operator, twoCharacters, 0.0});
            position += 2;
            continue;
        }
        if (character == QLatin1Char('>') || character == QLatin1Char('<')) {
            tokens.append({TokenType::Operator, QString(character), 0.0});
            ++position;
            continue;
        }

        return false;
    }

    tokens.append({TokenType::End, QString(), 0.0});
    return true;
}

}

class ConfigExecutor::ExpressionParser
{
public:
    explicit ExpressionParser(const QList<Token> &tokens) : m_tokens(tokens) {}

    bool parse(ConfigExecutor::ExpressionPtr &expression)
    {
        expression = parseOr();
        return expression && current().type == TokenType::End;
    }

private:
    const Token &current() const { return m_tokens.at(m_position); }

    std::shared_ptr<ConfigExecutor::ExpressionNode> parseOr()
    {
        auto expression = parseAnd();
        while (expression && current().type == TokenType::Operator &&
               current().text == QStringLiteral("||")) {
            ++m_position;
            auto right = parseAnd();
            if (!right)
                return nullptr;
            auto combined = std::make_shared<ConfigExecutor::ExpressionNode>();
            combined->type = ConfigExecutor::ExpressionNode::Or;
            combined->left = expression;
            combined->right = right;
            expression = combined;
        }
        return expression;
    }

    std::shared_ptr<ConfigExecutor::ExpressionNode> parseAnd()
    {
        auto expression = parsePrimary();
        while (expression && current().type == TokenType::Operator &&
               current().text == QStringLiteral("&&")) {
            ++m_position;
            auto right = parsePrimary();
            if (!right)
                return nullptr;
            auto combined = std::make_shared<ConfigExecutor::ExpressionNode>();
            combined->type = ConfigExecutor::ExpressionNode::And;
            combined->left = expression;
            combined->right = right;
            expression = combined;
        }
        return expression;
    }

    std::shared_ptr<ConfigExecutor::ExpressionNode> parsePrimary()
    {
        if (current().type == TokenType::LeftParen) {
            ++m_position;
            auto expression = parseOr();
            if (!expression || current().type != TokenType::RightParen)
                return nullptr;
            ++m_position;
            return expression;
        }

        if (current().type != TokenType::Identifier && current().type != TokenType::Number)
            return nullptr;
        const Token left = current();
        ++m_position;
        if (current().type != TokenType::Operator ||
            (current().text != QStringLiteral("==") && current().text != QStringLiteral(">=") &&
             current().text != QStringLiteral(">") && current().text != QStringLiteral("<=") &&
             current().text != QStringLiteral("<")))
            return nullptr;
        const QString comparator = current().text;
        ++m_position;
        if (current().type != TokenType::Identifier && current().type != TokenType::Number)
            return nullptr;
        const Token right = current();
        ++m_position;

        auto expression = std::make_shared<ConfigExecutor::ExpressionNode>();
        expression->type = ConfigExecutor::ExpressionNode::Compare;
        expression->comparator = comparator;
        expression->leftOperand.variable = left.type == TokenType::Identifier;
        expression->leftOperand.name = left.text;
        expression->leftOperand.value = left.number;
        expression->rightOperand.variable = right.type == TokenType::Identifier;
        expression->rightOperand.name = right.text;
        expression->rightOperand.value = right.number;
        return expression;
    }

    const QList<Token> &m_tokens;
    int m_position = 0;
};

ConfigExecutor::ConfigExecutor(SimConnectClient *simClient, QObject *parent)
    : QObject(parent), m_simClient(simClient)
{
}

void ConfigExecutor::setConfiguration(const AircraftConfiguration &configuration)
{
    clear();
    if (!configuration.valid)
        return;

    m_configuration = configuration;
    m_active = true;
    m_page = 0;
    emit pageChanged(1);
}

void ConfigExecutor::clear()
{
    ++m_nextExecutionId;
    m_active = false;
    m_busy = false;
    m_page = 0;
    m_ruleIndex = 0;
    m_assignmentIndex = 0;
    m_rules.clear();
    m_assignments.clear();
    m_waitingValue = false;
    m_waitingSet = false;
    m_pendingOperandCallback = nullptr;
    m_configuration = AircraftConfiguration();
    emit pageChanged(0);
}

void ConfigExecutor::handleKeyPressed(const QString &name)
{
    if (!m_active || m_busy)
        return;

    if (!m_configuration.pageSwitch.isEmpty() && name == m_configuration.pageSwitch) {
        m_page = 1 - m_page;
        emit pageChanged(m_page + 1);
        return;
    }

    const auto binding = m_configuration.pages[m_page].bindings.constFind(name);
    if (binding == m_configuration.pages[m_page].bindings.constEnd() || binding->isEmpty())
        return;

    m_rules = binding.value();
    m_ruleIndex = 0;
    m_assignments.clear();
    m_assignmentIndex = 0;
    m_busy = true;
    m_executionId = m_nextExecutionId++;
    if (m_executionId == 0)
        m_executionId = m_nextExecutionId++;
    startRule();
}

void ConfigExecutor::startRule()
{
    if (!m_busy)
        return;
    if (m_ruleIndex >= m_rules.size()) {
        finishExecution();
        return;
    }

    const ConfigRule &rule = m_rules.at(m_ruleIndex);
    QList<Assignment> thenAssignments;
    QList<Assignment> elseAssignments;
    if (!parseAssignments(rule.thenExpression, thenAssignments) ||
        (rule.hasElse && !parseAssignments(rule.elseExpression, elseAssignments))) {
        skipRule();
        return;
    }

    if (!rule.hasIf) {
        beginAssignments(thenAssignments);
        return;
    }

    ExpressionPtr expression;
    if (!parseExpression(rule.ifExpression, expression)) {
        skipRule();
        return;
    }

    evaluateNode(expression, [this, thenAssignments, elseAssignments, rule](bool success, bool result) {
        if (!m_busy)
            return;
        if (!success) {
            finishExecution();
            return;
        }
        if (result)
            beginAssignments(thenAssignments);
        else if (rule.hasElse)
            beginAssignments(elseAssignments);
        else
            skipRule();
    });
}

void ConfigExecutor::skipRule()
{
    if (!m_busy)
        return;
    ++m_ruleIndex;
    startRule();
}

void ConfigExecutor::beginAssignments(const QList<Assignment> &assignments)
{
    m_assignments = assignments;
    m_assignmentIndex = 0;
    sendNextAssignment();
}

void ConfigExecutor::sendNextAssignment()
{
    if (!m_busy)
        return;
    if (m_assignmentIndex >= m_assignments.size()) {
        ++m_ruleIndex;
        startRule();
        return;
    }

    const Assignment &assignment = m_assignments.at(m_assignmentIndex);
    const auto variable = m_configuration.variables.constFind(assignment.variable);
    if (variable == m_configuration.variables.constEnd()) {
        finishExecution();
        return;
    }

    bool hashOk = false;
    const quint64 hash = variable.value().toULongLong(&hashOk, 10);
    if (!hashOk) {
        finishExecution();
        return;
    }

    m_waitingSet = true;
    invokeSet(hash, assignment.value);
}

void ConfigExecutor::evaluateNode(const ExpressionPtr &node, const EvaluationCallback &callback)
{
    if (!node) {
        callback(false, false);
        return;
    }

    if (node->type == ExpressionNode::And) {
        evaluateNode(node->left, [this, node, callback](bool success, bool value) {
            if (!success || !value) {
                callback(success, value);
                return;
            }
            evaluateNode(node->right, callback);
        });
        return;
    }

    if (node->type == ExpressionNode::Or) {
        evaluateNode(node->left, [this, node, callback](bool success, bool value) {
            if (!success || value) {
                callback(success, value);
                return;
            }
            evaluateNode(node->right, callback);
        });
        return;
    }

    evaluateOperand(node->leftOperand, [this, node, callback](bool success, double left) {
        if (!success) {
            callback(false, false);
            return;
        }
        evaluateOperand(node->rightOperand, [node, callback, left](bool rightSuccess, double right) {
            if (!rightSuccess) {
                callback(false, false);
                return;
            }

            bool result = false;
            if (node->comparator == QStringLiteral("==")) result = left == right;
            else if (node->comparator == QStringLiteral(">=")) result = left >= right;
            else if (node->comparator == QStringLiteral(">")) result = left > right;
            else if (node->comparator == QStringLiteral("<=")) result = left <= right;
            else if (node->comparator == QStringLiteral("<")) result = left < right;
            else {
                callback(false, false);
                return;
            }
            callback(true, result);
        });
    });
}

void ConfigExecutor::evaluateOperand(const Operand &operand, const OperandCallback &callback)
{
    if (!operand.variable) {
        callback(true, operand.value);
        return;
    }

    const auto variable = m_configuration.variables.constFind(operand.name);
    if (variable == m_configuration.variables.constEnd()) {
        callback(false, 0.0);
        return;
    }

    bool hashOk = false;
    const quint64 hash = variable.value().toULongLong(&hashOk, 10);
    if (!hashOk) {
        callback(false, 0.0);
        return;
    }

    m_waitingValue = true;
    m_pendingOperandCallback = callback;
    invokeGet(hash);
}

void ConfigExecutor::finishExecution()
{
    m_busy = false;
    m_waitingValue = false;
    m_waitingSet = false;
    m_pendingOperandCallback = nullptr;
    m_rules.clear();
    m_assignments.clear();
    ++m_nextExecutionId;
}

void ConfigExecutor::invokeGet(quint64 hash)
{
    if (!m_simClient ||
        !QMetaObject::invokeMethod(m_simClient, "getInputEventForConfig",
                                    Qt::QueuedConnection, Q_ARG(quint64, m_executionId),
                                    Q_ARG(quint64, hash))) {
        const OperandCallback callback = m_pendingOperandCallback;
        m_pendingOperandCallback = nullptr;
        m_waitingValue = false;
        if (callback)
            callback(false, 0.0);
    }
}

void ConfigExecutor::invokeSet(quint64 hash, double value)
{
    if (!m_simClient ||
        !QMetaObject::invokeMethod(m_simClient, "sendInputEventForConfig",
                                   Qt::QueuedConnection, Q_ARG(quint64, m_executionId),
                                   Q_ARG(quint64, hash), Q_ARG(double, value))) {
        finishExecution();
    }
}

void ConfigExecutor::inputEventValueReceived(quint64 executionId, bool success, double value)
{
    if (!m_busy || !m_waitingValue || executionId != m_executionId)
        return;

    m_waitingValue = false;
    const OperandCallback callback = m_pendingOperandCallback;
    m_pendingOperandCallback = nullptr;
    if (callback)
        callback(success && std::isfinite(value), value);
}

void ConfigExecutor::inputEventSetFinished(quint64 executionId, bool success)
{
    if (!m_busy || !m_waitingSet || executionId != m_executionId)
        return;

    m_waitingSet = false;
    if (!success) {
        finishExecution();
        return;
    }

    ++m_assignmentIndex;
    sendNextAssignment();
}

bool ConfigExecutor::parseExpression(const QString &text, ExpressionPtr &expression) const
{
    QList<Token> tokens;
    if (!tokenize(text, tokens))
        return false;
    ExpressionParser parser(tokens);
    return parser.parse(expression);
}

bool ConfigExecutor::parseAssignments(const QString &text, QList<Assignment> &assignments) const
{
    assignments.clear();
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty())
        return true;

    QStringList statements = trimmed.split(QLatin1Char(';'), Qt::KeepEmptyParts);
    if (!statements.isEmpty() && statements.last().trimmed().isEmpty())
        statements.removeLast();

    static const QRegularExpression assignmentPattern(
        QStringLiteral("^\\s*([A-Za-z_][A-Za-z0-9_]*)\\s*=\\s*"
                       "([+-]?(?:[0-9]+(?:\\.[0-9]*)?|\\.[0-9]+)(?:[eE][+-]?[0-9]+)?)\\s*$"));

    for (const QString &statement : statements) {
        const QRegularExpressionMatch match = assignmentPattern.match(statement);
        if (!match.hasMatch())
            return false;
        bool ok = false;
        const double value = match.captured(2).toDouble(&ok);
        if (!ok || !std::isfinite(value))
            return false;
        assignments.append({match.captured(1), value});
    }
    return true;
}
