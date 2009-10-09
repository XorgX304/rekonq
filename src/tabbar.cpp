/* ============================================================
*
* This file is a part of the rekonq project
*
* Copyright (C) 2008 Benjamin C. Meyer <ben@meyerhome.net>
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


//Self Includes
#include "tabbar.h"
#include "tabbar.moc"

// Local Includes
#include "rekonq.h"
#include "application.h"
#include "mainwindow.h"
#include "urlbar.h"
#include "webview.h"
#include "websnap.h"

// KDE Includes
#include <KShortcut>
#include <KStandardShortcut>
#include <KDebug>
#include <KGlobalSettings>
#include <KPassivePopup>

// Qt Includes
#include <QtCore/QString>

#include <QtGui/QFont>
#include <QtGui/QToolButton>
#include <QtGui/QLabel>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QVBoxLayout>


#define BASE_WIDTH_DIVISOR    4
#define MIN_WIDTH_DIVISOR     8


TabBar::TabBar(MainView *parent)
        : KTabBar(parent)
        , m_parent(parent)
        , m_addTabButton(new QToolButton(this))
        , m_currentTabPreview(-1)
{
    setElideMode(Qt::ElideRight);

    setDocumentMode(true);
    setTabsClosable(true);
    setMovable(true);

    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this,
            SLOT(contextMenuRequested(const QPoint &)));

    QTimer::singleShot(0, this, SLOT(postLaunch()));
}


TabBar::~TabBar()
{
}


void TabBar::postLaunch()
{
    // HACK this is used to fix add tab button position
    QToolButton *secondAddTabButton = new QToolButton(this);
    secondAddTabButton->setAutoRaise(true);
    secondAddTabButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    
    // Find the correct MainWindow of this tab button
    MainWindowList list = Application::instance()->mainWindowList();
    Q_FOREACH(QPointer<MainWindow> w, list)
    {
        if (w->isAncestorOf(this))
        {
            m_addTabButton->setDefaultAction(w->actionByName("new_tab"));
            secondAddTabButton->setDefaultAction(w->actionByName("new_tab"));
            break;
        }
    }
    
    m_addTabButton->setAutoRaise(true);
    m_addTabButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_addTabButton->show();
    
    // stupid tabbar, that's what you gain...
    m_parent->setCornerWidget(secondAddTabButton);
    m_parent->cornerWidget()->hide();
}


QSize TabBar::tabSizeHint(int index) const
{
    int buttonSize = m_addTabButton->size().width();
    int tabBarWidth = m_parent->size().width() - buttonSize;
    int baseWidth =  m_parent->sizeHint().width()/BASE_WIDTH_DIVISOR;
    int minWidth =  m_parent->sizeHint().width()/MIN_WIDTH_DIVISOR;

    int w;
    if (baseWidth*count()<tabBarWidth)
    {
        w = baseWidth;
    }
    else 
    {
        if (count() > 0 && tabBarWidth/count()>minWidth)
        {
            w = tabBarWidth/count();
        }
        else
        {
            w = minWidth;
        }
    }
    
    int h = KTabBar::tabSizeHint(index).height();

    QSize ts = QSize(w, h);
    return ts;
}


void TabBar::tabLayoutChange()
{
    setTabButtonPosition();
    KTabBar::tabLayoutChange();
}


void TabBar::contextMenuRequested(const QPoint &position)
{
    KMenu menu;
    MainWindow *mainWindow = Application::instance()->mainWindow();

    menu.addAction(mainWindow->actionByName(QLatin1String("new_tab")));
    int index = tabAt(position);
    if (-1 != index)
    {
        m_actualIndex = index;

        menu.addAction(KIcon("tab-duplicate"), i18n("Clone Tab"), this, SLOT(cloneTab()));
        menu.addSeparator();
        menu.addAction(KIcon("tab-close"), i18n("&Close Tab"), this, SLOT(closeTab()));
        menu.addAction(KIcon("tab-close-other"), i18n("Close &Other Tabs"), this, SLOT(closeOtherTabs()));
        menu.addSeparator();
        menu.addAction(KIcon("view-refresh"), i18n("Reload Tab"), this, SLOT(reloadTab()));
    }
    else
    {
        menu.addSeparator();
    }
    menu.addAction(i18n("Reload All Tabs"), this, SIGNAL(reloadAllTabs()));
    menu.exec(QCursor::pos());
}


void TabBar::cloneTab()
{
    emit cloneTab(m_actualIndex);
}


void TabBar::closeTab()
{
    emit closeTab(m_actualIndex);
}


void TabBar::closeOtherTabs()
{
    emit closeOtherTabs(m_actualIndex);
}


void TabBar::reloadTab()
{
    emit reloadTab(m_actualIndex);
}


void TabBar::setTabButtonPosition()
{
    if(count() >= MIN_WIDTH_DIVISOR - 1)
    {
        if(m_addTabButton->isVisible())
        {
            m_addTabButton->hide();
            m_parent->cornerWidget()->show();
        }
        return;
    }
    
    if(m_addTabButton->isHidden())
    {
        m_addTabButton->show();
        m_parent->cornerWidget()->hide();
    }

    int tabWidgetWidth = frameSize().width();
    int tabBarWidth = tabSizeHint(0).width()*count();
    int newPosY = height() - m_addTabButton->height();
    int newPosX = tabWidgetWidth - m_addTabButton->width();

    if (tabBarWidth + m_addTabButton->width() < tabWidgetWidth)
        newPosX = tabBarWidth;

    m_addTabButton->move(newPosX, newPosY);
    m_addTabButton->show();
}


void TabBar::showTabPreview(int tab)
{
    WebView *view = m_parent->webView(tab);
    
    int w = tabSizeHint(tab).width();
    int h = w*((0.0 + view->height())/view->width());

    //delete previous tab preview
    if (m_previewPopup)
    {
        delete m_previewPopup;
    }
    
    m_previewPopup = new KPassivePopup(this);
    m_previewPopup->setFrameShape(QFrame::StyledPanel);
    m_previewPopup->setFrameShadow(QFrame::Plain);
    m_previewPopup->setFixedSize(w, h);
    QLabel *l = new QLabel();
    l->setPixmap(WebSnap::renderPreview(*(view->page()), w, h));
    m_previewPopup->setView(l);
    m_previewPopup->layout()->setAlignment(Qt::AlignTop);
    m_previewPopup->layout()->setMargin(0);

    QPoint pos( tabRect(tab).x() , tabRect(tab).y() + tabRect(tab).height() );
    m_previewPopup->show(mapToGlobal(pos));
}


void TabBar::mouseMoveEvent(QMouseEvent *event)
{
    if (ReKonfig::alwaysShowTabPreviews())
    {
        //Find the tab under the mouse
        int i = 0;
        int tab = -1;
        while (i<count() && tab==-1)
        {
            if (tabRect(i).contains(event->pos())) 
            {
                tab = i;
            }
            i++;
        }

        //if found and not the current tab then show tab preview
        if (tab != -1 && tab != currentIndex() && m_currentTabPreview != tab)
        {
            showTabPreview(tab);
            m_currentTabPreview = tab;
        }

        //if current tab or not found then hide previous tab preview
        if (tab==currentIndex() || tab==-1)
        {
            if ( m_previewPopup)
            {
                m_previewPopup->hide();
            }
            m_currentTabPreview = -1;
        }
    }

    KTabBar::mouseMoveEvent(event);
}


void TabBar::leaveEvent(QEvent *event)
{
    if (ReKonfig::alwaysShowTabPreviews())
    {
        //if leave tabwidget then hide previous tab preview
        if ( m_previewPopup)
        {
            m_previewPopup->hide();
        }
        m_currentTabPreview = -1;
    }

    KTabBar::leaveEvent(event);
}
