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
    DSVideoCodec::DSVideoCodec(const char *cfname, const GUID guid, BITMAPINFOHEADER *bih, unsigned int outfmt, const char *sfname) :
      m_guid(guid), m_bih(bih), m_hDll(NULL), m_outfmt(outfmt), m_vinfo(NULL), m_discontinuity(1), m_pFilter(NULL),
      m_pInputPin(NULL), m_pOutputPin(NULL), m_pOurInput(NULL), m_pOurOutput(NULL),
      m_pImp(NULL), m_pAll(NULL), m_pSFilter(NULL), m_pRFilter(NULL), m_pGraph(NULL), m_pMC(NULL), m_cfname(NULL), m_sfname(NULL)
    {
        int len;

        ASSERT(cfname);
        ASSERT(bih);

        len = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, cfname, -1, NULL, 0);
        if (len > 0)
        {
            m_cfname = new wchar_t[len];
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, cfname, -1, m_cfname, len);
        }

        if (sfname)
        {
            len = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, sfname, -1, NULL, 0);
            if (len > 0)
            {
                m_sfname = new wchar_t[len];
                MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, sfname, -1, m_sfname, len);
            }
        }
    }

    DSVideoCodec::~DSVideoCodec()
    {
        ReleaseGraph();
        if (m_cfname) delete m_cfname;
        if (m_sfname) delete m_sfname;
        if (m_vinfo) delete m_vinfo;
        if (m_hDll) FreeLibrary(m_hDll);
    }

    void ReleaseGraph(void)
    {
        if (m_pMC) m_res = m_pMC->Stop();
        else if (m_pFilter)
            m_res = m_pFilter->Stop();

        if (m_pGraph)
        {
            if (m_pFilter)
            {
                ASSERT(m_pSFilter);
                ASSERT(m_pRFilter);
                m_res = m_pGraph->RemoveFilter(m_pFilter);
                m_res = m_pGraph->RemoveFilter(m_pSFilter);
                m_res = m_pGraph->RemoveFilter(m_pRFilter);
            }
            RemoveFromRot(m_dwRegister);
        }
        else
        {
            if (m_pInputPin) m_res = m_pInputPin->Disconnect();
            if (m_pOutputPin) m_res = m_pOutputPin->Disconnect();
            if (m_pFilter) m_res = m_pFilter->JoinFilterGraph(NULL, NULL);
        }

        if (m_pImp) m_pImp->Release();
        if (m_pFilter) m_res = m_pFilter->Release();
        if (m_pSFilter) m_res = m_pSFilter->Release();
        if (m_pRFilter) m_res = m_pRFilter->Release();
    }

    BOOL LoadLibrary(void)
    {
        HKEY hKey = NULL;

        while ((m_hDll = ::LoadLibraryW(m_cfname)) == NULL)
        {
            /* Try picking path from the registry, if the codecs is registered in the system */
            LONG size;
            wchar_t subkey[61] = L"\\CLSID\\";
            size = (sizeof(subkey) / 2) - 7;

            if (StringFromGUID2(m_guid, subkey + 7, size) != 39)
                break;

            size -= 39;
            wcsncat(subkey, L"\\InprocServer32", size);

            if (RegOpenKeyW(HKEY_CLASSES_ROOT, subkey, &hKey) != ERROR_SUCCESS)
                break;

            if (RegQueryValueW(hKey, NULL, NULL, &size) != ERROR_SUCCESS)
                break;

            delete m_cfname;

            m_cfname = new wchar_t[size];
            if (RegQueryValueW(hKey, NULL, m_cfname, &size) == ERROR_SUCCESS)
                m_hDll = ::LoadLibraryW(m_cfname);

            break;
        }

        if (hKey) RegCloseKey(hKey);
        return (m_hDll != NULL);
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

    dsnerror_t SetOutputType(void)
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
            return DSN_OUTPUT_NOTSUPPORTED;

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
        return (m_res == S_OK) ? DSN_OK : DSN_OUTPUT_NOTACCEPTED;
    }

    BOOL isAVC(DWORD biCompression)
    {
        switch (biCompression)
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
                return TRUE;
        }
        return FALSE;
    }

    BOOL SetInputType(void)
    {
        ULONG cbFormat;

        m_pOurType.majortype = MEDIATYPE_Video;
        m_pOurType.subtype = MEDIATYPE_Video;
        m_pOurType.bFixedSizeSamples = FALSE;
        m_pOurType.bTemporalCompression = TRUE;
        m_pOurType.lSampleSize = 1;
        m_pOurType.pUnk = NULL;

        /* ffdshow (and others ?) needs AVC1 as fourcc for avc video */
        if (isAVC(m_bih->biCompression))
            m_pOurType.subtype.Data1 = mmioFOURCC('A', 'V', 'C', '1');
        else
            m_pOurType.subtype.Data1 = m_bih->biCompression;

        // probe FORMAT_MPEG2Video
        // this is done before FORMAT_VideoInfo because e.g. coreavc will accept anyway the format
        // but it will decode black frames

        int extra = m_bih->biSize - sizeof(BITMAPINFOHEADER);
        cbFormat = FIELD_OFFSET(MPEG2VIDEOINFO, dwSequenceHeader) + extra - 7;

        MPEG2VIDEOINFO *try_mp2vi = (MPEG2VIDEOINFO *) new BYTE[cbFormat];
        m_vinfo = (BYTE *) try_mp2vi;

        memset(try_mp2vi, 0, cbFormat);
        try_mp2vi->hdr.rcSource.left = try_mp2vi->hdr.rcSource.top = 0;
        try_mp2vi->hdr.rcSource.right = m_bih->biWidth;
        try_mp2vi->hdr.rcSource.bottom = m_bih->biHeight;
        try_mp2vi->hdr.rcTarget = try_mp2vi->hdr.rcSource;
        try_mp2vi->hdr.dwPictAspectRatioX = m_bih->biWidth;
        try_mp2vi->hdr.dwPictAspectRatioY = m_bih->biHeight;
        memcpy(&try_mp2vi->hdr.bmiHeader, m_bih, sizeof(BITMAPINFOHEADER));
        try_mp2vi->hdr.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        try_mp2vi->hdr.bmiHeader.biCompression = m_pOurType.subtype.Data1;

        /* From MPC-HC */
        if (extra > 0)
        {
            BYTE *extradata = (BYTE *) m_bih + sizeof(BITMAPINFOHEADER);
            try_mp2vi->dwProfile = extradata[1];
            try_mp2vi->dwLevel = extradata[3];
            try_mp2vi->dwFlags = (extradata[4] & 3) + 1;

            try_mp2vi->cbSequenceHeader = 0;

            BYTE* src = (BYTE *) extradata + 5;
            BYTE* dst = (BYTE *) try_mp2vi->dwSequenceHeader;

            BYTE* src_end = (BYTE *) extradata + extra;
            BYTE* dst_end = (BYTE *) try_mp2vi->dwSequenceHeader + extra;

            for (int i = 0; i < 2; i++)
            {
                for (int n = *src++ & 0x1f; n > 0; n--)
                {
                    int len = ((src[0] << 8) | src[1]) + 2;
                    if(src + len > src_end || dst + len > dst_end) { ASSERT(0); break; }
                    memcpy(dst, src, len);
                    src += len;
                    dst += len;
                    try_mp2vi->cbSequenceHeader += len;
                }
            }
        }

        m_pOurType.formattype = FORMAT_MPEG2Video;
        m_pOurType.pbFormat = m_vinfo;
        m_pOurType.cbFormat = cbFormat;

        if ((m_res = m_pInputPin->QueryAccept(&m_pOurType)) == S_OK)
            return TRUE;

        delete m_vinfo;
        m_vinfo = NULL;

        // probe FORMAT_VideoInfo
        cbFormat = sizeof(VIDEOINFOHEADER) + m_bih->biSize;
        VIDEOINFOHEADER *try_vi = (VIDEOINFOHEADER *) new BYTE[cbFormat];
        m_vinfo = (BYTE *) try_vi;

        memset(try_vi, 0, cbFormat);
        memcpy(&try_vi->bmiHeader, m_bih, m_bih->biSize);

        try_vi->rcSource.left = try_vi->rcSource.top = 0;
        try_vi->rcSource.right = m_bih->biWidth;
        try_vi->rcSource.bottom = m_bih->biHeight;
        try_vi->rcTarget = try_vi->rcSource;

        m_pOurType.formattype = FORMAT_VideoInfo;
        m_pOurType.pbFormat = m_vinfo;
        m_pOurType.cbFormat = cbFormat;

        if ((m_res = m_pInputPin->QueryAccept(&m_pOurType)) == S_OK)
            return TRUE;

        delete m_vinfo;
        m_vinfo = NULL;
        return FALSE;
    }

    BOOL EnumPins(void)
    {
        IEnumPins *enumpins;
        if (m_pFilter->EnumPins(&enumpins) != S_OK)
            return FALSE;

        enumpins->Reset();

        IPin *pin;
        PIN_INFO pInfo;

        // FIXME: ffdshow has 2 input pins "In" and "In Text"
        // there is not way to check mediatype before connection
        // I think I need a list of pins and then probe for all :(
        while ((m_res = enumpins->Next(1, &pin, NULL)) == S_OK)
        {
            pin->QueryPinInfo(&pInfo);
            /* wprintf(L"Pin: %s - %s\n", pInfo.achName, (pInfo.dir == PINDIR_INPUT) ? L"Input" : L"Output"); */
            if (!m_pInputPin && (pInfo.dir == PINDIR_INPUT))
                m_pInputPin = pin;
            else if (!m_pOutputPin && (pInfo.dir == PINDIR_OUTPUT))
                m_pOutputPin = pin;

            pin->Release();
            m_pFilter->Release();
        }

        enumpins->Release();
        if (!(m_pInputPin && m_pInputPin))
            return FALSE;

        if (m_pInputPin->QueryInterface(IID_IMemInputPin, (LPVOID *) &m_pImp) != S_OK)
            return FALSE;

        return TRUE;
    }

    dsnerror_t SetupAllocator(void)
    {
        DSN_CHECK(m_pImp->GetAllocator(&m_pAll), DSN_FAIL_ALLOCATOR);
        ALLOCATOR_PROPERTIES props, props1;

        props.cBuffers = 1;
	    props.cbBuffer = m_pDestType.lSampleSize;
	    props.cbAlign = 1;
	    props.cbPrefix = 0;

        DSN_CHECK(m_pAll->SetProperties(&props, &props1), DSN_FAIL_ALLOCATOR);
        DSN_CHECK(m_pImp->NotifyAllocator(m_pAll, FALSE), DSN_FAIL_ALLOCATOR);
        DSN_CHECK(m_pAll->Commit(), DSN_FAIL_ALLOCATOR);
        return DSN_OK;
    }

    dsnerror_t CreateGraph(bool buildgraph=false)
    {
        if (!EnumPins())
            return DSN_FAIL_ENUM;

        if (!SetInputType())
            return DSN_INPUT_NOTACCEPTED;

        m_pSFilter = new CSenderFilter();
        m_pOurInput = (CSenderPin *) m_pSFilter->GetPin(0);
        /* setup Source filename if someone wants to known it (i.e. ffdshow) */
        m_pSFilter->Load(m_sfname, NULL);
        m_pSFilter->AddRef();

        m_pRFilter = new CRenderFilter();
        m_pOurOutput = (CRenderPin *) m_pRFilter->GetPin(0);
        m_pRFilter->AddRef();

        if (buildgraph)
        {
            DSN_CHECK(CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void **) &m_pGraph), DSN_FAIL_GRAPH);
            DSN_CHECK(DSVideoCodec::AddToRot(m_pGraph, &m_dwRegister), DSN_FAIL_GRAPH);
            DSN_CHECK(m_pGraph->QueryInterface(IID_IMediaControl, (void **) &m_pMC), DSN_FAIL_GRAPH);

            m_pGraph->SetLogFile((DWORD_PTR) GetStdHandle(STD_OUTPUT_HANDLE));
            DSN_CHECK(m_pGraph->AddFilter(m_pFilter, L"Binary Codec"), (m_pInputPin = m_pOutputPin = NULL, DSN_FAIL_GRAPH));
            DSN_CHECK(m_pGraph->AddFilter(m_pSFilter, L"DS Sender"), DSN_FAIL_GRAPH);
            DSN_CHECK(m_pGraph->AddFilter(m_pRFilter, L"DS Render"), DSN_FAIL_GRAPH);
            // Connect our output pin to codec input pin otherwise QueryAccept on the codec output pin will fail
            DSN_CHECK(m_pGraph->ConnectDirect(m_pOurInput, m_pInputPin, &m_pOurType), DSN_INPUT_CONNFAILED);
        }
        else
        {
            m_res = m_pFilter->JoinFilterGraph((IFilterGraph *) m_pSFilter, L"DSNative Graph");
            /* same of above */
            DSN_CHECK(m_pInputPin->ReceiveConnection(m_pOurInput, &m_pOurType), DSN_INPUT_CONNFAILED);
        }

        SetOutputType();

        if (buildgraph)
            DSN_CHECK(m_pGraph->ConnectDirect(m_pOurOutput, m_pOutputPin, &m_pDestType), DSN_OUTPUT_CONNFAILED);
        else
            DSN_CHECK(m_pOutputPin->ReceiveConnection(m_pOurOutput, &m_pDestType), DSN_OUTPUT_CONNFAILED);

        return DSN_OK;
    }

    static HRESULT AddToRot(IUnknown *pUnkGraph, DWORD *pdwRegister)
    {
        IMoniker *pMoniker = NULL;
        IRunningObjectTable *pROT = NULL;

        if (FAILED(GetRunningObjectTable(0, &pROT)))
            return E_FAIL;

        WCHAR wsz[256];
        StringCchPrintfW(wsz, 256, L"FilterGraph %08x pid %08x (dsnative)", (DWORD_PTR) pUnkGraph, GetCurrentProcessId());
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

    dsnerror_t Decode(const BYTE *src, int size, double pts, double *newpts, BYTE *pImage)
    {
        IMediaSample* sample = NULL;
        REFERENCE_TIME start = PTS2RT(pts);
        REFERENCE_TIME stoptime = start + 1;
        BYTE *ptr;

        DSN_CHECK(m_pAll->GetBuffer(&sample, 0, 0, 0), DSN_FAIL_DECODESAMPLE);

        DSN_CHECK(sample->SetActualDataLength(size), DSN_FAIL_DECODESAMPLE);
        DSN_CHECK(sample->GetPointer(&ptr), DSN_FAIL_DECODESAMPLE);
        memcpy(ptr, src, size);
        DSN_CHECK(sample->SetTime(&start, &stoptime), DSN_FAIL_DECODESAMPLE);
        DSN_CHECK(sample->SetSyncPoint(0), DSN_FAIL_DECODESAMPLE);
        DSN_CHECK(sample->SetPreroll(pImage ? 0 : 1), DSN_FAIL_DECODESAMPLE);
        DSN_CHECK(sample->SetDiscontinuity(m_discontinuity), DSN_FAIL_DECODESAMPLE);
        m_discontinuity = 0;

        m_pOurOutput->SetPointer(pImage);
        DSN_CHECK(m_pImp->Receive(sample), DSN_FAIL_RECEIVE);
        sample->Release();

        *newpts = RT2PTS(m_pOurOutput->GetPTS());
        return DSN_OK;
    }

    dsnerror_t Resync(REFERENCE_TIME pts)
    {
        m_res = m_pInputPin->NewSegment(pts, 0, 1);
        m_discontinuity = 1;
        return DSN_OK;
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

        if ((check == mmioFOURCC(0, 'B', 'G', 'R')) || (check == mmioFOURCC(0, 'R', 'G', 'B')))
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
        /* fprintf(stderr, "Format not supported 0x%08x\n", m_outfmt); */
        return FALSE;
    }

private:
    HMODULE m_hDll;
    GUID m_guid;
    wchar_t *m_cfname, *m_sfname;
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
    BYTE *m_vinfo;
    VIDEOINFOHEADER m_vi;
    VIDEOINFOHEADER2 m_vi2;
};


extern "C" DSVideoCodec * WINAPI DSOpenVideoCodec(const char *dll, const GUID guid, BITMAPINFOHEADER* bih,
                                                  unsigned int outfmt, const char *filename, dsnerror_t *err)
{
    DSVideoCodec *vcodec = new DSVideoCodec(dll, guid, bih, outfmt, filename);
    dsnerror_t res = DSN_OK;

    if (!vcodec->LoadLibrary())
        res = DSN_LOADLIBRARY;
    else if (!vcodec->CreateFilter())
        res = DNS_FAIL_FILTER;
    else if ((res = vcodec->CreateGraph()) == DSN_OK)
    {
        vcodec->StartGraph();
        if (*err) *err = DSN_OK;
        return vcodec;
    }
    delete vcodec;
    if (*err) *err = res;
    return NULL;
}

extern "C" void WINAPI DSCloseVideoCodec(DSVideoCodec *vcodec)
{
    delete vcodec;
}

extern "C" dsnerror_t WINAPI DSVideoDecode(DSVideoCodec *vcodec, const BYTE *src, int size, double pts, double *newpts, BYTE *pImage)
{
    return vcodec->Decode(src, size, pts, newpts, pImage);
}

extern "C" dsnerror_t WINAPI DSVideoResync(DSVideoCodec *vcodec, double pts)
{
    return vcodec->Resync((REFERENCE_TIME) (pts * 1E9));
}

extern "C" BOOL WINAPI DSShowPropertyPage(DSVideoCodec *codec)
{
    return codec->ShowPropertyPage();
}

extern "C" unsigned int WINAPI DSGetApiVersion(void)
{
    return DSN_API_VERSION;
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
        {
            DisableThreadLibraryCalls(hModule);
#ifdef _DEBUG
        	DbgInitialise(hModule);
	        DbgSetModuleLevel(LOG_TRACE,5);
	        DbgSetModuleLevel(LOG_MEMORY,5);
	        DbgSetModuleLevel(LOG_ERROR,5);
	        DbgSetModuleLevel(LOG_TIMING,5);
	        DbgSetModuleLevel(LOG_LOCKING,5);
#endif
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
