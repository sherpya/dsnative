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

class CSenderFilter: public CBaseFilter
{
public:
    CSenderFilter::CSenderFilter();
    CSenderFilter::~CSenderFilter() { delete m_pin; }
    int GetPinCount() { return 1; }
    CBasePin *GetPin(int n) { return m_pin; }

private:
    CSenderPin *m_pin;
    HRESULT m_hr;
    CCritSec m_csFilter;
};

class CRenderFilter;
class CRenderPin;

class CRenderPin : public CBaseInputPin
{
public:
    CRenderPin::CRenderPin(HRESULT *phr, CRenderFilter *pFilter, CCritSec *pLock);
    HRESULT CheckMediaType(const CMediaType *) { return S_OK; };
    HRESULT STDMETHODCALLTYPE Receive(IMediaSample *pSample);

    HRESULT STDMETHODCALLTYPE BeginFlush(void) { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE EndFlush(void) { return E_NOTIMPL; }

    void SetPointer(BYTE *ptr) { m_gPtr = ptr; }

private:
    BYTE *m_gPtr;
};

class CRenderFilter: public CBaseFilter
{
public:
    CRenderFilter::CRenderFilter();
    CRenderFilter::~CRenderFilter() { delete m_pin; }
    int GetPinCount() { return 1; }
    CBasePin *GetPin(int n) { return m_pin; }

private:
    CRenderPin *m_pin;
    HRESULT m_hr;
    CCritSec m_csFilter;
};
