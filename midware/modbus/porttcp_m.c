/*
 * FreeModbus Libary: BARE Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id$
 */

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mb_m.h"
#include "mbport.h"

#if MB_MASTER_TCP_ENABLED > 0
BOOL xMBMasterTCPPortInit( USHORT usTCPPort )
{
    return TRUE;
}

void vMBMasterTCPPortClose( void )
{
    
}

void vMBMasterTCPPortDisable( void )
{
    
}

BOOL xMBMasterTCPPortGetRequest( UCHAR **ppucMBTCPFrame, USHORT * usTCPLength )
{
    return TRUE;
}

BOOL xMBMasterTCPPortSendResponse( const UCHAR *pucMBTCPFrame, USHORT usTCPLength )
{
    return TRUE;
}

#endif
