/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 1998, 1999 Michael Reiher <michael.reiher@gmx.de>
    SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KONQ_FRAMECONTAINER_H
#define KONQ_FRAMECONTAINER_H

#include "konqframe.h"
#include <QSplitter>

/**
 * Base class for containers
 * This implements the Composite pattern: a composite is a type of base element.
 */
class KONQ_TESTS_EXPORT KonqFrameContainerBase : public KonqFrameBase
{
public:
    ~KonqFrameContainerBase() override {}

    /**
     * Insert a new frame into the container.
     */
    virtual void insertChildFrame(KonqFrameBase *frame, int index = -1) = 0;
    /**
     * Replace a child frame with another
     */
    virtual void replaceChildFrame(KonqFrameBase *oldFrame, KonqFrameBase *newFrame);
    /**
     * Split one of our child frames
     */
    KonqFrameContainer *splitChildFrame(KonqFrameBase *frame, Qt::Orientation orientation);

    /**
     * Call this before deleting one of our children.
     */
    virtual void childFrameRemoved(KonqFrameBase *frame) = 0;

    bool isContainer() const override
    {
        return true;
    }

    KonqFrameBase::FrameType frameType() const override
    {
        return KonqFrameBase::ContainerBase;
    }

    KonqFrameBase *activeChild() const
    {
        return m_pActiveChild;
    }

    virtual void setActiveChild(KonqFrameBase *activeChild)
    {
        m_pActiveChild = activeChild;
        m_pParentContainer->setActiveChild(this);
    }

    void activateChild() override
    {
        if (m_pActiveChild) {
            m_pActiveChild->activateChild();
        }
    }

    KonqView *activeChildView() const override
    {
        if (m_pActiveChild) {
            return m_pActiveChild->activeChildView();
        } else {
            return nullptr;
        }
    }

protected:
    KonqFrameContainerBase() {}

    KonqFrameBase *m_pActiveChild;
};

/**
 * With KonqFrameContainers and @refKonqFrames we can create a flexible
 * storage structure for the views. The top most element is a
 * KonqFrameContainer. It's a direct child of the MainView. We can then
 * build up a binary tree of containers. KonqFrameContainers are the nodes.
 * That means that they always have two children. Which are either again
 * KonqFrameContainers or, as leaves, KonqFrames.
 */
class KONQ_TESTS_EXPORT KonqFrameContainer : public QSplitter, public KonqFrameContainerBase   // TODO rename to KonqFrameContainerSplitter?
{
    Q_OBJECT
public:
    KonqFrameContainer(Qt::Orientation o,
                       QWidget *parent,
                       KonqFrameContainerBase *parentContainer);
    ~KonqFrameContainer() override;

    bool accept(KonqFrameVisitor *visitor) override;

    void saveConfig(KConfigGroup &config, const QString &prefix, const KonqFrameBase::Options &options, KonqFrameBase *docContainer, int id = 0, int depth = 0) override;
    void copyHistory(KonqFrameBase *other) override;

    KonqFrameBase *firstChild()
    {
        return m_pFirstChild;
    }
    KonqFrameBase *secondChild()
    {
        return m_pSecondChild;
    }
    KonqFrameBase *otherChild(KonqFrameBase *child);

    void swapChildren();

    void setTitle(const QString &title, QWidget *sender) override;
    void setTabIcon(const QUrl &url, QWidget *sender) override;

    QWidget *asQWidget() override
    {
        return this;
    }
    KonqFrameBase::FrameType frameType() const override
    {
        return KonqFrameBase::Container;
    }

    /**
     * Insert a new frame into the splitter.
     */
    void insertChildFrame(KonqFrameBase *frame, int index = -1) override;
    /**
     * Call this before deleting one of our children.
     */
    void childFrameRemoved(KonqFrameBase *frame) override;

    void replaceChildFrame(KonqFrameBase *oldFrame, KonqFrameBase *newFrame) override;

    void setAboutToBeDeleted()
    {
        m_bAboutToBeDeleted = true;
    }

protected:
    void childEvent(QChildEvent *) override;

Q_SIGNALS:
    void setRubberbandCalled();

protected:
    KonqFrameBase *m_pFirstChild;
    KonqFrameBase *m_pSecondChild;
    bool m_bAboutToBeDeleted;
};

#endif /* KONQ_FRAMECONTAINER_H */

