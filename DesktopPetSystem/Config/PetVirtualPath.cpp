#pragma execution_character_set("utf-8")

#include "PetVirtualPath.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace PetVirtualPath {

namespace {

QString cleanRelativeSlashes(QString s)
{
    s.replace(QLatin1Char('\\'), QLatin1Char('/'));
    while (s.startsWith(QLatin1Char('/')))
        s = s.mid(1);
    return s;
}

QString stripVirtualPrefix(QString p)
{
    if (p.startsWith(QStringLiteral("pet://"), Qt::CaseInsensitive)) {
        p = p.mid(6);
    } else if (p.startsWith(QStringLiteral("pet:/"), Qt::CaseInsensitive)) {
        p = p.mid(5);
    }
    return cleanRelativeSlashes(p);
}

} // namespace

QString schemePrefix()
{
    return QStringLiteral("pet:/");
}

bool isVirtualPath(const QString& path)
{
    const QString p = path.trimmed();
    return p.startsWith(QStringLiteral("pet:/"), Qt::CaseInsensitive)
        || p.startsWith(QStringLiteral("pet://"), Qt::CaseInsensitive);
}

QString toVirtual(const QString& relativePathFromProjectRoot)
{
    QString rel = cleanRelativeSlashes(relativePathFromProjectRoot);
    if (rel.startsWith(QStringLiteral("pet:/"), Qt::CaseInsensitive)
        || rel.startsWith(QStringLiteral("pet://"), Qt::CaseInsensitive)) {
        rel = stripVirtualPrefix(rel);
    }
    return schemePrefix() + rel;
}

QString resolveToAbsolute(const QString& path, const QString& projectRoot)
{
    const QString root = QDir::cleanPath(QFileInfo(projectRoot).absoluteFilePath());
    const QString p = path.trimmed();
    if (p.isEmpty()) {
        return {};
    }
    if (QDir::isAbsolutePath(p)) {
        return QDir::cleanPath(QFileInfo(p).absoluteFilePath());
    }
    const QString rel = isVirtualPath(p) ? stripVirtualPrefix(p) : cleanRelativeSlashes(p);
    return QDir::cleanPath(QDir(root).absoluteFilePath(rel));
}

QString findProjectRootFromExe()
{
    QDir dir(QCoreApplication::applicationDirPath());
    for (int i = 0; i < 12; ++i) {
        const QString candidate = dir.absoluteFilePath(QStringLiteral("resources/config/pet_config.json"));
        if (QFile::exists(candidate)) {
            return dir.absolutePath();
        }
        if (!dir.cdUp()) {
            break;
        }
    }
    return QCoreApplication::applicationDirPath();
}

QString normalizeConfigurablePath(const QString& raw)
{
    const QString t = raw.trimmed();
    if (t.isEmpty()) {
        return {};
    }
    if (QDir::isAbsolutePath(t)) {
        return QDir::cleanPath(QFileInfo(t).absoluteFilePath());
    }
    if (isVirtualPath(t)) {
        return toVirtual(t);
    }
    return toVirtual(t);
}

} // namespace PetVirtualPath
