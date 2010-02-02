/*
 * DShow Native wrapper
 * Copyright (c) 2010 Gianluigi Tiesi <sherpya@netfarm.it>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "stdafx.h"

class DSVideoCodec
{
public:
    DSVideoCodec::DSVideoCodec(const char *filename, const GUID guid, BITMAPINFOHEADER *bih, unsigned int outfmt) :
      m_guid(guid), m_bih(bih), m_hDll(NULL), m_outfmt(outfmt), m_discontinuity(1), m_pFilter(NULL),
      m_pInputPin(NULL), m_pOutputPin(NULL), m_pOurInput(NULL), m_pOurOutput(NULL),
      m_pImp(NULL), m_pSFilter(NULL), m_pRFilter(NULL), m_pGraph(NULL), m_pMC(NULL)
    {
        strncpy(m_fname, filename, MAX_PATH);
    }

    DSVideoCodec::~DSVideoCodec()
    {
        ReleaseGraph();
        if (m_hDll)
            FreeLibrary(m_hDll);
    }

    void ReleaseGraph(void)
    {
        if (m_pGraph)
        {
            m_pMC->Stop();
            RemoveFromRot(m_dwRegister);
        }

        /* binary codec */
        m_pAll->Release();
        m_pImp->Release();
        m_pInputPin->Disconnect();
        m_pOutputPin->Disconnect();
        m_pInputPin->Release();
        m_pOutputPin->Release();
        m_pFilter->Release();

        if (m_pGraph)
        {
            m_pMC->Release();
            m_pGraph->Release();
        }
    }

    BOOL LoadLibrary(void)
    {
        return ((m_hDll = ::LoadLibrary(m_fname)) != NULL);
    }

    BOOL CreateFilter(void)
    {
        LPFNGETCLASSOBJECT pDllGetClassObject = (LPFNGETCLASSOBJECT) GetProcAddress(m_hDll, "DllGetClassObject");
        if (!pDllGetClassObject) return FALSE;

        IClassFactory *factory;
        m_res = pDllGetClassObject(m_guid, IID_IClassFactory, (LPVOID *) &factory);
        if (m_res != S_OK) return FALSE;

        IUnknown* object;
        m_res = factory->CreateInstance(NULL, IID_IUnknown, (LPVOID *) &object);
        factory->Release();

        if (m_res != S_OK) return FALSE;

        m_res = object->QueryInterface(IID_IBaseFilter, (LPVOID *) &m_pFilter);
        object->Release();

        return (m_res == S_OK);
    }

    BOOL CheckMediaTypes(IPin *pin)
    {
        IEnumMediaTypes *pMedia;
        AM_MEDIA_TYPE *pmt = NULL, *pfnt = NULL;
        HRESULT m_res = pin->EnumMediaTypes(&pMedia);
        pMedia->Reset();
        while((m_res = pMedia->Next(1, &pmt, NULL)) == S_OK)
        {
            if (pmt->formattype == FORMAT_VideoInfo)
            {
                VIDEOINFOHEADER *vih = (VIDEOINFOHEADER *) pmt->pbFormat;
                DeleteMediaType(pmt);
            }
        }
        pMedia->Release();
        return TRUE;
    }

    BOOL SetOutputType(void)
    {
        m_pDestType.majortype = MEDIATYPE_Video;
        m_pDestType.bFixedSizeSamples = TRUE;
        m_pDestType.bTemporalCompression = FALSE;
        m_pDestType.pUnk = 0;

        memset(&m_vi, 0, sizeof(m_vi));
        memcpy(&m_vi.bmiHeader, m_bih, sizeof(m_vi.bmiHeader));

        memset(&m_vi2, 0, sizeof(m_vi2));
        memcpy(&m_vi2.bmiHeader, m_bih, sizeof(m_vi2.bmiHeader));

        m_vi.bmiHeader.biSize = m_vi2.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        m_vi.bmiHeader.biCompression = m_vi2.bmiHeader.biCompression = m_outfmt;

        m_vi.bmiHeader.biPlanes = 1;

        /* Check if we support the desidered output format */
        if (!SetOutputFormat(&m_vi.bmiHeader.biBitCount, &m_vi.bmiHeader.biPlanes))
            return FALSE;

        m_vi2.bmiHeader.biBitCount = m_vi.bmiHeader.biBitCount;
        m_vi2.bmiHeader.biPlanes = m_vi.bmiHeader.biPlanes;

        RECT rImg = { 0 /* left */, 0 /* top */, m_bih->biWidth /* right */, m_bih->biHeight /* bottom */};
        m_vi.rcSource = m_vi2.rcSource = m_vi.rcTarget = m_vi2.rcTarget = rImg;

        //m_vi2.bmiHeader.biHeight *= -1;

        m_vi.bmiHeader.biSizeImage = m_pDestType.lSampleSize = labs(m_bih->biWidth * m_bih->biHeight * ((m_vi.bmiHeader.biBitCount + 7) / 8));
        m_vi2.bmiHeader.biSizeImage = m_vi.bmiHeader.biSizeImage;

        // try FORMAT_VideoInfo
        m_pDestType.formattype = FORMAT_VideoInfo;
        m_pDestType.cbFormat = sizeof(m_vi);
        m_pDestType.pbFormat = (BYTE *) &m_vi;
        m_res = m_pOutputPin->QueryAccept(&m_pDestType);

        // try FORMAT_VideoInfo2
        if (m_res != S_OK)
        {
            m_pDestType.formattype = FORMAT_VideoInfo2;
            m_pDestType.cbFormat = sizeof(m_vi2);
            m_pDestType.pbFormat = (BYTE *) &m_vi2;
            m_res = m_pOutputPin->QueryAccept(&m_pDestType);
        }

        return (m_res == S_OK);
    }

    BOOL SetInputType(void)
    {
        m_pOurType.majortype = MEDIATYPE_Video;
        m_pOurType.subtype = MEDIATYPE_Video;
        m_pOurType.subtype.Data1 = m_bih->biCompression;
        m_pOurType.formattype = FORMAT_VideoInfo;
        m_pOurType.bFixedSizeSamples = FALSE;
        m_pOurType.bTemporalCompression = TRUE;
        m_pOurType.lSampleSize = 1; // FIXME: correct ?
        m_pOurType.pUnk = NULL;

        switch (m_bih->biCompression)
        {
            case mmioFOURCC('H', '2', '6', '4'):
            case mmioFOURCC('h', '2', '6', '4'):
            case mmioFOURCC('X', '2', '6', '4'):
            case mmioFOURCC('x', '2', '6', '4'):
            case mmioFOURCC('A', 'V', 'C', '1'):
            case mmioFOURCC('a', 'v', 'c', '1'):
            case mmioFOURCC('d', 'a', 'v', 'c'):
            case mmioFOURCC('D', 'A', 'V', 'C'):
            case mmioFOURCC('V', 'S', 'S', 'H'):
                return SetInputMPEG2();
        }
        return SetInputVideoInfo();
    }

    BOOL SetInputVideoInfo(void)
    {
        memset(&m_vi, 0, sizeof(m_vi));
        memcpy(&m_vi.bmiHeader, m_bih, m_bih->biSize);
        m_vi.rcSource.left = m_vi.rcSource.top = 0;
        m_vi.rcSource.right = m_bih->biWidth;
        m_vi.rcSource.bottom = m_bih->biHeight;
        m_vi.rcTarget = m_vi.rcSource;

        m_pOurType.formattype = FORMAT_VideoInfo;
        m_pOurType.pbFormat = (BYTE *) &m_vi;
        m_pOurType.cbFormat = sizeof(VIDEOINFOHEADER);
        return TRUE;
    }

    DWORD avc_quant(BYTE *src, BYTE *dst, int len)
    {
        // Stolen from libavcodec h264.c
        BYTE *p = src, *d = dst;
        int cnt;

        cnt = *(p + 5) & 0x1f; // Number of sps

        if (src[0] != 0x01 || cnt > 1)
        {
            memcpy(dst, src, len);
            return len;
        }
        p += 6;

        // cnt > 1 not supported?
        cnt = (*p << 8) | *(p + 1) + 2;
        memcpy(d, p, cnt);
        d += cnt;
        p += cnt;

        // assume pps cnt == 1 too
        p++;
        cnt = (*p << 8) | *(p + 1) + 2;
        memcpy(d, p, cnt);
        return (int) (d + cnt - dst);
    }

    BOOL SetInputMPEG2(void)
    {
        memset(&m_mp2vi, 0, sizeof(m_mp2vi));
        m_mp2vi.hdr.rcSource.left = m_mp2vi.hdr.rcSource.top = 0;
        m_mp2vi.hdr.rcSource.right = m_bih->biWidth;
        m_mp2vi.hdr.rcSource.bottom = m_bih->biHeight;
        m_mp2vi.hdr.rcTarget = m_mp2vi.hdr.rcSource;
        m_mp2vi.hdr.dwPictAspectRatioX = m_bih->biWidth;
        m_mp2vi.hdr.dwPictAspectRatioY = m_bih->biHeight;
        memcpy(&m_mp2vi.hdr.bmiHeader, m_bih, sizeof(BITMAPINFOHEADER));
        m_mp2vi.hdr.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

        /* extradata */
        int extra = m_bih->biSize - sizeof(BITMAPINFOHEADER);
        int size = sizeof(MPEG2VIDEOINFO);
        if (extra > 0)
        {
            BYTE *extradata = (BYTE *) m_bih + sizeof(BITMAPINFOHEADER) + 4;
            m_mp2vi.dwFlags = (*extradata & 0x3) + 1;
            printf("NALU length field size %d\n", m_mp2vi.dwFlags);
            m_mp2vi.cbSequenceHeader = avc_quant((BYTE *)(m_bih) + sizeof(BITMAPINFOHEADER), (BYTE *)(&m_mp2vi.dwSequenceHeader[0]), extra);
            // The '4' is from the allocated space of dwSequenceHeader
            size += m_mp2vi.cbSequenceHeader - 4;
        }

        m_pOurType.formattype = FORMAT_MPEG2Video;
        m_pOurType.pbFormat = (BYTE *) &m_mp2vi;
        m_pOurType.cbFormat = size;

        return TRUE;
    }

    BOOL EnumPins(void)
    {
        IEnumPins *enumpins;
        m_res = m_pFilter->EnumPins(&enumpins);
        enumpins->Reset();

        IPin *pin;
        PIN_INFO pInfo;

        while ((m_res = enumpins->Next(1, &pin, NULL)) == S_OK)
        {
            pin->QueryPinInfo(&pInfo);
            wprintf(L"Pin: %s - %s\n", pInfo.achName, (pInfo.dir == PINDIR_INPUT) ? L"Input" : L"Output");
            if (pInfo.dir == PINDIR_INPUT)
                m_pInputPin = pin;
            else if (pInfo.dir == PINDIR_OUTPUT)
                m_pOutputPin = pin;
            pin->Release();
        }

        enumpins->Release();
        m_res = m_pInputPin->QueryInterface(IID_IMemInputPin, (LPVOID *) &m_pImp);
        return TRUE;
    }

    BOOL SetupAllocator(void)
    {
        m_res = m_pImp->GetAllocator(&m_pAll);
        ALLOCATOR_PROPERTIES props, props1;

        props.cBuffers = 1;
	    props.cbBuffer = m_pDestType.lSampleSize;
	    props.cbAlign = 1;
	    props.cbPrefix = 0;

        m_res = m_pAll->SetProperties(&props, &props1);
        m_res = m_pImp->NotifyAllocator(m_pAll, FALSE);

        m_res = m_pAll->Commit();
        return TRUE;
    }

    BOOL CreateGraph(bool buildgraph=false)
    {
        this->EnumPins();
        this->SetInputType();

        m_res = m_pInputPin->QueryAccept(&m_pOurType);

        m_pSFilter = new CSenderFilter();
        m_pOurInput = (CSenderPin *) m_pSFilter->GetPin(0);
        m_pRFilter = new CRenderFilter();
        m_pOurOutput = (CRenderPin *) m_pRFilter->GetPin(0);

        if (buildgraph)
        {
            m_res = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void **) &m_pGraph);
            m_res = DSVideoCodec::AddToRot(m_pGraph, &m_dwRegister);
            m_pGraph->QueryInterface(IID_IMediaControl, (void **) &m_pMC);    

            m_res = m_pGraph->AddFilter(m_pSFilter, L"DS Sender");
            m_res = m_pGraph->AddFilter(m_pRFilter, L"DS Render");
            m_res = m_pGraph->AddFilter(m_pFilter, L"Binary Codec");
            // Connect our output pin to codec input pin otherwise QueryAccept on the codec output pin will fail
            m_res = m_pGraph->ConnectDirect(m_pOurInput, m_pInputPin, &m_pOurType); 
        }
        else
            m_res = m_pInputPin->ReceiveConnection(m_pOurInput, &m_pOurType); /* same of above */            

        SetOutputType();

        if (buildgraph)
            m_res = m_pGraph->ConnectDirect(m_pOurOutput, m_pOutputPin, &m_pDestType);
        else
            m_res = m_pOutputPin->ReceiveConnection(m_pOurOutput, &m_pDestType);

        return TRUE;
    }

    static HRESULT AddToRot(IUnknown *pUnkGraph, DWORD *pdwRegister) 
    {
        IMoniker *pMoniker = NULL;
        IRunningObjectTable *pROT = NULL;

        if (FAILED(GetRunningObjectTable(0, &pROT)))
            return E_FAIL;

        WCHAR wsz[256];
        StringCchPrintfW(wsz, 256, L"FilterGraph %08x pid %08x", (DWORD_PTR) pUnkGraph, GetCurrentProcessId());
        HRESULT hr = CreateItemMoniker(L"!", wsz, &pMoniker);
        if (SUCCEEDED(hr))
        {
            hr = pROT->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE, pUnkGraph, pMoniker, pdwRegister);
            pMoniker->Release();
        }
        pROT->Release();

        return hr;
    }


    static void RemoveFromRot(DWORD pdwRegister)
    {
        IRunningObjectTable *pROT;
        if (SUCCEEDED(GetRunningObjectTable(0, &pROT)))
        {
            pROT->Revoke(pdwRegister);
            pROT->Release();
        }
    }

    BOOL StartGraph(void)
    {
        SetupAllocator();
        if (m_pMC)
            m_pMC->Run();
        else
            m_pFilter->Run(0);
        return TRUE;
    }

    BOOL Decode(const BYTE *src, int size, double pts, double *newpts, BYTE *pImage)
    {
        IMediaSample* sample = NULL;
        REFERENCE_TIME start = (REFERENCE_TIME) (pts * 1E9);
        REFERENCE_TIME stoptime = start + 1;
        BYTE *ptr;

        m_res = m_pAll->GetBuffer(&sample, 0, 0, 0);
        if (m_res != S_OK) return FALSE;

        m_res = sample->SetActualDataLength(size);
        m_res = sample->GetPointer(&ptr);
        memcpy(ptr, src, size);
        m_res = sample->SetTime(&start, &stoptime);
        m_res = sample->SetSyncPoint(0);
        m_res = sample->SetPreroll(pImage ? 0 : 1);
        m_res = sample->SetDiscontinuity(m_discontinuity);
        m_discontinuity = 0;

        m_pOurOutput->SetPointer(pImage);
        //m_res = m_pOurOutput->Receive(sample); // for debug it displays noise
        m_res = m_pImp->Receive(sample);
        sample->Release();

        *newpts = (double) (m_pOurOutput->GetPTS() / 1E9);
        return TRUE;
    }

    BOOL Resync(REFERENCE_TIME pts)
    {
        m_res = m_pInputPin->NewSegment(pts, 0, 1);
        m_discontinuity = 1;
        return (m_res == S_OK);
    }

    BOOL ShowPropertyPage(void)
    {
        if (!m_pFilter) return FALSE;
        ISpecifyPropertyPages *pProp;
        if ((m_res = m_pFilter->QueryInterface(IID_ISpecifyPropertyPages, (LPVOID *) &pProp)) == S_OK)
        {
            // Get the filter's name and IUnknown pointer.
            FILTER_INFO FilterInfo;
            m_res = m_pFilter->QueryFilterInfo(&FilterInfo); 
            IUnknown *pFilterUnk;
            m_res = m_pFilter->QueryInterface(IID_IUnknown, (LPVOID *) &pFilterUnk);
            CAUUID caGUID;
            pProp->GetPages(&caGUID);
            pProp->Release();

            __try
            {
                m_res = OleCreatePropertyFrame(
                    NULL,                   // Parent window
                    0, 0,                   // Reserved
                    FilterInfo.achName,     // Caption for the dialog box
                    1,                      // Number of objects (just the filter)
                    &pFilterUnk,            // Array of object pointers. 
                    caGUID.cElems,          // Number of property pages
                    caGUID.pElems,          // Array of property page CLSIDs
                    0,                      // Locale identifier
                    0, NULL                 // Reserved
                    );
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
            }
            // Clean up.
            pFilterUnk->Release();
            /* FIXME: it crashes (broken example on msdn?) */
            //FilterInfo.pGraph->Release(); 
            CoTaskMemFree(caGUID.pElems);
        }
        return (!FAILED(m_res));
    }

    BOOL SetOutputFormat(WORD *biBitCount, WORD *biPlanes)
    {
        switch (m_outfmt)
        {
            // YUV
            case mmioFOURCC('Y', 'U', 'Y', '2'):
                m_pDestType.subtype = MEDIASUBTYPE_YUY2;
                *biBitCount = 16;
                return TRUE;
            case mmioFOURCC('U', 'Y', 'V', 'Y'):
                m_pDestType.subtype = MEDIASUBTYPE_UYVY;
                *biBitCount = 16;
                return TRUE;
            case mmioFOURCC('Y', 'V', '1', '2'):
                m_pDestType.subtype = MEDIASUBTYPE_YV12;
                *biBitCount = 12;
                *biPlanes = 3;
                return TRUE;
            case mmioFOURCC('I', 'Y', 'U', 'V'):
                m_pDestType.subtype = MEDIASUBTYPE_IYUV;
                *biBitCount = 12;
                *biPlanes = 3;
                return TRUE;
            case mmioFOURCC('Y', 'V', 'U', '9'):
                m_pDestType.subtype = MEDIASUBTYPE_YVU9;
                *biBitCount = 9;
                return TRUE;
        }

        /* RGB */
        unsigned int bits = m_outfmt & 0xff;
        unsigned int check = m_outfmt ^ bits;

        if (check == mmioFOURCC(0, 'B', 'G', 'R'))
        {
            *biBitCount = bits;
            switch (bits)
            {
                case 15: m_pDestType.subtype = MEDIASUBTYPE_RGB555; return TRUE;
                case 16: m_pDestType.subtype = MEDIASUBTYPE_RGB565; return TRUE;
                case 24: m_pDestType.subtype = MEDIASUBTYPE_RGB24; return TRUE;
                case 32: m_pDestType.subtype = MEDIASUBTYPE_RGB32; return TRUE;
            }
        }
        fprintf(stderr, "Format not supported 0x%08x\n", m_outfmt);
        return FALSE;
    }

private:
    HMODULE m_hDll;
    GUID m_guid;
    char m_fname[MAX_PATH + 1];
    unsigned int m_outfmt;
    int m_discontinuity;
    HRESULT m_res;
    BITMAPINFOHEADER *m_bih;
    IBaseFilter *m_pFilter;

    IGraphBuilder *m_pGraph;
    DWORD m_dwRegister;
    IMediaControl *m_pMC;

    CSenderFilter *m_pSFilter;
    CRenderFilter *m_pRFilter;
    CRenderPin *m_pOurOutput;
    CSenderPin *m_pOurInput;

    IPin *m_pInputPin;
    IPin *m_pOutputPin;

    IMemInputPin *m_pImp;
    IMemAllocator *m_pAll;
    AM_MEDIA_TYPE m_pOurType, m_pDestType;
    MPEG2VIDEOINFO m_mp2vi;
    VIDEOINFOHEADER m_vi;
    VIDEOINFOHEADER2 m_vi2;
};


extern "C" DSVideoCodec * WINAPI DSOpenVideoCodec(const char *dll, const GUID guid, BITMAPINFOHEADER* bih, unsigned int outfmt)
{
    DSVideoCodec *vcodec = new DSVideoCodec(dll, guid, bih, outfmt);

    if (!vcodec->LoadLibrary())
    {
        fprintf(stderr, "LoadLibrary Failed %d\n", GetLastError());
        delete vcodec;
        return NULL;
    }

    if (vcodec->CreateFilter() && vcodec->CreateGraph() && vcodec->StartGraph())
        return vcodec;

    delete vcodec;
    return NULL;
}

extern "C" void WINAPI DSCloseVideoCodec(DSVideoCodec *vcodec)
{
    delete vcodec;
}

extern "C" BOOL WINAPI DSVideoDecode(DSVideoCodec *vcodec, const BYTE *src, int size, double pts, double *newpts, BYTE *pImage)
{
    return vcodec->Decode(src, size, pts, newpts, pImage);
}

extern "C" BOOL WINAPI DSVideoResync(DSVideoCodec *vcodec, double pts)
{
    return vcodec->Resync((REFERENCE_TIME) (pts * 1E9));
}

extern "C" BOOL WINAPI DSShowPropertyPage(DSVideoCodec *codec)
{
    return codec->ShowPropertyPage();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
        {
            DisableThreadLibraryCalls(hModule);
            return (OleInitialize(NULL) == S_OK);
        }
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            return TRUE;
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}

