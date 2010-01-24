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

CRemotePin2::CRemotePin2(HRESULT *phr, CBaseFilter2 *pFilter, CCritSec *pLock) : CBasePin(NAME("CRemotePin2"), pFilter, pLock, phr, L"Output", PINDIR_OUTPUT)
{
    fprintf(stderr, "CRemotePin2::CRemotePin2\n");
}

HRESULT CRemotePin2::CheckMediaType(const CMediaType *)
{
    fprintf(stderr, "CRemotePin2::CheckMediaType\n");
    return E_NOTIMPL;
}

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
