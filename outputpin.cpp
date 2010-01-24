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

COutputMemPin::COutputMemPin(COutputPin *parent) : parent(parent), CUnknown(NAME("COutputMemPin"), NULL)
{
}

HRESULT COutputMemPin::Receive(IMediaSample *pSample)
{
    if (parent->SampleProc)
        return parent->SampleProc(parent->pUserData, pSample);
    return S_FALSE;
}

HRESULT COutputMemPin::ReceiveMultiple(IMediaSample **pSamples, long nSamples, long *nSamplesProcessed)
{
    HRESULT hr = S_OK;
    for (*nSamplesProcessed=0; *nSamplesProcessed < nSamples; (*nSamplesProcessed)++)
    {
         hr = Receive(pSamples[*nSamplesProcessed]);
         if (hr != S_OK) break;
    }
    return hr;
}

COutputPin::COutputPin(const AM_MEDIA_TYPE *amt, SAMPLEPROC SampleProc, void *pUserData) :
    SampleProc(SampleProc), pUserData(pUserData), CUnknown(NAME("COutputPin"), NULL)
{
    CopyMediaType(&type, amt);
    mempin = new COutputMemPin(this);
    fprintf(stderr, "COutputPin::COutputPin\n");
}

HRESULT COutputPin::CheckMediaType(const CMediaType *)
{
    fprintf(stderr, "COutputPin::CheckMediaType\n");
    return E_NOTIMPL;
}