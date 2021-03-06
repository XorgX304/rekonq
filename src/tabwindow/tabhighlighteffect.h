/* ============================================================
*
* This file is a part of the rekonq project
*
* Copyright (C) 2011 Tröscher Johannes <fritz_van_tom@hotmail.com>
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


#ifndef TABHIGHLIGHTEFFECT_H
#define TABHIGHLIGHTEFFECT_H


// Rekonq Includes
#include "rekonq_defines.h"

// Qt Includes
#include <QGraphicsEffect>
#include <QColor>

// Forward Declarations
class TabBar;

class QEvent;
class QPainter;


class TabHighlightEffect : public QGraphicsEffect
{
    Q_OBJECT

public:
    explicit TabHighlightEffect(TabBar *tabBar = 0);

protected:
    virtual void draw(QPainter *painter);
    virtual bool event(QEvent *event);

private:
    TabBar * const m_tabBar;
    QColor m_highlightColor;
};

#endif // TABHIGHLIGHTEFFECT_H
