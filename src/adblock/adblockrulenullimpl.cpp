/* ============================================================
*
* This file is a part of the rekonq project
*
* Copyright (C) 2011-2012 by Andrea Diamantini <adjam7 at gmail dot com>
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
#include "adblockrulenullimpl.h"

// Rekonq Includes
#include "rekonq_defines.h"

// Qt Includes
#include <QStringList>


AdBlockRuleNullImpl::AdBlockRuleNullImpl(const QString &filter)
    : AdBlockRuleImpl(filter)
{
}


bool AdBlockRuleNullImpl::match(const QNetworkRequest &, const QString &, const QString &) const
{
    return false;
}


bool AdBlockRuleNullImpl::isNullFilter(const QString &filter)
{
    QString parsedLine = filter;

    const int optionsNumber = parsedLine.lastIndexOf(QL1C('$'));
    if (optionsNumber == 0)
        return false;

    const QStringList options(parsedLine.mid(optionsNumber + 1).split(QL1C(',')));

    Q_FOREACH(const QString & option, options)
    {
        // NOTE:
        // I moved the check from option == QL1S to option.endsWith()
        // to check option && ~option. Hope it will NOT be a problem...

        // third_party: managed inside adblockrulefallbackimpl
        if (option.endsWith(QL1S("third-party")))
            return false;

        // script
        if (option.endsWith(QL1S("script")))
            return true;

        // image
        if (option.endsWith(QL1S("image")))
            return true;

        // background
        if (option.endsWith(QL1S("background")))
            return true;

        // stylesheet
        if (option.endsWith(QL1S("stylesheet")))
            return true;

        // object
        if (option.endsWith(QL1S("object")))
            return true;

        // xbl
        if (option.endsWith(QL1S("xbl")))
            return true;

        // ping
        if (option.endsWith(QL1S("ping")))
            return true;

        // xmlhttprequest
        if (option.endsWith(QL1S("xmlhttprequest")))
            return true;

        // object_subrequest
        if (option.endsWith(QL1S("object-subrequest")))
            return true;

        // dtd
        if (option.endsWith(QL1S("dtd")))
            return true;

        // subdocument
        if (option.endsWith(QL1S("subdocument")))
            return true;

        // document
        if (option.endsWith(QL1S("document")))
            return true;

        // other
        if (option.endsWith(QL1S("other")))
            return true;

        // collapse
        if (option.endsWith(QL1S("collapse")))
            return true;
    }

    return false;
}


QString AdBlockRuleNullImpl::ruleString() const
{
    return QString();
}


QString AdBlockRuleNullImpl::ruleType() const
{
    return QL1S("AdBlockRuleNullImpl");
}
