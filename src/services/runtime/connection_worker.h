#pragma once
#include "core/protocol_session.h"
#include "services/data/sample_mailbox.h"
#include <QQueue>
#include <QSet>
#include <QTimer>
#include <memory>
#include <optional>

namespace industrial
{
class ConnectionWorker final : public QObject
{
    Q_OBJECT
  public:
    ConnectionWorker(ConnectionConfig connection, QVector<SessionConfig> sessions,
                     QVector<SourceConfig> sources, quint64 generation,
                     std::shared_ptr<SampleMailbox> mailbox, SessionFactory factory);
    void start();
    void submit(const CommandRequest &request);
    void stopSession(const QString &sessionId);
    void stop();
  signals:
    void online();
    void samplesAvailable();
    void commandFinished(const industrial::CommandResult &result);
    void stopped();

  private:
    void pump();
    void finish(const CommandResult &result);
    ConnectionConfig m_connection;
    QVector<SessionConfig> m_specs;
    QVector<SourceConfig> m_sources;
    quint64 m_generation;
    std::shared_ptr<SampleMailbox> m_mailbox;
    SessionFactory m_factory;
    QHash<QString, ProtocolSession *> m_sessions;
    QSet<QString> m_stoppedSessions;
    QQueue<CommandRequest> m_queue;
    std::optional<CommandRequest> m_active;
    QTimer *m_deadlineTimer = nullptr;
    bool m_running = false;
};
} // namespace industrial
