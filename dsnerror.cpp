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

extern "C" const char * WINAPI DSStrError(dsnerror_t error)
{
    switch (error)
    {
        case DSN_OK                  : return "Success";
        case DSN_LOADLIBRARY         : return "LoadLibrary Failed";
        case DSN_INPUT_NOTACCEPTED   : return "Input not accepted";
        case DSN_INPUT_CONNFAILED    : return "Connection to input pin failed";
        case DSN_OUTPUT_NOTACCEPTED  : return "Output not accepted";
        case DSN_OUTPUT_NOTSUPPORTED : return "Output not supported";
        case DSN_OUTPUT_CONNFAILED   : return "Connection to output pin failed";
        case DSN_FAIL_ALLOCATOR      : return "Error with Allocator";
        case DSN_FAIL_GRAPH          : return "Error building Graph";
        case DNS_FAIL_FILTER         : return "Error building Filter";
        case DSN_FAIL_DECODESAMPLE   : return "Error decoding sample";
        case DSN_FAIL_RECEIVE        : return "Error receiving sample from codec";
        case DSN_FAIL_ENUM           : return "Codec Enum Pins failed";
    }
    return "Unknown Error";
}
