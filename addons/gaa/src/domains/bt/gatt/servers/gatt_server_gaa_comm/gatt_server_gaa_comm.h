/*!
\copyright  Copyright (c) 2019 Qualcomm Technologies International, Ltd.
            All Rights Reserved.
            Qualcomm Technologies International, Ltd. Confidential and Proprietary.
\file       gatt_server_gaa_comm.h
\brief      Gaa communications component.
*/

#ifndef GAA_COMM_H
#define GAA_COMM_H

#if defined(INCLUDE_GAA_LE) && !defined(INCLUDE_GAA) 
    #error INCLUDE_GAA_LE must be used in conjunction with INCLUDE_GAA
#endif 
 

/*! \brief Initialise the Gaa communication component. */
bool GattServerGaaComm_Init(Task init_task);

#endif /* GAA_COMM_H */
