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

CRenderPin::CRenderPin(HRESULT *phr, CRenderFilter *pFilter, CCritSec *pLock) : m_gPtr(NULL), CBaseInputPin(NAME("CRenderPin"), pFilter, pLock, phr, L"Render")
{
    fprintf(stderr, "CRenderPin::CRenderPin\n");
}

HRESULT CRenderPin::CheckMediaType(const CMediaType *)
{
    // FIXME
    //fprintf(stderr, "CRenderPin::CheckMediaType\n");
    return S_OK;
}

HRESULT CRenderPin::Receive(IMediaSample *pSample)
{
    BYTE *ptr;
    assert(m_gPtr);
    pSample->GetPointer(&ptr);
    long len = pSample->GetActualDataLength();
    memcpy(m_gPtr, ptr, len);
    return S_OK;
}

CRenderFilter::CRenderFilter() : CBaseFilter(NAME("CRenderFilter"), NULL, &m_csFilter, GUID_NULL)
{
    fprintf(stderr, "CRenderFilter::CRenderFilter\n");
    m_pin = new CRenderPin(&m_hr, this, &m_csFilter);
}
