/*  This file is part of the KDE project
    SPDX-FileCopyrightText: 1998, 1999 Michael Reiher <michael.reiher@gmx.de>
    SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "konqframecontainer.h"
#include "konqdebug.h"
#include <kconfig.h>
#include <math.h> // pow()
#include <kconfiggroup.h>

#include "konqframevisitor.h"

void KonqFrameContainerBase::replaceChildFrame(KonqFrameBase *oldFrame, KonqFrameBase *newFrame)
{
    childFrameRemoved(oldFrame);
    insertChildFrame(newFrame);
}

KonqFrameContainer *KonqFrameContainerBase::splitChildFrame(KonqFrameBase *splitFrame, Qt::Orientation orientation)
{
    KonqFrameContainer *newContainer = new KonqFrameContainer(orientation, asQWidget(), this);
    replaceChildFrame(splitFrame, newContainer);
    newContainer->insertChildFrame(splitFrame);
    return newContainer;
}

////

KonqFrameContainer::KonqFrameContainer(Qt::Orientation o,
                                       QWidget *parent,
                                       KonqFrameContainerBase *parentContainer)
    : QSplitter(o, parent), m_bAboutToBeDeleted(false)
{
    m_pParentContainer = parentContainer;
    m_pFirstChild = nullptr;
    m_pSecondChild = nullptr;
    m_pActiveChild = nullptr;
    connect(this, &KonqFrameContainer::splitterMoved, this, &KonqFrameContainer::setRubberbandCalled);
//### CHECKME
}

KonqFrameContainer::~KonqFrameContainer()
{
    delete m_pFirstChild;
    delete m_pSecondChild;
}

void KonqFrameContainer::saveConfig(KConfigGroup &config, const QString &prefix, const KonqFrameBase::Options &options, KonqFrameBase *docContainer, int id, int depth)
{
    int idSecond = id + int(pow(2.0, depth));

    //write children sizes
    config.writeEntry(QStringLiteral("SplitterSizes").prepend(prefix), sizes());

    //write children
    QStringList strlst;
    if (firstChild()) {
        strlst.append(KonqFrameBase::frameTypeToString(firstChild()->frameType()) + QString::number(idSecond - 1));
    }
    if (secondChild()) {
        strlst.append(KonqFrameBase::frameTypeToString(secondChild()->frameType()) + QString::number(idSecond));
    }

    config.writeEntry(QStringLiteral("Children").prepend(prefix), strlst);

    //write orientation
    QString o;
    if (orientation() == Qt::Horizontal) {
        o = QStringLiteral("Horizontal");
    } else if (orientation() == Qt::Vertical) {
        o = QStringLiteral("Vertical");
    }
    config.writeEntry(QStringLiteral("Orientation").prepend(prefix), o);

    //write docContainer
    if (this == docContainer) {
        config.writeEntry(QStringLiteral("docContainer").prepend(prefix), true);
    }

    if (m_pSecondChild == m_pActiveChild) {
        config.writeEntry(QStringLiteral("activeChildIndex").prepend(prefix), 1);
    } else {
        config.writeEntry(QStringLiteral("activeChildIndex").prepend(prefix), 0);
    }

    //write child configs
    if (firstChild()) {
        QString newPrefix = KonqFrameBase::frameTypeToString(firstChild()->frameType()) + QString::number(idSecond - 1);
        newPrefix.append(QLatin1Char('_'));
        firstChild()->saveConfig(config, newPrefix, options, docContainer, id, depth + 1);
    }

    if (secondChild()) {
        QString newPrefix = KonqFrameBase::frameTypeToString(secondChild()->frameType()) + QString::number(idSecond);
        newPrefix.append(QLatin1Char('_'));
        secondChild()->saveConfig(config, newPrefix, options, docContainer, idSecond, depth + 1);
    }
}

void KonqFrameContainer::copyHistory(KonqFrameBase *other)
{
    Q_ASSERT(other->frameType() == KonqFrameBase::Container);
    if (firstChild()) {
        firstChild()->copyHistory(static_cast<KonqFrameContainer *>(other)->firstChild());
    }
    if (secondChild()) {
        secondChild()->copyHistory(static_cast<KonqFrameContainer *>(other)->secondChild());
    }
}

KonqFrameBase *KonqFrameContainer::otherChild(KonqFrameBase *child)
{
    if (m_pFirstChild == child) {
        return m_pSecondChild;
    } else if (m_pSecondChild == child) {
        return m_pFirstChild;
    }
    return nullptr;
}

void KonqFrameContainer::swapChildren()
{
    qSwap(m_pFirstChild, m_pSecondChild);
}

void KonqFrameContainer::setTitle(const QString &title, QWidget *sender)
{
    //qCDebug(KONQUEROR_LOG) << title << sender;
    if (m_pParentContainer && activeChild() && (sender == activeChild()->asQWidget())) {
        m_pParentContainer->setTitle(title, this);
    }
}

void KonqFrameContainer::setTabIcon(const QUrl &url, QWidget *sender)
{
    //qCDebug(KONQUEROR_LOG) << url << sender;
    if (m_pParentContainer && activeChild() && (sender == activeChild()->asQWidget())) {
        m_pParentContainer->setTabIcon(url, this);
    }
}

void KonqFrameContainer::insertChildFrame(KonqFrameBase *frame, int index)
{
    //qCDebug(KONQUEROR_LOG) << this << frame;
    if (frame) {
        QSplitter::insertWidget(index, frame->asQWidget());
        // Insert before existing child? Move first to second.
        if (index == 0 && m_pFirstChild && !m_pSecondChild) {
            qSwap(m_pFirstChild, m_pSecondChild);
        }
        if (!m_pFirstChild) {
            m_pFirstChild = frame;
            frame->setParentContainer(this);
            //qCDebug(KONQUEROR_LOG) << "Setting as first child";
        } else if (!m_pSecondChild) {
            m_pSecondChild = frame;
            frame->setParentContainer(this);
            //qCDebug(KONQUEROR_LOG) << "Setting as second child";
        } else {
            qCWarning(KONQUEROR_LOG) << this << "already has two children..."
                       << m_pFirstChild << "and" << m_pSecondChild;
        }
    } else {
        qCWarning(KONQUEROR_LOG) << "KonqFrameContainer" << this << ": insertChildFrame(NULL)!";
    }
}

void KonqFrameContainer::childFrameRemoved(KonqFrameBase *frame)
{
    //qCDebug(KONQUEROR_LOG) << this << "Child" << frame << "removed";

    if (m_pFirstChild == frame) {
        m_pFirstChild = m_pSecondChild;
        m_pSecondChild = nullptr;
    } else if (m_pSecondChild == frame) {
        m_pSecondChild = nullptr;
    } else {
        qCWarning(KONQUEROR_LOG) << this << "Can't find this child:" << frame;
    }
}

void KonqFrameContainer::childEvent(QChildEvent *c)
{
    // Child events cause layout changes. These are unnecessary if we are going
    // to be deleted anyway.
    if (!m_bAboutToBeDeleted) {
        QSplitter::childEvent(c);
    }
}

bool KonqFrameContainer::accept(KonqFrameVisitor *visitor)
{
    if (!visitor->visit(this)) {
        return false;
    }
    //Q_ASSERT( m_pFirstChild );
    if (m_pFirstChild && !m_pFirstChild->accept(visitor)) {
        return false;
    }
    //Q_ASSERT( m_pSecondChild );
    if (m_pSecondChild && !m_pSecondChild->accept(visitor)) {
        return false;
    }
    if (!visitor->endVisit(this)) {
        return false;
    }
    return true;
}

void KonqFrameContainer::replaceChildFrame(KonqFrameBase *oldFrame, KonqFrameBase *newFrame)
{
    const int idx = QSplitter::indexOf(oldFrame->asQWidget());
    const QList<int> splitterSizes = sizes();
    childFrameRemoved(oldFrame);
    insertChildFrame(newFrame, idx);
    setSizes(splitterSizes);
}

