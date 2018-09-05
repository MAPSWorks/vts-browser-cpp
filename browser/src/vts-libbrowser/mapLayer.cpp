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

#include "map.hpp"

namespace vts
{

FreeInfo::FreeInfo(const vtslibs::registry::FreeLayer &fl,
          const std::string &url)
    : FreeLayer(fl), url(url)
{}

MapLayer::MapLayer(MapImpl *map) : map(map),
    creditScope(Credits::Scope::Imagery)
{
    boundLayerParams = map->mapConfig->view.surfaces;
}

MapLayer::MapLayer(MapImpl *map, const std::string &name,
                   const vtslibs::registry::View::FreeLayerParams &params)
    : freeLayerName(name), freeLayerParams(params), map(map),
      creditScope(Credits::Scope::Imagery)
{
    boundLayerParams[""] = params.boundLayers;
}

bool MapLayer::prerequisitesCheck()
{
    if (traverseRoot)
        return true;
    if (freeLayerParams)
        return prerequisitesCheckFreeLayer();
    return prerequisitesCheckMainSurfaces();
}

bool MapLayer::isGeodata()
{
    if (freeLayer)
    {
        switch (freeLayer->type)
        {
        case vtslibs::registry::FreeLayer::Type::geodata:
        case vtslibs::registry::FreeLayer::Type::geodataTiles:
            return true;
        default:
            break;
        }
    }
    return false;
}

TraverseMode MapLayer::getTraverseMode()
{
    if (isGeodata())
        return map->options.traverseModeGeodata;
    else
        return map->options.traverseModeSurfaces;
}

BoundParamInfo::List MapLayer::boundList(const SurfaceInfo *surface,
                                         sint32 surfaceReference)
{
    const auto &n = surface->name;
    std::string surfaceName;
    if (n.size() > 1)
        surfaceName = n[surfaceReference - 1];
    else if (!n.empty())
        surfaceName = n.back();
    const vtslibs::registry::View::BoundLayerParams::list &boundList
            = boundLayerParams[surfaceName];
    BoundParamInfo::List bls(boundList.begin(), boundList.end());
    return bls;
}

bool MapLayer::prerequisitesCheckMainSurfaces()
{
    auto *mapConfig = map->mapConfig.get();

    // check for virtual surface
    if (map->options.debugEnableVirtualSurfaces)
    {
        std::vector<std::string> viewSurfaces;
        viewSurfaces.reserve(mapConfig->view.surfaces.size());
        for (auto &it : mapConfig->view.surfaces)
            viewSurfaces.push_back(it.first);
        std::sort(viewSurfaces.begin(), viewSurfaces.end());
        for (vtslibs::vts::VirtualSurfaceConfig &it
             : mapConfig->virtualSurfaces)
        {
            std::vector<std::string> virtSurfaces(it.id.begin(), it.id.end());
            if (virtSurfaces.size() != viewSurfaces.size())
                continue;
            std::vector<std::string> virtSurfaces2(virtSurfaces);
            std::sort(virtSurfaces2.begin(), virtSurfaces2.end());
            if (!boost::algorithm::equals(viewSurfaces, virtSurfaces2))
                continue;
            auto tilesetMapping = map->getTilesetMapping(
                    convertPath(it.mapping, mapConfig->name));
            if (!testAndThrow(
                    tilesetMapping->state.load(std::memory_order_relaxed),
                              "Tileset mapping failure."))
                return false;
            surfaceStack.generateVirtual(map, &it);
            tilesetStack.emplace();
            tilesetStack->generateTileset(map, virtSurfaces,
                                          tilesetMapping->dataRaw);
            break;
        }
    }

    // make real surface stack if no virtual was made
    if (surfaceStack.surfaces.empty())
        surfaceStack.generateReal(map);

    traverseRoot = std::make_shared<TraverseNode>(this, nullptr, NodeInfo(
                    mapConfig->referenceFrame, TileId(), false, *mapConfig));
    traverseRoot->priority = std::numeric_limits<double>::infinity();

    return true;
}

bool MapLayer::prerequisitesCheckFreeLayer()
{
    auto *mapConfig = map->mapConfig.get();

    auto *fl = mapConfig->getFreeInfo(freeLayerName);
    if (!fl)
        return false;
    freeLayer = *fl;

    surfaceStack.generateFree(map, *freeLayer);

    traverseRoot = std::make_shared<TraverseNode>(this, nullptr, NodeInfo(
                    mapConfig->referenceFrame, TileId(), false, *mapConfig));
    traverseRoot->priority = std::numeric_limits<double>::infinity();

    if (freeLayer->type != vtslibs::registry::FreeLayer::Type::meshTiles)
        creditScope = Credits::Scope::Geodata;

    return true;
}

bool MapImpl::generateMonolithicGeodataTrav(TraverseNode *trav)
{
    assert(!!trav->layer->freeLayer);
    assert(!!trav->layer->freeLayerParams);

    const vtslibs::registry::FreeLayer::Geodata &g
            = boost::get<vtslibs::registry::FreeLayer::Geodata>(
                trav->layer->freeLayer->definition);

    trav->meta.emplace();

    // extents
    {
        vec3 el = vecFromUblas<vec3>
                (mapConfig->referenceFrame.division.extents.ll);
        vec3 eu = vecFromUblas<vec3>
                (mapConfig->referenceFrame.division.extents.ur);
        vec3 ed = eu - el;
        ed = vec3(1 / ed[0], 1 / ed[1], 1 / ed[2]);
        trav->meta->extents.ll = vecToUblas<math::Point3>(
                    (vecFromUblas<vec3>(g.extents.ll) - el).cwiseProduct(ed));
        trav->meta->extents.ur = vecToUblas<math::Point3>(
                    (vecFromUblas<vec3>(g.extents.ur) - el).cwiseProduct(ed));
    }

    // aabb
    trav->aabbPhys[0] = vecFromUblas<vec3>(g.extents.ll);
    trav->aabbPhys[1] = vecFromUblas<vec3>(g.extents.ur);

    // other
    trav->meta->displaySize = g.displaySize;
    trav->meta->update(vtslibs::vts::MetaNode::Flag::applyDisplaySize);
    travDetermineMetaImpl(trav); // update physical corners
    trav->surface = &trav->layer->surfaceStack.surfaces[0];
    updateNodePriority(trav);
    return true;
}

namespace
{

static const std::string empty;

MapLayer *getLayer(MapImpl *map, const std::string &name)
{
    for (auto &it : map->layers)
    {
        if (it->freeLayerName == name)
            return it.get();
    }
    return nullptr;
}

} // namespace

std::pair<Validity, const std::string &> MapImpl::getActualGeoStyle(
        const std::string &name)
{
    FreeInfo *f = mapConfig->getFreeInfo(name);
    if (!f)
        return { Validity::Indeterminate, empty };
    if (!f->overrideStyle.empty())
        return { Validity::Valid, f->overrideStyle };
    if (!f->stylesheet)
    {
        std::string url;
        MapLayer *layer = getLayer(this, name);
        if (layer && layer->freeLayerParams->style)
            url = convertPath(*layer->freeLayerParams->style, mapConfigPath);
        else
        {
            switch (f->type)
            {
            case vtslibs::registry::FreeLayer::Type::geodata:
                url = boost::get<vtslibs::registry::FreeLayer::Geodata>(
                            f->definition).style;
                break;
            case vtslibs::registry::FreeLayer::Type::geodataTiles:
                url = boost::get<vtslibs::registry::FreeLayer::GeodataTiles>(
                            f->definition).style;
                break;
            default:
                assert(false);
                break;
            }
            if (url.empty())
                return { Validity::Invalid, empty };
            url = convertPath(url, f->url);
        }
        assert(!url.empty());
        f->stylesheet = getGeoStyle(url);
    }
    touchResource(f->stylesheet);
    return { getResourceValidity(f->stylesheet), f->stylesheet->data };
}

std::pair<Validity, const std::string &> MapImpl::getActualGeoFeatures(
        const std::string &name, const std::string &geoName, float priority)
{
    MapLayer *layer = getLayer(this, name);
    if (!layer)
        return { Validity::Invalid, empty };

    assert(layer->freeLayer);
    if (layer->freeLayer->type == vtslibs::registry::FreeLayer::Type::geodata
            && !layer->freeLayer->overrideGeodata.empty())
        return { Validity::Valid, layer->freeLayer->overrideGeodata };

    if (geoName.empty())
        return { Validity::Invalid, empty };

    auto g = getGeoFeatures(geoName);
    g->updatePriority(priority);
    return { getResourceValidity(g), g->data };
}

std::pair<Validity, const std::string &> MapImpl::getActualGeoFeatures(
        const std::string &name)
{
    MapLayer *layer = getLayer(this, name);
    assert(layer->freeLayer->type
           == vtslibs::registry::FreeLayer::Type::geodata);
    NodeInfo node(mapConfig->referenceFrame, *mapConfig);
    std::string geoName = layer->surfaceStack.surfaces[0].urlGeodata(
            UrlTemplate::Vars(node.nodeId(), vtslibs::vts::local(node)));
    return getActualGeoFeatures(name, geoName,
                                std::numeric_limits<float>::infinity());
}

} // namespace vts
