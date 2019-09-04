/******************************************************************************\
Copyright (c) 2005-2016, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This sample was distributed or derived from the Intel's Media Samples package.
The original version of this sample may be obtained from https://software.intel.com/en-us/intel-media-server-studio
or https://software.intel.com/en-us/media-client-solutions-support.
\**********************************************************************************/

//#include "mfx_samples_config.h"
//#include "sample_defs.h"

#if defined(_WIN32) || defined(_WIN64)

#include <objbase.h>
#include <initguid.h>
#include <assert.h>
#include <d3d9.h>

#include "d3d_allocator.h"

#define D3DFMT_NV12 (D3DFORMAT)MAKEFOURCC('N','V','1','2')
#define D3DFMT_YV12 (D3DFORMAT)MAKEFOURCC('Y','V','1','2')
#define D3DFMT_P010 (D3DFORMAT)MAKEFOURCC('P','0','1','0')
#define D3DFMT_IMC3 (D3DFORMAT)MAKEFOURCC('I','M','C','3')

#define MFX_FOURCC_IMC3 (MFX_MAKEFOURCC('I','M','C','3')) // This line should be moved into mfxstructures.h in new API version

D3DFORMAT ConvertMfxFourccToD3dFormat(mfxU32 fourcc)
{
    switch (fourcc)
    {
    case MFX_FOURCC_NV12:
        return D3DFMT_NV12;
    case MFX_FOURCC_YV12:
        return D3DFMT_YV12;
    case MFX_FOURCC_YUY2:
        return D3DFMT_YUY2;
    case MFX_FOURCC_RGB3:
        return D3DFMT_R8G8B8;
    case MFX_FOURCC_RGB4:
        return D3DFMT_A8R8G8B8;
    case MFX_FOURCC_P8:
        return D3DFMT_P8;
    case MFX_FOURCC_P010:
        return D3DFMT_P010;
    case MFX_FOURCC_A2RGB10:
        return D3DFMT_A2R10G10B10;
    case MFX_FOURCC_IMC3:
        return D3DFMT_IMC3;
    default:
        return D3DFMT_UNKNOWN;
    }
}

D3DFrameAllocator::D3DFrameAllocator()
: m_decoderService(0), m_processorService(0), m_hDecoder(0), m_hProcessor(0), m_manager(0), m_surfaceUsage(0)
{
}

D3DFrameAllocator::~D3DFrameAllocator()
{
    Close();
}

mfxStatus D3DFrameAllocator::Init(mfxAllocatorParams *pParams)
{
    D3DAllocatorParams *pd3dParams = 0;
    pd3dParams = dynamic_cast<D3DAllocatorParams *>(pParams);
    if (!pd3dParams)
        return MFX_ERR_NOT_INITIALIZED;

    m_manager = pd3dParams->pManager;
    m_surfaceUsage = pd3dParams->surfaceUsage;

    return MFX_ERR_NONE;
}

mfxStatus D3DFrameAllocator::Close()
{
    if (m_manager && m_hDecoder)
    {
        m_manager->CloseDeviceHandle(m_hDecoder);
        m_manager = 0;
        m_hDecoder = 0;
    }

    if (m_manager && m_hProcessor)
    {
        m_manager->CloseDeviceHandle(m_hProcessor);
        m_manager = 0;
        m_hProcessor = 0;
    }

    return BaseFrameAllocator::Close();
}

mfxStatus D3DFrameAllocator::LockFrame(mfxMemId mid, mfxFrameData *ptr)
{
    if (!ptr || !mid)
        return MFX_ERR_NULL_PTR;

    mfxHDLPair *dxmid = (mfxHDLPair*)mid;
    IDirect3DSurface9 *pSurface = static_cast<IDirect3DSurface9*>(dxmid->first);
    if (pSurface == 0)
        return MFX_ERR_INVALID_HANDLE;

    D3DSURFACE_DESC desc;
    HRESULT hr = pSurface->GetDesc(&desc);
    if (FAILED(hr))
        return MFX_ERR_LOCK_MEMORY;

    if (desc.Format != D3DFMT_NV12 &&
        desc.Format != D3DFMT_YV12 &&
        desc.Format != D3DFMT_YUY2 &&
        desc.Format != D3DFMT_R8G8B8 &&
        desc.Format != D3DFMT_A8R8G8B8 &&
        desc.Format != D3DFMT_P8 &&
        desc.Format != D3DFMT_P010 &&
        desc.Format != D3DFMT_A2R10G10B10 &&
        desc.Format != D3DFMT_IMC3)
        return MFX_ERR_LOCK_MEMORY;

    D3DLOCKED_RECT locked;

    hr = pSurface->LockRect(&locked, 0, D3DLOCK_NOSYSLOCK);
    if (FAILED(hr))
        return MFX_ERR_LOCK_MEMORY;

    switch ((DWORD)desc.Format)
    {
    case D3DFMT_NV12:
        ptr->Pitch = (mfxU16)locked.Pitch;
        ptr->Y = (mfxU8 *)locked.pBits;
        ptr->U = (mfxU8 *)locked.pBits + desc.Height * locked.Pitch;
        ptr->V = ptr->U + 1;
        break;
    case D3DFMT_YV12:
        ptr->Pitch = (mfxU16)locked.Pitch;
        ptr->Y = (mfxU8 *)locked.pBits;
        ptr->V = ptr->Y + desc.Height * locked.Pitch;
        ptr->U = ptr->V + (desc.Height * locked.Pitch) / 4;
        break;
    case D3DFMT_YUY2:
        ptr->Pitch = (mfxU16)locked.Pitch;
        ptr->Y = (mfxU8 *)locked.pBits;
        ptr->U = ptr->Y + 1;
        ptr->V = ptr->Y + 3;
        break;
    case D3DFMT_R8G8B8:
        ptr->Pitch = (mfxU16)locked.Pitch;
        ptr->B = (mfxU8 *)locked.pBits;
        ptr->G = ptr->B + 1;
        ptr->R = ptr->B + 2;
        break;
    case D3DFMT_A8R8G8B8:
    case D3DFMT_A2R10G10B10:
        ptr->Pitch = (mfxU16)locked.Pitch;
        ptr->B = (mfxU8 *)locked.pBits;
        ptr->G = ptr->B + 1;
        ptr->R = ptr->B + 2;
        ptr->A = ptr->B + 3;
        break;
    case D3DFMT_P8:
        ptr->Pitch = (mfxU16)locked.Pitch;
        ptr->Y = (mfxU8 *)locked.pBits;
        ptr->U = 0;
        ptr->V = 0;
        break;
    case D3DFMT_P010:
        ptr->PitchHigh = (mfxU16)(locked.Pitch / (1 << 16));
        ptr->PitchLow = (mfxU16)(locked.Pitch % (1 << 16));
        ptr->Y = (mfxU8 *)locked.pBits;
        ptr->U = (mfxU8 *)locked.pBits + desc.Height * locked.Pitch;
        ptr->V = ptr->U + 1;
        break;
    case D3DFMT_IMC3:
        ptr->Pitch = (mfxU16)locked.Pitch;
        ptr->Y = (mfxU8 *)locked.pBits;
        ptr->V = ptr->Y + desc.Height * locked.Pitch;
        ptr->U = ptr->Y + desc.Height * locked.Pitch *2;
        break;
    }

    return MFX_ERR_NONE;
}

mfxStatus D3DFrameAllocator::UnlockFrame(mfxMemId mid, mfxFrameData *ptr)
{
    if (!mid)
        return MFX_ERR_NULL_PTR;

    mfxHDLPair *dxmid = (mfxHDLPair*)mid;
    IDirect3DSurface9 *pSurface = static_cast<IDirect3DSurface9*>(dxmid->first);
    if (pSurface == 0)
        return MFX_ERR_INVALID_HANDLE;

    pSurface->UnlockRect();

    if (NULL != ptr)
    {
        ptr->Pitch = 0;
        ptr->Y     = 0;
        ptr->U     = 0;
        ptr->V     = 0;
    }

    return MFX_ERR_NONE;
}

mfxStatus D3DFrameAllocator::GetFrameHDL(mfxMemId mid, mfxHDL * handle)
{
    if (!mid || !handle)
        return MFX_ERR_NULL_PTR;

    mfxHDLPair *dxMid = (mfxHDLPair*)mid;
    *handle = dxMid->first;
    return MFX_ERR_NONE;
}

mfxStatus D3DFrameAllocator::CheckRequestType(mfxFrameAllocRequest *request)
{
    mfxStatus sts = BaseFrameAllocator::CheckRequestType(request);
    if (MFX_ERR_NONE != sts)
        return sts;

    if ((request->Type & (MFX_MEMTYPE_VIDEO_MEMORY_DECODER_TARGET | MFX_MEMTYPE_VIDEO_MEMORY_PROCESSOR_TARGET)) != 0)
        return MFX_ERR_NONE;
    else
        return MFX_ERR_UNSUPPORTED;
}

mfxStatus D3DFrameAllocator::ReleaseResponse(mfxFrameAllocResponse *response)
{
    if (!response)
        return MFX_ERR_NULL_PTR;

    mfxStatus sts = MFX_ERR_NONE;

    if (response->mids) {
        for (mfxU32 i = 0; i < response->NumFrameActual; i++) {
            if (response->mids[i]) {
                mfxHDLPair *dxMids = (mfxHDLPair*)response->mids[i];
                static_cast<IDirect3DSurface9*>(dxMids->first)->Release();
            }
        }
        MSDK_SAFE_FREE(response->mids[0]);
    }
    MSDK_SAFE_FREE(response->mids);

    return sts;
}

mfxStatus D3DFrameAllocator::AllocImpl(mfxFrameAllocRequest *request, mfxFrameAllocResponse *response)
{
    HRESULT hr;

    MSDK_CHECK_POINTER(request, MFX_ERR_NULL_PTR);
    if (request->NumFrameSuggested == 0)
        return MFX_ERR_UNKNOWN;

    D3DFORMAT format = ConvertMfxFourccToD3dFormat(request->Info.FourCC);

    if (format == D3DFMT_UNKNOWN)
        return MFX_ERR_UNSUPPORTED;

    DWORD   target;

    if (MFX_MEMTYPE_DXVA2_DECODER_TARGET & request->Type)
    {
        target = DXVA2_VideoDecoderRenderTarget;
    }
    else if (MFX_MEMTYPE_DXVA2_PROCESSOR_TARGET & request->Type)
    {
        target = DXVA2_VideoProcessorRenderTarget;
    }
    else
        return MFX_ERR_UNSUPPORTED;

    IDirectXVideoAccelerationService* videoService = NULL;

    if (target == DXVA2_VideoProcessorRenderTarget) {
        if (!m_hProcessor) {
            hr = m_manager->OpenDeviceHandle(&m_hProcessor);
            if (FAILED(hr))
                return MFX_ERR_MEMORY_ALLOC;

            hr = m_manager->GetVideoService(m_hProcessor, IID_IDirectXVideoProcessorService, (void**)&m_processorService);
            if (FAILED(hr))
                return MFX_ERR_MEMORY_ALLOC;
        }
        videoService = m_processorService;
    }
    else {
        if (!m_hDecoder)
        {
            hr = m_manager->OpenDeviceHandle(&m_hDecoder);
            if (FAILED(hr))
                return MFX_ERR_MEMORY_ALLOC;

            hr = m_manager->GetVideoService(m_hDecoder, IID_IDirectXVideoDecoderService, (void**)&m_decoderService);
            if (FAILED(hr))
                return MFX_ERR_MEMORY_ALLOC;
        }
        videoService = m_decoderService;
    }

    mfxHDLPair *dxMids = NULL, **dxMidPtrs = NULL;
    dxMids = (mfxHDLPair*)calloc(request->NumFrameSuggested, sizeof(mfxHDLPair));
    dxMidPtrs = (mfxHDLPair**)calloc(request->NumFrameSuggested, sizeof(mfxHDLPair*));

    if (!dxMids || !dxMidPtrs) {
        MSDK_SAFE_FREE(dxMids);
        MSDK_SAFE_FREE(dxMidPtrs);
        return MFX_ERR_MEMORY_ALLOC;
    }

    response->mids = (mfxMemId*)dxMidPtrs;
    response->NumFrameActual = request->NumFrameSuggested;

    if (request->Type & MFX_MEMTYPE_EXTERNAL_FRAME) {
        for (int i = 0; i < request->NumFrameSuggested; i++) {
            hr = videoService->CreateSurface(request->Info.Width, request->Info.Height, 0,  format,
                D3DPOOL_DEFAULT, m_surfaceUsage, target, (IDirect3DSurface9**)&dxMids[i].first, &dxMids[i].second);
            if (FAILED(hr)) {
                ReleaseResponse(response);
                MSDK_SAFE_FREE(dxMids);
                return MFX_ERR_MEMORY_ALLOC;
            }
            dxMidPtrs[i] = &dxMids[i];
        }
    } else {
        safe_array<IDirect3DSurface9*> dxSrf(new IDirect3DSurface9*[request->NumFrameSuggested]);
        if (!dxSrf.get())
        {
            MSDK_SAFE_FREE(dxMids);
            return MFX_ERR_MEMORY_ALLOC;
        }
        hr = videoService->CreateSurface(request->Info.Width, request->Info.Height, request->NumFrameSuggested - 1,  format,
                                            D3DPOOL_DEFAULT, m_surfaceUsage, target, dxSrf.get(), NULL);
        if (FAILED(hr))
        {
            MSDK_SAFE_FREE(dxMids);
            return MFX_ERR_MEMORY_ALLOC;
        }


        for (int i = 0; i < request->NumFrameSuggested; i++) {
            dxMids[i].first = dxSrf.get()[i];
            dxMidPtrs[i] = &dxMids[i];
        }
    }
    return MFX_ERR_NONE;
}

#endif // #if defined(_WIN32) || defined(_WIN64)