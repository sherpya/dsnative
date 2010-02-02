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

typedef enum
{
    DSN_OK = 0,
    DSN_LOADLIBRARY,
    DSN_INPUT_NOTACCEPTED,
    DSN_INPUT_CONNFAILED,
    DSN_OUTPUT_NOTACCEPTED,
    DSN_OUTPUT_NOTSUPPORTED,
    DSN_OUTPUT_CONNFAILED,
    DSN_FAIL_ALLOCATOR,
    DSN_FAIL_GRAPH,
    DNS_FAIL_FILTER,
    DSN_FAIL_DECODESAMPLE,
    DSN_FAIL_RECEIVE,
    DSN_FAIL_ENUM,
    DSN_MAX
} dsnerror_t;

#define DSN_CHECK(expr, err) do { if ((m_res = (expr)) != S_OK) return err; } while (0)
