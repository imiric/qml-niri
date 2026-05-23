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
    QString result;

    if (appId.isEmpty()) return result;

    auto it = s_cache.constFind(appId);
    if (it != s_cache.constEnd()) {
      return *it;
    }

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
    static QStringList dirs;
    if (!dirs.isEmpty()) return dirs;

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    QString xdgDataHome = env.value(QStringLiteral("XDG_DATA_HOME"));
    if (xdgDataHome.isEmpty()) {
        xdgDataHome = QDir::homePath() + QStringLiteral("/.local/share");
    }
    dirs.append(xdgDataHome);

    QString xdgDataDirs = env.value(QStringLiteral("XDG_DATA_DIRS"),
                                    QStringLiteral("/usr/local/share:/usr/share"));
    dirs.append(xdgDataDirs.split(':', Qt::SkipEmptyParts));

    dirs.removeDuplicates();

    return dirs;
}

QStringList getXdgIconDirs()
{
    static QStringList iconDirs;
    if (!iconDirs.isEmpty()) return iconDirs;

    // Legacy icon location
    QString legacyIcons = QDir::homePath() + QStringLiteral("/.icons");
    if (QDir(legacyIcons).exists()) {
        iconDirs.append(legacyIcons);
    }

    const QStringList dataDirs = getXdgDataDirs();

    for (const QString &dir : dataDirs) {
        QString iconDir = dir + QStringLiteral("/icons");
        if (QDir(iconDir).exists()) {
            iconDirs.append(iconDir);
        }
    }

    for (const QString &dir : dataDirs) {
        QString pixmapsDir = dir + QStringLiteral("/pixmaps");
        if (QDir(pixmapsDir).exists()) {
            iconDirs.append(pixmapsDir);
        }
    }

    iconDirs.removeDuplicates();

    return iconDirs;
}

QString findDesktopFile(const QString &appId)
{
    QStringList dataDirs = getXdgDataDirs();

    // Search patterns in priority order
    QStringList patterns = {
        appId + QStringLiteral(".desktop"),
        appId.toLower() + QStringLiteral(".desktop"),
        QStringLiteral("*") + appId.toLower() + QStringLiteral("*.desktop")
    };

    static const QStringList prefixes = {
        QStringLiteral("applications/"),
        QStringLiteral("applications/kde/"),
        QStringLiteral("applications/org.kde.")
    };

    QStringList candidates;

    for (const QString &dir : dataDirs) {
        for (const QString &prefix : prefixes) {
            QString searchDir = dir + QStringLiteral("/") + prefix;
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

    if (!candidates.isEmpty()) {
        return candidates.first();
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

        if (line == QStringLiteral("[Desktop Entry]")) {
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
        if (inDesktopEntry && line.startsWith(QStringLiteral("Icon="))) {
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
        QString absolutePath = desktopFileDir + QStringLiteral("/") + iconValue;
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
    themes.append({
        QStringLiteral("hicolor"),
        QStringLiteral("breeze"),
        QStringLiteral("Adwaita"),
        QStringLiteral("gnome"),
        QStringLiteral("oxygen"),
        QStringLiteral("Papirus")
    });
    themes.removeDuplicates();

    // Common sizes to try (prefer larger icons)
    static const QStringList sizes = {
        QStringLiteral("scalable"),
        QStringLiteral("512x512"),
        QStringLiteral("256x256"),
        QStringLiteral("128x128"),
        QStringLiteral("96x96"),
        QStringLiteral("64x64"),
        QStringLiteral("48x48"),
        QStringLiteral("32x32"),
        QStringLiteral("24x24"),
        QStringLiteral("16x16")
    };

    // Common contexts
    static const QStringList contexts = {
        QStringLiteral("apps"),
        QStringLiteral("applications"),
        QStringLiteral("mimetypes"),
        QStringLiteral("places"),
        QStringLiteral("devices")
    };

    // Common extensions
    static const QStringList extensions = {
        QStringLiteral(".svg"),
        QStringLiteral(".png"),
        QStringLiteral(".xpm")
    };

    // Icon name variants (case-insensitive)
    QStringList iconVariants = {
        iconName,
        iconName.toLower(),
        iconName.left(1).toLower() + iconName.mid(1)
    };
    iconVariants.removeDuplicates();

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
                            QString result = checkPath(QStringLiteral("%1/%2/%3/%4/%5%6")
                                .arg(iconDir, theme, size, context, variant, ext));
                            if (!result.isEmpty()) return result;
                        }
                    }
                }
            }
        }

        // Direct pixmaps check
        if (iconDir.endsWith(QStringLiteral("/pixmaps"))) {
            for (const QString &variant : iconVariants) {
                for (const QString &ext : extensions) {
                    QString result = checkPath(iconDir + QStringLiteral("/") + variant + ext);
                    if (!result.isEmpty()) return result;
                }
            }
        }
    }

    return QString();
}

} // namespace Internal
} // namespace IconLookup
