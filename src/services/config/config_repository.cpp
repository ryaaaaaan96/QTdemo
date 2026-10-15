#include "config_repository.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <cmath>

namespace industrial
{
namespace
{
class Reader
{
  public:
    QStringList errors;
    void error(const QString &path, const QString &message)
    {
        errors.append(path + ": " + message);
    }
    void keys(const QJsonObject &obj, const QStringList &allowed, const QString &path)
    {
        for (auto it = obj.begin(); it != obj.end(); ++it)
            if (!allowed.contains(it.key()))
                error(path + "." + it.key(), "unsupported field");
    }
    QString id(const QJsonObject &obj, const QString &key, const QString &path)
    {
        static const QRegularExpression pattern("^[A-Za-z0-9_-]{1,64}$");
        const auto value = obj.value(key);
        if (!value.isString() || !pattern.match(value.toString()).hasMatch())
            error(path + "." + key, "expected identifier: 1..64 letters, digits, _ or -");
        return value.toString();
    }
    double number(const QJsonObject &obj, const QString &key, double fallback, double minimum,
                  double maximum, const QString &path, bool integer = false)
    {
        if (!obj.contains(key))
            return fallback;
        const auto value = obj.value(key);
        const double n = value.toDouble();
        if (!value.isDouble() || !std::isfinite(n) || n < minimum || n > maximum ||
            (integer && std::floor(n) != n))
        {
            error(path + "." + key, "invalid numeric value or range");
            return fallback;
        }
        return n;
    }
    QJsonArray array(const QJsonObject &obj, const QString &key, const QString &path)
    {
        if (!obj.value(key).isArray() || obj.value(key).toArray().isEmpty())
            error(path + "." + key, "expected non-empty array");
        return obj.value(key).toArray();
    }
    void unique(QSet<QString> &ids, const QString &id, const QString &path)
    {
        if (ids.contains(id))
            error(path, "duplicate identifier");
        ids.insert(id);
    }
};
} // namespace

ConfigResult ConfigRepository::load(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
        return {{}, {file.errorString()}};
    if (file.size() > 1024 * 1024)
        return {{}, {"Configuration exceeds 1 MiB"}};
    return parse(file.readAll());
}

ConfigResult ConfigRepository::parse(const QByteArray &json)
{
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
        return {{}, {"Invalid JSON object: " + parseError.errorString()}};
    Reader r;
    ProjectConfig config;
    const auto root = document.object();
    r.keys(root, {"schemaVersion", "connections", "sessions", "sources"}, "root");
    if (root.value("schemaVersion") != QJsonValue(1))
        r.error("schemaVersion", "expected 1");
    QSet<QString> connectionIds, sessionIds, sourceIds;
    const auto connections = r.array(root, "connections", "root");
    for (qsizetype i = 0; i < connections.size(); ++i)
    {
        const QString path = QString("connections[%1]").arg(i);
        const auto obj = connections[i].toObject();
        r.keys(obj, {"id", "backendKind", "responseDelayMs"}, path);
        ConnectionConfig c;
        c.id = r.id(obj, "id", path);
        r.unique(connectionIds, c.id, path + ".id");
        if (obj.value("backendKind").toString() != "simulated")
            r.error(path + ".backendKind",
                    "only simulated is implemented; hardware is never emulated implicitly");
        c.responseDelayMs = int(r.number(obj, "responseDelayMs", 60, 1, 10000, path, true));
        config.connections.append(c);
    }
    const auto sessions = r.array(root, "sessions", "root");
    for (qsizetype i = 0; i < sessions.size(); ++i)
    {
        const QString path = QString("sessions[%1]").arg(i);
        const auto obj = sessions[i].toObject();
        r.keys(obj, {"id", "connectionId", "protocolType"}, path);
        SessionConfig s;
        s.id = r.id(obj, "id", path);
        s.connectionId = r.id(obj, "connectionId", path);
        s.protocolType = obj.value("protocolType").toString();
        r.unique(sessionIds, s.id, path + ".id");
        if (!connectionIds.contains(s.connectionId))
            r.error(path + ".connectionId", "unknown connection");
        if (s.protocolType != "simulated_counter" && s.protocolType != "simulated_constant")
            r.error(path + ".protocolType", "unsupported protocol type");
        config.sessions.append(s);
    }
    const auto sources = r.array(root, "sources", "root");
    qsizetype pointCount = 0;
    for (qsizetype i = 0; i < sources.size(); ++i)
    {
        const QString path = QString("sources[%1]").arg(i);
        const auto obj = sources[i].toObject();
        r.keys(obj, {"id", "sessionId", "intervalMs", "points"}, path);
        SourceConfig s;
        s.id = r.id(obj, "id", path);
        s.sessionId = r.id(obj, "sessionId", path);
        r.unique(sourceIds, s.id, path + ".id");
        if (!sessionIds.contains(s.sessionId))
            r.error(path + ".sessionId", "unknown session");
        s.intervalMs = int(r.number(obj, "intervalMs", 250, 10, 60000, path, true));
        const auto points = r.array(obj, "points", path);
        QSet<QString> pointIds;
        for (qsizetype j = 0; j < points.size(); ++j)
        {
            const auto p = points[j].toObject();
            const QString pp = path + QString(".points[%1]").arg(j);
            r.keys(p, {"id", "name", "unit", "access", "minimum", "maximum", "initial", "step"},
                   pp);
            PointDefinition point;
            point.id = r.id(p, "id", pp);
            r.unique(pointIds, point.id, pp + ".id");
            for (const auto &key : {"name", "unit"})
                if (p.contains(key) && !p.value(key).isString())
                    r.error(pp + "." + key, "expected string");
            point.name = p.value("name").toString(point.id);
            point.unit = p.value("unit").toString();
            const auto access = p.value("access").toString("read");
            if ((p.contains("access") && !p.value("access").isString()) ||
                (access != "read" && access != "read_write"))
                r.error(pp + ".access", "expected read or read_write");
            point.writable = access == "read_write";
            point.minimum = r.number(p, "minimum", 0, -1e9, 1e9, pp);
            point.maximum = r.number(p, "maximum", 65535, -1e9, 1e9, pp);
            if (point.minimum > point.maximum)
                r.error(pp, "minimum exceeds maximum");
            point.initial = r.number(p, "initial", point.minimum, point.minimum, point.maximum, pp);
            point.step = r.number(p, "step", 0, -1e6, 1e6, pp);
            s.points.append(point);
            ++pointCount;
        }
        config.sources.append(s);
    }
    // Allocation limits for this prototype, not device capacity/performance guarantees.
    if (config.connections.size() > 32 || config.sessions.size() > 128 || pointCount > 4096)
        r.error("root", "prototype limit: 32 connections, 128 sessions, 4096 points");
    if (!r.errors.isEmpty())
        return {{}, r.errors};
    return {config, {}};
}
} // namespace industrial
