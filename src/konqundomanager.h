/* This file is part of the KDE project
    SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>
    SPDX-FileCopyrightText: 2007 Eduardo Robles Elvira <edulix@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQUNDOMANAGER_H
#define KONQUNDOMANAGER_H

#include "konqprivate_export.h"
#include <QObject>
#include <QString>
#include <QList>
class KonqClosedWindowItem;
class KonqClosedTabItem;
class KonqClosedItem;
class KonqClosedWindowsManager;
class QAction;

/**
 * Note that there is one KonqUndoManager per mainwindow.
 * It integrates KonqFileUndoManager (undoing file operations)
 * and undoing the closing of tabs.
 */
class KONQ_TESTS_EXPORT KonqUndoManager : public QObject
{
    Q_OBJECT
public:
    /**
     * Constructor
     * @param parent the parent QObject, also used as the parent widget for KonqFileUndoManager::UiInterface.
     */
    explicit KonqUndoManager(KonqClosedWindowsManager *cwManager, QWidget *parent);
    ~KonqUndoManager() override;

    bool undoAvailable() const;
    QString undoText() const;
    quint64 newCommandSerialNumber();

    /**
     * This method is not constant because when calling it the m_closedItemsList
     * might get filled because of delayed initialization.
     */
    const QList<KonqClosedItem * > &closedItemsList();
    void undoClosedItem(int index);
    void addClosedTabItem(KonqClosedTabItem *closedTabItem);
    /**
     * Add current window as a closed window item to other windows
     */
    void addClosedWindowItem(KonqClosedWindowItem *closedWindowItem);
    void updateSupportsFileUndo(bool enable);

public Q_SLOTS:
    void undo();
    void clearClosedItemsList(bool onlyInthisWindow = false);
    void undoLastClosedItem();
    /**
     * Opens in a new tab/window the item the user selected from the closed tabs
     * menu (by emitting openClosedTab/Window), and takes it from the list.
     */
    void slotClosedItemsActivated(QAction *action);
    void slotAddClosedWindowItem(KonqUndoManager *real_sender,
                                 KonqClosedWindowItem *closedWindowItem);

Q_SIGNALS:
    void undoAvailable(bool canUndo);
    void undoTextChanged(const QString &text);

    /// Emitted when a closed tab should be reopened
    void openClosedTab(const KonqClosedTabItem &);
    /// Emitted when a closed window should be reopened
    void openClosedWindow(const KonqClosedWindowItem &);
    /// Emitted when closedItemsList() has changed.
    void closedItemsListChanged();

    /// Emitted to be received in other window instances, uing the singleton
    /// communicator
    void removeWindowInOtherInstances(KonqUndoManager *real_sender, const
                                      KonqClosedWindowItem *closedWindowItem);
    void addWindowInOtherInstances(KonqUndoManager *real_sender,
                                   KonqClosedWindowItem *closedWindowItem);
private Q_SLOTS:
    void slotFileUndoAvailable(bool);
    void slotFileUndoTextChanged(const QString &text);

    /**
     * Received from other window instances, removes/adds a reference of a
     * window from m_closedItemList.
     */
    void slotRemoveClosedWindowItem(KonqUndoManager *real_sender, const
                                    KonqClosedWindowItem *closedWindowItem);

private:
    /// Fill the m_closedItemList with closed windows
    void populate();

    QList<KonqClosedItem *> m_closedItemList;
    KonqClosedWindowsManager *m_cwManager;
    bool m_supportsFileUndo = false;
    bool m_populated = false;
};

#endif /* KONQUNDOMANAGER_H */
