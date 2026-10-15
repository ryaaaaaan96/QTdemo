#include "data_hub.h"
#include <QThread>

namespace industrial
{
DataHub::DataHub(QObject *parent) : QObject(parent)
{
    m_staleTimer.setInterval(250);
    connect(&m_staleTimer, &QTimer::timeout, this,
            [this]
            {
                bool dirty = false;
                const auto now = monotonicMs();
                for (auto &sample : m_samples)
                    if (sample.quality == Quality::Good &&
                        now - sample.receivedMonoMs > m_staleAfterMs.value(sample.key.sourceId))
                    {
                        sample.quality = Quality::Stale;
                        dirty = true;
                    }
                if (dirty)
                    emit changed();
            });
    m_staleTimer.start();
}
void DataHub::configure(const ProjectConfig &config)
{
    Q_ASSERT(thread() == QThread::currentThread());
    m_samples.clear();
    m_generations.clear();
    m_staleAfterMs.clear();
    for (const auto &source : config.sources)
    {
        m_staleAfterMs.insert(source.id, qMax(1000, source.intervalMs * 3));
        for (const auto &point : source.points)
        {
            DataSample sample;
            sample.key = {source.id, point.id};
            m_samples.insert(sample.key, sample);
        }
    }
    emit changed();
}
void DataHub::activateSource(const QString &sourceId, quint64 generation)
{
    invalidateSource(sourceId);
    m_generations.insert(sourceId, generation);
}
void DataHub::invalidateSource(const QString &sourceId)
{
    m_generations.remove(sourceId);
    for (auto &sample : m_samples)
        if (sample.key.sourceId == sourceId && sample.quality != Quality::NoData)
            sample.quality = Quality::Stale;
    emit changed();
}
void DataHub::applyBatch(const SampleBatch &batch)
{
    Q_ASSERT(thread() == QThread::currentThread());
    bool dirty = false;
    for (const auto &sample : batch)
    {
        auto it = m_samples.find(sample.key);
        if (it == m_samples.end() || sample.generation == 0 ||
            m_generations.value(sample.key.sourceId) != sample.generation)
            continue;
        if (it->generation == sample.generation && it->sequence >= sample.sequence)
            continue;
        *it = sample;
        if (monotonicMs() - sample.receivedMonoMs > m_staleAfterMs.value(sample.key.sourceId))
            it->quality = Quality::Stale;
        dirty = true;
    }
    if (dirty)
        emit changed();
}
} // namespace industrial
