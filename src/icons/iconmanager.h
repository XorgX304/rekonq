/* ============================================================
*
* This file is a part of the rekonq project
*
* Copyright (C) 2010-2014 by Andrea Diamantini <adjam7 at gmail dot com>
*
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License or (at your option) version 3 or any later version
* accepted by the membership of KDE e.V. (or its successor approved
* by the membership of KDE e.V.), which shall act as a proxy
* defined in Section 14 of version 3 of the license.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
* ============================================================ */


#ifndef ICON_MANAGER_H
#define ICON_MANAGER_H

// Rekonq Includes
#include "rekonq_defines.h"

// Qt Includes
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>

// Forward Declarations
class KIcon;
class QWebFrame;


class IconManager : public QObject
{
    Q_OBJECT

public:
    /**
     * Entry point.
     * Access to IconManager class by using
     * IconManager::self()->thePublicMethodYouNeed()
     */
    static IconManager *self();

    KIcon iconForUrl(const QUrl &url);
    
    QString iconPathForUrl(const QUrl &url);

    void clearIconCache();

    void saveDesktopIconForUrl(const QUrl &u);

    // Engine ToolBar needed method
    KIcon engineFavicon(const QUrl &);
    
private:
    IconManager(QObject *parent = 0);

    QString _faviconsDir;
    QString _tempIconsDir;

    QStringList _engineFaviconHosts;

    static QPointer<IconManager> s_iconManager;
};


#endif // ICON_MANAGER_H
