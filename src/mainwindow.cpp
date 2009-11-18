/* ============================================================
*
* This file is a part of the rekonq project
*
* Copyright (C) 2008-2009 by Andrea Diamantini <adjam7 at gmail dot com>
* Copyright (C) 2009 by Paweł Prażak <pawelprazak at gmail dot com>
* Copyright (C) 2009 by Lionel Chauvin <megabigbug@yahoo.fr>
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
#include "application.h"
#include "settings.h"
#include "history.h"
#include "cookiejar.h"
#include "networkaccessmanager.h"
#include "bookmarks.h"
#include "webview.h"
#include "mainview.h"
#include "findbar.h"
#include "sidepanel.h"
#include "bookmarkspanel/bookmarkspanel.h"
#include "urlbar.h"
#include "tabbar.h"
#include "newtabpage.h"

// Ui Includes
#include "ui_cleardata.h"

// KDE Includes
#include <KUrl>
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
#include <KToolBar>
#include <KJobUiDelegate>
#include <kdeprintdialog.h>
#include <KToggleAction>
#include <KStandardDirs>
#include <KActionCategory>

// Qt Includes
#include <QtCore/QTimer>
#include <QtCore/QRect>
#include <QtCore/QSize>
#include <QtCore/QList>
#include <QtCore/QPointer>

#include <QtGui/QWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QAction>
#include <QtGui/QFont>
#include <QtGui/QDesktopWidget>
#include <QtGui/QPrinter>
#include <QtGui/QPrintDialog>
#include <QtGui/QPrintPreviewDialog>
#include <QtGui/QFontMetrics>

#include <QtWebKit/QWebHistory>


MainWindow::MainWindow()
    : KMainWindow()
    , m_view(new MainView(this))
    , m_findBar(new FindBar(this))
    , m_sidePanel(0)
	, m_bookmarksPanel(0)
    , m_historyBackMenu(0)
    , m_mainBar( new KToolBar( QString("MainToolBar"), this, Qt::TopToolBarArea, true, false, false) )
    , m_bmBar( new KToolBar( QString("BookmarkToolBar"), this, Qt::TopToolBarArea, true, false, false) )
    , m_popup( new KPassivePopup(this) )
    , m_hidePopup( new QTimer(this) )
    , m_ac( new KActionCollection(this) )
    , m_loadingNewTabPage(false)
{
    // enable window size "auto-save"
    setAutoSaveSettings();

    // updating rekonq configuration
    updateConfiguration();

    // creating a centralWidget containing panel, m_view and the hidden findbar
    QWidget *centralWidget = new QWidget;
    centralWidget->setContentsMargins(0, 0, 0, 0);

    // setting layout
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_view);
    layout->addWidget(m_findBar);
    centralWidget->setLayout(layout);

    // central widget
    setCentralWidget(centralWidget);

    // setting size policies
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // then, setup our actions
    setupActions();

    // setting Side Panel
    setupSidePanel();
	setupBookmarksPanel();

    // setting up rekonq tools
    setupTools();

    // setting up rekonq toolbar(s)
    setupToolbars();

    // no more status bar..
    setStatusBar(0);

    // setting popup notification
    m_popup->setAutoDelete(false);
    connect(Application::instance(), SIGNAL(focusChanged(QWidget*,QWidget*)), m_popup, SLOT(hide()));
    m_popup->setFrameShape(QFrame::NoFrame);
    m_popup->setLineWidth(0);
    connect(m_hidePopup, SIGNAL(timeout()), m_popup, SLOT(hide()));

    QTimer::singleShot(0, this, SLOT(postLaunch()));
}


MainWindow::~MainWindow()
{
    Application::instance()->removeMainWindow(this);
    delete m_popup;
}


SidePanel *MainWindow::sidePanel()
{
    return m_sidePanel;
}

BookmarksPanel *MainWindow::bookmarksPanel()
{
    return m_bookmarksPanel;
}


void MainWindow::setupToolbars()
{
    // ============ Main ToolBar  ================================
    m_mainBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_mainBar->setContextMenuPolicy(Qt::PreventContextMenu);
    m_mainBar->setFloatable(false);
    m_mainBar->setMovable(false);

    m_mainBar->addAction( actionByName(KStandardAction::name(KStandardAction::Back)) );
    m_mainBar->addAction( actionByName(KStandardAction::name(KStandardAction::Forward)) );
    m_mainBar->addSeparator();
    m_mainBar->addAction( actionByName("stop_reload") );
    m_mainBar->addAction( actionByName(KStandardAction::name(KStandardAction::Home)) );

    // location bar
    KAction *urlBarAction = new KAction(this);
    urlBarAction->setDefaultWidget(m_view->urlBar());
    m_mainBar->addAction( urlBarAction );

    m_mainBar->addAction( actionByName("bookmarksActionMenu") );
    m_mainBar->addAction( actionByName("rekonq_tools") );

    // =========== Bookmarks ToolBar ================================
    m_bmBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_bmBar->setAcceptDrops(true);
    m_bmBar->setContextMenuPolicy(Qt::CustomContextMenu);
    m_bmBar->setFloatable(false);
    m_bmBar->setMovable(false);

    Application::bookmarkProvider()->setupBookmarkBar(m_bmBar);
}


void MainWindow::postLaunch()
{
    // notification system
    connect(m_view, SIGNAL(showStatusBarMessage(const QString&, Rekonq::Notify)), this, SLOT(notifyMessage(const QString&, Rekonq::Notify)));
    connect(m_view, SIGNAL(linkHovered(const QString&)), this, SLOT(notifyMessage(const QString&)));

    // --------- connect signals and slots
    connect(m_view, SIGNAL(setCurrentTitle(const QString &)), this, SLOT(updateWindowTitle(const QString &)));
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

    // setting up toolbars to NOT have context menu enabled
    setContextMenuPolicy(Qt::DefaultContextMenu);

    // accept d'n'd
    setAcceptDrops(true);

    // KActionCollection read settings
    m_ac->readSettings();
}


QSize MainWindow::sizeHint() const
{
    QRect desktopRect = QApplication::desktop()->screenGeometry();
    QSize size = desktopRect.size() * 0.8;
    return size;
}


KActionCollection *MainWindow::actionCollection() const
{
    return m_ac;
}


void MainWindow::setupActions()
{
    // this let shortcuts work..
    actionCollection()->addAssociatedWidget(this);

    KAction *a;

    // new window action
    a = new KAction(KIcon("window-new"), i18n("&New Window"), this);
    a->setShortcut(KShortcut(Qt::CTRL | Qt::Key_N));
    actionCollection()->addAction(QLatin1String("new_window"), a);
    connect(a, SIGNAL(triggered(bool)), Application::instance(), SLOT(newMainWindow()));

    // Standard Actions
    KStandardAction::open(this, SLOT(fileOpen()), actionCollection());
    KStandardAction::saveAs(this, SLOT(fileSaveAs()), actionCollection());
    KStandardAction::print(this, SLOT(printRequested()), actionCollection());
    KStandardAction::quit(this , SLOT(close()), actionCollection());

    a = KStandardAction::find(m_findBar, SLOT(show()), actionCollection());
    QList<QKeySequence> shortcutFindList;
    shortcutFindList << KStandardShortcut::find() << QKeySequence( Qt::Key_Slash );
    a->setShortcuts( shortcutFindList );

    KStandardAction::findNext(this, SLOT(findNext()) , actionCollection());
    KStandardAction::findPrev(this, SLOT(findPrevious()) , actionCollection());
   
    a = KStandardAction::fullScreen(this, SLOT(viewFullScreen(bool)), this, actionCollection());
    QList<QKeySequence> shortcutFullScreenList;
    shortcutFullScreenList << KStandardShortcut::fullScreen() << QKeySequence( Qt::Key_F11 );
    a->setShortcuts( shortcutFullScreenList );

    KStandardAction::home(this, SLOT(homePage()), actionCollection());
    KStandardAction::preferences(this, SLOT(preferences()), actionCollection());

    a = KStandardAction::redisplay(m_view, SLOT(webReload()), actionCollection());
    a->setText(i18n("Reload"));

    a = new KAction(KIcon("process-stop"), i18n("&Stop"), this);
    a->setShortcut(KShortcut(Qt::CTRL | Qt::Key_Period));
    actionCollection()->addAction(QLatin1String("stop"), a);
    connect(a, SIGNAL(triggered(bool)), m_view, SLOT(webStop()));

    // stop reload Action
    m_stopReloadAction = new KAction(this);
    actionCollection()->addAction(QLatin1String("stop_reload") , m_stopReloadAction);
    m_stopReloadAction->setShortcutConfigurable(false);
    connect(m_view, SIGNAL(browserTabLoading(bool)), this, SLOT(browserLoading(bool)));
    browserLoading(false); //first init for blank start page

    a = new KAction(i18n("Open Location"), this);
    a->setShortcut(Qt::CTRL + Qt::Key_L);
    actionCollection()->addAction(QLatin1String("open_location"), a);
    connect(a, SIGNAL(triggered(bool)) , this, SLOT(openLocation()));


    // ============================= Zoom Actions ===================================
    a = new KAction(KIcon("zoom-in"), i18n("&Enlarge Font"), this);
    a->setShortcut(KShortcut(Qt::CTRL | Qt::Key_Plus));
    actionCollection()->addAction(QLatin1String("bigger_font"), a);
    connect(a, SIGNAL(triggered(bool)), this, SLOT(viewTextBigger()));

    a = new KAction(KIcon("zoom-original"),  i18n("&Normal Font"), this);
    a->setShortcut(KShortcut(Qt::CTRL | Qt::Key_0));
    actionCollection()->addAction(QLatin1String("normal_font"), a);
    connect(a, SIGNAL(triggered(bool)), this, SLOT(viewTextNormal()));

    a = new KAction(KIcon("zoom-out"),  i18n("&Shrink Font"), this);
    a->setShortcut(KShortcut(Qt::CTRL | Qt::Key_Minus));
    actionCollection()->addAction(QLatin1String("smaller_font"), a);
    connect(a, SIGNAL(triggered(bool)), this, SLOT(viewTextSmaller()));

    // =============================== Tools Actions =================================
    a = new KAction(i18n("Page S&ource"), this);
    a->setIcon(KIcon("application-xhtml+xml"));
    actionCollection()->addAction(QLatin1String("page_source"), a);
    connect(a, SIGNAL(triggered(bool)), this, SLOT(viewPageSource()));

    a = new KAction(KIcon("tools-report-bug"), i18n("Web &Inspector"), this);
    a->setCheckable(true);
    actionCollection()->addAction(QLatin1String("web_inspector"), a);
    connect(a, SIGNAL(triggered(bool)), this, SLOT(toggleInspector(bool)));

    a = new KAction(KIcon("view-media-artist"), i18n("Private &Browsing"), this);
    a->setCheckable(true);
    actionCollection()->addAction(QLatin1String("private_browsing"), a);
    connect(a, SIGNAL(triggered(bool)), this, SLOT(privateBrowsing(bool)));

    a = new KAction(KIcon("edit-clear"), i18n("Clear Private Data..."), this);
    actionCollection()->addAction(QLatin1String("clear_private_data"), a);
    connect(a, SIGNAL(triggered(bool)), this, SLOT(clearPrivateData()));

    // ========================= History related actions ==============================
    a = KStandardAction::back(this, SLOT(openPrevious()) , actionCollection());

    m_historyBackMenu = new KMenu(this);
    a->setMenu(m_historyBackMenu);
    connect(m_historyBackMenu, SIGNAL(aboutToShow()), this, SLOT(aboutToShowBackMenu()));
    connect(m_historyBackMenu, SIGNAL(triggered(QAction *)), this, SLOT(openActionUrl(QAction *)));

    KStandardAction::forward(this, SLOT(openNext()) , actionCollection());

    // ============================== General Tab Actions ====================================
    a = new KAction(KIcon("tab-new"), i18n("New &Tab"), this);
    a->setShortcut(KShortcut(Qt::CTRL + Qt::Key_T));
    actionCollection()->addAction(QLatin1String("new_tab"), a);
    connect(a, SIGNAL(triggered(bool)), m_view, SLOT(newTab()));

    a = new KAction(KIcon("view-refresh"), i18n("Reload All Tabs"), this);
    actionCollection()->addAction( QLatin1String("reload_all_tabs"), a);
    connect(a, SIGNAL(triggered(bool)), m_view, SLOT(reloadAllTabs()) );

    a = new KAction(i18n("Show Next Tab"), this);
    a->setShortcuts(QApplication::isRightToLeft() ? KStandardShortcut::tabPrev() : KStandardShortcut::tabNext());
    actionCollection()->addAction(QLatin1String("show_next_tab"), a);
    connect(a, SIGNAL(triggered(bool)), m_view, SLOT(nextTab()));

    a = new KAction(i18n("Show Previous Tab"), this);
    a->setShortcuts(QApplication::isRightToLeft() ? KStandardShortcut::tabNext() : KStandardShortcut::tabPrev());
    actionCollection()->addAction(QLatin1String("show_prev_tab"), a);
    connect(a, SIGNAL(triggered(bool)), m_view, SLOT(previousTab()));

    // ============================== Indexed Tab Actions ====================================
    a = new KAction(KIcon("tab-close"), i18n("&Close Tab"), this);
    actionCollection()->addAction(QLatin1String("close_tab"), a);
    connect(a, SIGNAL(triggered(bool)), m_view->tabBar(), SLOT(closeTab()));

    a = new KAction(KIcon("tab-duplicate"), i18n("Clone Tab"), this);
    actionCollection()->addAction(QLatin1String("clone_tab"), a);
    connect(a, SIGNAL(triggered(bool)), m_view->tabBar(), SLOT(cloneTab()) );

    a = new KAction(KIcon("tab-close-other"), i18n("Close &Other Tabs"), this);
    actionCollection()->addAction( QLatin1String("close_other_tabs"), a);
    connect(a, SIGNAL(triggered(bool)), m_view->tabBar(), SLOT(closeOtherTabs()) );

    a = new KAction(KIcon("view-refresh"), i18n("Reload Tab"), this);
    actionCollection()->addAction( QLatin1String("reload_tab"), a);
    connect(a, SIGNAL(triggered(bool)), m_view->tabBar(), SLOT(reloadTab()) );

    // ----------------------- Bookmarks ToolBar Action --------------------------------------
    QAction *qa = m_bmBar->toggleViewAction();
    qa->setText( i18n("Bookmarks Toolbar") );
    qa->setIcon( KIcon("bookmark-toolbar") );
    actionCollection()->addAction(QLatin1String("bm_bar"), qa);

    // Bookmark Menu
    KActionMenu *bmMenu = Application::bookmarkProvider()->bookmarkActionMenu(this);
    bmMenu->setIcon(KIcon("bookmarks"));
    bmMenu->setDelayed(false);
    actionCollection()->addAction(QLatin1String("bookmarksActionMenu"), bmMenu);
}


void MainWindow::setupTools()
{
    KActionMenu *toolsMenu = new KActionMenu(KIcon("configure"), i18n("&Tools"), this);
    toolsMenu->setDelayed(false);

    toolsMenu->addAction(actionByName(KStandardAction::name(KStandardAction::Open)));
    toolsMenu->addAction(actionByName(KStandardAction::name(KStandardAction::SaveAs)));
    toolsMenu->addAction(actionByName(KStandardAction::name(KStandardAction::Print)));
    toolsMenu->addAction(actionByName(KStandardAction::name(KStandardAction::Find)));

    KActionMenu *fontMenu = new KActionMenu(KIcon("page-zoom"), i18n("Zoom"), this);
    fontMenu->addAction(actionByName(QLatin1String("smaller_font")));
    fontMenu->addAction(actionByName(QLatin1String("normal_font")));
    fontMenu->addAction(actionByName(QLatin1String("bigger_font")));
    toolsMenu->addAction(fontMenu);

    toolsMenu->addSeparator();

    toolsMenu->addAction(actionByName(QLatin1String("private_browsing")));
    toolsMenu->addAction(actionByName(QLatin1String("clear_private_data")));

    toolsMenu->addSeparator();

    KActionMenu *webMenu = new KActionMenu(KIcon("applications-development-web"), i18n("Web Development"), this);
    webMenu->addAction(actionByName(QLatin1String("web_inspector")));
    webMenu->addAction(actionByName(QLatin1String("page_source")));
    toolsMenu->addAction(webMenu);

    toolsMenu->addSeparator();

    toolsMenu->addAction(actionByName(QLatin1String("bm_bar")));
    toolsMenu->addAction(actionByName(QLatin1String("show_history_panel")));
	toolsMenu->addAction(actionByName(QLatin1String("show_bookmarks_panel")));
    toolsMenu->addAction(actionByName(KStandardAction::name(KStandardAction::FullScreen)));

    toolsMenu->addSeparator();

    helpMenu()->setIcon(KIcon("help-browser"));
    toolsMenu->addAction(helpMenu()->menuAction());
    toolsMenu->addAction(actionByName(KStandardAction::name(KStandardAction::Preferences)));

    // adding rekonq_tools to rekonq actionCollection
    actionCollection()->addAction(QLatin1String("rekonq_tools"), toolsMenu);
}


void MainWindow::setupSidePanel()
{
    // Setup history side panel
    m_sidePanel = new SidePanel(i18n("History Panel"), this);
    connect(m_sidePanel, SIGNAL(openUrl(const KUrl&)), Application::instance(), SLOT(loadUrl(const KUrl&)));
    connect(m_sidePanel, SIGNAL(destroyed()), Application::instance(), SLOT(saveConfiguration()));

    addDockWidget(Qt::LeftDockWidgetArea, m_sidePanel);

    // setup side panel actions
    KAction* a = (KAction *) m_sidePanel->toggleViewAction();
    a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_H)); // WARNING : is this the right shortcut ??
    a->setIcon(KIcon("view-history"));
    actionCollection()->addAction(QLatin1String("show_history_panel"), a);
}

void MainWindow::setupBookmarksPanel()
{
	m_bookmarksPanel = new BookmarksPanel(i18n("Bookmarks Panel"), this);
    connect(m_bookmarksPanel, SIGNAL(openUrl(const KUrl&)), Application::instance(), SLOT(loadUrl(const KUrl&)));
    connect(m_bookmarksPanel, SIGNAL(destroyed()), Application::instance(), SLOT(saveConfiguration()));

    addDockWidget(Qt::LeftDockWidgetArea, m_bookmarksPanel);

    // setup side panel actions
    KAction* a = (KAction *) m_bookmarksPanel->toggleViewAction();
    a->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_B)); // FIXME: this shortcut should be configurable !
    a->setIcon(KIcon("bookmarks-organize"));
    actionCollection()->addAction(QLatin1String("show_bookmarks_panel"), a);
}


void MainWindow::updateConfiguration()
{
    // ============== General ==================
    m_view->updateTabBar();

    // =========== Fonts ==============
    QWebSettings *defaultSettings = QWebSettings::globalSettings();

    int fnSize = ReKonfig::fontSize();

    QFont standardFont = ReKonfig::standardFont();
    defaultSettings->setFontFamily(QWebSettings::StandardFont, standardFont.family());
    defaultSettings->setFontSize(QWebSettings::DefaultFontSize, fnSize);

    QFont fixedFont = ReKonfig::fixedFont();
    defaultSettings->setFontFamily(QWebSettings::FixedFont, fixedFont.family());
    defaultSettings->setFontSize(QWebSettings::DefaultFixedFontSize, fnSize);

    // ================ WebKit ============================
    defaultSettings->setAttribute(QWebSettings::AutoLoadImages, ReKonfig::autoLoadImages());
    defaultSettings->setAttribute(QWebSettings::JavascriptEnabled, ReKonfig::javascriptEnabled());
    defaultSettings->setAttribute(QWebSettings::JavaEnabled, ReKonfig::javaEnabled());
    defaultSettings->setAttribute(QWebSettings::PluginsEnabled, ReKonfig::pluginsEnabled());
    defaultSettings->setAttribute(QWebSettings::JavascriptCanOpenWindows, ReKonfig::javascriptCanOpenWindows());
    defaultSettings->setAttribute(QWebSettings::JavascriptCanAccessClipboard, ReKonfig::javascriptCanAccessClipboard());
    defaultSettings->setAttribute(QWebSettings::LinksIncludedInFocusChain, ReKonfig::linksIncludedInFocusChain());
    defaultSettings->setAttribute(QWebSettings::ZoomTextOnly, ReKonfig::zoomTextOnly());
    defaultSettings->setAttribute(QWebSettings::PrintElementBackgrounds, ReKonfig::printElementBackgrounds());

    // ===== HTML 5 features WebKit support ======
    defaultSettings->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, ReKonfig::offlineStorageDatabaseEnabled());
    defaultSettings->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled, ReKonfig::offlineWebApplicationCacheEnabled());
    defaultSettings->setAttribute(QWebSettings::LocalStorageDatabaseEnabled, ReKonfig::localStorageDatabaseEnabled());
    if(ReKonfig::localStorageDatabaseEnabled())
    {
        QString path = KStandardDirs::locateLocal("cache", QString("WebkitLocalStorage/rekonq"), true);
        path.remove("rekonq");
        QWebSettings::setOfflineStoragePath(path);
        QWebSettings::setOfflineStorageDefaultQuota(50000);
    }

    // Applies user defined CSS to all open webpages. If there no longer is a
    // user defined CSS removes it from all open webpages.
    defaultSettings->setUserStyleSheetUrl(ReKonfig::userCSS());

    // ====== load Settings on main classes
    Application::historyManager()->loadSettings();

    defaultSettings = 0;
}


void MainWindow::updateBrowser()
{
    updateConfiguration();
    mainView()->reloadAllTabs();
}


void MainWindow::openLocation()
{
    m_view->urlBar()->selectAll();
    m_view->urlBar()->setFocus();
}


void MainWindow::fileSaveAs()
{
    KUrl srcUrl = currentTab()->url();

    QString name = srcUrl.fileName();
    if(name.isNull())
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
    connect(s, SIGNAL(settingsChanged(const QString&)), this, SLOT(updateBrowser()));

    s->exec();
    delete s;
}


void MainWindow::updateActions()
{
    QAction *historyBackAction = actionByName(KStandardAction::name(KStandardAction::Back));
    historyBackAction->setEnabled(currentTab()->history()->canGoBack());

    QAction *historyForwardAction = actionByName(KStandardAction::name(KStandardAction::Forward));
    historyForwardAction->setEnabled(currentTab()->history()->canGoForward());
}


void MainWindow::updateWindowTitle(const QString &title)
{
    if (title.isEmpty())
    {
        setWindowTitle("rekonq");
    }
    else
    {
        setWindowTitle(title + " - rekonq");
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
    if(frame == 0)
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
                            " site icons will not be stored, the session will not be saved, "
                            " and searches are not added to the pop-up menu in the Google search box."
                            "  Until you close the window, you can still click the Back and Forward buttons"
                            " to return to the web pages you have opened.</p>", title);

        int button = KMessageBox::questionYesNo(this, text, title);
        if (button == KMessageBox::Yes)
        {
            settings->setAttribute(QWebSettings::PrivateBrowsingEnabled, true);
            m_view->urlBar()->setBackgroundColor(Qt::lightGray); // palette().color(QPalette::Active, QPalette::Background));
        }
        else
        {
            actionCollection()->action("private_browsing")->setChecked(false);
        }
    }
    else
    {
        settings->setAttribute(QWebSettings::PrivateBrowsingEnabled, false);
        m_view->urlBar()->setBackgroundColor(palette().color(QPalette::Active, QPalette::Base));

        m_lastSearch.clear();
        m_view->clear();
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


void MainWindow::findNext()
{
    if (!currentTab() && m_lastSearch.isEmpty())
        return;

    QWebPage::FindFlags options = QWebPage::FindWrapsAroundDocument;
    if (m_findBar->matchCase())
        options |= QWebPage::FindCaseSensitively;

    m_findBar->notifyMatch(currentTab()->findText(m_lastSearch, options));
}


void MainWindow::findPrevious()
{
    if (!currentTab() && m_lastSearch.isEmpty())
        return;

    QWebPage::FindFlags options = QWebPage::FindBackward | QWebPage::FindWrapsAroundDocument;
    if (m_findBar->matchCase())
        options |= QWebPage::FindCaseSensitively;

    m_findBar->notifyMatch(currentTab()->findText(m_lastSearch, options));
}


void MainWindow::viewTextBigger()
{
    if (!currentTab())
        return;
    currentTab()->setTextSizeMultiplier(currentTab()->textSizeMultiplier() + 0.1);
}


void MainWindow::viewTextNormal()
{
    if (!currentTab())
        return;
    currentTab()->setTextSizeMultiplier(1.0);
}


void MainWindow::viewTextSmaller()
{
    if (!currentTab())
        return;
    currentTab()->setTextSizeMultiplier(currentTab()->textSizeMultiplier() - 0.1);
}


void MainWindow::viewFullScreen(bool makeFullScreen)
{
    // state flags
    static bool bookmarksToolBarFlag;
    static bool sidePanelFlag;
	static bool bookmarksPanelFlag;

    if (makeFullScreen == true)
    {
        // save current state
        bookmarksToolBarFlag = m_bmBar->isHidden();
        m_bmBar->hide();

        sidePanelFlag = sidePanel()->isHidden();
        sidePanel()->hide();

		 bookmarksPanelFlag = bookmarksPanel()->isHidden();
        bookmarksPanel()->hide();

        // hide main toolbar
        m_mainBar->hide();
    }
    else
    {
        // show main toolbar
        m_mainBar->show();

        // restore previous state
        if (!bookmarksToolBarFlag)
            m_bmBar->show();
        if (!sidePanelFlag)
            sidePanel()->show();
        if (!bookmarksPanelFlag)
            bookmarksPanel()->show();
    }

    KToggleFullScreenAction::setFullScreen(this, makeFullScreen);
}


void MainWindow::viewPageSource()
{
    if (!currentTab())
        return;

    KUrl url(currentTab()->url());
    bool isTempFile = false;
    if (!url.isLocalFile())
    {
        KTemporaryFile sourceFile;

        /// TODO: autochoose tempfile suffix
        sourceFile.setSuffix(QString(".html"));
        sourceFile.setAutoRemove(false);

        if (sourceFile.open())
        {
            sourceFile.write(currentTab()->page()->mainFrame()->toHtml().toUtf8());

            url = KUrl();
            url.setPath(sourceFile.fileName());
            isTempFile = true;
        }
    }
    KRun::runUrl(url, QLatin1String("text/plain"), this, isTempFile);
}


void MainWindow::homePage()
{
    currentTab()->load( QUrl(ReKonfig::homePage()) );
}


void MainWindow::toggleInspector(bool enable)
{
    QWebSettings::globalSettings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, enable);
    if (enable)
    {
        int result = KMessageBox::questionYesNo(this,
                        i18n("The web inspector will only work correctly for pages that were loaded after enabling.\n" \
                             "Do you want to reload all pages?"),
                        i18n("Web Inspector")
                     );

        if (result == KMessageBox::Yes)
        {
            m_view->reloadAllTabs();
        }
    }
}


MainView *MainWindow::mainView() const
{
    return m_view;
}



WebView *MainWindow::currentTab() const
{
    return m_view->currentWebView();
}


void MainWindow::browserLoading(bool v)
{
    QAction *stop = actionCollection()->action("stop");
    QAction *reload = actionCollection()->action("view_redisplay");
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


void MainWindow::openPrevious()
{
    QWebHistory *history = currentTab()->history();
    if (history->canGoBack())
        history->goToItem(history->backItem());
}


void MainWindow::openNext()
{
    QWebHistory *history = currentTab()->history();
    if (history->canGoForward())
        history->goToItem(history->forwardItem());
}


bool MainWindow::queryClose()
{
    if (m_view->count() > 1)
    {

        int answer = KMessageBox::questionYesNoCancel(
                         this,
                         i18np("Are you sure you want to close the window?\n" \
                               "You still have 1 tab open.",
                               "Are you sure you want to close the window?\n" \
                               "You still have %1 tabs open.",
                               m_view->count()),
                         i18n("Closing rekonq"),
                         KStandardGuiItem::quit(),
                         KGuiItem(i18n("C&lose Current Tab"), KIcon("tab-close")),
                         KStandardGuiItem::cancel(),
                         "confirmClosingMultipleTabs");

        switch (answer)
        {
        case KMessageBox::Yes:
            // Quit
            return true;
            break;
        case KMessageBox::No:
            // Close only the current tab
            m_view->closeTab();
        default:
            return false;
        }
    }

    return true;
}


void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // hide findbar
    if (event->key() == Qt::Key_Escape)
    {
        m_findBar->hide();
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

    // close current tab action
    if ((event->modifiers() == Qt::ControlModifier) && event->key() == Qt::Key_W)
    {
        m_view->closeTab(m_view->currentIndex());
        return;
    }

    KMainWindow::keyPressEvent(event);
}


QAction *MainWindow::actionByName(const QString name)
{
    QAction *ret = actionCollection()->action(name);

    if (ret)
        return ret;

    /* else */
    kWarning() << "Action named: " << name << " not found, returning empty action.";

    return new QAction(this);  // return empty object instead of NULL pointer
}


void MainWindow::notifyMessage(const QString &msg, Rekonq::Notify status)
{
    if (this != QApplication::activeWindow())
    {
        return;
    }

    // deleting popus if empty msgs
    if(msg.isEmpty())
    {
        m_hidePopup->start(250);
        return;
    }

    m_hidePopup->stop();


    switch(status)
    {
    case Rekonq::Info:
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
    QSize labelSize(label->fontMetrics().width(msg)+2*margin, label->fontMetrics().height()+2*margin);
    if (labelSize.width() > width()) labelSize.setWidth(width());
    m_popup->setFixedSize(labelSize);
    m_popup->layout()->setAlignment(Qt::AlignTop);
    m_popup->layout()->setMargin(margin);

    // useful values
    bool scrollbarIsVisible = m_view->currentWebView()->page()->currentFrame()->scrollBarMaximum(Qt::Horizontal);
    int scrollbarSize = 0;
    if (scrollbarIsVisible)
    {
        //TODO: detect QStyle size
        scrollbarSize = 17;
    }
    QPoint webViewOrigin = m_view->currentWebView()->mapToGlobal(QPoint(0,0));
    int bottomLeftY=webViewOrigin.y() + m_view->currentWebView()->page()->viewportSize().height() - labelSize.height() - scrollbarSize;

    // setting popup in bottom-left position
    int x = geometry().x();
    int y = bottomLeftY;

    QPoint mousePos = m_view->currentWebView()->mapToGlobal(m_view->currentWebView()->mousePos());
    if(QRect(webViewOrigin.x(),bottomLeftY,labelSize.width(),labelSize.height()).contains(mousePos))
    {
        // setting popup above the mouse
        y = bottomLeftY - labelSize.height();
    }

    m_popup->show(QPoint(x,y));
}


void MainWindow::clearPrivateData()
{
    QPointer<KDialog> dialog = new KDialog(this, Qt::Sheet);
    dialog->setButtons(KDialog::Ok | KDialog::Cancel);

    Ui::ClearDataWidget clearWidget;
    QWidget widget;
    clearWidget.setupUi(&widget);

    dialog->setMainWidget(&widget);

    if (dialog->exec() == KDialog::Accepted)
    {
        if(clearWidget.clearHistory->isChecked())
        {
            Application::historyManager()->clear();
        }

        if(clearWidget.clearCookies->isChecked())
        {
            Application::cookieJar()->clear();
        }

        if(clearWidget.clearCachedPages->isChecked())
        {
            Application::networkAccessManager()->resetDiskCache();
        }

        if(clearWidget.clearWebIcons->isChecked())
        {
            QWebSettings::clearIconDatabase();
        }

        if(clearWidget.homePageThumbs->isChecked())
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
}


void MainWindow::aboutToShowBackMenu()
{
    m_historyBackMenu->clear();
    if (!currentTab())
        return;
    QWebHistory *history = currentTab()->history();
    int historyCount = history->count();

    // Limit history views in the menu to 8
    if(historyCount > 8)
        historyCount = 8;

    for (int i = history->backItems(historyCount).count() - 1; i >= 0; --i)
    {
        QWebHistoryItem item = history->backItems(history->count()).at(i);
        KAction *action = new KAction(this);
        action->setData( i - history->currentItemIndex() );
        QIcon icon = Application::icon(item.url());
        action->setIcon(icon);
        action->setText(item.title());
        m_historyBackMenu->addAction(action);
    }
}


void MainWindow::openActionUrl(QAction *action)
{
    int offset = action->data().toInt();
    QWebHistory *history = currentTab()->history();

    if(!history->itemAt(offset).isValid())
    {
        kDebug() << "Invalid Offset!";
        return;
    }

    if (offset < 0)
    {
        history->goToItem(history->itemAt(offset)); // back
        return;
    }

    if (offset > 0)
    {
        history->goToItem(history->forwardItems(history->count() - offset).back()); // forward FIXME CRASH
    }

}


bool MainWindow::newTabPage(const KUrl &url)
{
    if(m_loadingNewTabPage)
        return false;

    if (    url == KUrl("about:closedTabs")
         || url == KUrl("about:history")
         || url == KUrl("about:bookmarks")
         || url == KUrl("about:favorites")
         || url == KUrl("about:home")
    )
    {
        m_loadingNewTabPage = true;
        kDebug() << "loading home: " << url;
        WebView *w = currentTab();
        NewTabPage p;
        QString html = p.newTabPageCode(url);
        w->setHtml(html, url);
        m_loadingNewTabPage = false;
        return true;
    }
    return false;
}
