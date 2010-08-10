/* ============================================================
*
* This file is a part of the rekonq project
*
* Copyright (C) 2008-2010 by Andrea Diamantini <adjam7 at gmail dot com>
* Copyright (C) 2009 by Paweł Prażak <pawelprazak at gmail dot com>
* Copyright (C) 2009-2010 by Lionel Chauvin <megabigbug@yahoo.fr>
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


// Self Includes
#include "mainwindow.h"
#include "mainwindow.moc"

// Auto Includes
#include "rekonq.h"

// Local Includes
#include "settingsdialog.h"
#include "historymanager.h"
#include "bookmarksmanager.h"
#include "webtab.h"
#include "mainview.h"
#include "findbar.h"
#include "zoombar.h"
#include "historypanel.h"
#include "bookmarkspanel.h"
#include "webinspectorpanel.h"
#include "urlbar.h"
#include "tabbar.h"
#include "adblockmanager.h"
#include "analyzerpanel.h"

// Ui Includes
#include "ui_cleardata.h"

// KDE Includes
#include <KShortcut>
#include <KStandardAction>
#include <KAction>
#include <KToggleFullScreenAction>
#include <KActionCollection>
#include <KMessageBox>
#include <KFileDialog>
#include <KGlobalSettings>
#include <KPushButton>
#include <KTemporaryFile>
#include <KPassivePopup>
#include <KMenuBar>
#include <KJobUiDelegate>
#include <kdeprintdialog.h>
#include <KToggleAction>
#include <KStandardDirs>
#include <KActionCategory>
#include <KProcess>

// Qt Includes
#include <QtCore/QTimer>
#include <QtCore/QRect>
#include <QtCore/QSize>
#include <QtCore/QList>
#include <QtCore/QWeakPointer>

#include <QtGui/QVBoxLayout>
#include <QtGui/QFont>
#include <QtGui/QDesktopWidget>
#include <QtGui/QPrinter>
#include <QtGui/QPrintDialog>
#include <QtGui/QPrintPreviewDialog>
#include <QtGui/QFontMetrics>

#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>

#include <QtWebKit/QWebHistory>


MainWindow::MainWindow()
        : KXmlGuiWindow()
        , m_view(new MainView(this))
        , m_findBar(new FindBar(this))
        , m_zoomBar(new ZoomBar(this))
        , m_historyPanel(0)
        , m_bookmarksPanel(0)
        , m_webInspectorPanel(0)
        , m_analyzerPanel(0)
        , m_historyBackMenu(0)
        , m_encodingMenu(new KMenu(this))
        , m_bookmarksBar(new BookmarkToolBar(QString("BookmarkToolBar"), this, Qt::TopToolBarArea, true, false, true))
        , m_popup(new KPassivePopup(this))
        , m_hidePopup(new QTimer(this))
{
    // creating a centralWidget containing panel, m_view and the hidden findbar
    QWidget *centralWidget = new QWidget;
    centralWidget->setContentsMargins(0, 0, 0, 0);

    // setting layout
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_view);
    layout->addWidget(m_findBar);
    layout->addWidget(m_zoomBar);
    centralWidget->setLayout(layout);

    // central widget
    setCentralWidget(centralWidget);

    // setting size policies
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // then, setup our actions
    setupActions();

    // setting Panels
    setupPanels();

    // setting up rekonq tools
    setupTools();

    // setting up rekonq toolbar(s)
    setupToolbars();

    // a call to KXmlGuiWindow::setupGUI() populates the GUI                                                                              
    // with actions, using KXMLGUI.                                                                                                       
    // It also applies the saved mainwindow settings, if any, and ask the                                                                 
    // mainwindow to automatically save settings if changed: window size,                                                                 
    // toolbar position, icon size, etc.                                                                                                  
    setupGUI();
    
    // no menu bar in rekonq: we have other plans..
    menuBar()->setVisible(false);
    
    // no more status bar..
    setStatusBar(0);

    // give me some time to do all the other stuffs...
    QTimer::singleShot(100, this, SLOT(postLaunch()));

    kDebug() << "MainWindow ctor...DONE";
}


MainWindow::~MainWindow()
{
    Application::bookmarkProvider()->removeToolBar(m_bookmarksBar);
    Application::bookmarkProvider()->removeBookmarkPanel(m_bookmarksPanel);
    Application::instance()->removeMainWindow(this);
    
    delete m_view;
    delete m_findBar;
    delete m_zoomBar;

    delete m_historyPanel;
    delete m_bookmarksPanel;
    delete m_webInspectorPanel;

    delete m_stopReloadAction;
    delete m_historyBackMenu;
    delete m_encodingMenu;

    delete m_bookmarksBar;

    delete m_popup;
    delete m_hidePopup;
}


void MainWindow::setupToolbars()
{
    kDebug() << "setup toolbars...";
    
    KAction *a;

    // location bar
    a = new KAction(i18n("Location Bar"), this);
    a->setDefaultWidget(m_view->widgetBar());
    actionCollection()->addAction( QL1S("url_bar"), a);

    KToolBar *mainBar = toolBar("mainToolBar");

    // bookmarks bar
    KAction *bookmarkBarAction = Application::bookmarkProvider()->bookmarkToolBarAction(m_bookmarksBar);
    a = actionCollection()->addAction( QL1S("bookmarks_bar"), bookmarkBarAction);

    mainBar->show();  // this just to fix reopening rekonq after fullscreen close

    // =========== Bookmarks ToolBar ================================
    m_bookmarksBar->setAcceptDrops(true);
    Application::bookmarkProvider()->setupBookmarkBar(m_bookmarksBar);
}


void MainWindow::postLaunch()
{
    KToolBar *mainBar = toolBar("mainToolBar");
    
    QToolButton *bookmarksButton = qobject_cast<QToolButton*>(mainBar->widgetForAction(actionByName( QL1S("bookmarksActionMenu") )));
    if(bookmarksButton)
    {
        connect(actionByName(QL1S("bookmarksActionMenu")), SIGNAL(triggered()), bookmarksButton, SLOT(showMenu()));
    }
    
    QToolButton *toolsButton = qobject_cast<QToolButton*>(mainBar->widgetForAction(actionByName( QL1S("rekonq_tools") )));
    if(toolsButton)
    {
        connect(actionByName(QL1S("rekonq_tools")), SIGNAL(triggered()), toolsButton, SLOT(showMenu()));
    }
    
    // setting popup notification
    m_popup->setAutoDelete(false);
    connect(Application::instance(), SIGNAL(focusChanged(QWidget*, QWidget*)), m_popup, SLOT(hide()));
    m_popup->setFrameShape(QFrame::NoFrame);
    m_popup->setLineWidth(0);
    connect(m_hidePopup, SIGNAL(timeout()), m_popup, SLOT(hide()));

    // notification system
    connect(m_view, SIGNAL(showStatusBarMessage(const QString&, Rekonq::Notify)), this, SLOT(notifyMessage(const QString&, Rekonq::Notify)));
    connect(m_view, SIGNAL(linkHovered(const QString&)), this, SLOT(notifyMessage(const QString&)));

    // --------- connect signals and slots
    connect(m_view, SIGNAL(currentTitle(const QString &)), this, SLOT(updateWindowTitle(const QString &)));
    connect(m_view, SIGNAL(printRequested(QWebFrame *)), this, SLOT(printRequested(QWebFrame *)));

    // (shift +) ctrl + tab switching
    connect(this, SIGNAL(ctrlTabPressed()), m_view, SLOT(nextTab()));
    connect(this, SIGNAL(shiftCtrlTabPressed()), m_view, SLOT(previousTab()));

    // update toolbar actions signals
    connect(m_view, SIGNAL(tabsChanged()), this, SLOT(updateActions()));
    connect(m_view, SIGNAL(currentChanged(int)), this, SLOT(updateActions()));

    // launch it manually. Just the first time...
    updateActions();

    // Find Bar signal
    connect(m_findBar, SIGNAL(searchString(const QString &)), this, SLOT(find(const QString &)));

    // Zoom Bar signal
    connect(m_view, SIGNAL(currentChanged(int)), m_zoomBar, SLOT(updateSlider(int)));
    // Ctrl + wheel handling
    connect(this->currentTab()->view(), SIGNAL(zoomChanged(int)), m_zoomBar, SLOT(setValue(int)));
    
    // setting up toolbars to NOT have context menu enabled
    setContextMenuPolicy(Qt::DefaultContextMenu);

    // accept d'n'd
    setAcceptDrops(true);
}


QSize MainWindow::sizeHint() const
{
    QRect desktopRect = QApplication::desktop()->screenGeometry();
    QSize size = desktopRect.size() * 0.8;
    return size;
}


void MainWindow::setupActions()
{
    kDebug() << "setup actions...";
    
    // this let shortcuts work..
    actionCollection()->addAssociatedWidget(this);

    KAction *a;

    // new window action
    a = new KAction(KIcon("window-new"), i18n("&New Window"), this);
    a->setShortcut(KShortcut(Qt::CTRL | Qt::Key_N));
    actionCollection()->addAction(QL1S("new_window"), a);
    connect(a, SIGNAL(triggered(bool)), Application::instance(), SLOT(newWindow()));

    // Standard Actions
    KStandardAction::open(this, SLOT(fileOpen()), actionCollection());
    KStandardAction::saveAs(this, SLOT(fileSaveAs()), actionCollection());
    KStandardAction::print(this, SLOT(printRequested()), actionCollection());
    KStandardAction::quit(this , SLOT(close()), actionCollection());

    a = KStandardAction::find(m_findBar, SLOT(toggleVisibility()), actionCollection());
    KShortcut findShortcut = KStandardShortcut::find();
    findShortcut.setAlternate(Qt::Key_Slash);
    a->setShortcut(findShortcut);
    a->setCheckable(true);
    connect(m_findBar, SIGNAL(visibilityChanged(bool)), a, SLOT(setChecked(bool)));

    KStandardAction::findNext(this, SLOT(findNext()) , actionCollection());
    KStandardAction::findPrev(this, SLOT(findPrevious()) , actionCollection());

    a = KStandardAction::fullScreen(this, SLOT(viewFullScreen(bool)), this, actionCollection());
    KShortcut fullScreenShortcut = KStandardShortcut::fullScreen();
    fullScreenShortcut.setAlternate(Qt::Key_F11);
    a->setShortcut(fullScreenShortcut);

    a = actionCollection()->addAction(KStandardAction::Home);
    connect(a, SIGNAL(triggered(Qt::MouseButtons, Qt::KeyboardModifiers)), this, SLOT(homePage(Qt::MouseButtons, Qt::KeyboardModifiers)));
    KStandardAction::preferences(this, SLOT(preferences()), actionCollection());

    a = KStandardAction::redisplay(m_view, SLOT(webReload()), actionCollection());
    a->setText(i18n("Reload"));
    KShortcut reloadShortcut = KStandardShortcut::reload();
    reloadShortcut.setAlternate(Qt::CTRL + Qt::Key_R);
    a->setShortcut(reloadShortcut);

    a = new KAction(KIcon("process-stop"), i18n("&Stop"), this);
    a->setShortcut(KShortcut(Qt::CTRL | Qt::Key_Period));
    actionCollection()->addAction(QL1S("stop"), a);
    connect(a, SIGNAL(triggered(bool)), m_view, SLOT(webStop()));

    // stop reload Action
    m_stopReloadAction = new KAction(this);
    actionCollection()->addAction(QL1S("stop_reload") , m_stopReloadAction);
    m_stopReloadAction->setShortcutConfigurable(false);
    connect(m_view, SIGNAL(browserTabLoading(bool)), this, SLOT(browserLoading(bool)));
    browserLoading(false); //first init for blank start page

    a = new KAction(i18n("Open Location"), this);
    KShortcut openLocationShortcut(Qt::CTRL + Qt::Key_L);
    openLocationShortcut.setAlternate(Qt::Key_F6);
    a->setShortcut(openLocationShortcut);
    actionCollection()->addAction(QL1S("open_location"), a);
    connect(a, SIGNAL(triggered(bool)) , this, SLOT(openLocation()));

    // set zoom bar actions
    m_zoomBar->setupActions(this);

    // =============================== Tools Actions =================================
    a = new KAction(i18n("Page S&ource"), this);
    a->setIcon(KIcon("application-xhtml+xml"));
    actionCollection()->addAction(QL1S("page_source"), a);
    connect(a, SIGNAL(triggered(bool)), this, SLOT(viewPageSource()));

    a = new KAction(KIcon("view-media-artist"), i18n("Private &Browsing"), this);
    a->setCheckable(true);
    actionCollection()->addAction(QL1S("private_browsing"), a);
    connect(a, SIGNAL(triggered(bool)), this, SLOT(privateBrowsing(bool)));

    a = new KAction(KIcon("edit-clear"), i18n("Clear Private Data..."), this);
    actionCollection()->addAction(QL1S("clear_private_data"), a);
    connect(a, SIGNAL(triggered(bool)), this, SLOT(clearPrivateData()));

    // ========================= History related actions ==============================
    a = actionCollection()->addAction(KStandardAction::Back);
    connect(a, SIGNAL(triggered(Qt::MouseButtons, Qt::KeyboardModifiers)), this, SLOT(openPrevious(Qt::MouseButtons, Qt::KeyboardModifiers)));

    m_historyBackMenu = new KMenu(this);
    a->setMenu(m_historyBackMenu);
    connect(m_historyBackMenu, SIGNAL(aboutToShow()), this, SLOT(aboutToShowBackMenu()));
    connect(m_historyBackMenu, SIGNAL(triggered(QAction *)), this, SLOT(openActionUrl(QAction *)));

    a = actionCollection()->addAction(KStandardAction::Forward);
    connect(a, SIGNAL(triggered(Qt::MouseButtons, Qt::KeyboardModifiers)), this, SLOT(openNext(Qt::MouseButtons, Qt::KeyboardModifiers)));

    // ============================== General Tab Actions ====================================
    a = new KAction(KIcon("tab-new"), i18n("New &Tab"), this);
    a->setShortcut(KShortcut(Qt::CTRL + Qt::Key_T));
    actionCollection()->addAction(QL1S("new_tab"), a);
    connect(a, SIGNAL(triggered(bool)), m_view, SLOT(newTab()));

    a = new KAction(KIcon("view-refresh"), i18n("Reload All Tabs"), this);
    actionCollection()->addAction(QL1S("reload_all_tabs"), a);
    connect(a, SIGNAL(triggered(bool)), m_view, SLOT(reloadAllTabs()));

    a = new KAction(i18n("Show Next Tab"), this);
    a->setShortcuts(QApplication::isRightToLeft() ? KStandardShortcut::tabPrev() : KStandardShortcut::tabNext());
    actionCollection()->addAction(QL1S("show_next_tab"), a);
    connect(a, SIGNAL(triggered(bool)), m_view, SLOT(nextTab()));

    a = new KAction(i18n("Show Previous Tab"), this);
    a->setShortcuts(QApplication::isRightToLeft() ? KStandardShortcut::tabNext() : KStandardShortcut::tabPrev());
    actionCollection()->addAction(QL1S("show_prev_tab"), a);
    connect(a, SIGNAL(triggered(bool)), m_view, SLOT(previousTab()));
    
    a = new KAction(KIcon("tab-new"), i18n("Open Closed Tabs"), this);
    a->setShortcut(KShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_T));
    actionCollection()->addAction(QL1S("open_closed_tabs"), a);
    connect(a, SIGNAL(triggered(bool)), m_view, SLOT(openClosedTabs()));

    // Closed Tabs Menu
    KActionMenu *closedTabsMenu = new KActionMenu(KIcon("tab-new"), i18n("Closed Tabs"), this);
    closedTabsMenu->setDelayed(false);
    actionCollection()->addAction(QL1S("closed_tab_menu"), closedTabsMenu);

    // shortcuts for quickly switching to a tab
    for( int i = 1; i <= 9; i++ ) {
        a = new KAction(i18n("Switch to Tab %1", i), this);
        a->setShortcut(KShortcut( QString("Alt+%1").arg(i) ));
        a->setData( QVariant(i) );
        actionCollection()->addAction(QL1S(("switch_tab_" + QString::number(i)).toAscii()), a);
        connect(a, SIGNAL(triggered(bool)), m_view, SLOT(switchToTab()));
    }

    // ============================== Indexed Tab Actions ====================================
    a = new KAction(KIcon("tab-close"), i18n("&Close Tab"), this);
    a->setShortcuts( KStandardShortcut::close() );
    actionCollection()->addAction(QL1S("close_tab"), a);
    connect(a, SIGNAL(triggered(bool)), m_view->tabBar(), SLOT(closeTab()));

    a = new KAction(KIcon("tab-duplicate"), i18n("Clone Tab"), this);
    actionCollection()->addAction(QL1S("clone_tab"), a);
    connect(a, SIGNAL(triggered(bool)), m_view->tabBar(), SLOT(cloneTab()));

    a = new KAction(KIcon("tab-close-other"), i18n("Close &Other Tabs"), this);
    actionCollection()->addAction(QL1S("close_other_tabs"), a);
    connect(a, SIGNAL(triggered(bool)), m_view->tabBar(), SLOT(closeOtherTabs()));

    a = new KAction(KIcon("view-refresh"), i18n("Reload Tab"), this);
    actionCollection()->addAction(QL1S("reload_tab"), a);
    connect(a, SIGNAL(triggered(bool)), m_view->tabBar(), SLOT(reloadTab()));

    a = new KAction(KIcon("tab-detach"), i18n("Detach Tab"), this);
    actionCollection()->addAction(QL1S("detach_tab"), a);
    connect(a, SIGNAL(triggered(bool)), m_view->tabBar(), SLOT(detachTab()));

    // Bookmark Menu
    KActionMenu *bmMenu = Application::bookmarkProvider()->bookmarkActionMenu(this);
    bmMenu->setIcon(KIcon("bookmarks"));
    bmMenu->setDelayed(false);
    bmMenu->setShortcutConfigurable(true);
    bmMenu->setShortcut( KShortcut(Qt::ALT + Qt::Key_B) );
    actionCollection()->addAction(QL1S("bookmarksActionMenu"), bmMenu);

    // ---------------- Encodings -----------------------------------
    a = new KAction(KIcon("character-set"), i18n("Set Encoding"), this);
    actionCollection()->addAction(QL1S("encodings"), a);
    a->setMenu(m_encodingMenu);
    connect(m_encodingMenu, SIGNAL(aboutToShow()), this, SLOT(populateEncodingMenu()));
    connect(m_encodingMenu, SIGNAL(triggered(QAction *)), this, SLOT(setEncoding(QAction *)));
}


void MainWindow::setupTools()
{
    kDebug() << "setup tools...";
    KActionMenu *toolsMenu = new KActionMenu(KIcon("configure"), i18n("&Tools"), this);
    toolsMenu->setDelayed(false);
    toolsMenu->setShortcutConfigurable(true);
    toolsMenu->setShortcut( KShortcut(Qt::ALT + Qt::Key_T) );
    
    toolsMenu->addAction(actionByName(KStandardAction::name(KStandardAction::Open)));
    toolsMenu->addAction(actionByName(KStandardAction::name(KStandardAction::SaveAs)));
    toolsMenu->addAction(actionByName(KStandardAction::name(KStandardAction::Print)));
    toolsMenu->addAction(actionByName(KStandardAction::name(KStandardAction::Find)));

    QAction *action = actionByName(KStandardAction::name(KStandardAction::Zoom));
    action->setCheckable(true);
    connect (m_zoomBar, SIGNAL(visibilityChanged(bool)), action, SLOT(setChecked(bool)));
    toolsMenu->addAction(action);

    toolsMenu->addAction(actionByName(QL1S("encodings")));

    toolsMenu->addSeparator();

    toolsMenu->addAction(actionByName(QL1S("private_browsing")));
    toolsMenu->addAction(actionByName(QL1S("clear_private_data")));

    toolsMenu->addSeparator();

    KActionMenu *webMenu = new KActionMenu(KIcon("applications-development-web"), i18n("Development"), this);
    webMenu->addAction(actionByName(QL1S("web_inspector")));
    webMenu->addAction(actionByName(QL1S("page_source")));
    webMenu->addAction(actionByName(QL1S("net_analyzer")));
    toolsMenu->addAction(webMenu);

    toolsMenu->addSeparator();

    toolsMenu->addAction(actionByName(QL1S("show_history_panel")));
    toolsMenu->addAction(actionByName(QL1S("show_bookmarks_panel")));
    toolsMenu->addAction(actionByName(KStandardAction::name(KStandardAction::FullScreen)));

    toolsMenu->addSeparator();

    helpMenu()->setIcon(KIcon("help-browser"));
    toolsMenu->addAction(helpMenu()->menuAction());
    toolsMenu->addAction(actionByName(KStandardAction::name(KStandardAction::Preferences)));

    // adding rekonq_tools to rekonq actionCollection
    actionCollection()->addAction(QL1S("rekonq_tools"), toolsMenu);
}


void MainWindow::setupPanels()
{
    kDebug() << "setup panels...";
    KAction* a;

    // STEP 1
    // Setup history panel
    m_historyPanel = new HistoryPanel(i18n("History Panel"), this);
    connect(m_historyPanel, SIGNAL(openUrl(const KUrl&, const Rekonq::OpenType &)), Application::instance(), SLOT(loadUrl(const KUrl&, const Rekonq::OpenType &)));
    connect(m_historyPanel, SIGNAL(itemHovered(QString)), this, SLOT(notifyMessage(QString)));
    connect(m_historyPanel, SIGNAL(destroyed()), Application::instance(), SLOT(saveConfiguration()));

    addDockWidget(Qt::LeftDockWidgetArea, m_historyPanel);

    // setup history panel action
    a = (KAction *) m_historyPanel->toggleViewAction();
    a->setShortcut(KShortcut(Qt::CTRL + Qt::Key_H));
    a->setIcon(KIcon("view-history"));
    actionCollection()->addAction(QL1S("show_history_panel"), a);

    // STEP 2
    // Setup bookmarks panel
    m_bookmarksPanel = new BookmarksPanel(i18n("Bookmarks Panel"), this);
    connect(m_bookmarksPanel, SIGNAL(openUrl(const KUrl&, const Rekonq::OpenType &)), Application::instance(), SLOT(loadUrl(const KUrl&, const Rekonq::OpenType &)));
    connect(m_bookmarksPanel, SIGNAL(itemHovered(QString)), this, SLOT(notifyMessage(QString)));
    connect(m_bookmarksPanel, SIGNAL(destroyed()), Application::instance(), SLOT(saveConfiguration()));

    addDockWidget(Qt::LeftDockWidgetArea, m_bookmarksPanel);

    Application::bookmarkProvider()->registerBookmarkPanel(m_bookmarksPanel);

    // setup bookmarks panel action
    a = (KAction *) m_bookmarksPanel->toggleViewAction();
    a->setShortcut(KShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_B));
    a->setIcon(KIcon("bookmarks-organize"));
    actionCollection()->addAction(QL1S("show_bookmarks_panel"), a);

    // STEP 3
    // Setup webinspector panel
    m_webInspectorPanel = new WebInspectorPanel(i18n("Web Inspector"), this);
    connect(mainView(), SIGNAL(currentChanged(int)), m_webInspectorPanel, SLOT(changeCurrentPage()));

    a = new KAction(KIcon("tools-report-bug"), i18n("Web &Inspector"), this);
    a->setCheckable(true);
    actionCollection()->addAction(QL1S("web_inspector"), a);
    connect(a, SIGNAL(triggered(bool)), m_webInspectorPanel, SLOT(toggle(bool)));

    addDockWidget(Qt::BottomDockWidgetArea, m_webInspectorPanel);
    m_webInspectorPanel->hide();
    
    // STEP 4
    // Setup Network analyzer panel
    m_analyzerPanel = new NetworkAnalyzerPanel( i18n("Network Analyzer"), this);
    connect(mainView(), SIGNAL(currentChanged(int)), m_analyzerPanel, SLOT(changeCurrentPage()));

    a = new KAction(KIcon("document-edit-decrypt-verify"), i18n("Network Analyzer"), this);
    a->setCheckable(true);
    actionCollection()->addAction(QL1S("net_analyzer"), a);
    connect(a, SIGNAL(triggered(bool)), this, SLOT(enableNetworkAnalysis(bool)));

    addDockWidget(Qt::BottomDockWidgetArea, m_analyzerPanel);
    m_analyzerPanel->hide();
}


void MainWindow::openLocation()
{
    m_view->urlBar()->selectAll();
    m_view->urlBar()->setFocus();
}


void MainWindow::fileSaveAs()
{
    KUrl srcUrl;
    WebTab *w = currentTab();
    if (w->page()->isOnRekonqPage())
    {
        QWebElement el = w->page()->mainFrame()->documentElement();
        srcUrl = KUrl( el.findFirst("object").attribute("data") );
    }
    else
    {
        srcUrl = w->url();
    }
    kDebug() << "URL to save: " << srcUrl;
    
    QString name = srcUrl.fileName();
    if (name.isNull())
    {
        name = srcUrl.host() + QString(".html");
    }
    const QString destUrl = KFileDialog::getSaveFileName(name, QString(), this);
    if (destUrl.isEmpty()) return;
    KIO::Job *job = KIO::file_copy(srcUrl, KUrl(destUrl), -1, KIO::Overwrite);
    job->addMetaData("MaxCacheSize", "0");  // Don't store in http cache.
    job->addMetaData("cache", "cache");     // Use entry from cache if available.
    job->uiDelegate()->setAutoErrorHandlingEnabled(true);
}


void MainWindow::preferences()
{
    // an instance the dialog could be already created and could be cached,
    // in which case you want to display the cached dialog
    if (SettingsDialog::showDialog("rekonfig"))
        return;

    // we didn't find an instance of this dialog, so lets create it
    QPointer<SettingsDialog> s = new SettingsDialog(this);

    // keep us informed when the user changes settings
    connect(s, SIGNAL(settingsChanged(const QString&)), Application::instance(), SLOT(updateConfiguration()));

    s->exec();
    delete s;
}


void MainWindow::updateActions()
{
    kDebug() << "updating actions..";
    bool rekonqPage = currentTab()->page()->isOnRekonqPage();
    
    QAction *historyBackAction = actionByName(KStandardAction::name(KStandardAction::Back));
    if( rekonqPage && currentTab()->view()->history()->count() > 0 )
        historyBackAction->setEnabled(true);
    else
        historyBackAction->setEnabled(currentTab()->view()->history()->canGoBack());

    QAction *historyForwardAction = actionByName(KStandardAction::name(KStandardAction::Forward));
    historyForwardAction->setEnabled(currentTab()->view()->history()->canGoForward());
}


void MainWindow::updateWindowTitle(const QString &title)
{
    QWebSettings *settings = QWebSettings::globalSettings();
    if (title.isEmpty())
    {
        if (settings->testAttribute(QWebSettings::PrivateBrowsingEnabled))
        {
            setWindowTitle(i18nc("Window title when private browsing is activated", "rekonq (Private Browsing)"));
        }
        else
        {
            setWindowTitle("rekonq");
        }
    }
    else
    {
        if (settings->testAttribute(QWebSettings::PrivateBrowsingEnabled))
        {
            setWindowTitle(i18nc("window title, %1 = title of the active website", "%1 – rekonq (Private Browsing)", title));
        }
        else
        {
            setWindowTitle(i18nc("window title, %1 = title of the active website", "%1 – rekonq", title));
        }
    }
}


void MainWindow::fileOpen()
{
    QString filePath = KFileDialog::getOpenFileName(KUrl(),
                       i18n("*.html *.htm *.svg *.png *.gif *.svgz|Web Resources (*.html *.htm *.svg *.png *.gif *.svgz)\n"
                            "*.*|All files (*.*)"),
                       this,
                       i18n("Open Web Resource"));

    if (filePath.isEmpty())
        return;

    Application::instance()->loadUrl(filePath);
}


void MainWindow::printRequested(QWebFrame *frame)
{
    if (!currentTab())
        return;

    QWebFrame *printFrame = 0;
    if (frame == 0)
    {
        printFrame = currentTab()->page()->mainFrame();
    }
    else
    {
        printFrame = frame;
    }

    QPrinter printer;
    QPrintPreviewDialog previewdlg(&printer, this);

    connect(&previewdlg, SIGNAL(paintRequested(QPrinter *)), printFrame, SLOT(print(QPrinter *)));

    previewdlg.exec();
}


void MainWindow::privateBrowsing(bool enable)
{
    QWebSettings *settings = QWebSettings::globalSettings();
    if (enable && !settings->testAttribute(QWebSettings::PrivateBrowsingEnabled))
    {
        QString title = i18n("Are you sure you want to turn on private browsing?");
        QString text = i18n("<b>%1</b>"
                            "<p>When private browsing is turned on,"
                            " web pages are not added to the history,"
                            " new cookies are not stored, current cookies cannot be accessed,"
                            " site icons will not be stored, the session will not be saved."
                            " Until you close the window, you can still click the Back and Forward buttons"
                            " to return to the web pages you have opened.</p>", title);

        int button = KMessageBox::warningContinueCancel(this, text, title);
        if (button == KMessageBox::Continue)
        {
            settings->setAttribute(QWebSettings::PrivateBrowsingEnabled, true);
            m_view->urlBar()->setPrivateMode(true);
        }
        else
        {
            actionCollection()->action( QL1S("private_browsing") )->setChecked(false);
        }
    }
    else
    {
        settings->setAttribute(QWebSettings::PrivateBrowsingEnabled, false);
        m_view->urlBar()->setPrivateMode(false);

        m_lastSearch.clear();
        m_view->reloadAllTabs();
    }
}


void MainWindow::find(const QString & search)
{
    if (!currentTab())
        return;
    m_lastSearch = search;

    findNext();
}


void MainWindow::matchCaseUpdate()
{
    if (!currentTab())
        return;

    currentTab()->view()->findText(m_lastSearch, QWebPage::FindBackward);
    findNext();
}


void MainWindow::findNext()
{
    if (!currentTab())
        return;

    highlightAll();

    if (m_findBar->isHidden())
    {
        QPoint previous_position = currentTab()->view()->page()->currentFrame()->scrollPosition();
        currentTab()->view()->page()->focusNextPrevChild(true);
        currentTab()->view()->page()->currentFrame()->setScrollPosition(previous_position);
        return;
    }

    highlightAll();

    QWebPage::FindFlags options = QWebPage::FindWrapsAroundDocument;
    if (m_findBar->matchCase())
        options |= QWebPage::FindCaseSensitively;

    bool found = currentTab()->view()->findText(m_lastSearch, options);
    m_findBar->notifyMatch(found);

    if (!found)
    {
        QPoint previous_position = currentTab()->view()->page()->currentFrame()->scrollPosition();
        currentTab()->view()->page()->focusNextPrevChild(true);
        currentTab()->view()->page()->currentFrame()->setScrollPosition(previous_position);
    }
}


void MainWindow::findPrevious()
{
    if (!currentTab())
        return;

    QWebPage::FindFlags options = QWebPage::FindBackward | QWebPage::FindWrapsAroundDocument;
    if (m_findBar->matchCase())
        options |= QWebPage::FindCaseSensitively;

    bool found = currentTab()->view()->findText(m_lastSearch, options);
    m_findBar->notifyMatch(found);
}

void MainWindow::highlightAll()
{
    if (!currentTab())
        return;

    QWebPage::FindFlags options = QWebPage::HighlightAllOccurrences;

    currentTab()->view()->findText("", options); //Clear an existing highlight

    if (m_findBar->highlightAllState() && !m_findBar->isHidden())
    {
        if (m_findBar->matchCase())
            options |= QWebPage::FindCaseSensitively;

        currentTab()->view()->findText(m_lastSearch, options);
    }
}


void MainWindow::viewFullScreen(bool makeFullScreen)
{
    setWidgetsVisible(!makeFullScreen);
    KToggleFullScreenAction::setFullScreen(this, makeFullScreen);
}


void MainWindow::setWidgetsVisible(bool makeVisible)
{
    // state flags
    static bool bookmarksToolBarFlag;
    static bool historyPanelFlag;
    static bool bookmarksPanelFlag;

    KToolBar *mainBar = toolBar("mainToolBar");
    KToolBar *bookBar = toolBar("bookmarksToolBar");
    
    if (!makeVisible)
    {
        // save current state, if in windowed mode
        if (!isFullScreen())
        {
            bookmarksToolBarFlag = bookBar->isHidden();
            historyPanelFlag = m_historyPanel->isHidden();
            bookmarksPanelFlag = m_bookmarksPanel->isHidden();
        }

        bookBar->hide();
        m_view->setTabBarHidden(true);
        m_historyPanel->hide();
        m_bookmarksPanel->hide();

        // hide main toolbar
        mainBar->hide();
    }
    else
    {
        // show main toolbar
        mainBar->show();
        m_view->setTabBarHidden(false);

        // restore state of windowed mode
        if (!bookmarksToolBarFlag)
            bookBar->show();
        if (!historyPanelFlag)
            m_historyPanel->show();
        if (!bookmarksPanelFlag)
            m_bookmarksPanel->show();
    }
}


void MainWindow::viewPageSource()
{
    if (!currentTab())
        return;

    KUrl url = currentTab()->url();
    KRun::runUrl(url, QL1S("text/plain"), this, false);
}


void MainWindow::homePage(Qt::MouseButtons mouseButtons, Qt::KeyboardModifiers keyboardModifiers)
{
    KUrl homeUrl = ReKonfig::useNewTabPage() 
        ? KUrl( QL1S("about:home") )
        : KUrl( ReKonfig::homePage() );
        
    if (mouseButtons == Qt::MidButton || keyboardModifiers == Qt::ControlModifier)
        Application::instance()->loadUrl( homeUrl, Rekonq::NewTab);
    else
        currentTab()->view()->load( homeUrl );
}


WebTab *MainWindow::currentTab() const
{
    return m_view->currentWebTab();
}


void MainWindow::browserLoading(bool v)
{
    QAction *stop = actionCollection()->action( QL1S("stop") );
    QAction *reload = actionCollection()->action( QL1S("view_redisplay") );
    if (v)
    {
        disconnect(m_stopReloadAction, SIGNAL(triggered(bool)), reload , SIGNAL(triggered(bool)));
        m_stopReloadAction->setIcon(KIcon("process-stop"));
        m_stopReloadAction->setToolTip(i18n("Stop loading the current page"));
        m_stopReloadAction->setText(i18n("Stop"));
        connect(m_stopReloadAction, SIGNAL(triggered(bool)), stop, SIGNAL(triggered(bool)));
    }
    else
    {
        disconnect(m_stopReloadAction, SIGNAL(triggered(bool)), stop , SIGNAL(triggered(bool)));
        m_stopReloadAction->setIcon(KIcon("view-refresh"));
        m_stopReloadAction->setToolTip(i18n("Reload the current page"));
        m_stopReloadAction->setText(i18n("Reload"));
        connect(m_stopReloadAction, SIGNAL(triggered(bool)), reload, SIGNAL(triggered(bool)));

    }
}


void MainWindow::openPrevious(Qt::MouseButtons mouseButtons, Qt::KeyboardModifiers keyboardModifiers)
{
    QWebHistory *history = currentTab()->view()->history();
    QWebHistoryItem *item = 0;
    
    if (currentTab()->page()->isOnRekonqPage())
    {
        item = new QWebHistoryItem(history->currentItem());
        currentTab()->view()->page()->setIsOnRekonqPage(false);
    }
    else
    {
        if (history->canGoBack())
        {
            item = new QWebHistoryItem(history->backItem());
        }
    }

    if(!item)
        return;
    
    if (mouseButtons == Qt::MidButton || keyboardModifiers == Qt::ControlModifier)
    {
        Application::instance()->loadUrl(item->url(), Rekonq::NewTab);
    }
    else
    {
        history->goToItem(*item);
    }

    updateActions();
}


void MainWindow::openNext(Qt::MouseButtons mouseButtons, Qt::KeyboardModifiers keyboardModifiers)
{
    QWebHistory *history = currentTab()->view()->history();
    QWebHistoryItem *item = 0;

    if (currentTab()->view()->page()->isOnRekonqPage())
    {
        item = new QWebHistoryItem(history->currentItem());
        currentTab()->view()->page()->setIsOnRekonqPage(false);
    }
    else
    {
        if (history->canGoForward())
        {
            item = new QWebHistoryItem(history->forwardItem());
        }
    }
    
    if(!item)
        return;
    
    if (mouseButtons == Qt::MidButton || keyboardModifiers == Qt::ControlModifier)
    {
        Application::instance()->loadUrl(item->url(), Rekonq::NewTab);
    }
    else
    {
        history->goToItem(*item);
    }
    
    updateActions();
}


void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // hide findbar
    if (event->key() == Qt::Key_Escape)
    {
        m_findBar->hide();
        currentTab()->setFocus();   // give focus to web pages
        return;
    }

    // ctrl + tab action
    if ((event->modifiers() == Qt::ControlModifier) && (event->key() == Qt::Key_Tab))
    {
        emit ctrlTabPressed();
        return;
    }

    // shift + ctrl + tab action
    if ((event->modifiers() == Qt::ControlModifier + Qt::ShiftModifier) && (event->key() == Qt::Key_Backtab))
    {
        emit shiftCtrlTabPressed();
        return;
    }

    KMainWindow::keyPressEvent(event);
}


void MainWindow::notifyMessage(const QString &msg, Rekonq::Notify status)
{
    if (this != QApplication::activeWindow())
    {
        return;
    }

    // deleting popus if empty msgs
    if (msg.isEmpty())
    {
        m_hidePopup->start(250);
        return;
    }

    m_hidePopup->stop();


    switch (status)
    {
    case Rekonq::Url:
        break;
    case Rekonq::Info:
        m_hidePopup->start(500);
        break;
    case Rekonq::Success:
        break;
    case Rekonq::Error:
        break;
    case Rekonq::Download:
        break;
    default:
        break;
    }

    int margin = 4;

    // setting the popup
    QLabel *label = new QLabel(msg);
    m_popup->setView(label);
    QSize labelSize(label->fontMetrics().width(msg) + 2*margin, label->fontMetrics().height() + 2*margin);
    if (labelSize.width() > width()) labelSize.setWidth(width());
    m_popup->setFixedSize(labelSize);
    m_popup->layout()->setAlignment(Qt::AlignTop);
    m_popup->layout()->setMargin(margin);

    // useful values
    WebTab *tab = m_view->currentWebTab();

    // fix crash on window close
    if (!tab)
        return;

    // fix crash on window close
    if (!tab->page())
        return;

    bool scrollbarIsVisible = tab->page()->currentFrame()->scrollBarMaximum(Qt::Horizontal);
    int scrollbarSize = 0;
    if (scrollbarIsVisible)
    {
        //TODO: detect QStyle size
        scrollbarSize = 17;
    }

    QPoint webViewOrigin = tab->view()->mapToGlobal(QPoint(0, 0));
    int bottomLeftY = webViewOrigin.y() + tab->page()->viewportSize().height() - labelSize.height() - scrollbarSize;

    // setting popup in bottom-left position
    int x = geometry().x();
    int y = bottomLeftY;

    QPoint mousePos = tab->mapToGlobal(tab->view()->mousePos());
    if (QRect(webViewOrigin.x() , bottomLeftY , labelSize.width() , labelSize.height()).contains(mousePos))
    {
        // setting popup above the mouse
        y = bottomLeftY - labelSize.height();
    }

    m_popup->show(QPoint(x, y));
}


void MainWindow::clearPrivateData()
{
    QPointer<KDialog> dialog = new KDialog(this);
    dialog->setCaption(i18n("Clear Private Data"));
    dialog->setButtons(KDialog::Ok | KDialog::Cancel);

    dialog->button(KDialog::Ok)->setIcon(KIcon("edit-clear"));
    dialog->button(KDialog::Ok)->setText(i18n("Clear"));

    Ui::ClearDataWidget clearWidget;
    QWidget widget;
    clearWidget.setupUi(&widget);

    dialog->setMainWidget(&widget);
    dialog->exec();

    if (dialog->result() == QDialog::Accepted)
    {
        if (clearWidget.clearHistory->isChecked())
        {
            Application::historyManager()->clear();
        }

        if (clearWidget.clearDownloads->isChecked())
        {
            Application::instance()->clearDownloadsHistory();
        }

        if (clearWidget.clearCookies->isChecked())
        {
            QDBusInterface kcookiejar("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer");
            QDBusReply<void> reply = kcookiejar.call("deleteAllCookies");
        }

        if (clearWidget.clearCachedPages->isChecked())
        {
            KProcess::startDetached(KStandardDirs::findExe("kio_http_cache_cleaner"),
                                    QStringList(QL1S("--clear-all")));
        }

        if (clearWidget.clearWebIcons->isChecked())
        {
            QWebSettings::clearIconDatabase();
        }

        if (clearWidget.homePageThumbs->isChecked())
        {
            QString path = KStandardDirs::locateLocal("cache", QString("thumbs/rekonq"), true);
            path.remove("rekonq");
            QDir cacheDir(path);
            QStringList fileList = cacheDir.entryList();
            foreach(const QString &str, fileList)
            {
                QFile file(path + str);
                file.remove();
            }
        }
    }

    dialog->deleteLater();
}


void MainWindow::aboutToShowBackMenu()
{
    m_historyBackMenu->clear();
    if (!currentTab())
        return;
    QWebHistory *history = currentTab()->view()->history();
    int pivot = history->currentItemIndex();
    int offset = 0;
    QList<QWebHistoryItem> historyList = history->backItems(8); //no more than 8 elements!
    int listCount = historyList.count();
    if (pivot >= 8)
        offset = pivot - 8;

    /*
     * Need a bug report upstream. 
     * Seems setHtml() do some garbage in history
     * Here history->currentItem() have backItem url and currentItem (error page) title
     */
    if (currentTab()->view()->page()->isOnRekonqPage())
    {
        QWebHistoryItem item = history->currentItem();
        KAction *action = new KAction(this);
        action->setData(listCount + offset++);
        KIcon icon = Application::icon(item.url());
        action->setIcon(icon);
        action->setText(item.title());
        m_historyBackMenu->addAction(action);
    }
         
    for (int i = listCount - 1; i >= 0; --i)
    {
        QWebHistoryItem item = historyList.at(i);
        KAction *action = new KAction(this);
        action->setData(i + offset);
        KIcon icon = Application::icon(item.url());
        action->setIcon(icon);
        action->setText(item.title());
        m_historyBackMenu->addAction(action);
    }
}


void MainWindow::openActionUrl(QAction *action)
{
    int index = action->data().toInt();

    QWebHistory *history = currentTab()->view()->history();
    if (!history->itemAt(index).isValid())
    {
        kDebug() << "Invalid Index!: " << index;
        return;
    }

    history->goToItem(history->itemAt(index));
}


void MainWindow::setEncoding(QAction *qa)
{
    QString currentCodec = qa->text().toLatin1();
    currentCodec = currentCodec.remove('&');
    kDebug() << "Setting codec: " << currentCodec;
    if(currentCodec == QL1S("Default") )
    {
        kDebug() << "QWebSettings::globalSettings()->defaultTextEncoding() = " << QWebSettings::globalSettings()->defaultTextEncoding();
        currentTab()->view()->settings()->setDefaultTextEncoding( QWebSettings::globalSettings()->defaultTextEncoding() );
        currentTab()->view()->reload();
        return;
    }
    
    currentTab()->view()->settings()->setDefaultTextEncoding(currentCodec);
    currentTab()->view()->reload();
}


void MainWindow::populateEncodingMenu()
{
    QStringList codecs;
    QList<int> mibs = QTextCodec::availableMibs();
    Q_FOREACH (const int &mib, mibs) 
    {
        QString codec = QLatin1String(QTextCodec::codecForMib(mib)->name());
        codecs.append(codec);
    }
    codecs.sort();

    QString currentCodec = currentTab()->page()->settings()->defaultTextEncoding();

    m_encodingMenu->clear();
    KMenu *isoMenu = new KMenu( QL1S("ISO"), m_encodingMenu);
    KMenu *winMenu = new KMenu( QL1S("Windows"), m_encodingMenu);
    KMenu *isciiMenu = new KMenu( QL1S("ISCII"), m_encodingMenu);
    KMenu *uniMenu = new KMenu( QL1S("Unicode"), m_encodingMenu);
    KMenu *otherMenu = new KMenu( i18n("Other"), m_encodingMenu);
    
    QAction *a;
    bool isDefaultCodecUsed = true;
    
    Q_FOREACH(const QString &codec, codecs)
    {
        
        if( codec.startsWith( QL1S("ISO"), Qt::CaseInsensitive ) )
            a = isoMenu->addAction(codec);
        else if( codec.startsWith( QL1S("win") ) )
            a = winMenu->addAction(codec);
        else if( codec.startsWith( QL1S("Iscii") ) )
            a = isciiMenu->addAction(codec);
        else if( codec.startsWith( QL1S("UT") ) )
            a = uniMenu->addAction(codec);
        else 
            a = otherMenu->addAction(codec);
            
        a->setCheckable(true);
        if (currentCodec == codec)
        {
            a->setChecked(true);
            isDefaultCodecUsed = false;
        }
    }
    
    a = new QAction( i18nc("Default website encoding", "Default"), this);
    a->setCheckable(true);
    a->setChecked(isDefaultCodecUsed);
            
    m_encodingMenu->addAction( a );
    m_encodingMenu->addMenu( isoMenu );
    m_encodingMenu->addMenu( winMenu );
    m_encodingMenu->addMenu( isciiMenu );
    m_encodingMenu->addMenu( uniMenu );
    m_encodingMenu->addMenu( otherMenu );
}


void MainWindow::enableNetworkAnalysis(bool b)
{
    currentTab()->page()->enableNetworkAnalyzer(b);
    m_analyzerPanel->toggle(b);
}


bool MainWindow::queryClose()
{
    // this should fux bug 240432
    if(Application::instance()->sessionSaving())
        return true;
    
    if (m_view->count() > 1)                                                                                                              
    {
        int answer = KMessageBox::questionYesNoCancel(                                                                                    
                        this,                                                                                                            
                        i18np("Are you sure you want to close the window?\n" "You have 1 tab open.",
                         "Are you sure you want to close the window?\n" "You have %1 tabs open.", 
                        m_view->count()),
                        i18n("Are you sure you want to close the window?"),                                                              
                        KStandardGuiItem::quit(),                                                                                        
                        KGuiItem(i18n("C&lose Current Tab"), KIcon("tab-close")),                                                        
                        KStandardGuiItem::cancel(),                                                                                      
                        "confirmClosingMultipleTabs"                                                                                     
                                                        );                                                                                                                   
                                                                                                                    
        switch (answer)                                                                                                                   
        {                                                                                                                                 
            case KMessageBox::Yes:  
                // Quit                                                                                                                         
                return true;                                                                                                                  
            
            case KMessageBox::No:                                                                                                             
                // Close only the current tab                                                                                                 
                m_view->closeTab();
            
            default:                                                                                                                          
                return false;                                                                                                                 
        }                                                                                                                                 
    }                                                                                                                                     
    return true;                                                                                                                          
}
