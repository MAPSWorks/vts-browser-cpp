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

#ifndef CALLBACKS_H_asergfrhg
#define CALLBACKS_H_asergfrhg

#include "foundation.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*vtsResourceCallbackType)(vtsHMap map, vtsHResource resource);
typedef void (*vtsEmptyCallbackType)(vtsHMap map);
typedef void (*vtsDoubleArrayCallbackType)(vtsHMap map, double values[]);
typedef void (*vtsDoubleCallbackType)(vtsHMap map, double *value);
typedef void (*vtsUint32ArrayCallbackType)(vtsHMap map, uint32 values[]);
typedef void (*vtsUint32CallbackType)(vtsHMap map, uint32 *value);
typedef const char *(*vtsProjFinderCallbackType)(const char *name);

VTS_API void vtsCallbacksLoadTexture(vtsHMap map,
                        vtsResourceCallbackType callback);
VTS_API void vtsCallbacksLoadMesh(vtsHMap map,
                        vtsResourceCallbackType callback);

VTS_API void vtsCallbacksMapconfigAvailable(vtsHMap map,
                        vtsEmptyCallbackType callback);
VTS_API void vtsCallbacksMapconfigReady(vtsHMap map,
                        vtsEmptyCallbackType callback);

VTS_API void vtsCallbacksCameraEye(vtsHMap map,
                        vtsDoubleArrayCallbackType callback);
VTS_API void vtsCallbacksCameraTarget(vtsHMap map,
                        vtsDoubleArrayCallbackType callback);
VTS_API void vtsCallbacksCameraUp(vtsHMap map,
                        vtsDoubleArrayCallbackType callback);
VTS_API void vtsCallbacksCameraFovAspectNearFar(vtsHMap map,
                        vtsDoubleArrayCallbackType callback);
VTS_API void vtsCallbacksCameraView(vtsHMap map,
                        vtsDoubleArrayCallbackType callback);
VTS_API void vtsCallbacksCameraProj(vtsHMap map,
                        vtsDoubleArrayCallbackType callback);

VTS_API void vtsCallbacksCollidersCenter(vtsHMap map,
                        vtsDoubleArrayCallbackType callback);
VTS_API void vtsCallbacksCollidersDistance(vtsHMap map,
                        vtsDoubleCallbackType callback);
VTS_API void vtsCallbacksCollidersLod(vtsHMap map,
                        vtsUint32CallbackType callback);

VTS_API void vtsProjFinder(vtsProjFinderCallbackType callback);

#ifdef __cplusplus
} // extern C
#endif

#endif
