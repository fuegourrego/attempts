/* This file is part of the KDE project
   Copyright (C) 2000 Carsten Pfeiffer <pfeiffer@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef KONQ_PIXMAPPROVIDER_H
#define KONQ_PIXMAPPROVIDER_H

#include <qmap.h>

#include <kconfig.h>
#include <kpixmapprovider.h>

class KonqPixmapProvider : public KPixmapProvider
{
public:
    virtual ~KonqPixmapProvider() {} // shut g++ up

    /**
     * Looks up a pixmap for @p url. Uses a cache for the iconname of url.
     */
    virtual QPixmap pixmapFor( const QString& url, int size = 0 );

    /**
     * Loads the cache to @p kc from the current KConfig-group from key @p key.
     */
    void load( KConfig * kc, const QString& key );
    /**
     * Saves the cache to @p kc into the current KConfig-group as key @p key.
     * Only those @p items are saved, otherwise the cache would grow forever.
     */
    void save( KConfig *, const QString& key, const QStringList& items );

private:
    QMap<QString,QString> iconMap;
};


#endif // KONQ_PIXMAPPROVIDER_H
