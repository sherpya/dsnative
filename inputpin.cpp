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
#include <initguid.h>

class CBaseFilter2;
class CRemotePin2;

class CRemotePin2 : public CBaseOutputPin
{
    CRemotePin2(CBaseFilter2 *parent);
    DECLARE_IUNKNOWN
    CBaseFilter2 *m_parent;
    IPin *m_pin;

    STDMETHODIMP QueryInterface(REFIID riid, void *ppvObject) { return S_OK; }
    //ULONG STDMETHODCALLTYPE AddRef(void) { return 0; }
    //ULONG STDMETHODCALLTYPE Release(void) { return 0; }
};

class CBaseFilter2 : public IMediaFilter
{
    STDMETHODIMP QueryInterface(const IID &riid, void **ppvObject)
    {
        return S_OK;
    }

    STDMETHODIMP GetClassID(CLSID *pClassID) { return S_OK; }
    STDMETHODIMP Stop(void) { return S_OK; }
    STDMETHODIMP Pause(void) { return S_OK; }
    STDMETHODIMP Run(REFERENCE_TIME tStart) { return S_OK; }
    STDMETHODIMP GetState(DWORD dwMilliSecsTimeout, FILTER_STATE *State) { return S_OK; }
    STDMETHODIMP SetSyncSource(IReferenceClock *pClock) { return S_OK; }
    STDMETHODIMP GetSyncSource(IReferenceClock **pClock) { return S_OK; }

    ULONG STDMETHODCALLTYPE AddRef(void) { return 0; }
    ULONG STDMETHODCALLTYPE Release(void) { return 0; }

};

void a()
{
    CBaseFilter2 a;
}
