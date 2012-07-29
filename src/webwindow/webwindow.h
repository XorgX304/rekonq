/***************************************************************************
 *   Copyright (C) 2012 by Andrea Diamantini <adjam7@gmail.com>                            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/


#ifndef WEB_WINDOW
#define WEB_WINDOW


// Rekonq Includes
#include "rekonq_defines.h"

// Qt Includes
#include <QWidget>

// Forward Declarations
class WebPage;

class QWebView;
class QLineEdit;
class QPixmap;
class QUrl;


class WebWindow : public QWidget
{
    Q_OBJECT

public:
    WebWindow(QWidget *parent = 0);
    WebWindow(WebPage *page, QWidget *parent = 0);

    void load(const QUrl &);

    WebPage *page();

    QUrl url() const;
    QString title() const;
    QIcon icon() const;
    
    QPixmap tabPreview(int width, int height);

    bool isLoading();

private:
    void init();
    
private Q_SLOTS:
    void checkLoadUrl();
    void setUrlText(const QUrl &);

    void checkLoadProgress(int);

Q_SIGNALS:
    void titleChanged(QString);

    void loadStarted();
    void loadProgress(int);
    void loadFinished(bool);

    void pageCreated(WebPage *);

private:
    int _progress;

    QWebView *_view;
    QLineEdit *_edit;
};

#endif // WEB_WINDOW
