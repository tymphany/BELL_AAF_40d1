/* ENABLE_TYM_PLATFORM added manufactturer data */
#include "earbud_config.h"

#include <connection.h>
#include <panic.h>
#include <string.h>
#include <stdio.h>
#include <logging.h>
#include "manufacturer_data.h"
#include "bt_device.h"

#define MANUFACTURER_DATA_NUM_ITEMS   (1)

static unsigned int manufacturedata_AdvGetNumberOfItems(const le_adv_data_params_t * params);
static le_adv_data_item_t manufacturedata_AdvertData(const le_adv_data_params_t * params, unsigned int);
static void manufacturedata_ReleaseItems(const le_adv_data_params_t *params);
uint32 encrypt_data_calucate(void);
uint32 hash_fnv1a32_update(uint8 *buf, uint8 len);
le_adv_data_callback_t manufacturedata_AdvCallback ={.GetNumberOfItems = &manufacturedata_AdvGetNumberOfItems,
                                             .GetItem = &manufacturedata_AdvertData,
                                             .ReleaseItems = &manufacturedata_ReleaseItems};
uint8 manufacture_adv_data[MANUFACTURE_DATA_ADV_SIZE];

bool ManufacturerData_Init(Task init_task)
{
    UNUSED(init_task);

    DEBUG_LOG("ManufacturerData_Init");

    LeAdvertisingManager_Register(NULL, &manufacturedata_AdvCallback);

    return TRUE;
}

static unsigned int manufacturedata_AdvGetNumberOfItems(const le_adv_data_params_t * params)
{
    /* Return number of items as 1 for these conditions
       1. TxPower is Mandatory and completeness_full.
       2. TxPower is Optional and completeness_skip 
     */
    DEBUG_LOG("manufacturedata: Completeness=%d,data %d,place %d", params->completeness,params->data_set,params->placement);
    if((params->completeness == le_adv_data_completeness_can_be_skipped)&&
        (params->data_set != le_adv_data_set_peer) && (le_adv_data_placement_advert == params->placement))
    {
        return MANUFACTURER_DATA_NUM_ITEMS;
    }
    else
    {
        return 0;
    }
}

static le_adv_data_item_t manufacturedata_AdvertData(const le_adv_data_params_t * params, unsigned int number)
{
    le_adv_data_item_t adv_data_item={0};
    uint32 encrypt_data;
    DEBUG_LOG("manufacturedata_AdvertData: Completeness=%d, number=%d", params->completeness, number);

    if((params->completeness == le_adv_data_completeness_can_be_skipped)&&
        (params->data_set != le_adv_data_set_peer) && (le_adv_data_placement_advert == params->placement))
    {
        adv_data_item.size = MANUFACTURE_DATA_ADV_SIZE;
        encrypt_data = encrypt_data_calucate();
        // Set the data field in the format of advertising packet format. Adhering to le_advertising_mgr 
        manufacture_adv_data[0] = MANUFACTURE_DATA_ADV_SIZE - 1 ; //model number 64 06 42 47 + hash 4 bytes
        manufacture_adv_data[1] = (uint8)ble_ad_type_manufacturer_specific_data;
        manufacture_adv_data[2] = 0x64;
        manufacture_adv_data[3] = 0x06;
        manufacture_adv_data[4] = (encrypt_data) & 0xff; //(encrypt_data >> 24) & 0xff;        
        manufacture_adv_data[5] = (encrypt_data >> 8) & 0xff;//(encrypt_data >> 16) & 0xff; 
        manufacture_adv_data[6] = (encrypt_data >> 16) & 0xff; //(encrypt_data >> 8) & 0xff; 
        manufacture_adv_data[7] = (encrypt_data >> 24) & 0xff;  //(encrypt_data) & 0xff; 
        manufacture_adv_data[8] = 0xC1; //G: 0x47 - 0x41: b 00110, B:0x42 - 0x41: b00001,big endian b00000000 00000000 00000000 0011000001 
        manufacture_adv_data[9] = 0x00; 
        manufacture_adv_data[10] = 0x00;        
        
        adv_data_item.data = manufacture_adv_data;
    }
    else
    {
        adv_data_item.size = 0;
        adv_data_item.data = NULL;
    }

    return adv_data_item;
}


static void manufacturedata_ReleaseItems(const le_adv_data_params_t * params)
{
    UNUSED(params);
}

uint32 encrypt_data_calucate(void)
{
    //0x00ff0A,0x5b,0x0002 mac address> + "BG" + "RHA"
    //00 02 5b 00 ff 0a 42 48 52 48 41
    uint8  buf[11];
    uint8  len = 11;
    bdaddr bt_addr = {0};
    uint32 result = 0;
    if(appDeviceGetPrimaryBdAddr(&bt_addr) == FALSE)
    {
        appDeviceGetMyBdAddr(&bt_addr);
    }    
    
//    bt_addr.nap
//    bt_addr.uap
//    bt_addr.lap
//    printf("----------nap : %x uap: %x  lap: %x \n \n",bt_addr.nap,bt_addr.uap,bt_addr.lap);


    buf[0] = (uint8)((bt_addr.nap >> 8) & 0xFF);
    buf[1] = (uint8)(bt_addr.nap & 0xFF);
    buf[2] = (uint8)(bt_addr.uap);
    buf[3] = (uint8)((bt_addr.lap >>16)  & 0xFF);
    buf[4] = (uint8)((bt_addr.lap >>8) & 0xFF);
    buf[5] = (uint8)(bt_addr.lap & 0xFF);
    /*BG + RHA*/
    buf[6]=0x42; /*B*/
    buf[7]=0x47; /*G*/
    buf[8]=0x52; /*R*/
    buf[9]=0x48; /*H*/
    buf[10]=0x41;/*A*/

    result = hash_fnv1a32_update(buf,len);
    DEBUG_LOG("encrypt_data 0x%x",result);
    return result;
}


uint32 hash_fnv1a32_update(uint8 *buf, uint8 len)
{
     uint8 *p = buf;
     uint32 hash = 0x811c9dc5UL;
     while (len--) {
         hash ^= (uint32)*p++;
         hash += (hash << 1) + (hash << 4) + (hash << 7) +
             (hash << 8) + (hash << 24);
     }
     return hash;
}







