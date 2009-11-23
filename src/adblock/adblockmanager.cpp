/* ============================================================
*
* This file is a part of the rekonq project
*
* Copyright (C) 2009 by Andrea Diamantini <adjam7 at gmail dot com>
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


#include "adblockmanager.h"
#include "adblockmanager.moc"


// Local Includes
#include "adblockrule.h"

// KDE Includes
#include <KSharedConfig>
#include <KConfigGroup>
#include <KDebug>

// Qt Includes
#include <QUrl>


AdBlockManager::AdBlockManager(QObject *parent)
    : QObject(parent)
    , _isAdblockEnabled(false)
{
    loadSettings();
}


AdBlockManager::~AdBlockManager()
{
}


void AdBlockManager::loadSettings()
{
    _blockList.clear();
    
    KSharedConfig::Ptr config = KSharedConfig::openConfig("webkitrc", KConfig::NoGlobals);
    KConfigGroup cg(config, "adblock");
    
    _isAdblockEnabled = cg.readEntry("Enabled", false);

    int num = cg.readEntry("Count", 0);
    for (int i = 0; i < num; ++i)
    {
        QString key = "Filter-" + QString::number(i);
        QString filter = cg.readEntry( key, QString() );
        if(!filter.isEmpty())
        {
            if (filter.startsWith(QLatin1String("@@")))
                _whiteList << filter;
            else
                _blockList << filter;
        }
    }
}


bool AdBlockManager::isUrlAllowed(const QUrl &url)
{
    kDebug() << "matching rule...";
    if (!_isAdblockEnabled)
        return true;

    QString urlString = QString::fromUtf8(url.toEncoded());
    
    foreach(const QString &str, _whiteList)
    {
        AdBlockRule rule(str);
        kDebug() << "checking white list rule...";
        if(rule.match(urlString))
            return true;
    }
    
    foreach(const QString &str, _blockList)
    {
        AdBlockRule rule(str);
        kDebug() << "checking block list rule...";
        if(rule.match(urlString))
            return false;
    }
    
    kDebug() << "done";
    return true;
}
