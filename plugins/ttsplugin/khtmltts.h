/*
    SPDX-FileCopyrightText: 2002 George Russell <george.russell@clara.net>
    SPDX-FileCopyrightText: 2003-2004 Olaf Schmidt <ojschmidt@kde.org>
    SPDX-FileCopyrightText: 2015 Jeremy Whiting <jpwhiting@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KHTMLTTS_H
#define KHTMLTTS_H

#include <konq_kpart_plugin.h>

class QTextToSpeech;

/**
 * KHTML KParts Plugin
 */
class KHTMLPluginTTS : public KonqParts::Plugin
{
    Q_OBJECT
public:

    /**
     * Construct a new KParts plugin.
     */
    KHTMLPluginTTS(QObject *parent, const QVariantList &);

    /**
     * Destructor.
     */
    ~KHTMLPluginTTS() override;
public Q_SLOTS:
    void slotReadOut();
private:
    std::unique_ptr<QTextToSpeech> m_tts;
};

#endif
