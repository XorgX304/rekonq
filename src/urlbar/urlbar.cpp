/* ============================================================
*
* This file is a part of the rekonq project
*
* Copyright (C) 2008-2009 by Andrea Diamantini <adjam7 at gmail dot com>
* Copyright (C) 2009 by Domrachev Alexandr <alexandr.domrachev@gmail.com>
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
#include "urlbar.h"
#include "urlbar.moc"

// Local Includes
#include "application.h"
#include "lineedit.h"
#include "mainwindow.h"
#include "webview.h"
#include "urlresolver.h"

// KDE Includes
#include <KDebug>
#include <KCompletionBox>
#include <KUrl>

// Qt Includes
#include <QPainter>
#include <QPaintEvent>
#include <QPalette>
#include <QTimer>
#include <QVBoxLayout>

QColor UrlBar::s_defaultBaseColor;


UrlBar::UrlBar(QWidget *parent)
    : KComboBox(true, parent)
    , m_lineEdit(new LineEdit)
    , m_progress(0)
    , m_box(new CompletionWidget(this))
{
    //cosmetic
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumWidth(180);

    // signal handlings
    setTrapReturnKey(true);
    setUrlDropsEnabled(true);
    
    // Make m_lineEdit background transparent
    QPalette p = m_lineEdit->palette();
    p.setColor(QPalette::Base, Qt::transparent);
    m_lineEdit->setPalette(p);

    if (!s_defaultBaseColor.isValid())
    {
        s_defaultBaseColor = palette().color(QPalette::Base);
    }

    setLineEdit(m_lineEdit);

    // clear the URL bar
    m_lineEdit->clear();
    // load urls on activated urlbar signal
    connect(this, SIGNAL(returnPressed(const QString&)), SLOT(activated(const QString&)));
    
    installEventFilter(m_box);
    connect(m_box, SIGNAL(chosenUrl(const QString&, Rekonq::OpenType)), SLOT(activated(const QString&, Rekonq::OpenType)));
}


UrlBar::~UrlBar()
{
}


void UrlBar::selectAll() const
{
    m_lineEdit->selectAll();
}


KUrl UrlBar::url() const
{
    return m_currentUrl;
}


void UrlBar::setUrl(const QUrl& url)
{
    if(url.scheme() == "about")
    {
        m_currentUrl = KUrl();
        updateUrl();
        setFocus();
    }
    else
    {
        m_currentUrl = KUrl(url);
        updateUrl();
    }

}


void UrlBar::setProgress(int progress)
{
    m_progress = progress;
    update();
}


void UrlBar::updateUrl()
{
    // Don't change my typed url...
    // FIXME this is not a proper solution (also if it works...)
    if(hasFocus())
    {
        kDebug() << "Don't change my typed url...";
        return;
    }

    KIcon icon;
    if(m_currentUrl.isEmpty()) 
    {
        icon = KIcon("arrow-right");
    }
    else 
    {
        icon = Application::icon(m_currentUrl);
    }

    if (count())
    {
        changeUrl(0, icon, m_currentUrl);
    }
    else
    {
        insertUrl(0, icon, m_currentUrl);
    }

    setCurrentIndex(0);

    // important security consideration: always display the beginning
    // of the url rather than its end to prevent spoofing attempts.
    // Must be AFTER setCurrentIndex
    if (!hasFocus())
    {
        m_lineEdit->setCursorPosition(0);
    }
}


void UrlBar::activated(const QString& urlString, Rekonq::OpenType type)
{
    disconnect(this, SIGNAL(editTextChanged(const QString &)), this, SLOT(suggestUrls(const QString &)));
    
    if (urlString.isEmpty())
        return;

    clearFocus();
    setUrl(urlString);
    Application::instance()->loadUrl(m_currentUrl, type);
}


void UrlBar::loadFinished(bool)
{
    // reset progress bar after small delay
    m_progress = 0;
    QTimer::singleShot(200, this, SLOT(update()));
}


void UrlBar::updateProgress(int progress)
{
    m_progress = progress;
    update();
}


void UrlBar::paintEvent(QPaintEvent *event)
{
    // set background color of UrlBar
    QPalette p = palette();
    p.setColor(QPalette::Base, s_defaultBaseColor);
    setPalette(p);

    KComboBox::paintEvent(event);

    if (!hasFocus())
    {
        QPainter painter(this);

        QColor loadingColor;
        if (m_currentUrl.scheme() == QLatin1String("https"))
        {
            loadingColor = QColor(248, 248, 100);
        }
        else
        {
            loadingColor = QColor(116, 192, 250);
        }
        painter.setBrush(generateGradient(loadingColor, height()));
        painter.setPen(Qt::transparent);

        QRect backgroundRect = m_lineEdit->frameGeometry();
        int mid = backgroundRect.width() * m_progress / 100;
        QRect progressRect(backgroundRect.x(), backgroundRect.y(), mid, backgroundRect.height());
        painter.drawRect(progressRect);
        painter.end();
    }
}


QSize UrlBar::sizeHint() const
{
    return m_lineEdit->sizeHint();
}


QLinearGradient UrlBar::generateGradient(const QColor &color, int height)
{
    QColor base = s_defaultBaseColor;
    base.setAlpha(0);
    QColor barColor = color;
    barColor.setAlpha(200);
    QLinearGradient gradient(0, 0, 0, height);
    gradient.setColorAt(0, base);
    gradient.setColorAt(0.25, barColor.lighter(120));
    gradient.setColorAt(0.5, barColor);
    gradient.setColorAt(0.75, barColor.lighter(120));
    gradient.setColorAt(1, base);
    return gradient;
}


void UrlBar::setBackgroundColor(QColor c)
{
    s_defaultBaseColor = c;
    update();
}


bool UrlBar::isLoading()
{
    if(m_progress == 0)
    {
        return false;
    }
    return true;
}

void UrlBar::keyPressEvent(QKeyEvent *event)
{

    // this handles the Modifiers + Return key combinations
    QString currentText = m_lineEdit->text().trimmed();
    if ((event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
        && !currentText.startsWith(QLatin1String("http://"), Qt::CaseInsensitive))
    {
        QString append;
        if (event->modifiers() == Qt::ControlModifier)
        {
            append = QLatin1String(".com");
        }
        else if (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier))
        {
            append = QLatin1String(".org");
        }
        else if (event->modifiers() == Qt::ShiftModifier)
        {
            append = QLatin1String(".net");
        }

        QUrl url(QLatin1String("http://www.") + currentText);
        QString host = url.host();
        if (!host.endsWith(append, Qt::CaseInsensitive))
        {
            host += append;
            url.setHost(host);
            m_lineEdit->setText(url.toString());
        }
    }
    
    KComboBox::keyPressEvent(event);
}


void UrlBar::suggestUrls(const QString &text)
{   
    if (!hasFocus())
    {
        return;
    }

    if(text.isEmpty())
    {
        m_box->hide();
        return;
    }

    UrlResolver res(text);
    UrlSearchList list = res.orderedSearchItems();

    if(list.count() > 0)
    {
        m_box->clear();
        m_box->insertSearchList(list);
        m_box->popup();
    }
}

void UrlBar::focusInEvent(QFocusEvent *event)
{
    // activate suggestions on edit text
    connect(this, SIGNAL(editTextChanged(const QString &)), this, SLOT(suggestUrls(const QString &)));
    
    KComboBox::focusInEvent(event);
}
