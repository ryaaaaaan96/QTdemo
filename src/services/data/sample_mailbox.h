#pragma once
#include "core/types.h"
#include <QMutex>
#include <QMutexLocker>
#include <QSet>

namespace industrial
{
// The only shared mutable data between an I/O thread and the GUI thread.
// One entry per configured point, and at most one outstanding wakeup.
class SampleMailbox
{
  public:
    explicit SampleMailbox(QSet<PointKey> allowed) : m_allowed(std::move(allowed)) {}
    bool publish(const SampleBatch &batch)
    {
        QMutexLocker lock(&m_mutex);
        for (const auto &sample : batch)
        {
            if (!m_allowed.contains(sample.key))
                continue;
            if (m_latest.contains(sample.key))
                ++m_coalesced;
            m_latest.insert(sample.key, sample);
        }
        if (m_notified || m_latest.isEmpty())
            return false;
        m_notified = true;
        return true;
    }
    SampleBatch take()
    {
        QMutexLocker lock(&m_mutex);
        SampleBatch batch;
        batch.reserve(m_latest.size());
        for (const auto &sample : std::as_const(m_latest))
            batch.append(sample);
        m_latest.clear();
        m_notified = false;
        return batch;
    }
    quint64 coalesced() const
    {
        QMutexLocker lock(&m_mutex);
        return m_coalesced;
    }

  private:
    const QSet<PointKey> m_allowed;
    mutable QMutex m_mutex;
    QHash<PointKey, DataSample> m_latest;
    bool m_notified = false;
    quint64 m_coalesced = 0;
};
} // namespace industrial
