/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2000 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2003 Waldo Bastian <bastian@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef KONQ_SIDEBARTREEITEM_H
#define KONQ_SIDEBARTREEITEM_H

#include <QStringList>
#include <Qt3Support/Q3ListView>
#include <Qt3Support/Q3UriDrag>
#include <QUrl>

class KonqSidebarTree;
class KonqSidebarTreeItem;
class KonqSidebarTreeModule;
class KonqSidebarTreeTopLevelItem;

/**
 * The base class for any item in the tree.
 * Items belonging to a given module are created and managed by the module,
 * but they should all be KonqSidebarTreeItems, for the event handling in KonqSidebarTree.
 */
class KonqSidebarTreeItem : public Q3ListViewItem
{
public:
    // Create an item under another one
    KonqSidebarTreeItem(KonqSidebarTreeItem *parentItem, KonqSidebarTreeTopLevelItem *topLevelItem);

    void initItem(KonqSidebarTreeTopLevelItem *topLevelItem);

    virtual ~KonqSidebarTreeItem();

    // Whether the item accepts a drop consisting in those @p formats
    virtual bool acceptsDrops(const Q3StrList &)
    {
        return false;
    }

    // Handle a drop on this item. If you didn't want it, you shouldn't
    // have return true in acceptsDrops :)
    virtual void drop(QDropEvent *) {}

    // Create a drag object from this item.
    virtual bool populateMimeData(QMimeData *mimeData, bool move) = 0;

    virtual void middleButtonClicked();
    virtual void rightButtonPressed() = 0;

    virtual void paste() {}
    virtual void trash() {}
    virtual void del() {}
    virtual void rename() {}
    virtual void rename(const QString &) {}

    // The URL to open when this link is clicked
    virtual QUrl externalURL() const = 0;

    // The mimetype to use when this link is clicked
    // If unknown, return QString(), konq will determine the mimetype itself
    virtual QString externalMimeType() const
    {
        return QString();
    }

    // overwrite this if you want a tooltip shown on your item
    virtual QString toolTipText() const
    {
        return QString();
    }

    // Called when this item is selected
    // Reimplement, and call tree()->part()->extension()->enableActions(...)
    virtual void itemSelected() = 0;

    // Basically, true for directories and toplevel items
    void setListable(bool b)
    {
        m_bListable = b;
    }
    bool isListable() const
    {
        return m_bListable;
    }

    // Whether clicking on the item should open the "external URL" of the item
    void setClickable(bool b)
    {
        m_bClickable = b;
    }
    bool isClickable() const
    {
        return m_bClickable;
    }

    // Whether the item is a toplevel item
    virtual bool isTopLevelItem() const
    {
        return false;
    }

    KonqSidebarTreeTopLevelItem *topLevelItem() const
    {
        return m_topLevelItem;
    }

    // returns the module associated to our toplevel item
    KonqSidebarTreeModule *module() const;

    // returns the tree inside which this item is
    KonqSidebarTree *tree() const;

    virtual QString key(int column, bool) const
    {
        return text(column).toLower();
    }

    // List of alternative names (URLs) this entry is known under
    QStringList alias;
protected:
    // Create an item at the toplevel - only for toplevel items -> protected
    KonqSidebarTreeItem(KonqSidebarTree *parent, KonqSidebarTreeTopLevelItem *topLevelItem);

    KonqSidebarTreeTopLevelItem *m_topLevelItem;
    bool m_bListable: 1;
    bool m_bClickable: 1;
};

#endif // KONQ_SIDEBARTREEITEM_H
