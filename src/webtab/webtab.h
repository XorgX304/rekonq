/* ============================================================
*
* This file is a part of the rekonq project
*
* Copyright (C) 2008-2012 by Andrea Diamantini <adjam7 at gmail dot com>
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


#ifndef WEBTAB_H
#define WEBTAB_H


// Rekonq Includes
#include "rekonq_defines.h"

// Local Includes
#include "webview.h"

// KDE Includes
#include <KParts/Part>

// Qt Includes
#include <QWidget>
#include <QWebInspector>
#include <QSplitter>

// Config
#include <config-kactivities.h>

// Forward Declarations
class PreviewSelectorBar;
class WalletBar;
class WebPage;

class WebWindow;

#ifdef HAVE_KACTIVITIES
namespace KActivities { class ResourceInstance; }
#endif


class REKONQ_TESTS_EXPORT WebTab : public QWidget
{
    Q_OBJECT

public:
    explicit WebTab(QWidget *parent = 0, bool isPrivateBrowsing = false);
    ~WebTab();

    WebView *view();
    WebPage *page();
    WebWindow *webWindow();

    inline int progress() const
    {
        return m_progress;
    }

    KUrl url();
    QString title();
    
    bool hasRSSInfo();

    void createPreviewSelectorBar(int index);
    void hideSelectorBar();

    void showCrashMessageBar();

    bool isPageLoading();

    KParts::ReadOnlyPart *part();
    void setPart(KParts::ReadOnlyPart *p, const KUrl &u);
    
    void focusIn();
    void focusOut();
    
    bool isWebApp();

private Q_SLOTS:
    void updateProgress(int progress);
    void resetProgress();

    void createWalletBar(const QString &, const QUrl &);

    void loadFinished();

    void showSearchEngineBar();

    void printFrame();

    void zoomIn();
    void zoomOut();
    void zoomDefault();
    void setZoom(int);
    
    // webapp slots per title & icon
    void webAppTitleChanged(QString);
    void webAppIconChanged();

    void toggleInspector(bool);

Q_SIGNALS:
    // NOTE: These signals are here to NOT expose webview directly and let
    // others use webtab instead. This will give us the ability to generate
    // our own signals when needed.
    void iconChanged();
    void loadFinished(bool);
    void loadProgress (int);
    void loadStarted();
    void urlChanged(const QUrl &);
    void titleChanged(const QString &);

    void loadProgressing();

    void triggerPartPrint();

    void infoToShow(QString);

private:
    WebView *m_webView;

    int m_progress;

    QWeakPointer<WalletBar> m_walletBar;
    QWeakPointer<PreviewSelectorBar> m_previewSelectorBar;
    QWeakPointer<QWebInspector> m_inspector;

    KParts::ReadOnlyPart *m_part;

    int m_zoomFactor;

    bool m_isPrivateBrowsing;
    bool m_isWebApp;
    
    QSplitter *m_splitter;
    
#ifdef HAVE_KACTIVITIES
    KActivities::ResourceInstance *m_activityResourceInstance;
#endif
};

#endif
