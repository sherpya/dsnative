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

class DSCodec
{
public:
    DSCodec::DSCodec(const char *filename, const GUID guid, BITMAPINFOHEADER *bih) :
      m_guid(guid), m_bih(bih), m_hDll(NULL), m_filter(NULL),
      m_iPin(NULL), m_oPin(NULL), m_iMem(NULL)
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

        res = object->QueryInterface(IID_IBaseFilter, (LPVOID *) &m_filter);
        object->Release();

        return (!FAILED(res));
    }

    BOOL ReleaseFilter(void)
    {
        return m_filter->Release();
    }

    BOOL SetInputType(void)
    {
        m_sOurType.majortype = MEDIATYPE_Video;
        m_sOurType.subtype = MEDIATYPE_Video;
        m_sOurType.subtype.Data1 = m_bih->biCompression;
        m_sOurType.formattype = FORMAT_VideoInfo;
        m_sOurType.bFixedSizeSamples = FALSE;
        m_sOurType.bTemporalCompression = TRUE;
        m_sOurType.lSampleSize = 1;
        m_sOurType.pUnk = NULL;

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

        m_sOurType.formattype = FORMAT_VideoInfo;
        m_sOurType.pbFormat = (BYTE *) &m_vi;
        m_sOurType.cbFormat = sizeof(VIDEOINFOHEADER);
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

        m_sOurType.formattype = FORMAT_MPEG2Video;
        m_sOurType.pbFormat = (BYTE *) &m_mp2vi;
        m_sOurType.cbFormat = sizeof(MPEG2VIDEOINFO);
        return TRUE;
    }

    BOOL EnumPins(void)
    {
        HRESULT res;
        IEnumPins *enumpins;
        res = m_filter->EnumPins(&enumpins);
        enumpins->Reset();

        IPin *pin;
        PIN_INFO pInfo;

        while ((res = enumpins->Next(1, &pin, NULL)) == S_OK)
        {
            pin->QueryPinInfo(&pInfo);
            wprintf(L"Pin: %s - %s\n", pInfo.achName, (pInfo.dir == PINDIR_INPUT) ? L"Input" : L"Output");
            if (pInfo.dir == PINDIR_INPUT)
            {
                m_iPin = pin;
                m_iPin->AddRef();
            }
            else if (pInfo.dir == PINDIR_OUTPUT)
            {
                m_oPin = pin;
                m_oPin->AddRef();
            }
            pin->Release();
        }

        enumpins->Release();
        res = m_iPin->QueryInterface(IID_IMemInputPin, (LPVOID *) &m_iMem);
        return TRUE;
    }

    BOOL CreateGraph(void)
    {
        this->EnumPins();
        this->SetInputType();
        HRESULT res;
        DebugBreak();
        res = m_iPin->QueryAccept(&m_sOurType);
        //CBaseFilter s_filter = CBaseFilter();
        return TRUE;
    }

    BOOL ShowPropertyPage(void)
    {
        if (!m_filter) return FALSE;
        HRESULT res;
        ISpecifyPropertyPages *pProp;
        if ((res = m_filter->QueryInterface(IID_ISpecifyPropertyPages, (LPVOID *) &pProp)) == S_OK)
        {
            // Get the filter's name and IUnknown pointer.
            FILTER_INFO FilterInfo;
            res = m_filter->QueryFilterInfo(&FilterInfo); 
            IUnknown *pFilterUnk;
            res = m_filter->QueryInterface(IID_IUnknown, (LPVOID *) &pFilterUnk);
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
    IBaseFilter *m_filter;

    IPin *m_iPin;
    IPin *m_oPin;
    IMemInputPin *m_iMem;
    AM_MEDIA_TYPE m_sOurType, m_sDestType;
    MPEG2VIDEOINFO m_mp2vi;
    VIDEOINFOHEADER m_vi;
};


extern "C" DSCodec * WINAPI DSOpenCodec(const char *dll, const GUID guid, BITMAPINFOHEADER* bih)
{
    DSCodec *codec = new DSCodec(dll, guid, bih);
    if (!codec->LoadLibrary())
        return NULL;
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

