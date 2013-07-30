/* ============================================================
*
* This file is a part of the rekonq project
*
* Copyright (C) 2013 by Paul Rohrbach <p.b.r at gmx dot net>
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
#include "adblockelementhiding.h"

// Rekonq Includes
#include "rekonq_defines.h"

AdBlockElementHiding::AdBlockElementHiding()
{
}


bool AdBlockElementHiding::addRule(const QString &rule)
{
    if (!rule.contains(QL1S("##")))
        return false;

    if (rule.startsWith(QL1S("##")))
    {
        m_GenericRules.push_back(rule.mid(2));
        return true;
    }

    QStringList lst = rule.split(QL1S("##"));
    QString domainSpecificRule = lst[1];

    QStringList domains = lst[0].split(QL1C(','));
    Q_FOREACH(QString domain, domains)
    {
        if(domain.startsWith(QL1C('~')))
        {
            m_DomainSpecificRulesWhitelist.insert(domain.mid(1),
                                                  domainSpecificRule);
            continue;
        }

        m_DomainSpecificRules.insert(domain, domainSpecificRule);
    }

    return true;
}

void AdBlockElementHiding::apply(QWebElement &document, QString domain) const
{

    //first apply generic rules
    Q_FOREACH(QString rule, m_GenericRules)
    {
        applyStringRule(document, rule);
    }

    //check for whitelisted rules
    QStringList whiteListedRules;
    QStringList subdomainList = generateSubdomainList(domain);

    Q_FOREACH(QString d, subdomainList)
    {
        whiteListedRules.append(m_DomainSpecificRulesWhitelist.values(d));
    }

    //apply rules if not whitelisted
    Q_FOREACH(QString d, subdomainList)
    {
        QList<QString> ruleList = m_DomainSpecificRules.values(d);
        Q_FOREACH(QString rule, ruleList)
        {
            if (!whiteListedRules.contains(rule))
                applyStringRule(document, rule);
        }
    }
}

void AdBlockElementHiding::clear()
{
    m_GenericRules.clear();
    m_DomainSpecificRules.clear();
    m_DomainSpecificRulesWhitelist.clear();
}

void AdBlockElementHiding::applyStringRule(QWebElement &document, const QString &rule) const
{
    QWebElementCollection elements = document.findAll(rule);

    Q_FOREACH(QWebElement el, elements)
    {
        if (el.isNull())
            continue;
        kDebug() << "Hide element: " << el.localName();
        el.removeFromDocument();
    }
}

QStringList AdBlockElementHiding::generateSubdomainList(const QString &domain) const
{
    QStringList returnList;

    int dotPosition = domain.lastIndexOf(QL1C('.'));
    dotPosition = domain.lastIndexOf(QL1C('.'), dotPosition - 1);
    while (dotPosition != -1)
    {
        returnList.append(domain.mid(dotPosition + 1));
        dotPosition = domain.lastIndexOf(QL1C('.'), dotPosition - 1);
    }
    returnList.append(domain);

    return returnList;
}
