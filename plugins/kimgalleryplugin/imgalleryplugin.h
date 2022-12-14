/* This file is part of the KDE project

    SPDX-FileCopyrightText: 2001, 2003 Lukas Tinkl <lukas@kde.org>
    SPDX-FileCopyrightText: Andreas Schlapbach <schlpbch@iam.unibe.ch>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#ifndef kimgalleryplugin_h
#define kimgalleryplugin_h

#include <kparts/readonlypart.h>
#include <konq_kpart_plugin.h>

#include <QDir>

class QProgressDialog;
class KIGPDialog;
class QTextStream;

typedef QMap<QString, QString> CommentMap;

class KImGalleryPlugin : public KonqParts::Plugin
{
    Q_OBJECT
public:
    KImGalleryPlugin(QObject *parent,
                     const QVariantList &);
    ~KImGalleryPlugin() override {}

public slots:
    void slotExecute();
    void slotCancelled();

private:
    bool m_cancelled;
    bool m_recurseSubDirectories;
    bool m_copyFiles;
    bool m_useCommentFile;

    int m_imgWidth;
    int m_imgHeight;
    int m_imagesPerRow;

    QProgressDialog *m_progressDlg;

    KParts::ReadOnlyPart *m_part;

    KIGPDialog *m_configDlg;

    CommentMap *m_commentMap;

    bool createDirectory(const QDir &thumb_dir, const QString &imgGalleryDir, const QString &dirName);

    void createHead(QTextStream &stream);
    void createCSSSection(QTextStream &stream);
    void createBody(QTextStream &stream, const QString &sourceDirName, const QStringList &subDirList, const QDir &imageDir, const QUrl &url, const QString &imageFormat);

    bool createThumb(const QString &imgName, const QString &sourceDirName, const QString &imgGalleryDir, const QString &imageFormat);

    bool createHtml(const QUrl &url, const QString &sourceDirName, int recursionLevel, const QString &imageFormat);
    void deleteCancelledGallery(const QUrl &url, const QString &sourceDirName, int recursionLevel, const QString &imageFormat);
    void loadCommentFile();

    static QString extension(const QString &imageFormat);
};

#endif
