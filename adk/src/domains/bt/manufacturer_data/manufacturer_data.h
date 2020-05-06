/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.\n
            All Rights Reserved.\n
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       manufacturer_data.h
\brief      Header file for manufacturer_data

ENABLE_TYM_PLATFORM

Support for BR/EDR Tx power is yet to be implemented.
*/

#ifndef __MANUFACTURER_DATA_H__
#define __MANUFACTURER_DATA_H__

#include "le_advertising_manager.h"

#define MANUFACTURE_DATA_ADV_SIZE    (12)

bool ManufacturerData_Init(Task init_task);



#endif/*__MANUFACTURER_DATA_H__*/
