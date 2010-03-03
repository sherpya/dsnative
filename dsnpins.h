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

class CSenderFilter;
class CSenderPin;

class CSenderPin : public CBaseOutputPin
{
public:
    CSenderPin::CSenderPin(HRESULT *phr, CSenderFilter *pFilter, CCritSec *pLock);
    HRESULT CheckMediaType(const CMediaType *) { return S_OK; };
    HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest);

    HRESULT STDMETHODCALLTYPE BeginFlush(void) { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE EndFlush(void) { return E_NOTIMPL; }
};

class CSenderFilter: public CBaseFilter, public IFileSourceFilter, public IFilterGraph
{
public:

    DECLARE_IUNKNOWN

    CSenderFilter::CSenderFilter();
    CSenderFilter::~CSenderFilter();
    int GetPinCount() { return 1; }
    CBasePin *GetPin(int n) { return m_pin; }

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

    /* IFileSourceFilter */
    HRESULT STDMETHODCALLTYPE Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE *pmt);
    HRESULT STDMETHODCALLTYPE GetCurFile(LPOLESTR *ppszFileName, AM_MEDIA_TYPE *pmt);

    /* IFilterGraph */
    HRESULT STDMETHODCALLTYPE AddFilter(IBaseFilter *pFilter, LPCWSTR pName) { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE RemoveFilter(IBaseFilter *pFilter) { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE EnumFilters(IEnumFilters **ppEnum) { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE FindFilterByName(LPCWSTR pName, IBaseFilter **ppFilter) { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetDefaultSyncSource(void) { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE ConnectDirect(IPin *ppinOut, IPin *ppinIn, const AM_MEDIA_TYPE *pmt) { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE Reconnect(IPin *ppin) { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE Disconnect(IPin *ppin) { return E_NOTIMPL; }

private:
    CSenderPin *m_pin;
    HRESULT m_hr;
    CCritSec m_csFilter;
    LPOLESTR m_pFileName;
};

class CRenderFilter;
class CRenderPin;

class CRenderPin : public CBaseInputPin
{
public:
    CRenderPin::CRenderPin(HRESULT *phr, CRenderFilter *pFilter, CCritSec *pLock);
    HRESULT CheckMediaType(const CMediaType *) { return S_OK; };
    HRESULT STDMETHODCALLTYPE Receive(IMediaSample *pSample);
    REFERENCE_TIME GetPTS(void) { return m_reftime; }

    HRESULT STDMETHODCALLTYPE BeginFlush(void) { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE EndFlush(void) { return E_NOTIMPL; }

    void SetPointer(BYTE *ptr) { m_gPtr = ptr; }
    void SetFrameSize(long size) { m_fSize = size; }

private:
    REFERENCE_TIME m_reftime;
    BYTE *m_gPtr;
    long m_fSize;
};

class CRenderFilter: public CBaseFilter
{
public:
    CRenderFilter::CRenderFilter();
    CRenderFilter::~CRenderFilter();
    int GetPinCount() { return 1; }
    CBasePin *GetPin(int n) { return m_pin; }

private:
    CRenderPin *m_pin;
    HRESULT m_hr;
    CCritSec m_csFilter;
};
