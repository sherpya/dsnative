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

class CBaseFilter1;
class CBaseFilter2;
class CRemotePin1;
class CRemotePin2;

class CRemotePin1: public CBasePin
{
public:
    CRemotePin1::CRemotePin1(HRESULT *phr, CBaseFilter1 *pFilter, IPin *remote, CCritSec *pLock);
    HRESULT CheckMediaType(const CMediaType *);

    HRESULT STDMETHODCALLTYPE BeginFlush(void) { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE EndFlush(void) { return E_NOTIMPL; }

    HRESULT STDMETHODCALLTYPE ConnectedTo(IPin **pPin);
    HRESULT STDMETHODCALLTYPE QueryPinInfo(PIN_INFO *pInfo);

    //HRESULT STDMETHODCALLTYPE Connect(IPin *pReceivePin, const AM_MEDIA_TYPE *pmt);
    // CRemotePin_Destroy
private:
    IPin *remote_pin;
    IBaseFilter *parent;
};

class CRemotePin2: public CBasePin
{
public:
    CRemotePin2::CRemotePin2(HRESULT *phr, CBaseFilter2 *pFilter, CCritSec *pLock);
    HRESULT CheckMediaType(const CMediaType *);

    HRESULT STDMETHODCALLTYPE BeginFlush(void) { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE EndFlush(void) { return E_NOTIMPL; }
    // CRemotePin2_QueryPinInfo
    // CRemotePin2_Destroy
};

class CBaseFilter1 : public CBaseFilter
{
public:
    CBaseFilter1::CBaseFilter1(const AM_MEDIA_TYPE* type, CBaseFilter2* parent);
    int GetPinCount(void);
    CBasePin *GetPin(int n);
private:
    CRemotePin1 *m_pin;
    IPin *m_unused_pin;
    HRESULT m_hr;
    CCritSec m_csFilter;
};

class CBaseFilter2 : public CBaseFilter
{
public:
    CBaseFilter2::CBaseFilter2();
    int GetPinCount(void);
    CBasePin *GetPin(int n);
    // CBaseFilter2_Destroy
private:
    CRemotePin2 *m_pin;
    HRESULT m_hr;
    CCritSec m_csFilter;
};

