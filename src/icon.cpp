#include <QFile>
#include <QDir>
#include <QDebug>
#include <QIcon>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QTextStream>
#include "icon.h"
#include "logging.h"

namespace IconLookup {

// Cache for appId -> iconPath mappings
static QHash<QString, QString> s_cache;

QString lookup(const QString &appId)
{
    auto it = s_cache.constFind(appId);
    if (it != s_cache.constEnd()) {
      return *it;
    }

    QString result;

    QString desktopFile = Internal::findDesktopFile(appId);
    if (desktopFile.isEmpty()) {
        qCDebug(niriLog) << "No desktop file found for app ID:" << appId;
        // Try fallback: direct icon theme lookup using the appId
        qCDebug(niriLog) << "Attempting fallback icon lookup for:" << appId;
        result = Internal::findIconInTheme(appId);
        if (!result.isEmpty()) {
            qCDebug(niriLog) << "Found fallback icon for" << appId << ":" << result;
        } else {
            qCDebug(niriLog) << "No fallback icon found for" << appId;
        }
        s_cache[appId] = result;
        return result;
    }

    qCDebug(niriLog) << "Found desktop file for" << appId << ":" << desktopFile;

    // Parse the Icon= field from desktop file
    QString iconValue = Internal::parseIconFromDesktopFile(desktopFile);
    if (iconValue.isEmpty()) {
        qCDebug(niriLog) << "No Icon field found in desktop file:" << desktopFile;
        s_cache[appId] = result;
        return result;
    }

    qCDebug(niriLog) << "Icon value from desktop file:" << iconValue;

    QFileInfo desktopFileInfo(desktopFile);
    QString desktopDir = desktopFileInfo.absolutePath();
    result = Internal::resolveIconPath(iconValue, desktopDir);

    if (!result.isEmpty()) {
        qCDebug(niriLog) << "Resolved icon path for" << appId << ":" << result;
    } else {
        qCDebug(niriLog) << "Could not resolve icon path for" << appId;
    }

    s_cache[appId] = result;
    return result;
}

void clearCache()
{
    s_cache.clear();
}

namespace Internal {

QStringList getXdgDataDirs()
{
    QStringList dirs;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    QString xdgDataHome = env.value("XDG_DATA_HOME");
    if (xdgDataHome.isEmpty()) {
        xdgDataHome = QDir::homePath() + "/.local/share";
    }
    dirs.append(xdgDataHome);

    QString xdgDataDirs = env.value("XDG_DATA_DIRS", "/usr/local/share:/usr/share");
    dirs.append(xdgDataDirs.split(':', Qt::SkipEmptyParts));

    dirs.removeDuplicates();

    return dirs;
}

QStringList getXdgIconDirs()
{
    QStringList iconDirs;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    QString legacyIcons = QDir::homePath() + "/.icons";
    if (QDir(legacyIcons).exists()) {
        iconDirs.append(legacyIcons);
    }

    QString xdgDataHome = env.value("XDG_DATA_HOME");
    if (xdgDataHome.isEmpty()) {
        xdgDataHome = QDir::homePath() + "/.local/share";
    }
    QString userIcons = xdgDataHome + "/icons";
    if (QDir(userIcons).exists()) {
        iconDirs.append(userIcons);
    }

    QString xdgDataDirs = env.value("XDG_DATA_DIRS", "/usr/local/share:/usr/share");
    for (const QString &dir : xdgDataDirs.split(':', Qt::SkipEmptyParts)) {
        QString iconDir = dir + "/icons";
        if (QDir(iconDir).exists() && !iconDirs.contains(iconDir)) {
            iconDirs.append(iconDir);
        }
    }

    for (const QString &dir : xdgDataDirs.split(':', Qt::SkipEmptyParts)) {
        QString pixmapsDir = dir + "/pixmaps";
        if (QDir(pixmapsDir).exists() && !iconDirs.contains(pixmapsDir)) {
            iconDirs.append(pixmapsDir);
        }
    }

    return iconDirs;
}

QString findDesktopFile(const QString &appId)
{
    QStringList dataDirs = getXdgDataDirs();

    // Search patterns in priority order
    QStringList patterns = {
        appId + ".desktop",
        appId.toLower() + ".desktop",
        "*" + appId.toLower() + "*.desktop"
    };

    QStringList prefixes = {
        "applications/",
        "applications/kde/",
        "applications/org.kde."
    };

    QStringList candidates;

    for (const QString &dir : dataDirs) {
        for (const QString &prefix : prefixes) {
            QString searchDir = dir + "/" + prefix;
            QDir directory(searchDir);

            if (!directory.exists()) continue;

            for (const QString &pattern : patterns) {
                if (!pattern.contains('*')) {
                    QString path = directory.absoluteFilePath(pattern);
                    if (QFileInfo(path).isFile()) {
                        candidates.append(path);
                    }
                } else {
                    QStringList matches = directory.entryList(
                        {pattern}, QDir::Files, QDir::Name
                    );
                    for (const QString &match : matches) {
                        candidates.append(directory.absoluteFilePath(match));
                    }
                }
            }
        }
    }

    for (const QString &candidate : candidates) {
        QString iconValue = parseIconFromDesktopFile(candidate);
        if (!iconValue.isEmpty()) {
            return candidate;
        }
    }

    return QString();
}

QString parseIconFromDesktopFile(const QString &desktopFilePath)
{
    QFile file(desktopFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(niriLog) << "Failed to open desktop file:" << desktopFilePath;
        return QString();
    }

    QTextStream in(&file);
    bool inDesktopEntry = false;
    QString iconValue;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        if (line == "[Desktop Entry]") {
            inDesktopEntry = true;
            continue;
        }

        // If we hit another section, stop looking
        if (line.startsWith('[') && line.endsWith(']')) {
            if (inDesktopEntry) {
                break;
            }
            continue;
        }

        // Look for Icon= key in Desktop Entry section
        if (inDesktopEntry && line.startsWith("Icon=")) {
            iconValue = line.mid(5).trimmed();
            break;
        }
    }

    file.close();
    return iconValue;
}

QString resolveIconPath(const QString &iconValue, const QString &desktopFileDir)
{
    if (iconValue.isEmpty()) {
        return QString();
    }

    if (iconValue.startsWith('/')) {
        QFileInfo fileInfo(iconValue);
        if (fileInfo.isFile()) {
            return fileInfo.absoluteFilePath();
        }
        return QString();
    }

    // If it contains '/' but doesn't start with it, make it absolute relative to desktop file
    if (iconValue.contains('/')) {
        QString absolutePath = desktopFileDir + "/" + iconValue;
        QFileInfo fileInfo(absolutePath);
        if (fileInfo.isFile()) {
            return fileInfo.canonicalFilePath();
        }
        return QString();
    }

    // Try to find the icon in theme directories
    return findIconInTheme(iconValue);
}

QString findIconInTheme(const QString &iconName)
{
    QStringList iconDirs = getXdgIconDirs();
    QStringList themes;

    // Current system icon theme first
    QString currentTheme = QIcon::themeName();
    if (!currentTheme.isEmpty()) {
        themes.append(currentTheme);
    }

    // Fallback themes
    themes.append({"hicolor", "breeze", "Adwaita", "gnome", "oxygen", "Papirus"});
    themes.removeDuplicates();

    // Common sizes to try (prefer larger icons)
    QStringList sizes = {"scalable", "512x512", "256x256", "128x128", "96x96",
                         "64x64", "48x48", "32x32", "24x24", "16x16"};

    // Common contexts
    QStringList contexts = {"apps", "applications", "mimetypes", "places", "devices"};

    // Common extensions
    QStringList extensions = {".svg", ".png", ".xpm"};

    // Icon name variants (case-insensitive)
    QStringList iconVariants = {
        iconName,
        iconName.toLower(),
        iconName.left(1).toLower() + iconName.mid(1)
    };

    auto checkPath = [](const QString &path) -> QString {
        QFileInfo info(path);
        return info.isFile() ? info.absoluteFilePath() : QString();
    };

    // Search pattern: iconDir/theme/size/context/iconName.ext
    for (const QString &iconDir : iconDirs) {
        for (const QString &theme : themes) {
            for (const QString &size : sizes) {
                for (const QString &context : contexts) {
                    for (const QString &variant : iconVariants) {
                        for (const QString &ext : extensions) {
                            QString result = checkPath(QString("%1/%2/%3/%4/%5%6")
                                .arg(iconDir, theme, size, context, variant, ext));
                            if (!result.isEmpty()) return result;
                        }
                    }
                }
            }
        }

        // Direct pixmaps check
        if (iconDir.endsWith("/pixmaps")) {
            for (const QString &variant : iconVariants) {
                for (const QString &ext : extensions) {
                    QString result = checkPath(iconDir + "/" + variant + ext);
                    if (!result.isEmpty()) return result;
                }
            }
        }
    }

    return QString();
}

} // namespace Internal
} // namespace IconLookup
