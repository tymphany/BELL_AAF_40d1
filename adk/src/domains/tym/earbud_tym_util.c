/*!
\copyright  Copyright (c) 2017 - 2019 TYMPHANY Technologies International, Ltd.
            All Rights Reserved.
            TYMPHANY Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       earbud_tym_util.c
\brief      Support for the util function
*/
/* ------------------------ Include ------------------------ */
#include <panic.h>
#include <pio.h>
#include <pio_common.h>
#include <logging.h>
#include "ps.h"      // for usb serial
#include "kymera_private.h" //for Switching PresetEQ
#include <stdio.h>
#include "earbud_tym_psid.h"
#include "earbud_tym_util.h"
#include "earbud_tym_acoustic_data.h"
#include "earbud_tym_sync.h"
#include "tym_anc.h"
/* ------------------------ Defines ------------------------ */
#define FACTORY_SAMPLE_RATE         44100

/* Create defines for volume manipulation.
 * The passthrough gain is the log2 of the required linear gain in Q6N format.
 * Convert a dB gain to Q6N as follows: 2^(32-6) * gain_db / 20log(2)
 * This can be simplified to a scaling of 2^26 / 20log2 = 67108864 / 6.0206
 */
#define GAIN_DB_TO_Q6N_SF (11146541)
#define GAIN_DB(x)      ((int32)(GAIN_DB_TO_Q6N_SF * (x)))

#define INITIAL_GAIN    GAIN_DB(10)
#define OPMSG_COMMON_ID_SET_PARAMS  0x2005
#define CAP_ID_BASIC_PASS           0x0001
/* ------------------------ Types -------------------------- */
/* --------------- Local Function Prototypes --------------- */
/* --------------- Static Global Variables ----------------- */
/* ------------------ Local Function ----------------------- */
tymDebugConfigData_s gTymDebug_configData = {0};
/* ----------------- Global Function ----------------------- */
/*! \brief check timeout */
bool checktimeout(uint32 oldtime,uint32 currtime,uint32 checktime)
{
    bool ret = FALSE;
    uint32 offsetime;
    if(currtime >= oldtime)  
    {
        offsetime = (currtime - oldtime);
    }
    else
    {
        offsetime = (0xFFFFFFFFu - oldtime) + currtime;
    }          
    if(offsetime > checktime)
        ret = TRUE;
      
    return ret;               
}

/*! \brief setup PIO interrupt and input */
void setupPIOInputAndInterrupt(uint8 pio)
{
    uint16 bank;
    uint32 mask;
        
    bank = PIO2BANK(pio);
    mask = PIO2MASK(pio);
    PanicNotZero(PioSetMapPins32Bank(bank, mask, mask));
    PanicNotZero(PioSetDir32Bank(bank, mask, 0));
    PanicNotZero(PioSet32Bank(bank, mask, mask));    
            
}

/*! \brief setup PIO disable interrupt */
void setupPIODisableInterrupt(uint8 pio)
{
    uint16 bank;
    uint32 mask;  
        
    bank = PIO2BANK(pio);
    mask = PIO2MASK(pio);
    PanicNotZero(PioSet32Bank(bank, mask, 0));            
}


/*! \brief control output hi/low */
void pioDriverPio(uint8 pin,bool enable)
{
    if(enable)
        PioCommonSetPio(pin, pio_drive, TRUE);
    else
        PioCommonSetPio(pin, pio_drive, FALSE);
}

/*! \brief set debug mode bit */
void setPSDebugModeBit(uint8 bit,uint8 enable)
{
    uint16 retwords;
    int i;
    uint8 shiftvale = 0;
	retwords = PsRetrieve(PSID_DEBUG,0,0);
    if(retwords != 0)
    {	
        retwords = PsRetrieve(PSID_DEBUG,&gTymDebug_configData,1);
    }
    for(i = 0;i < PSDEBUG_MAX;i++)
    {
        shiftvale = (1 << i);
        if(shiftvale == bit)
        {
            if(enable == 1)
                gTymDebug_configData.config |= shiftvale;
            else    
                gTymDebug_configData.config &= ~shiftvale;     
        }    
    }
    
    PsStore(PSID_DEBUG,&gTymDebug_configData,1);
}

/*! \brief get debug mode bit */
uint8 getPSDebugMode(void)
{
    uint16 retwords;
	uint8 retval = 0;
	retwords = PsRetrieve(PSID_DEBUG,0,0);
    if(retwords != 0)
    {	
        retwords = PsRetrieve(PSID_DEBUG,&gTymDebug_configData,1);
        retval = gTymDebug_configData.config;
    }
  	return retval;
}

void setPSPresetEQ(void)
{
    uint16 cur_eq;
    cur_eq = get_cur_preset_eq();

    DEBUG_LOG("setPSPresetEQ %x\n",cur_eq);
    PsStore(PSID_PRESET_EQ,&cur_eq,1);
}

void getPSPresetEQ(void)
{
    uint16 retwords;
    uint16 cur_eq = 0;

    retwords = PsRetrieve(PSID_PRESET_EQ,0,0);
    if(retwords != 0)
    {
        retwords = PsRetrieve(PSID_PRESET_EQ,&cur_eq,1);
    }

    DEBUG_LOG("getPSPresetEQ %x \n",cur_eq);
    set_cur_preset_eq((uint8) cur_eq);
}

void UpdateAudioPSKey(void)
{
    #if ENABLE_UPDATE_AUDIO_PS_KEY
    uint8 ret,key_index;

    key_index = sizeof(tymAco_AudioKeyList)/sizeof(tymAco_audioKey_s);
    for(int i =0;i < key_index ; i++){
        ret = PsUpdateAudioKey(tymAco_AudioKeyList[i].key, tymAco_AudioKeyList[i].data,tymAco_AudioKeyList[i].size,0,tymAco_AudioKeyList[i].size);
        if(!ret)
            DEBUG_LOG("write audio pskey error index %x psid %x \n",i,tymAco_AudioKeyList[i].key);
    }
    #endif
}

void configTYMRestoreDefault(void)
{
    /*clear PSKey*/
    PsStore(PSID_BTNAME, 0, 0);
    PsStore(PSID_APPCONFIG, 0, 0);
    PsStore(PSID_ANC_LEVEL, 0, 0);
}

void storeAppConfigData(void)
{
    tym_sync_app_configuration_t *app_set = TymGet_AppSetting();
    int pslen = PS_SIZE_ADJ(sizeof(tym_sync_app_configuration_t));
    uint16 appUiData[pslen]; 
    memcpy(appUiData, app_set, sizeof(tym_sync_app_configuration_t));  
    PsStore(PSID_APPCONFIG, appUiData, PS_SIZE_ADJ(sizeof(tym_sync_app_configuration_t))); 
}

void retrieveAppConfigData(void)
{
    tym_sync_app_configuration_t *app_set = TymGet_AppSetting();
    int pslen = PS_SIZE_ADJ(sizeof(tym_sync_app_configuration_t));
    uint16 appUiData[pslen];  
    if(PsRetrieve(PSID_APPCONFIG, 0, 0) != 0)
    {    
        PsRetrieve(PSID_APPCONFIG, appUiData, PS_SIZE_ADJ(sizeof(tym_sync_app_configuration_t)));
        memcpy(app_set, appUiData, sizeof(tym_sync_app_configuration_t)); 
    }
}

void storeANCLevelConfigData(void)
{
    tymAncTaskData *tymAnc = TymAncGetTaskData();
    uint16 psdata[1];
    psdata[0] = (tymAnc->speechLevel) | (tymAnc->ambientLevel << 8); 
    PsStore(PSID_ANC_LEVEL, psdata, PS_SIZE_ADJ(sizeof(psdata)));  
}

void retrieveANCLevelConfigData(void)
{
    tymAncTaskData *tymAnc = TymAncGetTaskData();
    uint16 psdata[1];
    if(PsRetrieve(PSID_ANC_LEVEL, 0, 0) != 0)
    {  
        PsRetrieve(PSID_ANC_LEVEL, psdata, PS_SIZE_ADJ(sizeof(psdata)));
        tymAnc->speechLevel = (psdata[0] & 0xff);
        tymAnc->ambientLevel = (psdata[0] >> 8)& 0xff;
    }
    DEBUG_LOG("amb level %d,speech level %d",tymAnc->ambientLevel,tymAnc->speechLevel);    
}
