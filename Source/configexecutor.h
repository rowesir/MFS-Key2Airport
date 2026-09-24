#ifndef CONFIGEXECUTOR_H
#define CONFIGEXECUTOR_H

#include <QObject>

#include <functional>
#include <memory>

#include "configmanager.h"

class SimConnectClient;

class ConfigExecutor : public QObject
{
    Q_OBJECT

public:
    explicit ConfigExecutor(SimConnectClient *simClient, QObject *parent = nullptr);

    void setConfiguration(const AircraftConfiguration &configuration);
    void clear();
    void handleKeyPressed(const QString &name);

    void inputEventValueReceived(quint64 executionId, bool success, double value);
    void inputEventSetFinished(quint64 executionId, bool success);

signals:
    void pageChanged(int page);

private:
    struct Operand
    {
        bool variable = false;
        QString name;
        double value = 0.0;
    };

    struct ExpressionNode
    {
        enum Type { Compare, And, Or } type = Compare;
        Operand leftOperand;
        Operand rightOperand;
        QString comparator;
        std::shared_ptr<ExpressionNode> left;
        std::shared_ptr<ExpressionNode> right;
    };

    struct Assignment
    {
        QString variable;
        double value = 0.0;
    };

    using ExpressionPtr = std::shared_ptr<ExpressionNode>;
    using EvaluationCallback = std::function<void(bool, bool)>;
    using OperandCallback = std::function<void(bool, double)>;

    class ExpressionParser;

    bool parseExpression(const QString &text, ExpressionPtr &expression) const;
    bool parseAssignments(const QString &text, QList<Assignment> &assignments) const;

    void startRule();
    void skipRule();
    void beginAssignments(const QList<Assignment> &assignments);
    void sendNextAssignment();
    void evaluateNode(const ExpressionPtr &node, const EvaluationCallback &callback);
    void evaluateOperand(const Operand &operand, const OperandCallback &callback);
    void finishExecution();
    void invokeGet(quint64 hash);
    void invokeSet(quint64 hash, double value);

    SimConnectClient *m_simClient = nullptr;
    AircraftConfiguration m_configuration;
    bool m_active = false;
    bool m_busy = false;
    int m_page = 0;
    int m_ruleIndex = 0;
    int m_assignmentIndex = 0;
    QList<ConfigRule> m_rules;
    QList<Assignment> m_assignments;
    quint64 m_executionId = 0;
    quint64 m_nextExecutionId = 1;
    bool m_waitingValue = false;
    bool m_waitingSet = false;
    OperandCallback m_pendingOperandCallback;
};

#endif // CONFIGEXECUTOR_H
