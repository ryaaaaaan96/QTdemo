#include "connection_worker.h"
#include <QThread>

namespace industrial
{
ConnectionWorker::ConnectionWorker(ConnectionConfig connection, QVector<SessionConfig> sessions,
                                   QVector<SourceConfig> sources, quint64 generation,
                                   std::shared_ptr<SampleMailbox> mailbox, SessionFactory factory)
    : m_connection(std::move(connection)), m_specs(std::move(sessions)),
      m_sources(std::move(sources)), m_generation(generation), m_mailbox(std::move(mailbox)),
      m_factory(std::move(factory))
{
}
void ConnectionWorker::start()
{
    Q_ASSERT(thread() == QThread::currentThread());
    if (m_running)
        return;
    m_running = true;
    m_deadlineTimer = new QTimer(this);
    m_deadlineTimer->setSingleShot(true);
    m_deadlineTimer->setTimerType(Qt::PreciseTimer);
    connect(m_deadlineTimer, &QTimer::timeout, this,
            [this]
            {
                if (!m_active)
                    return;
                const auto request = *m_active;
                m_sessions.value(request.sessionId)->abandon(request.requestId);
                finish({request.requestId, CommandOutcome::Unknown,
                        "Acknowledgement timeout; not replayed"});
            });
    for (const auto &spec : m_specs)
    {
        QVector<SourceConfig> sources;
        for (const auto &source : m_sources)
            if (source.sessionId == spec.id)
                sources.append(source);
        auto *session = m_factory(spec, sources, m_generation, m_connection.responseDelayMs, this);
        m_sessions.insert(spec.id, session);
        connect(session, &ProtocolSession::samplesReady, this,
                [this](const SampleBatch &batch)
                {
                    if (m_mailbox->publish(batch))
                        emit samplesAvailable();
                });
        connect(session, &ProtocolSession::operationFinished, this, &ConnectionWorker::finish);
    }
    emit online();
    for (auto *session : std::as_const(m_sessions))
        session->start();
}
void ConnectionWorker::submit(const CommandRequest &request)
{
    if (!m_running || request.generation != m_generation ||
        !m_sessions.contains(request.sessionId) || m_stoppedSessions.contains(request.sessionId))
    {
        emit commandFinished({request.requestId, CommandOutcome::Rejected, "Session unavailable"});
        return;
    }
    if (m_queue.size() + (m_active ? 1 : 0) >= 32)
    {
        emit commandFinished({request.requestId, CommandOutcome::Rejected, "QueueFull"});
        return;
    }
    m_queue.enqueue(request);
    pump();
}
void ConnectionWorker::pump()
{
    if (!m_running || m_active)
        return;
    while (!m_queue.isEmpty())
    {
        const auto request = m_queue.dequeue();
        const auto remaining = request.deadlineMonoMs - monotonicMs();
        if (remaining <= 0)
        {
            emit commandFinished(
                {request.requestId, CommandOutcome::Expired, "Queue deadline exceeded"});
            continue;
        }
        m_active = request;
        m_deadlineTimer->start(int(qMin<qint64>(remaining, 60000)));
        m_sessions.value(request.sessionId)->submit(request);
        return;
    }
}
void ConnectionWorker::finish(const CommandResult &result)
{
    if (!m_active || m_active->requestId != result.requestId)
        return;
    m_deadlineTimer->stop();
    m_active.reset();
    emit commandFinished(result);
    QTimer::singleShot(0, this, &ConnectionWorker::pump);
}
void ConnectionWorker::stopSession(const QString &sessionId)
{
    if (!m_sessions.contains(sessionId) || m_stoppedSessions.contains(sessionId))
        return;
    m_stoppedSessions.insert(sessionId);
    m_sessions.value(sessionId)->stop();
    if (m_active && m_active->sessionId == sessionId)
        finish({m_active->requestId, CommandOutcome::Unknown, "Session stopped after submission"});
    QQueue<CommandRequest> remaining;
    while (!m_queue.isEmpty())
    {
        const auto request = m_queue.dequeue();
        if (request.sessionId == sessionId)
            emit commandFinished({request.requestId, CommandOutcome::Cancelled,
                                  "Session stopped before submission"});
        else
            remaining.enqueue(request);
    }
    m_queue.swap(remaining);
}
void ConnectionWorker::stop()
{
    if (!m_running)
    {
        emit stopped();
        return;
    }
    m_running = false;
    for (const auto &id : m_sessions.keys())
        stopSession(id);
    emit stopped();
}
} // namespace industrial
