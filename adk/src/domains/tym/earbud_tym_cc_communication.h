#ifndef __EARBUD_TYM_CC_COMMUNICATION_H__
#define __EARBUD_TYM_CC_COMMUNICATION_H__

typedef enum statusOutputMsg {
    statusPairingMode = 0,
    statusConnetcedMode,
    statusDisconnectMode,
    statusRestoreMode,
    statusPowerOff,
    statusOTA,
    statusErr,
    statusBattCap_61,
    statusBattCap_95,
    statusANCCalreport,
    statusACKReport,
    statusSLPReport,
    statusSTBReport,
    statusOTADone,    
    statusReportCmdMAX,
    statusAskBattery,
    statusSendCmd,
    statusExecuteCmd,
    statusEndCmd,
    statusLeaveUSBCheck,
}statusOutputMsg_t;


bool tymChargingCaseCommunicationInit(Task init_task);
void reportDoAncCalibration(void);
void reportBtStatus(uint8 status);
void reportPowerOffStatus(void);
uint8 tymGetBTStatus(void);
void setSystemReady(bool ready);
bool getSystemReady(void);
void sendCmdToChargingCase(uint8 command);
void reportACKReport(void);
void earbudCC_RecoveryCommPort(void);
void earbudCC_ChangeUSBPort(void);
void reportPowerOnStatus(void);
void reportSleepStandbyStatus(bool sleep);
#endif



