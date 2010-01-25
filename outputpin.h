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

typedef HRESULT (__stdcall *SAMPLEPROC)(void *pUserData, IMediaSample *sample);

class COutputMemPin;
class COutputPin;

class COutputMemPin: public CUnknown, public IMemInputPin
{
public:
    COutputMemPin::COutputMemPin(COutputPin *parent);
    DECLARE_IUNKNOWN
    HRESULT STDMETHODCALLTYPE GetAllocator(IMemAllocator **ppAllocator) { *ppAllocator = this->pAllocator; return S_OK; } // MemAllocatorCreate()
    HRESULT STDMETHODCALLTYPE NotifyAllocator(IMemAllocator *pAllocator, BOOL bReadOnly) { this->pAllocator = pAllocator; return S_OK; }
    HRESULT STDMETHODCALLTYPE GetAllocatorRequirements(ALLOCATOR_PROPERTIES *pProps) { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE Receive(IMediaSample *pSample);
    HRESULT STDMETHODCALLTYPE ReceiveMultiple(IMediaSample **pSamples, long nSamples, long *nSamplesProcessed);
    HRESULT STDMETHODCALLTYPE ReceiveCanBlock(void) { return E_NOTIMPL; }
private:
    IMemAllocator *pAllocator;
    COutputPin *parent;
};

class COutputPin: public CBasePin
{
public:
    COutputPin::COutputPin(const AM_MEDIA_TYPE *amt, SAMPLEPROC SampleProc, void *pUserData, CBaseFilter2 *pFilter);
    //DECLARE_IUNKNOWN;
    HRESULT CheckMediaType(const CMediaType *);

    HRESULT STDMETHODCALLTYPE BeginFlush(void) { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE EndFlush(void) { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE QueryAccept(const AM_MEDIA_TYPE *pmt)
    {
        //if(memcmp(&pmt->formattype, &FORMAT_VideoInfo, sizeof(GUID)))
        //    return S_OK;

        //if(memcmp(&pmt->formattype, &FORMAT_VideoInfo2, sizeof(GUID)))
        //    return S_OK;

        return S_OK;
    }

#if 0

    HRESULT STDMETHODCALLTYPE Connect(IPin *pReceivePin, const AM_MEDIA_TYPE *pmt) { return S_OK; } // TODO
    HRESULT STDMETHODCALLTYPE ReceiveConnection(IPin *pConnector, const AM_MEDIA_TYPE *pmt)
    {
        remote = pConnector;
        return S_OK; // or VFW_E_TYPE_NOT_ACCEPT
    }
    HRESULT STDMETHODCALLTYPE Disconnect(void) { return S_OK; } // why 1???
    HRESULT STDMETHODCALLTYPE ConnectedTo(IPin **pPin)
    {
        /* if (!pPin) return E_INVALIDARG; */
        *pPin = remote;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ConnectionMediaType(AM_MEDIA_TYPE *pmt)
    {
        /* if (!pmt) return E_INVALIDARG; */
        CopyMediaType(pmt, &type);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE QueryPinInfo(PIN_INFO *pInfo) { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE QueryDirection(PIN_DIRECTION *pPinDir)
    {
        /* if (!pPinDir) return E_INVALIDARG; */
        *pPinDir = PINDIR_INPUT; /* INPUT ? */
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE QueryId(LPWSTR *Id) { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE EnumMediaTypes(IEnumMediaTypes **ppEnum) { return E_NOTIMPL; } // TODO

    HRESULT STDMETHODCALLTYPE QueryInternalConnections(IPin **apPin, ULONG *nPin) { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE EndOfStream(void) { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate) { return E_NOTIMPL; }
#endif

private:
    friend class COutputMemPin;
    SAMPLEPROC SampleProc;
    void *pUserData;
    AM_MEDIA_TYPE type;
    COutputMemPin *mempin;
    IPin *remote;
    HRESULT m_hr;
    CCritSec pLock;
};
