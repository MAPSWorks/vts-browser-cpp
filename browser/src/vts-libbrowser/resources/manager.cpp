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

#include "../include/vts-browser/log.hpp"
#include "../map.hpp"

namespace vts
{

////////////////////////////
// A FETCH THREAD
////////////////////////////

CacheWriteData::CacheWriteData() : expires(-1)
{}

CacheWriteData::CacheWriteData(FetchTaskImpl *task) :
    buffer(task->reply.content.copy()),
    name(task->name), expires(task->reply.expires)
{}

void FetchTaskImpl::fetchDone()
{
    LOG(debug) << "Resource <" << name << "> finished downloading";
    assert(map);
    map->resources.downloads.fetch_add(-1, std::memory_order_relaxed);
    Resource::State state = Resource::State::downloading;

    // handle error or invalid codes
    if (reply.code >= 400 || reply.code < 200)
    {
        if (reply.code == FetchTask::ExtraCodes::ProhibitedContent)
        {
            state = Resource::State::errorFatal;
        }
        else
        {
            LOG(err2) << "Error downloading <" << name
                << ">, http code " << reply.code;
            state = Resource::State::errorRetry;
        }
    }

    // some resources must always revalidate
    if (!Resource::allowDiskCache(query.resourceType))
        reply.expires = -2;

    // availability tests
    if (state == Resource::State::downloading
        && !performAvailTest())
    {
        LOG(info1) << "Resource <" << name
            << "> failed availability test";
        state = Resource::State::availFail;
    }

    // handle redirections
    if (state == Resource::State::downloading
        && reply.code >= 300 && reply.code < 400)
    {
        if (redirectionsCount++ > map->options.maxFetchRedirections)
        {
            LOG(err2) << "Too many redirections in <"
                << name << ">, last url <"
                << query.url << ">, http code " << reply.code;
            state = Resource::State::errorRetry;
        }
        else
        {
            query.url.swap(reply.redirectUrl);
            std::string().swap(reply.redirectUrl);
            LOG(info1) << "Download of <"
                << name << "> redirected to <"
                << query.url << ">, http code " << reply.code;
            reply = Reply();
            state = Resource::State::initializing;
        }
    }

    // update the actual resource
    if (state == Resource::State::downloading)
        state = Resource::State::downloaded;
    else
        reply.content.free();
    {
        std::shared_ptr<Resource> rs = resource.lock();
        if (rs)
        {
            assert(&*rs->fetch == this);
            assert(rs->state.load(std::memory_order_relaxed)
                == Resource::State::downloading);
            rs->info.ramMemoryCost = reply.content.size();
            rs->state.store(state, std::memory_order_release);
            if (state == Resource::State::downloaded)
                map->resources.queUpload.push(rs);
        }
    }

    // (deferred) write to cache
    if (state == Resource::State::availFail
        || state == Resource::State::downloaded)
        map->resources.queCacheWrite.push(this);
}

////////////////////////////
// DATA THREAD
////////////////////////////

void MapImpl::resourceUploadProcess(const std::shared_ptr<Resource> &r)
{
    assert(r->state.load(std::memory_order_relaxed)
        == Resource::State::downloaded);
    r->info.gpuMemoryCost = r->info.ramMemoryCost = 0;
    statistics.resourcesProcessed++;
    try
    {
        r->load();
        r->state.store(Resource::State::ready,
                       std::memory_order_release);
    }
    catch (const std::exception &e)
    {
        LOG(err3) << "Failed processing resource <" << r->name
            << ">, exception <" << e.what() << ">";
        if (options.debugSaveCorruptedFiles)
        {
            std::string path = std::string() + "corrupted/"
                + convertNameToPath(r->name, false);
            try
            {
                writeLocalFileBuffer(path, r->fetch->reply.content);
                LOG(info1) << "Resource <" << r->name
                    << "> saved into file <"
                    << path << "> for further inspection";
            }
            catch (...)
            {
                LOG(warn1) << "Failed saving resource <" << r->name
                    << "> into file <"
                    << path << "> for further inspection";
            }
        }
        statistics.resourcesFailed++;
        r->state.store(Resource::State::errorFatal,
                       std::memory_order_release);
    }
    r->fetch.reset();
}

void MapImpl::resourceDataInitialize()
{
    LOG(info3) << "Data initialize";
}

void MapImpl::resourceDataFinalize()
{
    LOG(info3) << "Data finalize";
}

void MapImpl::resourceDataTick()
{
    statistics.dataTicks = ++resources.tickIndex;

    for (uint32 proc = 0; proc
        < options.maxResourceProcessesPerTick; proc++)
    {
        std::weak_ptr<Resource> w;
        if (!resources.queUpload.tryPop(w))
            break;
        std::shared_ptr<Resource> r = w.lock();
        if (!r)
            continue;
        resourceUploadProcess(r);
    }
}

void MapImpl::resourceDataRun()
{
    while (!resources.queUpload.stopped())
    {
        std::weak_ptr<Resource> w;
        resources.queUpload.waitPop(w);
        std::shared_ptr<Resource> r = w.lock();
        if (!r)
            continue;
        resourceUploadProcess(r);
    }
}

////////////////////////////
// CACHE WRITE THREAD
////////////////////////////

void MapImpl::cacheWriteEntry()
{
    setLogThreadName("cache writer");
    while (!resources.queCacheWrite.stopped())
    {
        CacheWriteData cwd;
        resources.queCacheWrite.waitPop(cwd);
        if (!cwd.name.empty())
            resources.cache->write(cwd.name, cwd.buffer, cwd.expires);
    }
}

////////////////////////////
// CACHE READ THREAD
////////////////////////////

void MapImpl::cacheReadEntry()
{
    setLogThreadName("cache reader");
    while (!resources.queCacheRead.stopped())
    {
        std::weak_ptr<Resource> w;
        resources.queCacheRead.waitPop(w);
        std::shared_ptr<Resource> r = w.lock();
        if (!r)
            continue;
        try
        {
            cacheReadProcess(r);
        }
        catch (const std::exception &e)
        {
            statistics.resourcesFailed++;
            r->state.store(Resource::State::errorFatal,
                           std::memory_order_release);
            LOG(err3) << "Failed preparing resource <" << r->name
                << ">, exception <" << e.what() << ">";
        }
    }
}

namespace
{

bool startsWith(const std::string &text, const std::string &start)
{
    return text.substr(0, start.length()) == start;
}

} // namespace

void MapImpl::cacheReadProcess(const std::shared_ptr<Resource> &r)
{
    assert(r->state.load(std::memory_order_relaxed)
        == Resource::State::checkCache);
    if (!r->fetch)
        r->fetch = std::make_shared<FetchTaskImpl>(r);
    r->info.gpuMemoryCost = r->info.ramMemoryCost = 0;
    if (r->allowDiskCache() && resources.cache->read(
        r->name, r->fetch->reply.content,
        r->fetch->reply.expires))
    {
        statistics.resourcesDiskLoaded++;
        r->fetch->reply.code = 200;
        r->state.store(r->fetch->reply.content.size() > 0
                       ? Resource::State::downloaded
                       : Resource::State::availFail,
                       std::memory_order_release);
    }
    else if (startsWith(r->name, "file://"))
    {
        r->fetch->reply.content = readLocalFileBuffer(r->name.substr(7));
        r->fetch->reply.code = 200;
        r->state.store(Resource::State::downloaded,
                       std::memory_order_release);
    }
    else if (startsWith(r->name, "internal://"))
    {
        r->fetch->reply.content = readInternalMemoryBuffer(r->name.substr(11));
        r->fetch->reply.code = 200;
        r->state.store(Resource::State::downloaded,
                       std::memory_order_release);
    }
    else if (startsWith(r->name, "generate://"))
    {
        r->state.store(Resource::State::errorRetry,
                       std::memory_order_release);
        // will be handled elsewhere
    }
    else
    {
        r->state.store(Resource::State::startDownload,
                       std::memory_order_release);
        // will be handled in main thread
    }

    if (r->state.load(std::memory_order_relaxed)
            == Resource::State::downloaded)
        resources.queUpload.push(r);
}

////////////////////////////
// FETCHER THREAD
////////////////////////////

void MapImpl::resourcesDownloadsEntry()
{
    resources.fetcher->initialize();
    while (!resources.fetching.stop.load(std::memory_order_relaxed))
    {
        std::vector<std::weak_ptr<Resource>> res1;
        {
            boost::mutex::scoped_lock lock(resources.fetching.mut);
            resources.fetching.con.wait(lock);
            res1.swap(resources.fetching.resources);
        }
        if (resources.downloads.load(std::memory_order_relaxed)
                >= options.maxConcurrentDownloads)
            continue; // skip processing if no download slots are available
        typedef std::pair<float, std::shared_ptr<Resource>> PR;
        std::vector<PR> res;
        for (auto &w : res1)
        {
            std::shared_ptr<Resource> r = w.lock();
            if (r)
            {
                res.emplace_back(r->priority, r);
                if (r->priority < std::numeric_limits<float>::infinity())
                    r->priority = 0;
            }
        }
        std::sort(res.begin(), res.end(), [](PR &a, PR &b) {
            return a.first > b.first;
        });
        for (auto &pr : res)
        {
            if (resources.downloads.load(std::memory_order_relaxed)
                    >= options.maxConcurrentDownloads)
                break;
            std::shared_ptr<Resource> r = pr.second;
            r->state.store(Resource::State::downloading,
                           std::memory_order_relaxed);
            resources.downloads.fetch_add(1, std::memory_order_relaxed);
            LOG(debug) << "Initializing fetch of <" << r->name << ">";
            r->fetch->query.headers["X-Vts-Client-Id"]
                    = createOptions.clientId;
            if (resources.auth)
                resources.auth->authorize(r);
            resources.fetcher->fetch(r->fetch);
            statistics.resourcesDownloaded++;
        }
    }
    resources.fetcher->finalize();
    resources.fetcher.reset();
}

////////////////////////////
// MAIN THREAD
////////////////////////////

bool MapImpl::resourcesTryRemove(std::shared_ptr<Resource> &r)
{
    std::string name = r->name;
    assert(resources.resources.count(name) == 1);
    {
        // release the pointer if we are the last one holding it
        std::weak_ptr<Resource> w = r;
        try
        {
            r.reset();
        }
        catch (...)
        {
            LOGTHROW(fatal, std::runtime_error)
                    << "Exception in destructor";
        }
        r = w.lock();
    }
    if (!r)
    {
        LOG(info1) << "Released resource <" << name << ">";
        resources.resources.erase(name);
        statistics.resourcesReleased++;
        return true;
    }
    return false;
}

void MapImpl::resourcesRemoveOld()
{
    struct Res
    {
        std::string n; // name
        uint32 m; // mem used
        uint32 a; // lastAccessTick
        Res(const std::string &n, uint32 m, uint32 a) : n(n), m(m), a(a)
        {}
    };
    // successfully loaded resources are removed
    //   only when we are tight on memory
    std::vector<Res> loadedToRemove;
    // resources that errored or are still being loaded
    //   are removed immediately if not used recently
    std::vector<Res> loadingToRemove;
    uint64 memRamUse = 0;
    uint64 memGpuUse = 0;
    // find resource candidates for removal
    for (auto &it : resources.resources)
    {
        memRamUse += it.second->info.ramMemoryCost;
        memGpuUse += it.second->info.gpuMemoryCost;
        // consider long time not used resources only
        if (it.second->lastAccessTick + 5 < renderer.tickIndex)
        {
            Res r(it.first,
                it.second->info.ramMemoryCost + it.second->info.gpuMemoryCost,
                it.second->lastAccessTick);
            if (it.second->state.load(std::memory_order_relaxed)
                    == Resource::State::ready)
                loadedToRemove.push_back(std::move(r));
            else
                loadingToRemove.push_back(std::move(r));
        }
    }
    uint64 memUse = memRamUse + memGpuUse;
    // remove loadingToRemove
    for (Res &res : loadingToRemove)
    {
        if (resourcesTryRemove(resources.resources[res.n]))
            memUse -= res.m;
    }
    // remove loadedToRemove
    uint64 trs = (uint64)options.targetResourcesMemoryKB * 1024;
    if (memUse > trs)
    {
        std::sort(loadedToRemove.begin(), loadedToRemove.end(),
                  [](const Res &a, const Res &b){
            return a.a < b.a;
        });
        for (Res &res : loadedToRemove)
        {
            if (resourcesTryRemove(resources.resources[res.n]))
            {
                memUse -= res.m;
                if (memUse < trs)
                    break;
            }
        }
    }
    statistics.currentGpuMemUseKB = memGpuUse / 1024;
    statistics.currentRamMemUseKB = memRamUse / 1024;
}

void MapImpl::resourcesCheckInitialized()
{
    std::time_t current = std::time(nullptr);
    for (auto it : resources.resources)
    {
        const std::shared_ptr<Resource> &r = it.second;
        if (r->lastAccessTick + 1 != renderer.tickIndex)
            continue; // skip resources that were not accessed last tick
        switch (r->state.load(std::memory_order_acquire))
        {
        case Resource::State::errorRetry:
            if (r->retryNumber >= options.maxFetchRetries)
            {
                LOG(err3) << "All retries for resource <"
                    << r->name << "> has failed";
                r->state.store(Resource::State::errorFatal,
                               std::memory_order_relaxed);
                statistics.resourcesFailed++;
                break;
            }
            if (r->retryTime == -1)
            {
                r->retryTime = (1 << r->retryNumber)
                    * options.fetchFirstRetryTimeOffset + current;
                LOG(warn2) << "Resource <" << r->name
                    << "> may retry in "
                    << (r->retryTime - current) << " seconds";
                break;
            }
            if (r->retryTime > current)
                break;
            r->retryNumber++;
            LOG(info2) << "Trying again to download resource <"
                << r->name << "> (attempt "
                << r->retryNumber << ")";
            r->retryTime = -1;
            UTILITY_FALLTHROUGH;
        case Resource::State::initializing:
            r->state.store(Resource::State::checkCache,
                           std::memory_order_relaxed);
            resources.queCacheRead.push(r);
            break;
        default:
            break;
        }
    }
}

void MapImpl::resourcesStartDownloads()
{
    if (resources.downloads.load(std::memory_order_relaxed)
            >= options.maxConcurrentDownloads)
        return; // early exit
    std::vector<std::weak_ptr<Resource>> res;
    for (auto it : resources.resources)
    {
        const std::shared_ptr<Resource> &r = it.second;
        if (r->lastAccessTick + 1 != renderer.tickIndex)
            continue; // skip resources that were not accessed last tick
        if (r->state.load(std::memory_order_relaxed)
                == Resource::State::startDownload)
            res.push_back(r);
    }
    {
        boost::mutex::scoped_lock lock(resources.fetching.mut);
        res.swap(resources.fetching.resources);
    }
    resources.fetching.con.notify_one();
}

void MapImpl::resourcesUpdateStatistics()
{
    statistics.resourcesPreparing = 0;
    for (auto &rp : resources.resources)
    {
        std::shared_ptr<Resource> &r = rp.second;
        switch (r->state.load(std::memory_order_relaxed))
        {
        case Resource::State::initializing:
        case Resource::State::checkCache:
        case Resource::State::startDownload:
        case Resource::State::downloading:
        case Resource::State::downloaded:
        case Resource::State::uploading:
            statistics.resourcesPreparing++;
            break;
        case Resource::State::ready:
        case Resource::State::errorFatal:
        case Resource::State::errorRetry:
        case Resource::State::availFail:
            break;
        }
    }
    statistics.resourcesActive = resources.resources.size();
    statistics.resourcesDownloading
            = resources.downloads.load(std::memory_order_relaxed);
}

void MapImpl::resourceRenderInitialize()
{}

void MapImpl::resourceRenderFinalize()
{
    resources.queUpload.terminate();
    resources.resources.clear();
}

void MapImpl::resourceRenderTick()
{
    // split workload into multiple render frames
    switch (renderer.tickIndex % 4)
    {
    case 0: return resourcesRemoveOld();
    case 1: return resourcesCheckInitialized();
    case 2: return resourcesStartDownloads();
    case 3: return resourcesUpdateStatistics();
    }
}

} // namespace vts
