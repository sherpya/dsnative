#include "stdafx.h"

class DSCodec
{
public:
    DSCodec::DSCodec(const char *filename, const GUID guid, BITMAPINFOHEADER *bih) :
      m_guid(guid), m_bih(bih), m_hDll(NULL), m_filter(NULL)
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
};


extern "C" DSCodec * WINAPI DSOpenCodec(const char *dll, const GUID guid, BITMAPINFOHEADER* bih)
{
    DSCodec *codec = new DSCodec(dll, guid, bih);
    if (!codec->LoadLibrary())
        return NULL;
    if (!codec->CreateFilter())
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

