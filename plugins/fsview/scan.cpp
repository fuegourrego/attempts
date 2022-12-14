/* This file is part of FSView.
    SPDX-FileCopyrightText: 2002, 2003 Josef Weidendorfer <Josef.Weidendorfer@gmx.de>

    SPDX-License-Identifier: GPL-2.0-only
*/

#include "scan.h"

#include <QDir>
#include <QStringList>
#include <QSet>
#include <qplatformdefs.h>

#include <kauthorized.h>
#include <kurlauthorized.h>

#include "inode.h"
#include "fsviewdebug.h"

// ScanManager

ScanManager::ScanManager()
{
    _topDir = nullptr;
    _listener = nullptr;
}

ScanManager::ScanManager(const QString &path)
{
    _topDir = nullptr;
    _listener = nullptr;
    setTop(path);
}

ScanManager::~ScanManager()
{
    stopScan();
    delete _topDir;
}

void ScanManager::setListener(ScanListener *l)
{
    _listener = l;
}

ScanDir *ScanManager::setTop(const QString &path, int data)
{
    stopScan();
    if (_topDir) {
        delete _topDir;
        _topDir = nullptr;
    }
    if (!path.isEmpty()) {
        _topDir = new ScanDir(path, this, nullptr, data);
    }
    return _topDir;
}

bool ScanManager::scanRunning()
{
    if (!_topDir) {
        return false;
    }

    return _topDir->scanRunning();
}

void ScanManager::startScan(ScanDir *from)
{
    if (!_topDir) {
        return;
    }
    if (!from) {
        from = _topDir;
    }

    if (scanRunning()) {
        stopScan();
    }

    from->clear();
    if (from->parent()) {
        from->parent()->setupChildRescan();
    }

    _list.append(new ScanItem(from->path(), from));
}

void ScanManager::stopScan()
{
    if (!_topDir) {
        return;
    }

    if (0) qCDebug(FSVIEWLOG) << "ScanManager::stopScan, scanLength "
                              << _list.count();

    while (!_list.isEmpty()) {
        ScanItem *si = _list.takeFirst();
        si->dir->finish();
        delete si;
    }
}

int ScanManager::scan(int data)
{
    if (_list.isEmpty()) {
        return false;
    }
    ScanItem *si = _list.takeFirst();

    int newCount = si->dir->scan(si, _list, data);
    delete si;

    return newCount;
}

// ScanFile

ScanFile::ScanFile()
{
    _size = 0;
    _listener = nullptr;
}

ScanFile::ScanFile(const QString &n, KIO::fileoffset_t s)
{
    _name = n;
    _size = s;
    _listener = nullptr;
}

ScanFile::~ScanFile()
{
    if (_listener) {
        _listener->destroyed(this);
    }
}

// ScanDir

ScanDir::ScanDir()
{
    _dirty = true;
    _dirsFinished = -1; /* scan not started */

    _parent = nullptr;
    _manager = nullptr;
    _listener = nullptr;
    _data = 0;
}

ScanDir::ScanDir(const QString &n, ScanManager *m,
                 ScanDir *p, int data)
    : _name(n)
{
    _dirty = true;
    _dirsFinished = -1; /* scan not started */

    _parent = p;
    _manager = m;
    _listener = nullptr;
    _data = data;
}

ScanDir::~ScanDir()
{
    if (_listener) {
        _listener->destroyed(this);
    }
}

void ScanDir::setListener(ScanListener *l)
{
    _listener = l;
}

QString ScanDir::path()
{
    if (_parent) {
        QString p = _parent->path();
        if (!p.endsWith(QLatin1Char('/'))) {
            p += QLatin1Char('/');
        }
        return p + _name;
    }

    return _name;
}

void ScanDir::clear()
{
    _dirty = true;
    _dirsFinished = -1; /* scan not started */

    _files.clear();
    _dirs.clear();
}

void ScanDir::update()
{
    if (!_dirty) {
        return;
    }
    _dirty = false;

    _fileCount = 0;
    _dirCount = 0;
    _size = 0;

    if (_dirsFinished == -1) {
        return;
    }

    if (_files.count() > 0) {
        _fileCount += _files.count();
        _size = _fileSize;
    }
    if (_dirs.count() > 0) {
        _dirCount += _dirs.count();
        ScanDirVector::iterator it;
        for (it = _dirs.begin(); it != _dirs.end(); ++it) {
            (*it).update();
            _fileCount += (*it)._fileCount;
            _dirCount  += (*it)._dirCount;
            _size      += (*it)._size;
        }
    }
}

bool ScanDir::isForbiddenDir(QString &d)
{
    static QSet<QString> *s = nullptr;

    if (!s) {
        s = new QSet<QString>;
        // directories without real files on Linux
        // TODO: should be OS specific
        s->insert(QStringLiteral("/proc"));
        s->insert(QStringLiteral("/dev"));
        s->insert(QStringLiteral("/sys"));
    }
    return (s->contains(d));
}

int ScanDir::scan(ScanItem *si, ScanItemList &list, int data)
{
    clear();
    _dirsFinished = 0;
    _fileSize = 0;
    _dirty = true;

    if (isForbiddenDir(si->absPath)) {
        if (_parent) {
            _parent->subScanFinished();
        }
        return 0;
    }

    QUrl u = QUrl::fromLocalFile(si->absPath);
    if (!KUrlAuthorized::authorizeUrlAction(QStringLiteral("list"), QUrl(), u)) {
        if (_parent) {
            _parent->subScanFinished();
        }

        return 0;
    }

    QDir d(si->absPath);
    const QStringList fileList = d.entryList(QDir::Files |
                                 QDir::Hidden | QDir::NoSymLinks);

    if (fileList.count() > 0) {
        QT_STATBUF buff;

        _files.reserve(fileList.count());

        QStringList::ConstIterator it;
        for (it = fileList.constBegin(); it != fileList.constEnd(); ++it) {
            QString tmp(si->absPath + QLatin1Char('/') + (*it));
            if (QT_LSTAT(tmp.toStdString().c_str(), &buff) != 0) {
                continue;
            }
            _files.append(ScanFile(*it, buff.st_size));
            _fileSize += buff.st_size;
        }
    }

    const QStringList dirList = d.entryList(QDir::Dirs |
                                            QDir::Hidden | QDir::NoSymLinks | QDir::NoDotAndDotDot);

    if (dirList.count() > 0) {
        _dirs.reserve(dirList.count());

        QStringList::ConstIterator it;
        for (it = dirList.constBegin(); it != dirList.constEnd(); ++it) {
            _dirs.append(ScanDir(*it, _manager, this, data));
            QString newpath = si->absPath;
            if (!newpath.endsWith(QChar('/'))) {
                newpath.append("/");
            }
            newpath.append(*it);
            list.append(new ScanItem(newpath, &(_dirs.last())));
        }
        _dirCount += _dirs.count();
    }

    callScanStarted();
    callSizeChanged();

    if (_dirs.count() == 0) {
        callScanFinished();

        if (_parent) {
            _parent->subScanFinished();
        }
    }

    return _dirs.count();
}

void ScanDir::subScanFinished()
{
    _dirsFinished++;
    callSizeChanged();

    if (0) qCDebug(FSVIEWLOG) << "ScanDir::subScanFinished [" << path()
                              << "]: " << _dirsFinished << "/" << _dirs.count();

    if (_dirsFinished < _dirs.count()) {
        return;
    }

    /* all subdirs read */
    callScanFinished();

    if (_parent) {
        _parent->subScanFinished();
    }
}

void ScanDir::finish()
{
    if (scanRunning()) {
        _dirsFinished = _dirs.count();
        callScanFinished();
    }

    if (_parent) {
        _parent->finish();
    }
}

void ScanDir::setupChildRescan()
{
    if (_dirs.count() == 0) {
        return;
    }

    _dirsFinished = 0;
    ScanDirVector::iterator it;
    for (it = _dirs.begin(); it != _dirs.end(); ++it)
        if ((*it).scanFinished()) {
            _dirsFinished++;
        }

    if (_parent &&
            (_dirsFinished < _dirs.count())) {
        _parent->setupChildRescan();
    }

    callScanStarted();
}

void ScanDir::callScanStarted()
{
    if (0) qCDebug(FSVIEWLOG) << "ScanDir:Started [" << path()
                              << "]: size " << size() << ", files " << fileCount();

    ScanListener *mListener = _manager ? _manager->listener() : nullptr;

    if (_listener) {
        _listener->scanStarted(this);
    }
    if (mListener) {
        mListener->scanStarted(this);
    }
}

void ScanDir::callSizeChanged()
{
    if (0) qCDebug(FSVIEWLOG) << ". [" << path()
                              << "]: size " << size() << ", files " << fileCount();

    _dirty = true;

    if (_parent) {
        _parent->callSizeChanged();
    }

    ScanListener *mListener = _manager ? _manager->listener() : nullptr;

    if (_listener) {
        _listener->sizeChanged(this);
    }
    if (mListener) {
        mListener->sizeChanged(this);
    }
}

void ScanDir::callScanFinished()
{
    if (0) qCDebug(FSVIEWLOG) << "ScanDir:Finished [" << path()
                              << "]: size " << size() << ", files " << fileCount();

    ScanListener *mListener = _manager ? _manager->listener() : nullptr;

    if (_listener) {
        _listener->scanFinished(this);
    }
    if (mListener) {
        mListener->scanFinished(this);
    }
}

