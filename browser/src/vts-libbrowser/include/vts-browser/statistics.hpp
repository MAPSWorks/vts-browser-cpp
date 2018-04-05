/**
 * Copyright (c) 2017 Melown Technologies SE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * *  Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef STATISTICS_HPP_wqieufhbvgjh
#define STATISTICS_HPP_wqieufhbvgjh

#include <string>

#include "foundation.hpp"

namespace vts
{

class VTS_API MapStatistics
{
public:
    MapStatistics();
    MapStatistics(const std::string &json);
    std::string toJson() const;

    void resetAll();
    void resetFrame();

    static const uint32 MaxLods = 25;

    // frame statistics

    uint32 nodesRenderedTotal;
    uint32 nodesRenderedPerLod[MaxLods];
    uint32 metaNodesTraversedTotal;
    uint32 metaNodesTraversedPerLod[MaxLods];

    // global statistics

    uint32 resourcesDownloaded;
    uint32 resourcesDiskLoaded;
    uint32 resourcesProcessed;
    uint32 resourcesCreated;
    uint32 resourcesReleased;
    uint32 resourcesFailed;
    uint32 renderTicks;
    uint32 dataTicks;

    // current statistics

    uint64 currentGpuMemUse;
    uint64 currentRamMemUse;
    uint32 resourcesActive;
    uint32 resourcesDownloading;
    uint32 resourcesPreparing;
    uint32 currentNodeMetaUpdates;
    uint32 currentNodeDrawsUpdates;
    NavigationMode currentNavigationMode;
};

} // namespace vts

#endif
