/*!
\copyright  Copyright (c) 2017 - 2019 TYMPHANY Technologies International, Ltd.
            All Rights Reserved.
            TYMPHANY Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       earbud_tym_factory.c
\brief      Support for the factory command in USB HID port
*/
/* ------------------------ Include ------------------------ */
#include <panic.h>
#include <pio.h>
#include <usb_device_class.h>
#include <usb_hub.h>
#include <stdlib.h>
#include <stdio.h>
#include <byte_utils.h>
#include <vmal.h> //for vmal operator
#include <audio_voice_common.h> //for cvc mic
#include <logging.h>
#include "earbud_tym_factory.h"
#include "connection_manager.h" //set can connect for pairing
#include "pairing.h" // for discovery
#include "upgrade.h" // for firmware version
#include "ps.h"      // for usb serial
#include "earbud_tym_util.h"
#include "earbud_sm.h"
#include "power_manager.h"
#include "battery_monitor.h"
#include "tws_topology_rule_events.h"//for TWSTOP_RULE_EVENT_NO_PEER
#include "tws_topology_private.h" //for twsTopolgy_RulesSetEvent
#include "earbud_tym_psid.h" //for ps key id
#include "state_proxy.h" //for bt status
#include "earbud_tym_eq.h" //for tym eq version
#include "earbud_config.h" //for get left/right
#include "tym_anc.h" //for stanc3_on stanc3_off
#include "charger_monitor.h" 
#include "proximity.h"
#include "earbud_tym_sensor.h"
#include "tym_touch.h"
#include "audio_sources.h"
#include "kymera.h"
#include "kymera_adaptation.h"
#include "volume_messages.h"
#include "peer_signalling.h"
#include "kymera_private.h"
#include "earbud_test.h"
#include "earbud_tym_cc_communication.h"
#include "multidevice.h"
/* ------------------------ Defines ------------------------ */

#define xprint(x)            DEBUG_LOG(x)
#define xprintf(x, ...)      DEBUG_LOG(x,  __VA_ARGS__)

/*must the same as usb_device_hid.c */
#define HID_REPORT_ID         0x01
#define HID_REPORT_OUTPUT_LEN 0x20
#define HID_REPORT_INPUT_LEN  0x20

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
#define PSKEY_SN_STRING             0x02C3
#define PSKEY_BDADDR                0x0001
#define LAP_MSW_OFFSET 0
#define LAP_LSW_OFFSET 1
#define UAP_OFFSET 2
#define NAP_OFFSET 3
/* ------------------------ Types -------------------------- */
typedef struct _tymFactory_ {
    const char factoryCmd[4];
    void (*funcptr)(void *);
} tymFactory_s;

typedef struct _tymFactoryVar_{
	uint16  vthmvolt;
	uint8   touchEvent;
}tymFactoryVar_s; 
/* --------------- Local Function Prototypes --------------- */
void dmicstart(audio_channel chan1,audio_channel chan2);
void dmicend(audio_channel chan1,audio_channel chan2);

void tymSendDatatoHost(uint8 *data,int len);

void factoryOn(void *);
void factoryOff(void *);
void factoryTopOn(void *);
void factory_poweron(void *);
void factory_poweroff(void *);
void factory_discovery(void *);
void factory_restore(void *);
void factory_getsn(void *);
void factory_fwversion(void *);
void factory_eqversion(void *);
void factory_macaddr(void *);
void factory_btstatus(void *);
void factory_ntc(void *);
void factory_battpercent(void *);
void factory_touchevt(void *);
void factory_touchhold(void *);
void factory_proxthr(void *);
void factory_irevt(void *);
void factory_readir(void *);
void factory_calhiir(void *);
void factory_callowir(void *);
void factory_dmic1start(void *);
void factory_dmic1end(void *);
void factory_dmic2start(void *);
void factory_dmic2end(void *);
void factory_anc(void *);
void factory_ancvol(void *);
void factory_powercharge(void *);
void factory_cvcmic(void *);
void factory_reboot(void *);
void factory_setvolume(void *);
void factory_getvolume(void *);
void factory_dutmode(void *);
void factory_reboot(void *);
void factory_twspair(void *);
void factory_twsrole(void *);
void factory_checktwsconnect(void *);
void factory_readthrir(void *);
void factory_writeir(void *);
void factory_recoverycommport(void *);
void factory_anctuning(void *);
void factory_iroffset(void *);
void factory_restoreall(void *);
void factory_mutecontrol(void *);
void factory_stanc3mutecontrol(void *);
void factory_switching_eq_preset(void *);
void factory_get_eq_preset(void *);
static void configrationInitConnect(void);
/* --------------- Static Global Variables ----------------- */
/* The operators */
static Operator passthrough;
uint8 send_data_to_host[HID_REPORT_INPUT_LEN];
uint8 gFactoryModeSelect = factory_disable;
tymFactoryVar_s tymFactoryVar = {
    .vthmvolt   = 0,
    .touchEvent = noact,
};


const tymFactory_s tymFactoryCmdList[] = {
  { "001", factoryOn},
  { "002", factoryTopOn},
  { "003", factoryOff},
  { "004", factory_poweron},
  { "005", factory_poweroff},  
  { "006", factory_discovery},
  { "007", factory_restore},
  { "008", factory_getsn},
  { "009", factory_fwversion},
  { "010", factory_eqversion},      
  { "011", factory_macaddr},   
  { "012", factory_btstatus},  
  { "013", factory_ntc}, 
  { "014", factory_battpercent}, 
  { "015", factory_touchevt},
  { "016", factory_touchhold},
  { "017", factory_proxthr},
  { "018", factory_irevt},
  { "019", factory_readir},
  { "020", factory_calhiir},
  { "021", factory_callowir},   
  { "022", factory_dmic1start},
  { "023", factory_dmic1end},
  { "024", factory_dmic2start},
  { "025", factory_dmic2end},  
  { "031", factory_anc}, 
  { "032", factory_ancvol},
  { "037", factory_powercharge},
  { "038", factory_cvcmic},
  { "039", factory_reboot},
  { "048", factory_setvolume},       // "Set volume"
  { "049", factory_getvolume},       // "Get volume"  
  { "052", factory_dutmode},
   //for TWS
  { "100", factory_twspair},
  { "101", factory_twsrole},  
  { "102", factory_restoreall},    
  { "103", factory_checktwsconnect}, 
  //for backup
  { "200", factory_readthrir},
  { "201", factory_writeir},     
  { "204", factory_recoverycommport},   
  { "205", factory_anctuning},
  { "206", factory_iroffset},
  { "207", factory_mutecontrol},     //mutecontrol
  { "208", factory_stanc3mutecontrol},     //mutecontrol
  { "998", factory_get_eq_preset},         //get_eq_preset
  { "999", factory_switching_eq_preset},   //swithcing_eq_preset
};
/* ------------------ Local Function ----------------------- */
/*! \brief send data to USB HID */
void tymSendDatatoHost(uint8 *data,int len)
{
    memset(send_data_to_host,0x0,sizeof(send_data_to_host));
	memcpy(send_data_to_host,data,len);
    UsbDeviceClassSendReport(USB_DEVICE_CLASS_TYPE_HID_CONSUMER_TRANSPORT_CONTROL,HID_REPORT_ID,HID_REPORT_INPUT_LEN,send_data_to_host);
}

static void configrationInitConnect(void)
{
    ConManagerAllowConnection(cm_transport_bredr, TRUE);
    ConManagerAllowConnection(cm_transport_ble, FALSE);
    ConManagerAllowHandsetConnect(TRUE);    
}

/*! \brief factory On */
void factoryOn(void *dataptr)
{
    UNUSED(dataptr);    
    xprint("factoryOn");
    gFactoryModeSelect = factory_enable;
    Volume_MuteRequest(FALSE);
    configrationInitConnect();
    appUserPowerOn();    
    tymSendDatatoHost((uint8 *)"1", sizeof("1"));       
}

/*! \brief factory top on */
void factoryTopOn(void *dataptr)
{
    UNUSED(dataptr);    
    xprint("topFactoryOn");   
    gFactoryModeSelect = factory_enable_top; 
    Volume_MuteRequest(FALSE);
    configrationInitConnect();    
    appUserPowerOn();
    tymSendDatatoHost((uint8 *)"1", sizeof("1"));    
}

/*! \brief factory Off  */
void factoryOff(void *dataptr)
{
    UNUSED(dataptr);    
    xprint("factoryOff");      
    //gFactoryModeSelect = factory_disable; call function will disable
    appUserPowerOffRequest();
    Volume_MuteRequest(TRUE);
    tymSendDatatoHost((uint8 *)"1", sizeof("1"));    
}

/*! \brief normal poweron  */
void factory_poweron(void *dataptr)
{
    UNUSED(dataptr);
    appUserPowerOn();
    tymSendDatatoHost((uint8 *)"1", sizeof("1"));    
}

/*! \brief normal power off  */
void factory_poweroff(void *dataptr)
{
    UNUSED(dataptr);
    appUserPowerOffRequest();
    tymSendDatatoHost((uint8 *)"1", sizeof("1"));    
}

/*! \brief  factory signal peer device enter pairing*/
void factory_discovery(void *dataptr)
{
    UNUSED(dataptr);
    xprint("factory_discovery");
    Pairing_Pair(NULL,0);
    //appSmEnterPairing();
    tymSendDatatoHost((uint8 *)"1", sizeof("1"));
}

/*! \brief factory restore default clear mobile pairing list*/
void factory_restore(void *dataptr)
{
   UNUSED(dataptr);
   tymSendDatatoHost((uint8 *)"1", sizeof("1"));
   appSmSetState(APP_STATE_IN_CASE_IDLE);
   earbudCC_RecoveryCommPort();
   appSmDeleteHandsets(); 
}

/*! \brief  factory get serial number */
void factory_getsn(void* dataptr)
{
    UNUSED(dataptr);
    uint8 str_len=0;
    uint8* buffer;
    uint8 snstr[32];
    xprint("factory_getsn"); 
    str_len = PsFullRetrieve(PSKEY_SN_STRING, NULL, 0);
    if(str_len)
    {
        buffer = malloc(str_len*2+1);
        PsFullRetrieve(PSKEY_SN_STRING, buffer, str_len);
        buffer[str_len*2] = 0;
        xprintf("sn %s\n",buffer);
        memcpy(snstr,buffer, str_len*2+1);
        snprintf((char *)snstr,sizeof(snstr),"%s",snstr);
        if(buffer != NULL)
            free(buffer);

        tymSendDatatoHost(snstr,sizeof(snstr));
    }
    else
    {
        tymSendDatatoHost((uint8 *)"Err_01",sizeof("Err_01"));
    }
}

/*! \brief  factory get firmware version*/
void factory_fwversion(void *dataptr)
{
    UNUSED(dataptr);
    uint16 major,minor;
    uint8  verstr[32];    
    xprint("factory_fwversion");    
    UpgradeGetVersion(&major, &minor, NULL);
    memset(verstr,0x0,sizeof(verstr));
    snprintf((char *)verstr,sizeof(verstr),"%d.%d.%d.%d",(major >> 8),(major & 0xff),(minor >> 8),(minor & 0xff));
    tymSendDatatoHost(verstr,sizeof(verstr));
}

/*! \brief  factory get eq version */
void factory_eqversion(void *dataptr)
{
    UNUSED(dataptr);
    uint8  verstr[32];
    memset(verstr,0x0,sizeof(verstr));
    snprintf((char *)verstr,sizeof(verstr),"%s",EQ_VER);
    tymSendDatatoHost(verstr,sizeof(verstr));
}

/*! \brief  factory get mac address */
void factory_macaddr(void *dataptr)
{
    UNUSED(dataptr);
    bdaddr local_bd_addr;
    uint16* bd_addr_data = (uint16*)PanicUnlessNew(bdaddr);
    uint16 size = PS_SIZE_ADJ(sizeof(local_bd_addr));
    char mac_addr[32];

    xprint("factory_macaddr");   
    /*if(appDeviceGetMyBdAddr(&self))*/
    BdaddrSetZero(&local_bd_addr);
    if(size == PsFullRetrieve(PSKEY_BDADDR, bd_addr_data, size))
    {
        local_bd_addr.nap = bd_addr_data[NAP_OFFSET];
        local_bd_addr.uap = bd_addr_data[UAP_OFFSET];
        local_bd_addr.lap = MAKELONG(bd_addr_data[LAP_LSW_OFFSET], bd_addr_data[LAP_MSW_OFFSET]);

        xprintf("BD ADDR %04x%02x%06lx",local_bd_addr.nap,local_bd_addr.uap, local_bd_addr.lap);
        snprintf((char *)mac_addr,sizeof(mac_addr),"%04x%02x%06lx",local_bd_addr.nap,local_bd_addr.uap, local_bd_addr.lap);

        tymSendDatatoHost((uint8 *)mac_addr,strlen(mac_addr));
    }
    else
    {
        tymSendDatatoHost((uint8 *)"Err_01",sizeof("Err_01"));
    }
    free(bd_addr_data);
}

/*! \brief  factory get bt status */
void factory_btstatus(void *dataptr)
{
    UNUSED(dataptr);
    if(StateProxy_IsPairing())
    {
        tymSendDatatoHost((uint8 *)"Pairing",sizeof("Pairing"));           
    }
    else if(StateProxy_IsHandsetA2dpConnected())
    {
        tymSendDatatoHost((uint8 *)"Connected",sizeof("Connected"));                 
    }
    else
    {
        tymSendDatatoHost((uint8 *)"Err_01",sizeof("Err_01"));        
    }              
}

/*! \brief  factory get ntc */
void factory_ntc(void *dataptr)
{
    UNUSED(dataptr);
    char  voltstr[32];
    xprint("factory_ntc");
    memset(voltstr,0x0,sizeof(voltstr));
    snprintf((char *)voltstr,sizeof(voltstr),"%d mv",tymFactoryVar.vthmvolt);
    tymSendDatatoHost((uint8 *)voltstr,strlen(voltstr));
}

/*! \brief  factory get battery percent */
void factory_battpercent(void *dataptr)
{
    UNUSED(dataptr);
    char outstr[32];
    uint8 batt_percent;
    batt_percent = appBatteryGetPercent();
    xprintf("percent %d",batt_percent);
    memset(outstr,0x0,sizeof(outstr));
    snprintf((char *)outstr,sizeof(outstr),"battery %d%%",batt_percent);
    tymSendDatatoHost((uint8 *)outstr,strlen(outstr));
}

/*! \brief  factory get touch event, tap,swipL,swipR */
void factory_touchevt(void *dataptr)
{
    UNUSED(dataptr);
    if(tymFactoryVar.touchEvent == noact)
        tymSendDatatoHost((uint8 *)"noAct",sizeof("noAct"));        
    else if(tymFactoryVar.touchEvent == tap1)
        tymSendDatatoHost((uint8 *)"tap",sizeof("tap"));        
    else if(tymFactoryVar.touchEvent == swipL)
        tymSendDatatoHost((uint8 *)"swipL",sizeof("swipL"));          
    else if(tymFactoryVar.touchEvent == swipR)
        tymSendDatatoHost((uint8 *)"swipR",sizeof("swipR"));                              
    
}

/*! \brief  factory get hold status */
void factory_touchhold(void *dataptr)
{
    UNUSED(dataptr);
    bool level;
    level = getFactoryTouchHoldLevel();
    if(level == TRUE)
        tymSendDatatoHost((uint8 *)"Hold Release",sizeof("Hold Release"));   
    else
        tymSendDatatoHost((uint8 *)"Hold Press",sizeof("Hold Press"));                          
}

/*! \brief  factory get prox threshold value*/
void factory_proxthr(void *dataptr)
{
    UNUSED(dataptr);
    char outstr[32];
    uint8 thr;
    thr = getFactoryTouchCh1threshold();
    memset(outstr,0x0,sizeof(outstr));
	snprintf((char *)outstr,sizeof(outstr),"prox thr %d",thr);
	tymSendDatatoHost((uint8 *)outstr,strlen(outstr));    
}

/*! \brief  factory get IR event*/
void factory_irevt(void *dataptr)
{
    UNUSED(dataptr);
    if(appProximityStatus() == notwear)
        tymSendDatatoHost((uint8 *)"0", sizeof("0"));
    else
        tymSendDatatoHost((uint8 *)"1", sizeof("1"));     
}

/*! \brief  factory read IR PS count value*/
void factory_readir(void *dataptr)
{
    UNUSED(dataptr);
	uint16 read;
    char outd[32];
    read= appProximityDataRead();
	memset(outd,0x0,sizeof(outd));    
    snprintf(outd,sizeof(outd),"IR read:%d",read);

    tymSendDatatoHost((uint8 *)outd,sizeof(outd));
}

/*! \brief  factory calibration hi value*/
void factory_calhiir(void *dataptr)
{
    UNUSED(dataptr);
    uint16 hithr,realhi;
    int irps;
    uint16 thr[3];
    char outd[32];

    realhi = appProximityDataRead();
    hithr = (realhi*0.8);
	memset(outd,0x0,sizeof(outd));
	snprintf(outd,sizeof(outd),"c:%d,w:%d",realhi,hithr);
    irps = PsRetrieve(PSID_IRTH,thr,3);
    if(irps == 3)
	{
		thr[0] = hithr;
		PsStore(PSID_IRTH,thr,3);
	}    
	    
    tymSendDatatoHost((uint8 *)outd,strlen(outd));    		    
}

/*! \brief  factory calibration low value*/
void factory_callowir(void *dataptr)
{
    UNUSED(dataptr);
    uint16 lothr,reallo;    
    uint16 thr[3];
    char outd[32];
    int irps;

    reallo= appProximityDataRead();

    lothr = (reallo*1.2);
    memset(outd,0x0,sizeof(outd));
    snprintf(outd,sizeof(outd),"c:%d,w:%d",reallo,lothr);
    irps = PsRetrieve(PSID_IRTH,thr,3);
    if(irps == 3)
   	{
		thr[1] = lothr;
		PsStore(PSID_IRTH,thr,3);
	}
    tymSendDatatoHost((uint8 *)outd,strlen(outd));      
}


/*! \brief  factory dmic1 start*/
void factory_dmic1start(void *dataptr)
{
    UNUSED(dataptr);
    dmicstart(AUDIO_CHANNEL_A,AUDIO_CHANNEL_B);
}

/*! \brief  factory dmic1 end*/
void factory_dmic1end(void *dataptr)
{
    UNUSED(dataptr);
    dmicend(AUDIO_CHANNEL_A,AUDIO_CHANNEL_B);    
}

/*! \brief  factory dmic2 start*/
void factory_dmic2start(void *dataptr)
{
    UNUSED(dataptr);
    dmicstart(AUDIO_CHANNEL_B,AUDIO_CHANNEL_A);    
}

/*! \brief  factory dmic2 end*/
void factory_dmic2end(void *dataptr)
{
    UNUSED(dataptr);
    dmicend(AUDIO_CHANNEL_B,AUDIO_CHANNEL_A);   
}

/*! \brief  factory anc on/off*/
void factory_anc(void *dataptr)
{
    const char delim[2] = " ";
    uint8 par[3];
    int dataindex = 0;
    if(dataptr == NULL)
    {
        tymSendDatatoHost((uint8 *)"Err_00", sizeof("Err_00"));
        return;
    }

    while(dataptr != NULL)
    {
        par[dataindex] = atoi(dataptr);
        dataindex++;
        dataptr = strtok(NULL,delim);
        if(dataindex > 1)
            break;
    }
    if(dataindex > 1)
    {
        tymSendDatatoHost((uint8 *)"Err_00", sizeof("Err_00"));
        return;
    }    
    if(par[0] == 0) //disable
    {
        stanc3_ancoff();
        tymSendDatatoHost((uint8 *)"1", sizeof("1"));
    }    
    else if(par[0] == 1) //enable       
    {    
        stanc3_ancon();
        tymSendDatatoHost((uint8 *)"1", sizeof("1"));        
    }
    else
    {
        tymSendDatatoHost((uint8 *)"Err_00", sizeof("Err_00"));        
    }    
}

/*! \brief  factory anc volume*/
void factory_ancvol(void *dataptr)
{
    UNUSED(dataptr);     
    uint8 vol;
    char volstr[32];
    
    vol = stanc3_ancvol(); 
    memset(volstr,0x0,sizeof(volstr));
	snprintf((char *)volstr,sizeof(volstr),"anc vol %d",vol);
    tymSendDatatoHost((uint8 *)volstr,strlen(volstr));            
}

/*! \brief  factory get power change */
void factory_powercharge(void *dataptr)
{
    const char delim[2] = " ";
    uint8 par[3];
    int dataindex = 0;
    if(dataptr == NULL)
    {
        tymSendDatatoHost((uint8 *)"Err_00", sizeof("Err_00"));
        return;
    }

    while(dataptr != NULL)
    {
        par[dataindex] = atoi(dataptr);
        dataindex++;
        dataptr = strtok(NULL,delim);
        if(dataindex > 1)
            break;
    }
    if(dataindex > 1)
    {
        tymSendDatatoHost((uint8 *)"Err_00", sizeof("Err_00"));
        return;
    }    

    if(par[0] == 0) //disable
    {
        appChargerForceDisable();
        tymSendDatatoHost((uint8 *)"1", sizeof("1"));
    }    
    else if(par[0] == 1) //enable       
    {    
        appChargerRestoreState(); 
        tymSendDatatoHost((uint8 *)"1", sizeof("1"));
    }
    else
    {
        tymSendDatatoHost((uint8 *)"Err_00", sizeof("Err_00"));        
    }    
    
}

/*! \brief  factory set cvc mic */
void factory_cvcmic(void *dataptr)
{
    UNUSED(dataptr); 
    const char delim[2] = " ";
    uint16 par[3];
    int dataindex = 0;
    if(dataptr == NULL)
    {
        tymSendDatatoHost((uint8 *)"Err_00", sizeof("Err_00"));
        return;
    }

    while(dataptr != NULL)
    {
        par[dataindex] = atoi(dataptr);
        dataindex++;
        dataptr = strtok(NULL,delim);
    }

    if(par[0] == 0) //full
    {
        appKymeraChangeCVCMode(2);
        tymSendDatatoHost((uint8 *)"1", sizeof("1"));  
    }
    else if(par[0] == 1)//left
    {
        appKymeraChangeCVCMode(4);
        tymSendDatatoHost((uint8 *)"1", sizeof("1"));  
    }
    else if(par[0] == 2)//right
    {
        appKymeraChangeCVCMode(5);        
        tymSendDatatoHost((uint8 *)"1", sizeof("1"));  
    }
    else
    {
        tymSendDatatoHost((uint8 *)"Err_00", sizeof("Err_00"));
    }
}

/*! \brief  factory set reboot */
void factory_reboot(void *dataptr)
{
    UNUSED(dataptr);  
    tymSendDatatoHost((uint8 *)"1",sizeof("1"));
    appPowerReboot();    
}

/*! \brief  factory set volume */
void factory_setvolume(void *dataptr)
{
    int dataindex = 0;
    const char delim[2] = " ";
    int vol = 0;
    volume_t voldata;
    while(dataptr != NULL)
    {
        vol = atoi(dataptr);
        dataindex++;
        dataptr = strtok(NULL,delim);
    }
    if(dataindex != 1)
    {
        tymSendDatatoHost((uint8 *)"Err_00",sizeof("Err_00"));  
        return;
    }    
    voldata = AudioSources_GetVolume(audio_source_a2dp_1);
    voldata.value = vol;
    AudioSources_SetVolume(audio_source_a2dp_1, voldata);
    AudioSources_OnVolumeChange(audio_source_a2dp_1, event_origin_local, voldata);
    if(audio_source_a2dp_1 == AudioSources_GetRoutedSource())
    {
        volume_parameters_t volume_params = { .source_type = source_type_audio, .volume = voldata };
        KymeraAdaptation_SetVolume(&volume_params);
    }
    
    tymSendDatatoHost((uint8 *)"1",sizeof("1"));
}

/*! \brief  factory get volume */
void factory_getvolume(void *dataptr)
{
    UNUSED(dataptr);
    char volstr[32];
    volume_t vol;
    vol = AudioSources_GetVolume(audio_source_a2dp_1);
    memset(volstr,0x0,sizeof(volstr));
	snprintf((char *)volstr,sizeof(volstr),"a2dp vol %d",vol.value);
    DEBUG_LOG("vol %d",vol.value);    
    tymSendDatatoHost((uint8 *)volstr,strlen(volstr));
    
}

/*! \brief  factory enter dutmode */
void factory_dutmode(void *dataptr)
{
    UNUSED(dataptr);
    ConnectionEnterDutMode();
}

/*! \brief  factory pair tws */
void factory_twspair(void *dataptr)
{
    UNUSED(dataptr);      
    gFactoryModeSelect = factory_disable; //disable factory mode for two earbud pairing
    twsTopology_RulesSetEvent(TWSTOP_RULE_EVENT_NO_PEER); 
    tymSendDatatoHost((uint8 *)"1",sizeof("1"));   
}

/*! \brief  factory pair tws role */
void factory_twsrole(void *dataptr)
{
    bool left = Multidevice_IsLeft();
    bool primary = appSmIsPrimary();
    UNUSED(dataptr);  
    if (primary == TRUE)
    {
        if (left == TRUE)
        {
            tymSendDatatoHost((uint8 *)"M L", sizeof("M L"));
        }
        else
        {
            tymSendDatatoHost((uint8 *)"M R", sizeof("M R"));
        }
    }
    else
    {
        if (left == TRUE)
        {
            tymSendDatatoHost((uint8 *)"S L", sizeof("S L"));
        }
        else
        {
            tymSendDatatoHost((uint8 *)"S R", sizeof("S R"));
        }
    }        
}

/*! \brief  factory check tws  */
void factory_checktwsconnect(void *dataptr)
{
    UNUSED(dataptr);  
    bool paired = FALSE;
    paired = appPeerSigIsConnected();    
    if(paired == TRUE)
    {
        tymSendDatatoHost((uint8 *)"1",sizeof("1"));         
    }
    else
    {
        tymSendDatatoHost((uint8 *)"0",sizeof("0"));           
    }        
   
}

/*! \brief  factory read IR calibration hi/low/off  */
void factory_readthrir(void *dataptr)
{
    UNUSED(dataptr);      
    int irps;
    uint16 IRThresoldData[8];
    char outputd[32];
    irps = PsRetrieve(PSID_IRTH,0,0);
    if(irps != 0)	
    {
    	//read
        PsRetrieve(PSID_IRTH, IRThresoldData, 3);
        memset(outputd,0x0,sizeof(outputd));
    	snprintf(outputd,32,"H:%d L:%d O:%d",IRThresoldData[0],IRThresoldData[1],IRThresoldData[2]);
		tymSendDatatoHost((uint8 *)outputd,strlen(outputd));
    }	
    else
 		tymSendDatatoHost((uint8 *)"Err_00",sizeof("Err_00"));        
}

/*! \brief  factory write IR calibration hi/low/off  */
void factory_writeir(void *dataptr)
{
	const char delim[2] = " ";
	uint16 thr[5];
	uint16 wrthr[5];
	int dataindex = 0,irps;
    while(dataptr != NULL)
    {
        wrthr[dataindex] = atoi(dataptr);
        dataindex++;
        dataptr = strtok(NULL,delim);
    }
    if(dataindex == 2)
    {
        irps = PsRetrieve(PSID_IRTH,thr,3);
    	if(irps == 3)
    	{
    		thr[0] = wrthr[0];
    		thr[1] = wrthr[1];
    		PsStore(PSID_IRTH,thr,3);
        }
        tymSendDatatoHost((uint8 *)"1", sizeof("1"));
    }
    else if(dataindex == 3)
    {
        irps = PsRetrieve(PSID_IRTH,thr,3);
    	if(irps == 3)
    	{
    		thr[0] = wrthr[0];
    		thr[1] = wrthr[1];
            thr[2] = wrthr[2];
    		PsStore(PSID_IRTH,thr,3);
        }
        tymSendDatatoHost((uint8 *)"1", sizeof("1"));
        
    }
    else
    {
    	tymSendDatatoHost((uint8 *)"Err_00",sizeof("Err_00"));
    }		  
}

/*! \brief  factory recovery communication port  */
void factory_recoverycommport(void *dataptr)
{
    UNUSED(dataptr); 
    tymSendDatatoHost((uint8 *)"1",sizeof("1"));
    setFactoryModeStatus(factory_disable);
    earbudCC_RecoveryCommPort();            
}

/*! \brief  factory enter anc tuning  */
void factory_anctuning(void *dataptr)
{
    UNUSED(dataptr);     
    tymSendDatatoHost((uint8 *)"NOT IMPLEMENT",sizeof("NOT IMPLEMENT"));    
}

/*! \brief  factory write ir offset  */
void factory_iroffset(void *dataptr)
{
    UNUSED(dataptr);     
    const char delim[2] = " ";
	uint16 thr[5];
	uint16 wrthr[5];
	int dataindex = 0,irps;
    while(dataptr != NULL)
    {
        wrthr[dataindex] = atoi(dataptr);
        dataindex++;
        dataptr = strtok(NULL,delim);
    }
    if(dataindex == 1)
    {
        irps = PsRetrieve(PSID_IRTH,thr,3);
    	if(irps == 3)
    	{
            thr[2] = wrthr[0];
    		PsStore(PSID_IRTH,thr,3);
        }
        tymSendDatatoHost((uint8 *)"1", sizeof("1"));        
    }
    else
    {
        tymSendDatatoHost((uint8 *)"Err_00", sizeof("Err_00"));
    }                     
}

/*! \brief  factory restore all  */
void factory_restoreall(void *dataptr)
{
    UNUSED(dataptr);
    tymSendDatatoHost((uint8 *)"1",sizeof("1"));
    /*clean battery predict PSKEY*/
    PsStore(PSID_BATT, NULL, 0);
    appSmFactoryReset();
}

/*! \brief for incase force unmute */
void factory_mutecontrol(void *dataptr)
{    
    const char delim[2] = " ";
    int dataindex = 0;
    int mutev[6];
    while(dataptr != NULL)
    {
        mutev[dataindex] = atoi(dataptr);
        dataindex++;
        dataptr = strtok(NULL,delim);
    }
    if(dataindex == 1)
    {
        if(mutev[0] == 1)
        {
            Volume_MuteRequest(TRUE);
        }
        else
        {
            Volume_MuteRequest(FALSE);
        }
        tymSendDatatoHost((uint8 *)"1",sizeof("1"));        
    }
    else
    {
        tymSendDatatoHost((uint8 *)"Err_00",sizeof("Err_00"));
    }            
}

/*! \brief for stanc3 anc mute/unmute */
void factory_stanc3mutecontrol(void *dataptr)
{
    const char delim[2] = " ";
    int dataindex = 0;
    int mutev[6];
    while(dataptr != NULL)
    {
        mutev[dataindex] = atoi(dataptr);
        dataindex++;
        dataptr = strtok(NULL,delim);
    }
    if(dataindex == 1)
    {
        if(mutev[0] == 1)
        {
            stanc3_audiomute(TRUE);
        }
        else
        {
            stanc3_audiomute(FALSE);
        }
        tymSendDatatoHost((uint8 *)"1",sizeof("1"));
    }
    else
    {
        tymSendDatatoHost((uint8 *)"Err_00",sizeof("Err_00"));
    }
}

/*! \brief  configuration microphone output channel link */
void dmicstart(audio_channel chan1,audio_channel chan2)
{
    Source src_a,src_b;
    Sink   snk_a,snk_b;
    uint16 set_gain[] = { OPMSG_COMMON_ID_SET_PARAMS, 1, 1, 1,
                          UINT32_MSW(INITIAL_GAIN), UINT32_LSW(INITIAL_GAIN) };
    OperatorFrameworkEnable(MAIN_PROCESSOR_ON);
	/*source:Digital mic, snk:speaker output*/
    src_a = StreamAudioSource(AUDIO_HARDWARE_DIGITAL_MIC, AUDIO_INSTANCE_0, chan1);
    PanicFalse(SourceConfigure(src_a, STREAM_DIGITAL_MIC_INPUT_RATE, FACTORY_SAMPLE_RATE));
    PanicFalse(SourceConfigure(src_a, STREAM_DIGITAL_MIC_INPUT_GAIN, 0x0007));
    PanicFalse(SourceConfigure(src_a, STREAM_DIGITAL_MIC_CLOCK_RATE, 2000));
    PanicFalse(SourceConfigure(src_a, STREAM_DIGITAL_MIC_SIDETONE_SOURCE_POINT, TRUE));
    PanicFalse(SourceConfigure(src_a, STREAM_AUDIO_SAMPLE_SIZE, 16));
    PanicFalse(SourceConfigure(src_a, STREAM_DIGITAL_MIC_INDIVIDUAL_SIDETONE_GAIN, 0));

    src_b = StreamAudioSource(AUDIO_HARDWARE_DIGITAL_MIC, AUDIO_INSTANCE_0, chan2);
    PanicFalse(SourceConfigure(src_b, STREAM_DIGITAL_MIC_INPUT_RATE, FACTORY_SAMPLE_RATE));
    PanicFalse(SourceConfigure(src_b, STREAM_DIGITAL_MIC_INPUT_GAIN, 0x0007));
    PanicFalse(SourceConfigure(src_b, STREAM_DIGITAL_MIC_CLOCK_RATE, 2000));
    PanicFalse(SourceConfigure(src_b, STREAM_DIGITAL_MIC_SIDETONE_SOURCE_POINT, TRUE));
    PanicFalse(SourceConfigure(src_b, STREAM_AUDIO_SAMPLE_SIZE, 16));
    PanicFalse(SourceConfigure(src_b, STREAM_DIGITAL_MIC_INDIVIDUAL_SIDETONE_GAIN, 0));

	/* sink is codec speaker output */
	//left channel 
    snk_a = StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A);
    
	PanicFalse(SinkConfigure(snk_a, STREAM_CODEC_OUTPUT_RATE, FACTORY_SAMPLE_RATE));
	PanicFalse(SinkConfigure(snk_a, STREAM_AUDIO_SAMPLE_SIZE, 16));	
    /*channel B is none */
    // right channel 
    snk_b = StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_B);
	 /* Now create the operator for routing the audio */
    passthrough = PanicZero(VmalOperatorCreate(CAP_ID_BASIC_PASS));
    PanicZero(VmalOperatorMessage(passthrough, set_gain, 6, NULL, 0));

	PanicNull(StreamConnect(src_a,
                                StreamSinkFromOperatorTerminal(passthrough, 0)));
    /* ...and passthrough to line out */
    PanicNull(StreamConnect(StreamSourceFromOperatorTerminal(passthrough,0),
                                snk_a));

    PanicNull(StreamConnect(src_b,snk_b));

    PanicFalse(OperatorStartMultiple(1,&passthrough,NULL));
    tymSendDatatoHost((uint8 *)"1", sizeof("1"));
}

/*! \brief  configuration microphone output channel link disconnect */
void dmicend(audio_channel chan1,audio_channel chan2)
{
	Source src_a,src_b;
    Sink   snk_a,snk_b;
	
	PanicFalse(OperatorStopMultiple(1,&passthrough,NULL));
	/*source:Digital mic, snk:speaker output*/
    src_a = StreamAudioSource(AUDIO_HARDWARE_DIGITAL_MIC, AUDIO_INSTANCE_0, chan1);
    src_b = StreamAudioSource(AUDIO_HARDWARE_DIGITAL_MIC, AUDIO_INSTANCE_0, chan2);
	snk_a = StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_A);
	/*channel B is none */
    snk_b = StreamAudioSink(AUDIO_HARDWARE_CODEC, AUDIO_INSTANCE_0, AUDIO_CHANNEL_B); 
    
    StreamDisconnect(src_a,StreamSinkFromOperatorTerminal(passthrough, 0));   		
    StreamDisconnect(StreamSourceFromOperatorTerminal(passthrough, 0), snk_a);
	StreamDisconnect(src_b,snk_b);
	
	OperatorFrameworkEnable(MAIN_PROCESSOR_OFF);
	tymSendDatatoHost((uint8 *)"1", sizeof("1"));
}

void factory_switching_eq_preset(void *dataptr)
{
    const char delim[2] = " ";
    int dataindex = 0;
    int preset[6];

    while(dataptr != NULL)
    {
        preset[dataindex] = atoi(dataptr);
        dataindex++;
        dataptr = strtok(NULL,delim);
    }

    if(dataindex == 1)
    {
        printf("Switching EQ prset %x\n",preset[0]);
        appKymeraSelectUsrEQPreset(preset[0]);
        appTestSwitchPresetEQ(preset[0]);
        tymSendDatatoHost((uint8 *)"1",sizeof("1"));
    }
    else
    {
        tymSendDatatoHost((uint8 *)"Err_00",sizeof("Err_00"));
    }
}

void factory_get_eq_preset(void *dataptr)
{
    UNUSED(dataptr);
    uint8 eq = get_cur_preset_eq();
    char data[8] = {0};
    snprintf((char *)data,sizeof(data),"%d",eq);

    tymSendDatatoHost((uint8 *)data,sizeof(data));
}

/* ----------------- Global Function ----------------------- */
/*! \brief get factory mode status */
bool getFactoryModeEnable(void)
{
    if(gFactoryModeSelect & factory_enable)
        return TRUE;
    else
        return FALSE;    
}

/*! \brief get factory mode boot top board status */
bool getFactoryModeTopEnable(void)
{
    if(gFactoryModeSelect & factory_top)
        return TRUE;
    else
        return FALSE;       
}

/*! \brief set factory mode status */
void setFactoryModeStatus(uint8 mode)
{
    gFactoryModeSelect = mode;    
}

/*! \brief receive data from USB HID */
void tymReceivedDataFromHost(uint8 *recv_ptr)
{
    char *func_ptr;
    char *param_ptr;
    const char delim[2] = " ";
    int num;
    int totallist = sizeof(tymFactoryCmdList)/ sizeof(tymFactory_s);
    uint8 err[7] = "Err_00";
         
    func_ptr = strtok((char *)recv_ptr,delim);
    param_ptr = strtok((char *)NULL,delim);	    	
    	
    for(num = 0;num < totallist;num++)
    {
        if(strcmp((const char *)func_ptr,tymFactoryCmdList[num].factoryCmd) == 0)
        {	
            tymFactoryCmdList[num].funcptr((void *)param_ptr);
            break;
        }
    }
    if(num == totallist)
    {
        xprint("unknown cmd");
        tymSendDatatoHost(err,sizeof(err));  
    }	  	
}

/*! \brief update vthm voltage from thermistor */
void updateFactoryVthmVolt(uint16 vol)
{
    tymFactoryVar.vthmvolt = vol;
}

void updateFactoryTouchEvent(uint8 iqs_events)
{
    uint8 event = 0;
    if(iqs_events & 0x80)
        event = swipR;
    else if(iqs_events & 0x40)
        event = swipL; 
    else if(iqs_events & 0x20)
        event = tap1;                 
    tymFactoryVar.touchEvent = event;
}

