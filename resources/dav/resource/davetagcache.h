/*
    SPDX-FileCopyrightText: 2026 Benjamin Port <benjamin.port@enioka.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KDAV/EtagCache>

/**
 * @short Purpose of this class is to expose setInternalEtag protected method to DavItemCache
 */
class DavEtagCache : public KDAV::EtagCache
{
public:
    using KDAV::EtagCache::EtagCache;

private:
    friend class DavItemCache;
};
