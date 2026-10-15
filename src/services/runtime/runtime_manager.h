#pragma once
#include "core/protocol_session.h"
#include "core/types.h"
#include "services/data/data_hub.h"
#include <QSet>
#include <QThread>

namespace industrial
{
class ConnectionWorker;
class RuntimeManager final : public QObject
{
    Q_OBJECT
  public:
    RuntimeManager(DataHub &hub, SessionFactory factory, QObject *parent = nullptr);
    ~RuntimeManager() override;
    bool configure(const ProjectConfig &config);
    const ProjectConfig &config() const { return m_config; }
    bool isRunning() const { return !m_entries.isEmpty(); }
    void start();
    void stop();
    void stopConnection(const QString &connectionId);
    void stopSession(const QString &sessionId);
    bool dispatch(CommandRequest request);
    bool sourceAvailable(const QString &sourceId) const;
  signals:
    void connectionStateChanged(const QString &connectionId, const QString &state);
    void sessionStopped(const QString &sessionId);
    void commandFinished(const industrial::CommandResult &result);
    void allStopped();

  private:
    struct Entry
    {
        QThread *thread = nullptr;
        ConnectionWorker *worker = nullptr;
        quint64 generation = 0;
        bool online = false;
        bool stopping = false;
    };
    QString connectionForSource(const QString &sourceId) const;
    DataHub &m_hub;
    SessionFactory m_factory;
    ProjectConfig m_config;
    QHash<QString, Entry> m_entries;
    QHash<QString, QString> m_sourceSessions;
    QHash<QString, QString> m_sessionConnections;
    QSet<QString> m_stoppedSessions;
    quint64 m_nextGeneration = 0;
};
} // namespace industrial
