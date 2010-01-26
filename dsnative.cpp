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
      m_guid(guid), m_bih(bih), m_hDll(NULL), m_outfmt(outfmt), m_pFilter(NULL),
      m_pInputPin(NULL), m_pOutputPin(NULL), m_pOurInput(NULL), m_pOurOutput(NULL),
      m_pImp(NULL), m_pSFilter(NULL)
    {
        strncpy(m_fname, filename, MAX_PATH);
    }

    BOOL LoadLibrary(void)
    {
        return ((m_hDll = ::LoadLibrary(m_fname)) != NULL);
    }

    BOOL CreateFilter(void)
    {
        LPFNGETCLASSOBJECT pDllGetClassObject = (LPFNGETCLASSOBJECT) GetProcAddress(m_hDll, "DllGetClassObject");
        if (!pDllGetClassObject) return FALSE;

        HRESULT res;

        IClassFactory *factory;
        res = pDllGetClassObject(m_guid, IID_IClassFactory, (LPVOID *) &factory);
        if (FAILED(res))return FALSE;

        IUnknown* object;
        res = factory->CreateInstance(NULL, IID_IUnknown, (LPVOID *) &object);
        factory->Release();

        if (FAILED(res)) return FALSE;

        res = object->QueryInterface(IID_IBaseFilter, (LPVOID *) &m_pFilter);
        object->Release();

        return (!FAILED(res));
    }

    BOOL ReleaseFilter(void)
    {
        return m_pFilter->Release();
    }

    BOOL CheckMediaTypes(IPin *pin)
    {
        IEnumMediaTypes *pMedia;
        AM_MEDIA_TYPE *pmt = NULL, *pfnt = NULL;
        HRESULT res = pin->EnumMediaTypes(&pMedia);
        pMedia->Reset();
        while((res = pMedia->Next(1, &pmt, NULL)) == S_OK)
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
        HRESULT res;

        m_pDestType.majortype = MEDIATYPE_Video;
        m_pDestType.subtype = MEDIASUBTYPE_YV12;
        m_pDestType.formattype = FORMAT_VideoInfo2;
        m_pDestType.bFixedSizeSamples = TRUE;
        m_pDestType.bTemporalCompression = FALSE;
        m_pDestType.lSampleSize = 1;
        m_pDestType.pUnk = 0;

        memset(&m_vi2, 0, sizeof(m_vi2));
        memcpy(&m_vi2.bmiHeader, m_bih, sizeof(m_vi2.bmiHeader));
        m_vi2.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        //m_vi2.bmiHeader.biCompression = 0x32315659;
        //m_vi2.bmiHeader.biBitCount = 12;
        //m_vi2.bmiHeader.biPlanes = 3; // FIXME: planes?
        m_vi2.bmiHeader.biSizeImage = labs(m_bih->biWidth * m_bih->biHeight * ((m_bih->biBitCount + 7) / 8)) / 2;

        SetOutputFormat();

        m_pDestType.cbFormat = sizeof(VIDEOINFOHEADER2);
        m_pDestType.pbFormat = (BYTE *) &m_vi2;

        res = m_pOutputPin->QueryAccept(&m_pDestType);

#if 0
        memset(&m_viOut, 0, sizeof(m_viOut));

        m_viOut.rcSource.left = m_viOut.rcSource.top = 0;
        m_viOut.rcSource.right = m_bih->biWidth;
        m_viOut.rcSource.bottom = m_bih->biHeight;
        m_viOut.rcTarget = m_viOut.rcSource;

        memcpy(&m_viOut.bmiHeader, m_bih, sizeof(BITMAPINFOHEADER));
        m_viOut.bmiHeader.biSizeImage = m_pDestType.lSampleSize;

        m_viOut.bmiHeader.biCompression = 0;
        m_viOut.bmiHeader.biBitCount = 24;

        m_pDestType.cbFormat = sizeof(VIDEOINFOHEADER);
        m_pDestType.pbFormat = (BYTE *) &m_viOut;

        //m_viOut.bmiHeader.biHeight *= -1;

        res = m_pOutputPin->QueryAccept(&m_pDestType);
        printf("Decoder supports the following YUV formats:\n");
        ct* c;
        for (c = check; c->bits; c++)
        {
            m_viOut.bmiHeader.biBitCount = c->bits;
            m_viOut.bmiHeader.biCompression = c->fcc;
            m_pDestType.subtype = *c->subtype;
            res = m_pOutputPin->QueryAccept(&m_pDestType);
            printf("%.4s : %s\n", (char *) &c->fcc, (res == S_OK) ? "yes" : "no");
        }
#endif
        return TRUE;
    }

    BOOL SetInputType(void)
    {
        m_pOurType.majortype = MEDIATYPE_Video;
        m_pOurType.subtype = MEDIATYPE_Video;
        m_pOurType.subtype.Data1 = m_bih->biCompression;
        m_pOurType.formattype = FORMAT_VideoInfo;
        m_pOurType.bFixedSizeSamples = FALSE;
        m_pOurType.bTemporalCompression = TRUE;
        m_pOurType.lSampleSize = 1;
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

        m_pOurType.formattype = FORMAT_MPEG2Video;
        m_pOurType.pbFormat = (BYTE *) &m_mp2vi;
        m_pOurType.cbFormat = sizeof(MPEG2VIDEOINFO);
        return TRUE;
    }

    BOOL EnumPins(void)
    {
        HRESULT res;
        IEnumPins *enumpins;
        res = m_pFilter->EnumPins(&enumpins);
        enumpins->Reset();

        IPin *pin;
        PIN_INFO pInfo;

        while ((res = enumpins->Next(1, &pin, NULL)) == S_OK)
        {
            pin->QueryPinInfo(&pInfo);
            wprintf(L"Pin: %s - %s\n", pInfo.achName, (pInfo.dir == PINDIR_INPUT) ? L"Input" : L"Output");
            if (pInfo.dir == PINDIR_INPUT)
            {
                m_pInputPin = pin;
                m_pInputPin->AddRef();
            }
            else if (pInfo.dir == PINDIR_OUTPUT)
            {
                m_pOutputPin = pin;
                m_pOutputPin->AddRef();
            }
            pin->Release();
        }

        enumpins->Release();
        res = m_pInputPin->QueryInterface(IID_IMemInputPin, (LPVOID *) &m_pImp);
        return TRUE;
    }

    BOOL CreateGraph(void)
    {
        HRESULT res;
        this->EnumPins();
        this->SetInputType();

        res = m_pInputPin->QueryAccept(&m_pOurType);

        m_pSFilter = new CSenderFilter();
        m_pOurInput = (CSenderPin *) m_pSFilter->GetPin(0);

        res = m_pInputPin->ReceiveConnection(m_pOurInput, &m_pOurType); 
        res = m_pImp->GetAllocator(&m_pAll);
        ALLOCATOR_PROPERTIES props, props1;

        props.cBuffers = 1;
	    props.cbBuffer = m_pOurType.lSampleSize;
	    props.cbAlign = 1;
	    props.cbPrefix = 0;

        res = m_pAll->SetProperties(&props, &props1);
        res = m_pImp->NotifyAllocator(m_pAll, FALSE);

        m_pOurOutput = (CRenderPin *) m_pSFilter->GetPin(1);

        DebugBreak();
        SetOutputType();
        res = m_pOurOutput->QueryAccept(&m_pDestType);
        res = m_pOutputPin->ReceiveConnection(m_pOurOutput, &m_pDestType);
        
        return TRUE;
    }

    BOOL StartGraph(void)
    {
        HRESULT res;
        res = m_pAll->Commit();
        res = m_pSFilter->Run(0);
        return TRUE;
    }

    BOOL ShowPropertyPage(void)
    {
        if (!m_pFilter) return FALSE;
        HRESULT res;
        ISpecifyPropertyPages *pProp;
        if ((res = m_pFilter->QueryInterface(IID_ISpecifyPropertyPages, (LPVOID *) &pProp)) == S_OK)
        {
            // Get the filter's name and IUnknown pointer.
            FILTER_INFO FilterInfo;
            res = m_pFilter->QueryFilterInfo(&FilterInfo); 
            IUnknown *pFilterUnk;
            res = m_pFilter->QueryInterface(IID_IUnknown, (LPVOID *) &pFilterUnk);
            CAUUID caGUID;
            pProp->GetPages(&caGUID);
            pProp->Release();

            __try
            {
                res = OleCreatePropertyFrame(
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
        return (!FAILED(res));
    }

    void SetOutputFormat(void)
    {
        switch (m_outfmt)
        {
            // YUV
            case mmioFOURCC('Y', 'U', 'Y', '2'):
                m_pDestType.subtype = MEDIASUBTYPE_YUY2;
                m_vi2.bmiHeader.biBitCount = 16;
                break;
            case mmioFOURCC('U', 'Y', 'V', 'Y'):
                m_pDestType.subtype = MEDIASUBTYPE_UYVY;
                m_vi2.bmiHeader.biBitCount = 16;
                break;
            case mmioFOURCC('Y', 'V', '1', '2'):
                m_pDestType.subtype = MEDIASUBTYPE_YV12;
                m_vi2.bmiHeader.biBitCount = 12;
                break;
            case mmioFOURCC('I', 'Y', 'U', 'V'):
                m_pDestType.subtype = MEDIASUBTYPE_IYUV;
                m_vi2.bmiHeader.biBitCount = 12;
                break;
            case mmioFOURCC('Y', 'V', 'U', '9'):
                m_pDestType.subtype = MEDIASUBTYPE_YVU9;
                m_vi2.bmiHeader.biBitCount = 9;
                break;
            default: // RGB // FIXME: 'R', 'G', 'B', bits ??
                {
                    unsigned int bits = m_outfmt & 0xff;
                    m_vi2.bmiHeader.biBitCount = bits;
                    switch (bits)
                    {
                        case 15: m_pDestType.subtype = MEDIASUBTYPE_RGB555; break;
                        case 16: m_pDestType.subtype = MEDIASUBTYPE_RGB565; break;
                        case 24: m_pDestType.subtype = MEDIASUBTYPE_RGB24; break;
                        case 32: m_pDestType.subtype = MEDIASUBTYPE_RGB32; break;
                    }
                }
        }
    }

private:
    HMODULE m_hDll;
    GUID m_guid;
    char m_fname[MAX_PATH + 1];
    unsigned int m_outfmt;
    BITMAPINFOHEADER *m_bih;
    IBaseFilter *m_pFilter;

    CSenderFilter *m_pSFilter;
    CRenderPin *m_pOurOutput;
    CSenderPin *m_pOurInput;

    IPin *m_pInputPin;
    IPin *m_pOutputPin;

    IMemInputPin *m_pImp;
    IMemAllocator *m_pAll;
    AM_MEDIA_TYPE m_pOurType, m_pDestType;
    MPEG2VIDEOINFO m_mp2vi;
    VIDEOINFOHEADER m_vi, m_viOut;
    VIDEOINFOHEADER2 m_vi2;
};


extern "C" DSVideoCodec * WINAPI DSOpenVideoCodec(const char *dll, const GUID guid, BITMAPINFOHEADER* bih, unsigned int outfmt)
{
    DSVideoCodec *vcodec = new DSVideoCodec(dll, guid, bih, outfmt);
    if (!vcodec->LoadLibrary())
    {
        fprintf(stderr, "LoadLibrary Failed %d\n", GetLastError());
        return NULL;
    }
    if (!vcodec->CreateFilter())
        return NULL;
    if (!vcodec->CreateGraph())
        return NULL;
    if (!vcodec->StartGraph())
        return NULL;
    return vcodec;
}

extern "C" void WINAPI DSCloseVideoCodec(DSVideoCodec *vcodec)
{
    vcodec->ReleaseFilter();
    delete vcodec;
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

