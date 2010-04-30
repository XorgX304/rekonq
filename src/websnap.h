/* ============================================================
*
* This file is a part of the rekonq project
*
* Copyright (C) 2009 Nokia Corporation <qt-info@nokia.com>
* Copyright (C) 2009-2010 by Andrea Diamantini <adjam7 at gmail dot com>
* Copyright (C) 2010 by Matthieu Gicquel <matgic78 at gmail dot com>
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


#ifndef WEB_SNAP_H
#define WEB_SNAP_H


// Rekonq Includes
#include "rekonq_defines.h"

// KDE Includes
#include <KUrl>

// Qt Includes
#include <QtCore/QObject>
#include <QPixmap>
#include <QImage>
#include <QWebPage>

#define WIDTH  200
#define HEIGHT 150


/**
 * This class is used in many classes of rekonq to produce an image
 * based on the site corresponding to the url passed as argument.
 * It also cached the images to not retrieve them every time :)
 *
 * Heavily based on Graphics-Dojo WebSnap example (thanks!)
 *
 * We use this in the following rekonq classes:
 *
 * - TabBar class:          to show a tab preview                (given a page, you show AND save an image)
 * - PreviewSelector class: to save new favorite selection       (given a page, you show AND save an image)
 *
 * - NewTabPage class:      to show the favorites page "preview" (given an url, you show AND save an image)
 *
 */
class REKONQ_TESTS_EXPORT WebSnap : public QObject
{
    Q_OBJECT

public:
    /**
     * Creates a WebSnap object. It will load the url in one WebPage
     * and snap an image from it.
     *
     * @param url the url to load
     * @param parent the object parent
     */
    WebSnap(const KUrl &url, QObject *parent = 0);

    /**
     * Snaps a pixmap of size w * h from a page and save it to cache
     *
     * @param page the page to snap
     * @param w the image width
     * @param h the image height
     *
     * @return the pixmap snapped from the page
     */
    static QPixmap renderPreview(const QWebPage &page, int w = WIDTH, int h = HEIGHT);

    /**
     * Guess the local path where the image for the url provided
     * should be
     *
     * @param url the url to guess snap path
     *
     * @return the local path of the url snap
     */
    static QString imagePathFromUrl(const KUrl &url);

    /**
     * Determines if a snap exists for that url
     *
     */
    static bool existsImage(const KUrl &url);


private slots:
    void saveResult(bool ok = true);
    void load();

signals:
    void snapDone(bool ok);

private:
    QWebPage m_page;
    KUrl m_url;
};

#endif // WEB_SNAP_H
