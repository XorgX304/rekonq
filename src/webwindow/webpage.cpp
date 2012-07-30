/* ============================================================
*
* This file is a part of the rekonq project
*
* Copyright (C) 2008 Benjamin C. Meyer <ben@meyerhome.net>
* Copyright (C) 2008 Dirk Mueller <mueller@kde.org>
* Copyright (C) 2008 Urs Wolfer <uwolfer @ kde.org>
* Copyright (C) 2008 Michael Howell <mhowell123@gmail.com>
* Copyright (C) 2008-2012 by Andrea Diamantini <adjam7 at gmail dot com>
* Copyright (C) 2010 by Matthieu Gicquel <matgic78 at gmail dot com>
* Copyright (C) 2009-2010 Dawit Alemayehu <adawit at kde dot org>
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


// Self Includes
#include "webpage.h"
#include "webpage.moc"

#include "networkaccessmanager.h"


WebPage::WebPage(QWidget *parent)
    : KWebPage(parent, KWalletIntegration)
{
    // rekonq Network Manager
    NetworkAccessManager *manager = new NetworkAccessManager(this);

    // set network reply object to emit readyRead when it receives meta data
    manager->setEmitReadyReadOnMetaDataChange(true);

    // disable QtWebKit cache to just use KIO one..
    manager->setCache(0);

    // set cookieJar window..
    if (parent && parent->window())
        manager->setWindow(parent->window());

    setNetworkAccessManager(manager);
}


WebPage::~WebPage()
{
}


WebPage *WebPage::createWindow(QWebPage::WebWindowType type)
{
    // added to manage web modal dialogs
    if (type == QWebPage::WebModalDialog)
        kDebug() << "Modal Dialog";

    WebPage* p = new WebPage;
    emit pageCreated(p);
    return p;
}