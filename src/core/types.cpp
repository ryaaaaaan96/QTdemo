#include "types.h"

namespace industrial
{
QString outcomeText(CommandOutcome outcome)
{
    switch (outcome)
    {
    case CommandOutcome::Confirmed:
        return QStringLiteral("已确认");
    case CommandOutcome::Rejected:
        return QStringLiteral("已拒绝");
    case CommandOutcome::Expired:
        return QStringLiteral("排队过期");
    case CommandOutcome::Cancelled:
        return QStringLiteral("已取消");
    case CommandOutcome::Unknown:
        return QStringLiteral("结果未知");
    }
    return {};
}
} // namespace industrial
