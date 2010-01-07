#include "stdafx.h"

class DSCodec
{
public:
    DSCodec::DSCodec(const char *filename, const GUID guid) : m_guid(guid), m_hDll(NULL), m_filter(NULL)
    {
        strncpy(m_fname, filename, MAX_PATH);
    };

    BOOL LoadLibrary(void)
    {
        return ((m_hDll = ::LoadLibraryA(m_fname)) != NULL);
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

        if (FAILED(res)) return FALSE;
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
            /* FIXME: it crashes with coreavc */
            //FilterInfo.pGraph->Release(); 
            CoTaskMemFree(caGUID.pElems);
        }
    }

private:
    HMODULE m_hDll;
    GUID m_guid;
    char m_fname[MAX_PATH + 1];
    IBaseFilter *m_filter;
};


extern "C" DSCodec * WINAPI DSOpenCodec(const char *dll, const GUID guid, BITMAPINFOHEADER* bih)
{
    return NULL;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
        {
            DisableThreadLibraryCalls((HMODULE) hModule);
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

