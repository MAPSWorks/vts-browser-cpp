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

#include <boost/algorithm/string.hpp>
#include <utility/enum-io.hpp>
#include <dbglog/dbglog.hpp>
#include "include/vts-browser/boostProgramOptions.hpp"
#include "include/vts-browser/options.hpp"
#include "include/vts-browser/fetcher.hpp"
#include "include/vts-browser/log.hpp"

namespace po = boost::program_options;

namespace vts
{

namespace
{

void notifyLogMask(const std::string &mask)
{
    setLogMask(mask);
}

void notifyLogFile(const std::string &name)
{
    setLogFile(name);
}

void notifyLogConsole(bool console)
{
    setLogConsole(console);
}

void sanitizeSection(std::string &s)
{
    if (s.empty())
        return;
    if (s[s.size() - 1] != '.')
        s += '.';
}

} // namespace

void optionsConfigLog(
        boost::program_options::options_description &desc,
        std::string section)
{
    sanitizeSection(section);
    desc.add_options()

    ((section + "mask").c_str(),
        po::value<std::string>()->notifier(&notifyLogMask)
        ->default_value(dbglog::get_mask_string()),
        "Set log mask.\n"
        "Format: I?E?W?\n"
        "Eg.: I3W1E4")

    ((section + "file").c_str(),
        po::value<std::string>()->notifier(&notifyLogFile),
        "Set log output file path.")

    ((section + "console").c_str(),
        po::value<bool>()->notifier(&notifyLogConsole)
        ->default_value(dbglog::get_log_console())
        ->implicit_value(!dbglog::get_log_console()),
        "Enable log output to console.")
    ;
}

void optionsConfigCreateOptions(
        boost::program_options::options_description &desc,
        MapCreateOptions *opts,
        std::string section)
{
    sanitizeSection(section);
    desc.add_options()

    ((section + "clientId").c_str(),
        po::value<std::string>(&opts->clientId)
        ->default_value(opts->clientId),
        "Identification of the application for the authentication server.")

    ((section + "cachePath").c_str(),
        po::value<std::string>(&opts->cachePath)
        ->default_value(opts->cachePath),
        "Path to a directory where all downloaded resources are cached.")

    ((section + "disableCache").c_str(),
        po::value<bool>(&opts->disableCache)
        ->default_value(opts->disableCache)
        ->implicit_value(!opts->disableCache),
        "Set to yes to completly disable the cache.")
    ;
}

void optionsConfigMapOptions(
        boost::program_options::options_description &desc,
        MapOptions *opts,
        std::string section)
{
    sanitizeSection(section);
    desc.add_options()

    ((section + "maxTexelToPixelScale").c_str(),
        po::value<double>(&opts->maxTexelToPixelScale)
        ->default_value(opts->maxTexelToPixelScale),
        "Maximum ratio of texture details to the viewport resolution.")

    ((section + "renderTilesScale").c_str(),
        po::value<double>(&opts->renderTilesScale)
        ->default_value(opts->renderTilesScale),
        "Scale of every tile. "
        "Small up-scale may reduce occasional holes on tile borders.")

    ((section + "maxResourcesMemory").c_str(),
        po::value<uint64>(&opts->maxResourcesMemory)
        ->default_value(opts->maxResourcesMemory),
        "Maximum memory (in bytes) used by resources "
        "before they begin to unload.")

    ((section + "maxConcurrentDownloads").c_str(),
        po::value<uint32>(&opts->maxConcurrentDownloads)
        ->default_value(opts->maxConcurrentDownloads),
        "Maximum size of the queue for the resources to be downloaded.")

    ((section + "maxResourceProcessesPerTick").c_str(),
        po::value<uint32>(&opts->maxResourceProcessesPerTick)
        ->default_value(opts->maxResourceProcessesPerTick),
        "Maximum number of resources processed per dataTick.")

    ((section + "maxFetchRedirections").c_str(),
        po::value<uint32>(&opts->maxFetchRedirections)
        ->default_value(opts->maxFetchRedirections),
        "Maximum number of redirections before the download fails.")

    ((section + "maxFetchRetries").c_str(),
        po::value<uint32>(&opts->maxFetchRetries)
        ->default_value(opts->maxFetchRetries),
        "Maximum number of attempts to redownload a resource.")

    ((section + "fetchFirstRetryTimeOffset").c_str(),
        po::value<uint32>(&opts->fetchFirstRetryTimeOffset)
        ->default_value(opts->fetchFirstRetryTimeOffset),
        "Delay in seconds for first resource download retry.")

    ((section + "traverseMode").c_str(),
        po::value<TraverseMode>(&opts->traverseMode)
        ->default_value(opts->traverseMode),
        "Render traversal mode:\n"
        "hierarchical\n"
        "flat\n"
        "balanced")
    ;
}

void optionsConfigDebugOptions(
        boost::program_options::options_description &desc,
        MapOptions *opts,
        std::string section)
{
    sanitizeSection(section);
    desc.add_options()

    ((section + "debugDetachedCamera").c_str(),
        po::value<bool>(&opts->debugDetachedCamera)
        ->default_value(opts->debugDetachedCamera)
        ->implicit_value(!opts->debugDetachedCamera),
        "debugDetachedCamera")

    ((section + "debugDisableVirtualSurfaces").c_str(),
        po::value<bool>(&opts->debugDisableVirtualSurfaces)
        ->default_value(opts->debugDisableVirtualSurfaces)
        ->implicit_value(!opts->debugDisableVirtualSurfaces),
        "debugDisableVirtualSurfaces")

    ((section + "debugDisableSri").c_str(),
        po::value<bool>(&opts->debugDisableSri)
        ->default_value(opts->debugDisableSri)
        ->implicit_value(!opts->debugDisableSri),
        "debugDisableSri")

    ((section + "debugSaveCorruptedFiles").c_str(),
        po::value<bool>(&opts->debugSaveCorruptedFiles)
        ->default_value(opts->debugSaveCorruptedFiles)
        ->implicit_value(!opts->debugSaveCorruptedFiles),
        "debugSaveCorruptedFiles")

    ((section + "debugFlatShading").c_str(),
        po::value<bool>(&opts->debugFlatShading)
        ->default_value(opts->debugFlatShading)
        ->implicit_value(!opts->debugFlatShading),
        "debugFlatShading")

    ((section + "debugRenderSurrogates").c_str(),
        po::value<bool>(&opts->debugRenderSurrogates)
        ->default_value(opts->debugRenderSurrogates)
        ->implicit_value(!opts->debugRenderSurrogates),
        "debugRenderSurrogates")

    ((section + "debugRenderMeshBoxes").c_str(),
        po::value<bool>(&opts->debugRenderMeshBoxes)
        ->default_value(opts->debugRenderMeshBoxes)
        ->implicit_value(!opts->debugRenderMeshBoxes),
        "debugRenderMeshBoxes")

    ((section + "debugRenderTileBoxes").c_str(),
        po::value<bool>(&opts->debugRenderTileBoxes)
        ->default_value(opts->debugRenderTileBoxes)
        ->implicit_value(!opts->debugRenderTileBoxes),
        "debugRenderTileBoxes")

    ((section + "debugRenderObjectPosition").c_str(),
        po::value<bool>(&opts->debugRenderObjectPosition)
        ->default_value(opts->debugRenderObjectPosition)
        ->implicit_value(!opts->debugRenderObjectPosition),
        "debugRenderObjectPosition")

    ((section + "debugRenderTargetPosition").c_str(),
        po::value<bool>(&opts->debugRenderTargetPosition)
        ->default_value(opts->debugRenderTargetPosition)
        ->implicit_value(!opts->debugRenderTargetPosition),
        "debugRenderTargetPosition")

    ((section + "debugRenderAltitudeShiftCorners").c_str(),
        po::value<bool>(&opts->debugRenderAltitudeShiftCorners)
        ->default_value(opts->debugRenderAltitudeShiftCorners)
        ->implicit_value(!opts->debugRenderAltitudeShiftCorners),
        "debugRenderAltitudeShiftCorners")

    ((section + "debugRenderNoMeshes").c_str(),
        po::value<bool>(&opts->debugRenderNoMeshes)
        ->default_value(opts->debugRenderNoMeshes)
        ->implicit_value(!opts->debugRenderNoMeshes),
        "debugRenderNoMeshes")
    ;
}

void optionsConfigFetcherOptions(
        boost::program_options::options_description &desc,
        FetcherOptions *opts,
        std::string section)
{
    sanitizeSection(section);
    desc.add_options()

    ((section + "threads").c_str(),
        po::value<uint32>(&opts->threads)
        ->default_value(opts->threads),
        "Number of threads created for the fetcher.")

    ((section + "maxHostConnections").c_str(),
        po::value<uint32>(&opts->maxHostConnections)
        ->default_value(opts->maxHostConnections),
        "Maximum concurrent connections to same host.")

    ((section + "maxTotalConections").c_str(),
        po::value<uint32>(&opts->maxTotalConections)
        ->default_value(opts->maxTotalConections),
        "Total limit of concurrent connections.")

    ((section + "maxCacheConections").c_str(),
        po::value<uint32>(&opts->maxCacheConections)
        ->default_value(opts->maxCacheConections),
        "Size of curl connection cache.")

    ((section + "pipelining").c_str(),
        po::value<sint32>(&opts->pipelining)
        ->default_value(opts->pipelining),
        "HTTP pipelining mode.")
    ;
}

UTILITY_GENERATE_ENUM_IO(TraverseMode,
                         ((Hierarchical)("hierarchical"))
                         ((Flat)("flat"))
                         ((Balanced)("balanced"))
                         )

} // namespace vts
