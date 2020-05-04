/* Copyright (c) 2019 Qualcomm Technologies International, Ltd. */

/*!
@file    gatt_gaa_comm_server.h
@brief   Header file for the GATT GAA COMM server library
*/

#ifndef GATT_GAA_COMM_SERVER_H_
#define GATT_GAA_COMM_SERVER_H_

#include "gatt_manager.h"

#include <csrtypes.h>
#include <message.h>
#include <library.h>


typedef enum {
  gatt_gaa_channel_control,
  gatt_gaa_channel_audio,

  gatt_gaa_channel_last
} gatt_gaa_channel_t;

typedef enum {
  tx_failed_no_space_available,
  tx_failed_sink_invalid,
  tx_success
} gatt_gaa_tx_status_t;

/*! @brief Enumeration of messages an application task can receive from the GAA COMM server library.
 */
typedef enum
{
    /* Server messages */
    GATT_GAA_COMM_SERVER_WRITE_IND = GATT_GAA_COMM_SERVER_MESSAGE_BASE,
    GATT_GAA_COMM_SERVER_CONNECT_IND,
    GATT_GAA_COMM_SERVER_DISCONNECT_IND,
    GATT_GAA_COMM_SERVER_TICK,
    GATT_GAA_COMM_TX_AVAILABLE,

    /* Library message limit */
    GATT_GAA_COMM_SERVER_MESSAGE_TOP
} gatt_gaa_comm_server_message_id_t;

/*!
    @brief Status code returned from the GATT GAA COMM server library

    This status code indicates the outcome of the request.
*/
typedef enum
{
    gatt_gaa_comm_server_status_success,
    gatt_gaa_comm_server_status_registration_failed,
    gatt_gaa_comm_server_status_invalid_parameter,
    gatt_gaa_comm_server_status_not_allowed,
    gatt_gaa_comm_server_status_failed
} gatt_gaa_comm_server_status_t;


typedef struct __GATT_GAA_COMM_SERVER_WRITE_IND
{
    uint16 cid;                 /*! Connection ID */
    gatt_gaa_channel_t channel;
    uint16 handle;
    uint16 length;        /*! Client Configuration value to be written */
    uint8  value[1];
}GATT_GAA_COMM_SERVER_WRITE_IND_T;

typedef struct
{
    bdaddr addr;    /*! Public Bluetooth Device address  */
    gatt_gaa_channel_t channel; /*! Gaa channel to connect */
    
} GATT_GAA_COMM_SERVER_CONNECT_IND_T;

typedef struct
{
    gatt_gaa_channel_t channel; /*! Gaa channel to disconnect */
    
} GATT_GAA_COMM_SERVER_DISCONNECT_IND_T;

/*!
    @brief Initialises the GAA COMM Service Library in the Server role.

    @param app_task The Task that will receive the messages sent from this GAA COMM server library.
    @param start_handle start handle
    @param end_handle end handle

    @return TRUE if successful, FALSE otherwise

*/
bool GattGaaCommServerInit(
                           Task app_task,
                           uint16 start_handle,
                           uint16 end_handle);

/*!
    @brief Send data across the GAA COMM Service

    @param channel Gaa channel type
    @param data    Data to send
    @param length  Length of the data being sent

    @return status of the transmission

*/
gatt_gaa_tx_status_t GaaCommServerSendNotification(gatt_gaa_channel_t channel, const uint8 *data, uint32 length);

/*!
    @brief Respond to a GATT_GAA_COMM_CONNECT_IND message

    @param channel Gaa channel type
    @param ok TRUE if the connection was accepted

*/
void GattGaaCommConnectRsp(gatt_gaa_channel_t channel, bool ok);

/*!
    @brief Respond to a GATT_GAA_COMM_DISCONNECT_IND message

    @param channel Gaa channel type
*/
void GattGaaCommDisconnectRsp(gatt_gaa_channel_t channel);

/*!
    @brief Disconnect from the Gaa client
*/
void GattGaaCommDisconnect(void);

/*!
    @brief Return client connection identifier
*/
uint16 GattGaaCommServerGetCid(void);

/*!
    @brief Return connection MTU
*/
uint16 GattGaaCommServerGetMtu(void);
/*!
    @brief Return client Link Manager Protocol version
*/
uint8 GattGaaCommServerGetLmpVersion(void);

#endif  /* GATT_GAA_COMM_SERVER_H_ */
