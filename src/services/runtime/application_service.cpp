#include "application_service.h"
#include <QTimer>
#include <QUuid>
#include <cmath>

namespace industrial
{
ApplicationService::ApplicationService(RuntimeManager &runtime, QObject *parent)
    : QObject(parent), m_runtime(runtime)
{
    connect(&runtime, &RuntimeManager::commandFinished, this,
            [this](const CommandResult &result)
            {
                if (m_pending.remove(result.requestId))
                    emit commandFinished(result);
            });
}
QString ApplicationService::submitWrite(const PointKey &key, double value, int timeoutMs)
{
    const auto id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    auto reject = [this, id](const QString &detail)
    {
        QTimer::singleShot(0, this,
                           [this, id, detail] {
                               emit commandFinished({id, CommandOutcome::Rejected, detail});
                           });
        return id;
    };
    if (m_pending.size() >= 64)
        return reject("QueueFull");
    if (timeoutMs < 1 || timeoutMs > 60000)
        return reject("Invalid timeout");
    const PointDefinition *definition = nullptr;
    for (const auto &source : m_runtime.config().sources)
        if (source.id == key.sourceId)
            for (const auto &point : source.points)
                if (point.id == key.pointId)
                    definition = &point;
    if (!definition || !definition->writable)
        return reject("Unknown or read-only point");
    if (!std::isfinite(value) || value < definition->minimum || value > definition->maximum)
        return reject("Value outside configured range");
    if (!m_runtime.sourceAvailable(key.sourceId))
        return reject("Source is stopped or unavailable");
    m_pending.insert(id);
    CommandRequest request{id, key, {}, 0, monotonicMs() + timeoutMs, value};
    if (!m_runtime.dispatch(request))
    {
        m_pending.remove(id);
        return reject("Source is stopped or unavailable");
    }
    return id;
}
} // namespace industrial
