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

class COutputPin: public CUnknown, public IPin
{
public:
    COutputPin::COutputPin(const AM_MEDIA_TYPE *amt, SAMPLEPROC SampleProc, void *pUserData);
    DECLARE_IUNKNOWN;
    HRESULT CheckMediaType(const CMediaType *);

    HRESULT STDMETHODCALLTYPE BeginFlush(void) { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE EndFlush(void) { return E_NOTIMPL; }

private:
    friend class COutputMemPin;
    SAMPLEPROC SampleProc;
    void *pUserData;
    AM_MEDIA_TYPE type;
    COutputMemPin *mempin;
};
