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

CSenderPin::CSenderPin(HRESULT *phr, CSenderFilter *pFilter, CCritSec *pLock) : CBaseOutputPin(NAME("CSenderPin"), pFilter, pLock, phr, L"Output")
{
    fprintf(stderr, "CSenderPin::CSenderPin\n");
}

HRESULT CSenderPin::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest)
{
    return E_NOTIMPL;
}

HRESULT CSenderPin::CheckMediaType(const CMediaType *)
{
    fprintf(stderr, "CSenderPin::CheckMediaType\n");
    return S_OK;
}

CSenderFilter::CSenderFilter() : CBaseFilter(NAME("CSenderFilter"), NULL, &m_csFilter, GUID_NULL)
{
    fprintf(stderr, "CSenderFilter::CSenderFilter\n");
    m_pin = new CSenderPin(&m_hr, this, &m_csFilter);
}
