#pragma once

#include <QString>
#include <QHash>

namespace IconLookup {
    /**
     * Look up the icon path for an application ID.
     * This function caches results for performance.
     *
     * @param appId The application ID (e.g., "firefox", "org.gnome.Nautilus")
     * @return Absolute path to the icon file, or empty string if not found
     */
    QString lookup(const QString &appId);

    /**
     * Clear the internal cache.
     * Useful for testing or if icon theme changes at runtime.
     */
    void clearCache();

namespace Internal {
    // Internal functions exposed for testing purposes
    QString findDesktopFile(const QString &appId);
    QString parseIconFromDesktopFile(const QString &desktopFilePath);
    QString resolveIconPath(const QString &iconValue, const QString &desktopFileDir);
    QString findIconInTheme(const QString &iconName);
    QStringList getXdgDataDirs();
    QStringList getXdgIconDirs();
}

} // namespace IconLookup
