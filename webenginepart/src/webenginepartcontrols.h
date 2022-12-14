/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2021 Stefano Crocco <posta@stefanocrocco.it>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef WEBENGINEPARTCONTROLS_H
#define WEBENGINEPARTCONTROLS_H

#include <QObject>
#include <QWebEngineCertificateError>

class QWebEngineProfile;
class WebEnginePartCookieJar;
class SpellCheckerManager;
class WebEnginePartDownloadManager;
class WebEnginePage;

namespace KonqWebEnginePart {
    class CertificateErrorDialogManager;
}

class WebEnginePartControls : public QObject
{
    Q_OBJECT

public:

    static WebEnginePartControls *self();

    ~WebEnginePartControls();

    bool isReady() const;

    void setup(QWebEngineProfile *profile);

    SpellCheckerManager* spellCheckerManager() const;

    WebEnginePartDownloadManager* downloadManager() const;

    bool handleCertificateError(const QWebEngineCertificateError &ce, WebEnginePage *page);

private:

    WebEnginePartControls();

    QWebEngineProfile *m_profile;
    WebEnginePartCookieJar *m_cookieJar;
    SpellCheckerManager *m_spellCheckerManager;
    WebEnginePartDownloadManager *m_downloadManager;
    KonqWebEnginePart::CertificateErrorDialogManager *m_certificateErrorDialogManager;
};

#endif // WEBENGINEPARTCONTROLS_H
