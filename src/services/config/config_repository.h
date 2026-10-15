#pragma once
#include "core/types.h"
#include <QByteArray>
#include <QStringList>
#include <optional>

namespace industrial
{
struct ConfigResult
{
    std::optional<ProjectConfig> config;
    QStringList errors;
};
class ConfigRepository
{
  public:
    static ConfigResult parse(const QByteArray &json);
    static ConfigResult load(const QString &fileName);
};
} // namespace industrial
