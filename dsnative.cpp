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

#define fccYUV  mmioFOURCC('Y', 'U', 'V', ' ')
#define fccYUY2 mmioFOURCC('Y', 'U', 'Y', '2')
#define fccYV12 mmioFOURCC('Y', 'V', '1', '2')/* Planar mode: Y + V + U  (3 planes) */
#define fccI420 mmioFOURCC('I', '4', '2', '0')
#define fccIYUV mmioFOURCC('I', 'Y', 'U', 'V')/* Planar mode: Y + U + V  (3 planes) */
#define fccUYVY mmioFOURCC('U', 'Y', 'V', 'Y')/* Packed mode: U0+Y0+V0+Y1 (1 plane) */
#define fccYVYU mmioFOURCC('Y', 'V', 'Y', 'U')/* Packed mode: Y0+V0+Y1+U0 (1 plane) */
#define fccYVU9 mmioFOURCC('Y', 'V', 'U', '9')/* Planar 4:1:0 */
#define fccIF09 mmioFOURCC('I', 'F', '0', '9')/* Planar 4:1:0 + delta */

enum CAPS
{
    CAP_NONE = 0,
    CAP_YUY2 = 1,
    CAP_YV12 = 2,
    CAP_IYUV = 4,
    CAP_UYVY = 8,
    CAP_YVYU = 16,
    CAP_I420 = 32,
    CAP_YVU9 = 64,
    CAP_IF09 = 128,
};

typedef struct _ct
{
    unsigned int bits;
    unsigned int fcc;
    const GUID *subtype;
    int cap;
} ct;
            
static ct check[] = {
        {16, fccYUY2, &MEDIASUBTYPE_YUY2, CAP_YUY2},
        {12, fccIYUV, &MEDIASUBTYPE_IYUV, CAP_IYUV},
        {16, fccUYVY, &MEDIASUBTYPE_UYVY, CAP_UYVY},
        {12, fccYV12, &MEDIASUBTYPE_YV12, CAP_YV12},
        {16, fccYV12, &MEDIASUBTYPE_YV12, CAP_YV12},
        {16, fccYVYU, &MEDIASUBTYPE_YVYU, CAP_YVYU},
//        {12, fccI420, &MEDIASUBTYPE_I420, CAP_I420},
        {9,  fccYVU9, &MEDIASUBTYPE_YVU9, CAP_YVU9},
        {0, 0, 0, 0},
        };

class DSCodec
{
public:
    DSCodec::DSCodec(const char *filename, const GUID guid, BITMAPINFOHEADER *bih) :
      m_guid(guid), m_bih(bih), m_hDll(NULL), m_pFilter(NULL),
      m_pInputPin(NULL), m_pOutputPin(NULL), m_pOurInput(NULL), m_pOurOutput(NULL),
      m_pImp(NULL), m_pSrcFilter(NULL), m_pParentFilter(NULL)
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
        m_vi2.bmiHeader.biCompression = 0x32315659;
        m_vi2.bmiHeader.biBitCount = 12;
        m_vi2.bmiHeader.biPlanes = 3;
        m_vi2.bmiHeader.biSizeImage = labs(m_bih->biWidth * m_bih->biHeight * ((m_bih->biBitCount + 7) / 8)) / 2;

        m_pDestType.cbFormat = sizeof(VIDEOINFOHEADER2);
        m_pDestType.pbFormat = (BYTE *) &m_vi2;

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
        this->SetOutputType();

        res = m_pInputPin->QueryAccept(&m_pOurType);
        m_pParentFilter = new CBaseFilter2();
        m_pSrcFilter = new CBaseFilter1(&m_pOurType, m_pParentFilter);

        m_pOurInput = m_pSrcFilter->GetPin(1);
        m_pOurInput->AddRef();

        DebugBreak();
        res = m_pInputPin->ReceiveConnection(m_pOurInput, &m_pOurType); 

        res = m_pImp->GetAllocator(&m_pAll);
        ALLOCATOR_PROPERTIES props, props1;

        props.cBuffers = 1;
	    props.cbBuffer = m_pOurType.lSampleSize;
	    props.cbAlign = 1;
	    props.cbPrefix = 0;

        res = m_pAll->SetProperties(&props, &props1);

        m_pOurOutput = new COutputPin(&m_pDestType, NULL, NULL, m_pParentFilter);

        SetOutputType();
        res = m_pOutputPin->ReceiveConnection(m_pOurOutput, &m_pDestType);
        res = m_pImp->NotifyAllocator(m_pAll, FALSE);

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

private:
    HMODULE m_hDll;
    GUID m_guid;
    char m_fname[MAX_PATH + 1];
    BITMAPINFOHEADER *m_bih;
    IBaseFilter *m_pFilter;

    CBaseFilter1 *m_pSrcFilter;
    CBaseFilter2 *m_pParentFilter;

    COutputPin* m_pOurOutput;

    IPin *m_pInputPin;
    IPin *m_pOutputPin;
    IPin *m_pOurInput;

    IMemInputPin *m_pImp;
    IMemAllocator *m_pAll;
    AM_MEDIA_TYPE m_pOurType, m_pDestType;
    MPEG2VIDEOINFO m_mp2vi;
    VIDEOINFOHEADER m_vi, m_viOut;
    VIDEOINFOHEADER2 m_vi2;
};


extern "C" DSCodec * WINAPI DSOpenCodec(const char *dll, const GUID guid, BITMAPINFOHEADER* bih)
{
    DSCodec *codec = new DSCodec(dll, guid, bih);
    if (!codec->LoadLibrary())
    {
        fprintf(stderr, "LoadLibrary Failed %d\n", GetLastError());
        return NULL;
    }
    if (!codec->CreateFilter())
        return NULL;
    if (!codec->CreateGraph())
        return NULL;
    return codec;
}

extern "C" void WINAPI DSCloseCodec(DSCodec *codec)
{
    codec->ReleaseFilter();
    delete codec;
}

extern "C" BOOL WINAPI DSShowPropertyPage(DSCodec *codec)
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

