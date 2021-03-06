/*!
\copyright  Copyright (c) 2017 - 2019 TYMPHANY Technologies International, Ltd.
            All Rights Reserved.
            TYMPHANY Technologies International, Ltd. Confidential and Proprietary.
\version    
\file       earbud_tym_cc_communication.c
\brief      Support for commuinication to charging case
*/
/* ------------------------ Include ------------------------ */
#include <message.h>
#include <panic.h>
#include "adk_log.h"
#include "tym_pin_config.h"
#include "earbud_tym_util.h"
#include "phy_state.h"
#include "battery_monitor.h"
#include "charger_monitor.h"
#include "earbud_tym_sync.h"
#include "earbud_tym_cc_communication.h"
#include "tym_touch.h"
#include "ui_prompts.h"
#include "tym_power_control.h"
#include "earbud_tym_factory.h"
#include "state_proxy.h"
#include "gaia.h"
#include "device_upgrade.h"
/* ------------------------ Defines ------------------------ */

#define xprint(x)            DEBUG_LOG(x)
#define xprintf(x, ...)      DEBUG_LOG(x,  __VA_ARGS__)

#define POLLING_BATTERY_TIME 3000 //3 seconds
#define POLLING_BATT_NUM     10

typedef struct processCmd{
    bool   haveCmdExist;
    bool   startstop;
    bool   systemReady;
    bool   otamode;
    uint8  count;
    uint8  finalNum;
    uint8  battpercent;
    uint8  askBattery;
    uint8  btStatus;
    uint8  debugmode;
    uint8  speedup;
}processCmd_s;

typedef struct
{
    uint8 event;
} statusSendCmd_T;


/* ------------------------ Types -------------------------- */

/* --------------- Local Function Prototypes --------------- */
void reportBattStatus(void);
void _sendStatusCmd(uint8 event);
void startEndCommand (bool start, uint8 datanum);
void startCommunicationToChargingCase(void);
void stopCommunicationToChargingCase(void);
static void _statusOutputMessageHandler ( Task pTask, MessageId pId, Message pMessage );
static TaskData _statusReportTask = { _statusOutputMessageHandler };
void statusCommunicationMessage(MessageId pId,statusSendCmd_T *cmdId);
void localDelay(void);
void startPluseSignal(void);
void endPluseSignal(void);
/* --------------- Static Global Variables ----------------- */
processCmd_s  procCmd;
//3 1 + 2 *x ==> 1 period
const uint8 sendCmdDataArray[statusReportCmdMAX] = {
    (1 + 2 * 6 ),       //statusPairingMode     ,6 period , anc report
    (1 + 2 * 16),       //statusConnetcedMode   ,16
    (1 + 2 * 10),       //statusDisconnectMode  ,10
    (1 + 2 * 12),       //statusRestoreMode     ,12
    (1 + 2 * 14),       //statusPowerOff        ,14
    (1 + 2 * 8 ),       //StartOTA              ,8
    (1 + 2 * 18),       //Error                 ,18
    (1 + 2 * 2 ),       //statusBattCap_61      ,2
    (1 + 2 * 4 ),       //statusBattCap_95      ,4
    (1 + 2 * 6 ),       //anc report            ,6 same as pairing
    (1 + 2 * 6 ),       //ACK report            ,6 same as pairing
};    
/* ------------------ Local Function ----------------------- */
/*! \brief delay */
void localDelay(void)
{

}
void startPluseSignal(void)
{
    pioDriverPio(STATUS_PIN,FALSE);
    localDelay();
    pioDriverPio(STATUS_PIN,TRUE);
    localDelay();
}

void endPluseSignal(void)
{
    int i = 0;
    /*keep high level for MCU detect*/
    pioDriverPio(STATUS_PIN,TRUE);
    for(i = 0;i < 200;i++)
        localDelay();
    pioDriverPio(STATUS_PIN,FALSE);
    localDelay();
}
/*! \brief startendcommand */
void startEndCommand (bool start,uint8 datanum)
{          
    if(start == TRUE)
    {
        procCmd.haveCmdExist = TRUE;
        procCmd.count = 0;
        procCmd.finalNum = datanum;
        startPluseSignal();
        MessageSendLater((TaskData *)&_statusReportTask, statusExecuteCmd, 0, 0);
    }
    else
    {            
        endPluseSignal();
        MessageSendLater((TaskData *)&_statusReportTask, statusEndCmd, 0, 500);
    }        
}

/*! \brief handle statusCommunicationMessage */
void statusCommunicationMessage(MessageId pId,statusSendCmd_T *cmdId)
{
    uint8 datanum = 0; 
    if(pId == statusExecuteCmd)
    {
        if(procCmd.count != procCmd.finalNum)
        {
            if(procCmd.count%2)
            {
                pioDriverPio(STATUS_PIN,TRUE); //1 3
                MessageSendLater((TaskData *)&_statusReportTask, statusExecuteCmd, 0, 30);
            }
            else
            {
                pioDriverPio(STATUS_PIN,FALSE);//0 2
                MessageSendLater((TaskData *)&_statusReportTask, statusExecuteCmd, 0, 10);
            }
            procCmd.count++;		
        }
        else
        {
            startEndCommand(0, datanum);
        }
    }
    else if(pId == statusSendCmd)
    {
        if(procCmd.haveCmdExist == FALSE)
        {
#ifdef ENABLE_UART
            procCmd.haveCmdExist = TRUE;
            sendCmdToChargingCase(cmdId->event);
            MessageSendLater((TaskData *)&_statusReportTask, statusEndCmd, 0, 20);

#else
            datanum = sendCmdDataArray[cmdId->event];
            startEndCommand(1, datanum);
#endif
        }
        else
        {                   
            MESSAGE_MAKE(sendCmdId, statusSendCmd_T);
            sendCmdId->event = cmdId->event;
            MessageSendLater((TaskData *)&_statusReportTask, statusSendCmd, sendCmdId, 20);
        }        
    }
    else if(pId == statusEndCmd)
    {
        procCmd.haveCmdExist = FALSE;
    }
    else if(pId == statusAskBattery)
    {
        if (procCmd.askBattery == 0) 
        {
            procCmd.battpercent = appBatteryGetPercent();            
        }
        if(procCmd.otamode)
        {            
            if(procCmd.askBattery == 0)
            {
                DEBUG_LOG("startOTA");    
                _sendStatusCmd(statusOTA);
            }
        }
        else
        {        
            reportBattStatus();
        }
    	procCmd.askBattery = ((procCmd.askBattery + 1) % POLLING_BATT_NUM);
    	if(procCmd.speedup != 0)
    	{
    	    procCmd.speedup++;
    	    if(procCmd.speedup > 2)
    	        procCmd.speedup = 0;
    	    MessageSendLater((TaskData *)&_statusReportTask, statusAskBattery, 0, 500);
    	}
    	else
    	{        
            MessageSendLater((TaskData *)&_statusReportTask, statusAskBattery, 0, POLLING_BATTERY_TIME);
        }
    }
#if 0  /*tmp remove change USB port when leave USB 5v*/
    else if(pId == statusLeaveUSBCheck)    
    {
        if(appChargerIsConnected() == CHARGER_DISCONNECTED)
        {
            if(procCmd.debugmode & PORTBIT)
            {
                procCmd.debugmode = getPSDebugMode();
            }
            else
            {
                setCommunPort(CH_USB);
            }   
        }    
    }
#endif
}



/*! \brief handle output charging case event */
static void _statusOutputMessageHandler ( Task pTask, MessageId pId, Message pMessage )
{    
    UNUSED(pTask);   
    switch(pId)
    {
        case statusExecuteCmd:
        case statusSendCmd: 
        case statusEndCmd:
        case statusAskBattery:
        case statusLeaveUSBCheck:
            statusCommunicationMessage(pId,(statusSendCmd_T *)pMessage);    
            break;
        case statusChangeUART:
            earbudCC_RecoveryCommPort();
            break;    
        case CHARGER_MESSAGE_DETACHED:
            stopCommunicationToChargingCase();
            break;
        case CHARGER_MESSAGE_ATTACHED:
            startCommunicationToChargingCase();            
            break;    
        default:
            break;    
    }    	     
}

/*! \brief reportBattStatus */
void reportBattStatus(void)
{
    if (procCmd.battpercent >= 90) /*>= 90 report 95%*/
    {    
        _sendStatusCmd(statusBattCap_95);          
    }
    else
    {
        _sendStatusCmd(statusBattCap_61);          
    }
}

void _sendStatusCmd(uint8 event)
{
    if((appChargerIsConnected() != CHARGER_DISCONNECTED) && (procCmd.systemReady == TRUE))
    {
        MESSAGE_MAKE(sendCmdId, statusSendCmd_T);
        sendCmdId->event = event;
        MessageSend((TaskData *)&_statusReportTask, statusSendCmd, sendCmdId);
    }
    else
    {
        /* force send status don't care ready */
        /* ack, sleep mode,standby,restore mode*/
        if((event == statusACKReport) || (event == statusSLPReport) || (event == statusSTBReport) || (event == statusRestoreMode))
        {
            MESSAGE_MAKE(sendCmdId, statusSendCmd_T);
            sendCmdId->event = event;
            MessageSend((TaskData *)&_statusReportTask, statusSendCmd, sendCmdId);   
        }        
    }
}

/*! \brief startCommunicationToChargingCase */
void startCommunicationToChargingCase(void)
{
    xprint("startCommunicationToChargingCase");   
    MessageCancelFirst((TaskData *)&_statusReportTask, statusLeaveUSBCheck); 
    if(procCmd.debugmode & PORTBIT)
    {
        procCmd.debugmode = getPSDebugMode();
    }
    else
    {
        setCommunPort(CH_UART);
    }
    /*if pairing wait pairing send then send battery*/
    if(procCmd.startstop == 0)
    {
        procCmd.startstop = 1;    
        procCmd.askBattery = 0;    
        /*modify for speed up illumated when insert earbud into case*/
        MessageCancelFirst((TaskData *)&_statusReportTask, statusAskBattery); 
        if(procCmd.btStatus == btPairing)
            MessageSendLater((TaskData *)&_statusReportTask, statusAskBattery, 0, 500);
        else
        {    
            procCmd.speedup = 1;
            MessageSend((TaskData *)&_statusReportTask, statusAskBattery, 0);          
        }
    }
}

/*! \brief stopCommunicationToChargingCase */
void stopCommunicationToChargingCase(void)
{
    xprint("stopCommunicationToChargingCase");   
#if 0  /*tmp remove change USB port when leave USB 5v*/
    if(procCmd.debugmode & PORTBIT)
    {
        procCmd.debugmode = getPSDebugMode();
    }
    else
    {
        MessageSendLater((TaskData *)&_statusReportTask, statusLeaveUSBCheck, NULL, D_SEC(1));
    }     
#endif
    
    if(procCmd.startstop == 1)
    {
        procCmd.startstop = 0;    
        procCmd.speedup = 0;
        MessageCancelAll((TaskData *)&_statusReportTask, statusSendCmd);
        MessageCancelAll((TaskData *)&_statusReportTask, statusAskBattery);    
    }
}
/* ----------------- Global Function ----------------------- */
/*! \brief tymChargingCaseCommunicationInit */
bool tymChargingCaseCommunicationInit(Task init_task)
{
    UNUSED(init_task);
    xprint("tymChargingCaseCommunicationInit");
    memset(&procCmd,0x0,sizeof(processCmd_s)); 
    procCmd.debugmode = getPSDebugMode();
    /* Register for physical state changes */       
    appChargerClientRegister(&_statusReportTask);
    return TRUE;
}

/*! \brief report ANC Calibration */
void reportDoAncCalibration(void)
{
    _sendStatusCmd(statusANCCalreport);
}

/*! \brief report BT status to CC */
void reportBtStatus(uint8 status)
{
    xprintf("CC:reportBT %d",status);
    if(status == btPairing)
    {
        Prompts_SetConnectedStatus(0);//clean connected prompts,can have connected prompt when connected
        _sendStatusCmd(statusPairingMode);
    }
    else if(status == btPairingSuccessful)
    {
        _sendStatusCmd(statusConnetcedMode);
        Prompts_CancelPairingContinue();
    }
    else if(status == btDisconnect)
    {
        _sendStatusCmd(statusDisconnectMode);
    }                    
    else if(status == restoreDefault)
    {
        _sendStatusCmd(statusRestoreMode);        
    }    
    else if(status == btConnected)
    {
        //Prompts_CancelPairingContinue();
        if(procCmd.btStatus == btPairing)//before status btPairing -> connected. list re-connect
        {
            _sendStatusCmd(statusConnetcedMode);
        }    
    }
    else if(status == btPairingTimeOut)
    {
        DEBUG_LOG("statusPairingTimeOut");         
        _sendStatusCmd(statusPairingTimeOut);  
    }    
    else if(status == startOTA)
    {
        procCmd.otamode = TRUE;
        DEBUG_LOG("tym send startOTA"); 
        _sendStatusCmd(statusOTA);
    }
    else if(status == OTAFinish)                                
    {
        if(procCmd.otamode || appUpgradeIsDfuRebootDone() == TRUE)
        {    
            DEBUG_LOG("tym send OTAFinish,otamode %d,Upgrade %d",procCmd.otamode,appUpgradeIsDfuRebootDone()); 
            procCmd.otamode = FALSE;
            _sendStatusCmd(statusOTADone);
        }
        else
        {
        	DEBUG_LOG("No OTA status");
        	return;
        }
    }
    else if(status == happenErr)
    {
        procCmd.otamode = FALSE;
        DEBUG_LOG("tym send happenErr"); 
        _sendStatusCmd(statusErr);
    }        
    if(status == btDisconnect)
        procCmd.btStatus = btConnectable; //disconnect is temporary information
    else                    
        procCmd.btStatus = status; 
    //update touch pad
    updateTouchPadMode();                   
}

/*! \brief report power off to CC */
void reportPowerOffStatus(void)
{
    Prompts_SetConnectedStatus(0);//clean connected prompts,can have connected prompt when connected
    _sendStatusCmd(statusPowerOff);
}

/*! \brief report power on battery to CC */
void reportPowerOnStatus(void)
{
    setSystemReady(TRUE);
    procCmd.battpercent = appBatteryGetPercent(); 
    reportBattStatus(); 
    sendCmdToChargingCase(statusACKReport);           
}

/*! \brief report standby battery to CC */
void reportSleepStandbyStatus(bool sleep)
{
    if(sleep)
        _sendStatusCmd(statusSLPReport);
    else  
        _sendStatusCmd(statusSTBReport);           
}

/*! \brief report ACK status */
void reportACKReport(void)
{
    _sendStatusCmd(statusACKReport);
}

/*! \brief get current BT status */
uint8 tymGetBTStatus(void)
{
    return procCmd.btStatus;
}

/*! \brief set system ready */
void setSystemReady(bool ready)
{
    procCmd.systemReady = ready;
}

/*! \brief get system ready status*/
bool getSystemReady(void)
{
    return procCmd.systemReady;
}

/*! \brief Handle debug mode change GPIO port. */
void earbudCC_RecoveryCommPort(void)
{
    setPSDebugModeBit(PORTBIT,0);
    procCmd.debugmode = getPSDebugMode();
    setCommunPort(CH_UART);
}

void earbudCC_RecoveryCommPort1s(void)
{
    MessageSendLater((TaskData *)&_statusReportTask, statusChangeUART, NULL, D_SEC(1));
}
/*! \brief Handle debug mode change USB port. */
void earbudCC_ChangeUSBPort(void)
{
    if(getFactoryModeEnable() == TRUE)
    {
        DEBUG_LOG("TYM Change USB Port - enter");
        setCommunPort(CH_USB);
        setPSDebugModeBit(PORTBIT,1);
        procCmd.debugmode = getPSDebugMode();
    }
}

/*! \brief Handle gaia disconnect */
void tymCleanOTAFLAG(void)
{
    procCmd.otamode = FALSE;
}
