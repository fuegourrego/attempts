/*
    This file is part of the KDE project.

    SPDX-FileCopyrightText: 2021 Anthony Fieroni <bvbfan@abv.bg>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "settings/webenginesettings.h"
#include "webengineurlrequestinterceptor.h"

WebEngineUrlRequestInterceptor::WebEngineUrlRequestInterceptor(QObject* parent) :
    QWebEngineUrlRequestInterceptor(parent)
{
}

void WebEngineUrlRequestInterceptor::interceptRequest(QWebEngineUrlRequestInfo &info)
{
    if (info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeImage) {
        info.block(WebEngineSettings::self()->isAdFiltered(info.requestUrl().url()));
    }
}
