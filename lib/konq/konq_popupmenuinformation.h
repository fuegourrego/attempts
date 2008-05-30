/* This file is part of the KDE project
   Copyright 2008 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published
   by the Free Software Foundation; either version 2 of the License or
   ( at your option ) version 3 or, at the discretion of KDE e.V.
   ( which shall act as a proxy as in section 14 of the GPLv3 ), any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KONQ_POPUPMENUINFORMATION_H
#define KONQ_POPUPMENUINFORMATION_H

#include <QtCore/QSharedDataPointer>
#include <libkonq_export.h>
#include <kurl.h>

class KonqPopupMenuInformationPrivate;
class KFileItemList;
class QWidget;

/**
 * Holds the information about the items shown by KonqPopupMenu.
 * This information is used by KonqMenuActions to insert the appropriate
 * actions (for user-defined services and for associated applications),
 * and by KonqPopupMenuPlugin for plugins to decide what to show.
 *
 * KonqPopupMenuInformation is implicitly shared, i.e. it can be used as a value and copied around at almost no cost.
 */
class LIBKONQ_EXPORT KonqPopupMenuInformation
{
public:
    /**
     * Constructor
     */
    KonqPopupMenuInformation();

    /**
     * Copy constructor
     */
    KonqPopupMenuInformation(const KonqPopupMenuInformation&);

    /**
     * Destructor
     */
    ~KonqPopupMenuInformation();

    KonqPopupMenuInformation & operator=(const KonqPopupMenuInformation& o);

    /**
     * Sets the list of fileitems which the actions apply to.
     */
    void setItems(const KFileItemList& items);

    /**
     * List of fileitems
     */
    KFileItemList items() const;

    /**
     * List of urls, gathered from the fileitems
     */
    KUrl::List urlList() const;

    /**
     * Call this if the items are readonly, i.e. no actions should modify them.
     */
    void setReadOnly(bool ro);

    /**
     * @return true if some the items are readonly
     */
    bool readOnly() const;

    /**
     * @return true if all items are directories
     */
    bool isDirectory() const;

    /**
     * @return the mimetype of all items, if they all have the same, otherwise empty
     */
    QString mimeType() const;

    /**
     * @return the mimetype group (e.g. "text") of all items, if they all have the same, otherwise empty
     */
    QString mimeGroup() const;

    /**
     * Call this to set a parent widget (e.g. for error message boxes)
     */
    void setParentWidget(QWidget* parentWidget);

    /**
     * Parent widget (e.g. for error message boxes)
     */
    QWidget* parentWidget() const;

private:
    QSharedDataPointer<KonqPopupMenuInformationPrivate> d;
};

#endif /* KONQ_POPUPMENUINFORMATION_H */

