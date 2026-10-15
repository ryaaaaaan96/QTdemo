#include "point_table_model.h"
#include <QDateTime>

namespace industrial
{
PointTableModel::PointTableModel(DataHub &hub, QObject *parent)
    : QAbstractTableModel(parent), m_hub(hub)
{
    connect(&hub, &DataHub::changed, this,
            [this]
            {
                if (!m_rows.isEmpty())
                    emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
            });
}
void PointTableModel::configure(const ProjectConfig &config)
{
    beginResetModel();
    m_rows.clear();
    for (const auto &source : config.sources)
        for (const auto &point : source.points)
            m_rows.append({{source.id, point.id}, point});
    endResetModel();
}
int PointTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}
int PointTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 6;
}
QVariant PointTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return {};
    const auto &row = m_rows[index.row()];
    const auto sample = m_hub.sample(row.key);
    const QString value =
        sample.quality == Quality::NoData ? "—" : QString::number(sample.value, 'f', 2);
    const QString quality = sample.quality == Quality::Good    ? QStringLiteral("有效")
                            : sample.quality == Quality::Stale ? QStringLiteral("已失效")
                                                               : QStringLiteral("无数据");
    switch (role)
    {
    case SourceRole:
        return row.key.sourceId;
    case NameRole:
        return row.definition.name;
    case ValueRole:
        return value;
    case UnitRole:
        return row.definition.unit;
    case QualityRole:
        return quality;
    case Qt::DisplayRole:
        switch (index.column())
        {
        case 0:
            return row.key.sourceId;
        case 1:
            return row.definition.name +
                   (row.definition.writable ? QStringLiteral(" · 可写") : QString());
        case 2:
            return value;
        case 3:
            return row.definition.unit;
        case 4:
            return quality;
        case 5:
            return sample.receivedAtMs ? QDateTime::fromMSecsSinceEpoch(sample.receivedAtMs)
                                             .toString("HH:mm:ss.zzz")
                                       : "—";
        }
        break;
    }
    return {};
}
QVariant PointTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    const QStringList titles = {QStringLiteral("数据源"), QStringLiteral("点位"),
                                QStringLiteral("数值"),   QStringLiteral("单位"),
                                QStringLiteral("质量"),   QStringLiteral("接收时间")};
    return titles.value(section);
}
QHash<int, QByteArray> PointTableModel::roleNames() const
{
    return {{SourceRole, "sourceName"},
            {NameRole, "pointName"},
            {ValueRole, "valueText"},
            {UnitRole, "unitText"},
            {QualityRole, "qualityText"}};
}
PointKey PointTableModel::keyAt(int row) const
{
    return row >= 0 && row < m_rows.size() ? m_rows[row].key : PointKey{};
}
PointDefinition PointTableModel::definitionAt(int row) const
{
    return row >= 0 && row < m_rows.size() ? m_rows[row].definition : PointDefinition{};
}
} // namespace industrial
