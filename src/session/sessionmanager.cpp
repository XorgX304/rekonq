/* ============================================================
*
* This file is a part of the rekonq project
*
* Copyright (C) 2009-2012 by Andrea Diamantini <adjam7 at gmail dot com>
* Copyright (C) 2009 by Yoram Bar-Haim <<yoram.b at zend dot com>
* Copyright (C) 2009-2011 by Lionel Chauvin <megabigbug@yahoo.fr>
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
#include "sessionmanager.h"
#include "sessionmanager.moc"

// Local Includes
#include "application.h"
#include "autosaver.h"
#include "tabhistory.h"
#include "tabwindow.h"
#include "webwindow.h"
#include "webpage.h"

// KDE Includes
#include <KStandardDirs>
#include <KUrl>

// Qt Includes
#include <QFile>
#include <QDomDocument>


// Only used internally
bool readSessionDocument(QDomDocument & document, const QString & sessionFilePath)
{
    QFile sessionFile(sessionFilePath);

    if (!sessionFile.exists())
        return false;

    if (!sessionFile.open(QFile::ReadOnly))
    {
        kDebug() << "Unable to open session file" << sessionFile.fileName();
        return false;
    }

    if (!document.setContent(&sessionFile, false))
    {
        kDebug() << "Unable to parse session file" << sessionFile.fileName();
        return false;
    }

    return true;
}


int loadTabs(TabWindow *tw, QDomElement & window, bool useFirstTab)
{
    int currentTab = 0;

    for (unsigned int tabNo = 0; tabNo < window.elementsByTagName("tab").length(); tabNo++)
    {
        QDomElement tab = window.elementsByTagName("tab").at(tabNo).toElement();
        if (tab.hasAttribute("currentTab"))
            currentTab = tabNo;

        KUrl u = KUrl(tab.attribute("url"));

        TabHistory tabHistory;
        tabHistory.title = tab.attribute("title");
        tabHistory.url = tab.attribute("url");
        QDomCDATASection historySection = tab.firstChild().toCDATASection();
        tabHistory.history = QByteArray::fromBase64(historySection.data().toAscii());

        if (tabNo == 0 && useFirstTab)
        {
            tw->loadUrl(u, Rekonq::CurrentTab, &tabHistory);
        }
        else
        {
            tw->loadUrl(u, Rekonq::NewTab, &tabHistory);
        }
    }

    return currentTab;
}


// -------------------------------------------------------------------------------------------------


QWeakPointer<SessionManager> SessionManager::s_sessionManager;


SessionManager *SessionManager::self()
{
    if (s_sessionManager.isNull())
    {
        s_sessionManager = new SessionManager(qApp);
    }
    return s_sessionManager.data();
}


// ----------------------------------------------------------------------------------------------


SessionManager::SessionManager(QObject *parent)
    : QObject(parent)
    , m_safe(true)
    , m_isSessionEnabled(false)
    , m_saveTimer(new AutoSaver(this))
{
    // AutoSaver. Save your hd from frying...
    connect(m_saveTimer, SIGNAL(saveNeeded()), this, SLOT(save()));

    m_sessionFilePath = KStandardDirs::locateLocal("appdata" , "session");
}


void SessionManager::saveSession()
{
    if (!m_isSessionEnabled)
        return;
    
    m_saveTimer->changeOccurred();
}


void SessionManager::save()
{
    if (!m_isSessionEnabled || !m_safe)
        return;

    m_safe = false;

    kDebug() << "SAVING SESSION...";

    QFile sessionFile(m_sessionFilePath);
    if (!sessionFile.open(QFile::WriteOnly | QFile::Truncate))
    {
        kDebug() << "Unable to open session file" << sessionFile.fileName();
        return;
    }
    TabWindowList wl = rApp->tabWindowList();
    QDomDocument document("session");
    QDomElement session = document.createElement("session");
    document.appendChild(session);

    Q_FOREACH(const QWeakPointer<TabWindow> &w, wl)
    {
        QDomElement window = document.createElement("window");
        int tabInserted = 0;

        window.setAttribute("name", w.data()->objectName());

        for (signed int tabNo = 0; tabNo < w.data()->count(); tabNo++)
        {
            KUrl u = w.data()->webWindow(tabNo)->url();

            tabInserted++;
            QDomElement tab = document.createElement("tab");
            tab.setAttribute("title", w.data()->webWindow(tabNo)->title()); // redundant, but needed for closedSites()
            // as there's not way to read out the historyData
            tab.setAttribute("url", u.url());
            if (w.data()->currentIndex() == tabNo)
            {
                tab.setAttribute("currentTab", 1);
            }
            QByteArray history;
            QDataStream historyStream(&history, QIODevice::ReadWrite);
            historyStream << *(w.data()->webWindow(tabNo)->page()->history());
            QDomCDATASection historySection = document.createCDATASection(history.toBase64());

            tab.appendChild(historySection);
            window.appendChild(tab);
        }
        if (tabInserted > 0)
            session.appendChild(window);
    }

    QTextStream TextStream(&sessionFile);
    document.save(TextStream, 2);
    sessionFile.close();

    m_safe = true;
    return;
}


bool SessionManager::restoreSessionFromScratch()
{
    QDomDocument document("session");

    if (!readSessionDocument(document, m_sessionFilePath))
        return false;

    for (unsigned int winNo = 0; winNo < document.elementsByTagName("window").length(); winNo++)
    {
        QDomElement window = document.elementsByTagName("window").at(winNo).toElement();

        TabWindow *tw = rApp->newTabWindow();

        int currentTab = loadTabs(tw, window, false);

        tw->setCurrentIndex(currentTab);
    }

    return true;
}


void SessionManager::restoreCrashedSession()
{
    QDomDocument document("session");

    if (!readSessionDocument(document, m_sessionFilePath))
        return;

    for (unsigned int winNo = 0; winNo < document.elementsByTagName("window").length(); winNo++)
    {
        QDomElement window = document.elementsByTagName("window").at(winNo).toElement();

        TabWindow *tw = (winNo == 0)
            ? rApp->tabWindow()
            : rApp->newTabWindow();

        KUrl u = tw->currentWebWindow()->url();
        bool useCurrentTab = (u.isEmpty() || u.protocol() == QL1S("about"));
        int currentTab = loadTabs(tw, window, useCurrentTab);

        tw->setCurrentIndex(currentTab);
    }

    setSessionManagementEnabled(true);
}


int SessionManager::restoreSavedSession()
{
    QDomDocument document("session");

    if (!readSessionDocument(document, m_sessionFilePath))
        return 0;

    unsigned int winNo;

    for (winNo = 0; winNo < document.elementsByTagName("window").length(); winNo++)
    {
        QDomElement window = document.elementsByTagName("window").at(winNo).toElement();

        TabWindow *tw = rApp->newTabWindow();

        int currentTab = loadTabs(tw, window, true);

        tw->setCurrentIndex(currentTab);
    }

    return winNo;
}


bool SessionManager::restoreTabWindow(TabWindow* window)
{
    QDomDocument document("session");

    if (!readSessionDocument(document, m_sessionFilePath))
        return false;

    unsigned int winNo;

    for (winNo = 0; winNo < document.elementsByTagName("window").length(); winNo++)
    {
        QDomElement savedWindowElement = document.elementsByTagName("window").at(winNo).toElement();

        if (window->objectName() != savedWindowElement.attribute("name", ""))
            continue;

        int currentTab = loadTabs(window, savedWindowElement, false);

        window->setCurrentIndex(currentTab);

        return true;
    }

    return false;
}


QList<TabHistory> SessionManager::closedSites()
{
    QList<TabHistory> list;
    QDomDocument document("session");

    if (!readSessionDocument(document, m_sessionFilePath))
        return list;

    for (unsigned int tabNo = 0; tabNo < document.elementsByTagName("tab").length(); tabNo++)
    {
        QDomElement tab = document.elementsByTagName("tab").at(tabNo).toElement();

        TabHistory tabHistory;

        tabHistory.title = tab.attribute("title");
        tabHistory.url = tab.attribute("url");

        QDomCDATASection historySection = tab.firstChild().toCDATASection();
        tabHistory.history = QByteArray::fromBase64(historySection.data().toAscii());

        list << tabHistory;
    }

    return list;
}