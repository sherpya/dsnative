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

/* ---------------------------------------------- */

CRemotePin1::CRemotePin1(HRESULT *phr, CBaseFilter1 *pFilter, IPin *remote, CCritSec *pLock) :
    remote_pin(remote), parent(pFilter), CBasePin(NAME("CRemotePin1"), pFilter, pLock, phr, L"Output", PINDIR_OUTPUT)
{
    fprintf(stderr, "CRemotePin1::CRemotePin\n");
}

HRESULT CRemotePin1::CheckMediaType(const CMediaType *)
{
    fprintf(stderr, "CRemotePin1::CheckMediaType\n");
    return E_NOTIMPL;
}

//HRESULT CRemotePin1::Connect(IPin *pReceivePin, const AM_MEDIA_TYPE *pmt)
//{
//    return E_NOTIMPL;
//}

HRESULT CRemotePin1::ConnectedTo(IPin **pPin)
{
    if (!pPin) return E_POINTER;
    *pPin = remote_pin;
    remote_pin->AddRef();
    return S_OK;
}

HRESULT CRemotePin1::QueryPinInfo(PIN_INFO *pInfo)
{
    pInfo->dir = PINDIR_OUTPUT;
    pInfo->pFilter = parent;
    pInfo->achName[0] = 0;
    return S_OK;
}

/* ---------------------------------------------- */

CRemotePin2::CRemotePin2(HRESULT *phr, CBaseFilter2 *pFilter, CCritSec *pLock) : CBasePin(NAME("CRemotePin2"), pFilter, pLock, phr, L"Output", PINDIR_OUTPUT)
{
    fprintf(stderr, "CRemotePin2::CRemotePin2\n");
}

HRESULT CRemotePin2::CheckMediaType(const CMediaType *)
{
    fprintf(stderr, "CRemotePin2::CheckMediaType\n");
    return E_NOTIMPL;
}

/* ---------------------------------------------- */

CBaseFilter1::CBaseFilter1(const AM_MEDIA_TYPE* type, CBaseFilter2* parent) : CBaseFilter(NAME("CBaseFilter2"), NULL, &m_csFilter, GUID_NULL)
{
    fprintf(stderr, "CBaseFilter1::CBaseFilter1\n");
    m_pin = new CRemotePin1(&m_hr, this,  parent->GetPin(1), &m_csFilter);
}

int CBaseFilter1::GetPinCount(void)
{
    fprintf(stderr, "CBaseFilter::GetPinCount\n");
    return 1;
}

CBasePin *CBaseFilter1::GetPin(int n)
{
    fprintf(stderr, "CBaseFilter::GetPin\n");
    return this->m_pin;
}

/* ---------------------------------------------- */

CBaseFilter2::CBaseFilter2() : CBaseFilter(NAME("CBaseFilter2"), NULL, &m_csFilter, GUID_NULL)
{
    fprintf(stderr, "CBaseFilter2::CBaseFilter2\n");
    m_pin = new CRemotePin2(&m_hr, this, &m_csFilter);
}

int CBaseFilter2::GetPinCount(void)
{
    fprintf(stderr, "CBaseFilter2::GetPinCount\n");
    return 1;
}

CBasePin *CBaseFilter2::GetPin(int n)
{
    fprintf(stderr, "CBaseFilter2::GetPin\n");
    return this->m_pin;
}
