#include "runtime_manager.h"
#include "connection_worker.h"

namespace industrial
{
RuntimeManager::RuntimeManager(DataHub &hub, SessionFactory factory, QObject *parent)
    : QObject(parent), m_hub(hub), m_factory(std::move(factory))
{
}
RuntimeManager::~RuntimeManager()
{
    // Emergency RAII cleanup for callers that skip the normal asynchronous shutdown.
    // Worker::stopped -> QThread::quit is direct, so no GUI event is required to finish.
    stop();
    for (const auto &entry : std::as_const(m_entries))
        entry.thread->wait();
}
bool RuntimeManager::configure(const ProjectConfig &config)
{
    if (isRunning())
        return false;
    m_config = config;
    m_sourceSessions.clear();
    m_sessionConnections.clear();
    m_stoppedSessions.clear();
    for (const auto &session : config.sessions)
        m_sessionConnections.insert(session.id, session.connectionId);
    for (const auto &source : config.sources)
        m_sourceSessions.insert(source.id, source.sessionId);
    m_hub.configure(config);
    return true;
}
void RuntimeManager::start()
{
    for (const auto &connection : m_config.connections)
    {
        if (m_entries.contains(connection.id))
            continue;
        const quint64 generation = ++m_nextGeneration;
        QVector<SessionConfig> sessions;
        QVector<SourceConfig> sources;
        QSet<PointKey> allowed;
        for (const auto &session : m_config.sessions)
            if (session.connectionId == connection.id)
            {
                sessions.append(session);
                m_stoppedSessions.remove(session.id);
            }
        for (const auto &source : m_config.sources)
            if (connectionForSource(source.id) == connection.id)
            {
                sources.append(source);
                m_hub.activateSource(source.id, generation);
                for (const auto &point : source.points)
                    allowed.insert({source.id, point.id});
            }
        auto mailbox = std::make_shared<SampleMailbox>(allowed);
        auto *thread = new QThread(this);
        thread->setObjectName("io_" + connection.id);
        auto *worker =
            new ConnectionWorker(connection, sessions, sources, generation, mailbox, m_factory);
        worker->moveToThread(thread);
        m_entries.insert(connection.id, {thread, worker, generation, false, false});
        connect(thread, &QThread::started, worker, &ConnectionWorker::start);
        connect(worker, &ConnectionWorker::stopped, thread, &QThread::quit, Qt::DirectConnection);
        connect(thread, &QThread::finished, worker, &QObject::deleteLater);
        connect(worker, &ConnectionWorker::online, this,
                [this, id = connection.id, generation]
                {
                    auto it = m_entries.find(id);
                    if (it == m_entries.end() || it->generation != generation || it->stopping)
                        return;
                    it->online = true;
                    emit connectionStateChanged(id, "Online");
                });
        connect(worker, &ConnectionWorker::samplesAvailable, this,
                [this, mailbox] { m_hub.applyBatch(mailbox->take()); });
        connect(worker, &ConnectionWorker::commandFinished, this, &RuntimeManager::commandFinished);
        connect(thread, &QThread::finished, this,
                [this, id = connection.id, thread]
                {
                    m_entries.remove(id);
                    thread->deleteLater();
                    emit connectionStateChanged(id, "Stopped");
                    if (m_entries.isEmpty())
                        emit allStopped();
                });
        emit connectionStateChanged(connection.id, "Starting");
        thread->start();
    }
}
QString RuntimeManager::connectionForSource(const QString &sourceId) const
{
    return m_sessionConnections.value(m_sourceSessions.value(sourceId));
}
bool RuntimeManager::sourceAvailable(const QString &sourceId) const
{
    if (!m_sourceSessions.contains(sourceId) ||
        m_stoppedSessions.contains(m_sourceSessions.value(sourceId)))
        return false;
    const auto entry = m_entries.value(connectionForSource(sourceId));
    return entry.online && !entry.stopping;
}
bool RuntimeManager::dispatch(CommandRequest request)
{
    if (!sourceAvailable(request.key.sourceId))
        return false;
    const auto entry = m_entries.value(connectionForSource(request.key.sourceId));
    request.sessionId = m_sourceSessions.value(request.key.sourceId);
    request.generation = entry.generation;
    QMetaObject::invokeMethod(
        entry.worker, [worker = entry.worker, request] { worker->submit(request); },
        Qt::QueuedConnection);
    return true;
}
void RuntimeManager::stopSession(const QString &sessionId)
{
    const auto entry = m_entries.value(m_sessionConnections.value(sessionId));
    if (!entry.worker || entry.stopping || m_stoppedSessions.contains(sessionId))
        return;
    m_stoppedSessions.insert(sessionId);
    for (const auto &source : m_config.sources)
        if (source.sessionId == sessionId)
            m_hub.invalidateSource(source.id);
    QMetaObject::invokeMethod(
        entry.worker, [worker = entry.worker, sessionId] { worker->stopSession(sessionId); },
        Qt::QueuedConnection);
    emit sessionStopped(sessionId);
}
void RuntimeManager::stopConnection(const QString &connectionId)
{
    auto it = m_entries.find(connectionId);
    if (it == m_entries.end() || it->stopping)
        return;
    it->stopping = true;
    it->online = false;
    for (const auto &source : m_config.sources)
        if (connectionForSource(source.id) == connectionId)
            m_hub.invalidateSource(source.id);
    auto *worker = it->worker;
    QMetaObject::invokeMethod(
        worker, [worker] { worker->stop(); }, Qt::QueuedConnection);
    emit connectionStateChanged(connectionId, "Stopping");
}
void RuntimeManager::stop()
{
    for (const auto &id : m_entries.keys())
        stopConnection(id);
}
} // namespace industrial
