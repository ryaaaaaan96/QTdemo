#include "greeting.h"

QString app::greeting(const QString &name)
{
    const auto trimmed = name.trimmed();
    return trimmed.isEmpty() ? QStringLiteral("Hello, Qt!")
                             : QStringLiteral("Hello, %1!").arg(trimmed);
}
