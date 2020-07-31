
//----------Header----------
#ifndef __TYM_UART_C__
#define __TYM_UART_C__

//----------Private Include----------

//----------System Include----------
#include <stream.h>
#include <sink.h>
#include <source.h>
#include <string.h>
#include <panic.h>
#include <logging.h>
#include <message.h>
#include <app/uart/uart_if.h>
#include "earbud_tym_cc_communication.h"
#include "logical_input_switch.h"
#include "1_button.h"
#include "earbud_sm.h"

//----------Private Define----------
typedef struct {
    TaskData task;
    Sink uart_sink;
    Source uart_source;
} UARTStreamTaskData;

typedef union {
  unsigned int StrVal;
  struct {
    char cmd[4];
  } xs_str;
} xu_ptrstr;

typedef struct sendCmdRaw{
  char cmd[5];
}sendCmdRaw_s;

typedef union {
    unsigned char u8Byte;
    struct {
        unsigned bit0:1;
        unsigned bit1:1;
        unsigned bit2:1;
        unsigned bit3:1;
        unsigned bit4:1;
        unsigned bit5:1;
        unsigned bit6:1;
        unsigned bit7:1;
    } xs_bits;
} xu_Flag;

//----------System Define----------
#define UartBufferSize 32

//----------Private Variable----------
UARTStreamTaskData theUARTStreamTask;

char InputCMD[UartBufferSize] = {0};
unsigned char u16InputCNT = 0;

xu_Flag xuFlag[1] = {0};
#define UR_GET_CMD   xuFlag[0].xs_bits.bit0
#define UR_NG_CMD    xuFlag[0].xs_bits.bit1

typedef enum {
    //typedef enum statusOutputMsg {                     ASCII  0x00 ~ 0x7F
        xetx_statusPairingMode          =  0,  //BTP           42 54 50  E6  /*  7, BT paring     */
        xetx_statusConnetcedMode        =  1,  //BTC           42 54 43  D9  /*  8, BT connect    */
        xetx_statusDisconnectMode       =  2,  //BTD           42 54 44  DA  /*  9, BT disconnect */
        xetx_statusRestoreMode          =  3,  //RSD           52 53 44  E9  /*  3, Reset Default */
        xetx_statusPowerOff             =  4,  //PW0           50 57 30  D7  /*  5, poweroff      */
        xetx_statusOTA                  =  5,  //OTA           4F 54 41  E4  /* 10, ota           */
        xetx_statusErr                  =  6,  //ERR           45 52 52  E9  /* 11, error         */
        xetx_statusBattCap_61           =  7,  //BA0           42 41 30  B3  /*  1, battery 0     */
        xetx_statusBattCap_95           =  8,  //BA1           42 41 31  B4  /*  2, battery 1     */
        xetx_statusANCCalreport         =  9,  //AN0           41 4E 30  BF  /* 16, Anc Enter Calibration, EventUsrGaiaUser12 */
                                               //AN1           41 4E 31  C0  /* 17, Anc Exit Calibration,  EventUsrGaiaUser12 */
        xetx_statusReportCmdMAX         = 10,  //SRC           53 52 43  8A
        xetx_statusAskBattery           = 11,  //INF           49 4E 46  DD  /* 14, EventUsrAskInfo        EventUsrGaiaUser9  */
        xetx_statusSendCmd              = 12,  //SSC           53 53 43  8C
        xetx_statusExecuteCmd           = 13,  //SEC           53 45 43  8D
        xetx_statusEndCmd               = 14,  //SEN  reserve  53 45 4e  8E,, return "ACK@" when receive whole command
    //}statusOutputMsg_t;

        xerx_APP_POWER_ON               = 15,  //PW1           50 57 31  D8  /*  6, poweron       */
        xerx_APP_BUTTON_DELETE_HANDSET  = 16,  //ADH           41 44 48  90
        xerx_APP_POWER_OFF              = 17,  //PW0           50 57 30  D7  /*  5, poweroff      */
        xerx_APP_CHANGE_USB_PORT        = 18,  //CPT           43 50 54  E7  /* 13, EventUsrChangePort     EventUsrGaiaUser8  */
        xerx_APP_MFB_ANC_CAL            = 19,  //AN0           41 4E 30  BF  /* 16, Anc Enter Calibration, EventUsrGaiaUser12 */
        xerx_APP_FACTORY_MODE           = 20,  //FCT           46 43 54  DD  /* 15, EventUsrFactoryMode,   EventUsrGaiaUser10 */
        xerx_APP_BUTTON_DFU             = 21,  //ADF           41 44 46  95
        xerx_APP_BUTTON_VOLUME_UP       = 22,  //AVU           41 56 55  96
        xerx_APP_BUTTON_TAPX2           = 23,  //AT2           41 54 32  97
        xerx_APP_BUTTON_TAPX3           = 24,  //AT3           41 54 33  98
        xerx_APP_BUTTON_TAP_BISTO       = 25,  //ATB           41 54 42  99
        xerx_APP_BUTTON_TAPX1           = 26,  //AT1           41 54 31  9A
        xerx_APP_BUTTON_VOLUME_DOWN     = 27,  //AVD           41 56 44  9B
        xerx_APP_BUTTON_TAP_ANC         = 28,  //AN0           41 4E 30  BF  /* 16, Anc Enter Calibration, EventUsrGaiaUser12 */
                                               //AN1           41 4E 31  C0  /* 17, Anc Exit Calibration,  EventUsrGaiaUser12 */
        xerx_APP_BUTTON_HANDSET_PAIRING = 29,  //TWP           54 57 50  FB  /* 18, TWS Paring */
        xerx_APP_BUTTON_FORWARD         = 30,  //ABF           41 42 46  9E
        xerx_APP_BUTTON_BACKWARD        = 31,  //ABB           41 42 42  9F

        xetx_FCT_TEST_UART_FUNCTION     = 32,  //FTU           46 54 55  A0  /* FCT for Test UART Function */
        xerx_APP_ACK_COMMAND            = 33,  //ACK           41 43 4B  CF

#if 0
    const xu_ptrstr CMD[CMD_SIZE] = {  //From STM MCU Command Code
      {.xs_str={0x41, 0x43, 0x4B, 0xCF}}, /*  0, ACK, ack when receive any str+0x0d or timeout */

      {.xs_str={0x42, 0x41, 0x30, 0xB3}}, /*  1, BA0 */
      {.xs_str={0x42, 0x41, 0x31, 0xB4}}, /*  2, BA1 */

      {.xs_str={0x52, 0x53, 0x44, 0xE9}}, /*  3, RSD */
      {.xs_str={0x52, 0x53, 0x54, 0xF9}}, /*  4, RST */

      {.xs_str={0x50, 0x57, 0x30, 0xD7}}, /*  5, PW0, poweroff      */
      {.xs_str={0x50, 0x57, 0x31, 0xD8}}, /*  6, PW1, poweron       */

      {.xs_str={0x42, 0x54, 0x50, 0xE6}}, /*  7, BTP, BT paring     */
      {.xs_str={0x42, 0x54, 0x43, 0xD9}}, /*  8, BTC, BT connect    */
      {.xs_str={0x42, 0x54, 0x44, 0xDA}}, /*  9, BTD, BT disconnect */

      {.xs_str={0x4F, 0x54, 0x41, 0xE4}}, /* 10, OTA */
      {.xs_str={0x45, 0x52, 0x52, 0xE9}}, /* 11, ERR */

      {.xs_str={0x4C, 0x50, 0x42, 0xDE}}, /* 12, LPB, LoopBack Test */

      {.xs_str={0x43, 0x50, 0x54, 0xE7}}, /* 13, CPT, EventUsrChangePort     EventUsrGaiaUser8  */
      {.xs_str={0x49, 0x4E, 0x46, 0xDD}}, /* 14, INF, EventUsrAskInfo        EventUsrGaiaUser9  */
      {.xs_str={0x46, 0x43, 0x54, 0xDD}}, /* 15, FCT, EventUsrFactoryMode,   EventUsrGaiaUser10 */
      {.xs_str={0x41, 0x4E, 0x30, 0xBF}}, /* 16, AN0, Anc Enter Calibration, EventUsrGaiaUser12 */
      {.xs_str={0x41, 0x4E, 0x31, 0xC0}}, /* 17, AN1, Anc Exit Calibration,  EventUsrGaiaUser12 */

      {.xs_str={0x54, 0x57, 0x50, 0xFB}}, /* 18, TWP,  TWS Paring */
    };
#endif

} xe_rxCMD;



//----------Global Variable----------

//----------Private Function----------
void UARTStreamMessageHandler (Task pTask, MessageId pId, Message pMessage);
void uart_data_stream_rx_data(Source src);
void UART_RX_ISR(char Input);
static void receviedPowerOn(void);
//----------Global Function----------
void uart_data_stream_init(uint16 PIO_UART_TX, uint16 PIO_UART_RX, int BRD);
void uart_data_stream_tx_data(const uint8 *data, uint16 length);

//----------Code Implement----------
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
void uart_data_stream_init(uint16 PIO_UART_TX, uint16 PIO_UART_RX, int BRD) {
    //#define PIO_UART_TX (20)
    //#define PIO_UART_RX (0)

    #define UART_TX_BANK (PIO_UART_TX >> 5)
    #define UART_TX_MASK (1UL << (PIO_UART_TX & 31))
    #define UART_RX_BANK (PIO_UART_RX >> 5)
    #define UART_RX_MASK (1UL << (PIO_UART_RX & 31))

    float c_0 =  1.208000000000000000000000000000, x_0 = 1,
          c_1 =  0.004100000000000000000000000000, x_1 = BRD,
          c_2 = -0.000000000400000000000000000000, x_2 = x_1*x_1,
          c_3 =  0.000000000000000900000000000000, x_3 = x_2*x_1,
          c_4 = -0.000000000000000000000500000000, x_4 = x_2*x_2,
          c_5 =  0.000000000000000000000000000000, x_5 = x_3*x_2,
          c_6 =  0.000000000000000000000000000000, x_6 = x_3*x_3;
    float fBRD = c_6*x_6 +
                 c_5*x_5 +
                 c_4*x_4 +
                 c_3*x_3 +
                 c_2*x_2 +
                 c_1*x_1 +
                 c_0*x_0;
    int iBRD = fBRD;

    /*configure PIO as hardware control*/
    PioSetMapPins32Bank(UART_TX_BANK, UART_TX_MASK, 0);
    PioSetMapPins32Bank(UART_RX_BANK, UART_RX_MASK, 0);

    /*configure PIO */
    PioSetFunction(PIO_UART_TX, UART_TX);
    PioSetFunction(PIO_UART_RX, UART_RX);

    theUARTStreamTask.task.handler = UARTStreamMessageHandler;
    StreamUartConfigure( iBRD, /* VM_UART_RATE_460K8 */
                         VM_UART_STOP_ONE,
                         VM_UART_PARITY_NONE );

    /* Get the sink for the uart */
    theUARTStreamTask.uart_sink = StreamUartSink();
    PanicZero(theUARTStreamTask.uart_sink);

    theUARTStreamTask.uart_source = StreamSourceFromSink(theUARTStreamTask.uart_sink);
    PanicZero(theUARTStreamTask.uart_source);

    MessageStreamTaskFromSink(theUARTStreamTask.uart_sink, &theUARTStreamTask.task);
}
/*----------------------------------------------------------------------------*/
void uart_data_stream_tx_data(const uint8 *data, uint16 length) {
    uint16  offset = 0;
    uint8  *dest   = NULL;

    /* Claim space in the sink, getting the offset to it */
    do {
        offset = SinkClaim(theUARTStreamTask.uart_sink, length);
    } while(offset == 0xFFFF);  //Panic();

    /* Map the sink into memory space */
    dest = SinkMap(theUARTStreamTask.uart_sink);
    PanicNull(dest);

    /* Copy data into the claimed space */
    memcpy(dest+offset, data, length);

    /* Flush the data out to the uart */
    PanicZero(SinkFlush(theUARTStreamTask.uart_sink, length));
}
/*----------------------------------------------------------------------------*/
void uart_data_stream_rx_data(Source src) {
    uint16 length = 0;
    const uint8 *data = NULL;

    /* Get the number of bytes in the specified source before the next packet boundary */
    if(!(length = SourceBoundary(src)))
        return;

    /* Maps the specified source into the address map */
    data = SourceMap(src);
    PanicNull((void*)data);

    /* Transmit the received data */
    if(0) uart_data_stream_tx_data(data, length);
    for(unsigned int i=0; i<length; i++) {
        UART_RX_ISR((char)data[i]);
    }

    /* Discards the specified amount of bytes from the front of the specified source */
    SourceDrop(src, length);
}
/*----------------------------------------------------------------------------*/
void UARTStreamMessageHandler (Task pTask, MessageId pId, Message pMessage) {
    UNUSED(pTask);
    if (pId == MESSAGE_MORE_DATA) {
        uart_data_stream_rx_data(((MessageMoreData *)pMessage)->source);
    }
}

static void receviedPowerOn(void)
{
    /*set ready and report battery for battery report to slow*/
    reportPowerOnStatus();
    MessageSendLater(LogicalInputSwitch_GetTask(), APP_POWER_ON, NULL, 400);   
}

/*----------------------------------------------------------------------------*/
void UART_RX_ISR(char Input) {

    switch(Input) {
        case '\r':
        case '\n':
        case '\a':  LBL_GetCMD_End:
            u16InputCNT = 0;
            UR_GET_CMD = 1;

            // UR CMD Implementation, can be moved into other function
            if(UR_GET_CMD) {
                UR_GET_CMD = 0;
                UR_NG_CMD = 1;

                xu_ptrstr getcmd = {0}; //{ .xs_str.cmd={0x42, 0x41, 0x30, 0xB3} };
                for(int i=0; i<UartBufferSize; i++) {
                    getcmd.xs_str.cmd[3] = getcmd.xs_str.cmd[2];
                    getcmd.xs_str.cmd[2] = getcmd.xs_str.cmd[1];
                    getcmd.xs_str.cmd[1] = getcmd.xs_str.cmd[0];
                    getcmd.xs_str.cmd[0] = InputCMD[i];
                    switch(getcmd.StrVal) {
                      //case 0x425450E6: UR_NG_CMD = 0;  uart_data_stream_tx_data((const uint8*)"BTP", 3);  break;  /* BTP           xetx_statusPairingMode          =  0 */
                      //case 0x425443D9: UR_NG_CMD = 0;  uart_data_stream_tx_data((const uint8*)"BTC", 3);  break;  /* BTC           xetx_statusConnetcedMode        =  1 */
                      //case 0x425444DA: UR_NG_CMD = 0;  uart_data_stream_tx_data((const uint8*)"BTD", 3);  break;  /* BTD           xetx_statusDisconnectMode       =  2 */
                      //case 0x525344E9: UR_NG_CMD = 0;  uart_data_stream_tx_data((const uint8*)"RSD", 3);  break;  /* RSD           xetx_statusRestoreMode          =  3 */
                        case 0x505730D7: UR_NG_CMD = 0;  MessageSend(LogicalInputSwitch_GetTask(), APP_POWER_OFF, NULL);      break;  /* PW0           xetx_statusPowerOff             =  4 */
                      //case 0x4F5441E4: UR_NG_CMD = 0;  uart_data_stream_tx_data((const uint8*)"OTA", 3);  break;  /* OTA           xetx_statusOTA                  =  5 */
                      //case 0x455252E9: UR_NG_CMD = 0;  uart_data_stream_tx_data((const uint8*)"ERR", 3);  break;  /* ERR           xetx_statusErr                  =  6 */
                      //case 0x424130B3: UR_NG_CMD = 0;  uart_data_stream_tx_data((const uint8*)"BA0", 3);  break;  /* BA0           xetx_statusBattCap_61           =  7 */
                      //case 0x424131B4: UR_NG_CMD = 0;  uart_data_stream_tx_data((const uint8*)"BA1", 3);  break;  /* BA1           xetx_statusBattCap_95           =  8 */
                        case 0x414E30BF: UR_NG_CMD = 0;  MessageSend(LogicalInputSwitch_GetTask(), APP_MFB_ANC_CAL, NULL);    break;  /* AN0           xetx_statusANCCalreport         =  9 */
                        case 0x414E31C0: UR_NG_CMD = 0;  MessageSend(LogicalInputSwitch_GetTask(), APP_MFB_ANC_CAL, NULL);    break;  /* AN1           xetx_statusANCCalreport         =  9 */
                      //case 0x5352438A: UR_NG_CMD = 0;  uart_data_stream_tx_data((const uint8*)"SRC", 3);  break;  /* SRC           xetx_statusReportCmdMAX         = 10 */
                      //case 0x494E46DD: UR_NG_CMD = 0;  uart_data_stream_tx_data((const uint8*)"INF", 3);  break;  /* INF           xetx_statusAskBattery           = 11 */
                      //case 0x5353438C: UR_NG_CMD = 0;  uart_data_stream_tx_data((const uint8*)"SSC", 3);  break;  /* SSC           xetx_statusSendCmd              = 12 */
                      //case 0x5345438D: UR_NG_CMD = 0;  uart_data_stream_tx_data((const uint8*)"SEC", 3);  break;  /* SEC           xetx_statusExecuteCmd           = 13 */
                      //case 0x53454e8E: UR_NG_CMD = 0;  uart_data_stream_tx_data((const uint8*)"SEN@",4);  break;  /* SEN  reserve  xetx_statusEndCmd               = 14 return "SEN@" when receive whole command  */


                        case 0x505731D8: UR_NG_CMD = 0;  receviedPowerOn();     break;  /* PW1           xerx_APP_POWER_ON               = 15 */
                      //case 0x41444890: UR_NG_CMD = 0;  uart_data_stream_tx_data((const uint8*)"ADH", 3);  break;  /* ADH           xerx_APP_BUTTON_DELETE_HANDSET  = 16 */
                      //case 0x505730D7: UR_NG_CMD = 0; uart_data_stream_tx_data((const uint8*)"PW0", 3);  break;  /* PW0           xerx_APP_POWER_OFF              = 17 */
                        case 0x435054E7: UR_NG_CMD = 0;  MessageSend(LogicalInputSwitch_GetTask(), APP_CHANGE_USB_PORT, NULL);   break;  /* CPT           xerx_APP_CHANGE_USB_PORT        = 18 */
                      //case 0x414E30BF: UR_NG_CMD = 0;  uart_data_stream_tx_data((const uint8*)"AN0", 3);  break;  /* AN0           xerx_APP_MFB_ANC_CAL            = 19 */
                        case 0x464354DD: UR_NG_CMD = 0;  MessageSend(LogicalInputSwitch_GetTask(), APP_FACTORY_MODE, NULL);  break;  /* FCT           xerx_APP_FACTORY_MODE           = 20 */
                            
                      //case 0x41444695: UR_NG_CMD = 0;  uart_data_stream_tx_data((const uint8*)"ADF", 3);  break;  /* ADF           xerx_APP_BUTTON_DFU             = 21 */
                      //case 0x41565596: UR_NG_CMD = 0;  uart_data_stream_tx_data((const uint8*)"AVU", 3);  break;  /* AVU           xerx_APP_BUTTON_VOLUME_UP       = 22 */
                      //case 0x41543297: UR_NG_CMD = 0;  uart_data_stream_tx_data((const uint8*)"AT2", 3);  break;  /* AT2           xerx_APP_BUTTON_TAPX2           = 23 */
                      //case 0x41543398: UR_NG_CMD = 0;  uart_data_stream_tx_data((const uint8*)"AT3", 3);  break;  /* AT3           xerx_APP_BUTTON_TAPX3           = 24 */
                      //case 0x41544299: UR_NG_CMD = 0;  uart_data_stream_tx_data((const uint8*)"ATB", 3);  break;  /* ATB           xerx_APP_BUTTON_TAP_BISTO       = 25 */
                      //case 0x4154319A: UR_NG_CMD = 0;  uart_data_stream_tx_data((const uint8*)"AT1", 3);  break;  /* AT1           xerx_APP_BUTTON_TAPX1           = 26 */
                      //case 0x4156449B: UR_NG_CMD = 0;  uart_data_stream_tx_data((const uint8*)"AVD", 3);  break;  /* AVD           xerx_APP_BUTTON_VOLUME_DOWN     = 27 */
                      //case 0x414E30BF: UR_NG_CMD = 0;  uart_data_stream_tx_data((const uint8*)"AN0", 3);  break;  /* AN0           xerx_APP_BUTTON_TAP_ANC         = 28 */
                      //case 0x414E31C0: UR_NG_CMD = 0;  uart_data_stream_tx_data((const uint8*)"AN1", 3);  break;  /* AN1           xerx_APP_BUTTON_TAP_ANC         = 28 */
                      //case 0x545750FB: UR_NG_CMD = 0;  uart_data_stream_tx_data((const uint8*)"TWP", 3);  break;  /* TWP           xerx_APP_BUTTON_HANDSET_PAIRING = 29 */
                      //case 0x4142469E: UR_NG_CMD = 0;  uart_data_stream_tx_data((const uint8*)"ABF", 3);  break;  /* ABF           xerx_APP_BUTTON_FORWARD         = 30 */
                      //case 0x4142429F: UR_NG_CMD = 0;  uart_data_stream_tx_data((const uint8*)"ABB", 3);  break;  /* ABB           xerx_APP_BUTTON_BACKWARD        = 31 */

                      //case 0x465455A0: UR_NG_CMD = 0;  uart_data_stream_tx_data((const uint8*)"FTU", 3);  break;  /* FTU           xetx_FCT_TEST_UART_FUNCTION     = 32 */
                        case 0x41434BCF: UR_NG_CMD = 0;  sendCmdToChargingCase(statusACKReport);                         break;  /* ACK           xerx_APP_ACK_COMMAND            = 33 */
                        case 0x534850EB: UR_NG_CMD = 0;  MessageSendLater(SmGetTask(), ui_input_shipping, NULL, D_SEC(1));  break;  /* SHP     QUALCOMM SLEEP MODE   */
                    }
                }

                if(UR_NG_CMD){
                    uart_data_stream_tx_data((const uint8*)"ng", 2);
                } else {
                    uart_data_stream_tx_data((const uint8*)"ok", 2);
                }
                memset(InputCMD, 0, UartBufferSize);
            }

            break;

        default:
            InputCMD[u16InputCNT] = Input;
            u16InputCNT++;
            if(u16InputCNT>=UartBufferSize) {
                goto LBL_GetCMD_End;
            }
            break;
    }
}
/*----------------------------------------------------------------------------*/
void sendCmdToChargingCase(uint8 cmdId)
{
    const sendCmdRaw_s cmdRaw[] = {
      {0x42, 0x54, 0x50, 0xE6, 0x0d},  /*BTP, BT paring, 0 */
      {0x42, 0x54, 0x43, 0xD9, 0x0d},  /*BTC, BT connect, 1 */
      {0x42, 0x54, 0x44, 0xDA, 0x0d},  /*BTD, BT disconnect ,2  */
      {0x52, 0x53, 0x44, 0xE9, 0x0d},  /*RSD, Reset Default , 3*/
      {0x50, 0x57, 0x30, 0xD7, 0x0d},  /*PW0, poweroff ,4*/
      {0x4F, 0x54, 0x41, 0xE4, 0x0d},  /*OTA, ota ,5*/
      {0x45, 0x52, 0x52, 0xE9, 0x0d},  /*ERR, error ,6*/
      {0x42, 0x41, 0x30, 0xB3, 0x0d},  /*BA0, battery 0,7 */
      {0x42, 0x41, 0x31, 0xB4, 0x0d},  /*BA1, battery 1 ,8*/
      {0x41, 0x4E, 0x30, 0xBF, 0x0d},  /*AN0, ANC Enter Calbiration,9 */
      {0x41, 0x43, 0x4B, 0xCF, 0x0d},  /*ACK, Acknowledge , 10 */
      {0x53, 0x4C, 0x50, 0xEF ,0x0d},  /*SLEEP mode, 11 */
      {0x53, 0x54, 0x42, 0xE9 ,0x0d},  /*STANDBY mode, 12 STB */      
      {0x4F, 0x54, 0x46, 0xE9, 0x0d},  /*OTA Finish, 13 OTF*/
      {0x50, 0x54, 0x4F, 0xF3, 0x0d},  /*PTO,Pairing TimeOut 14 PTO*/         
    };
    if(cmdId < statusReportCmdMAX)
    {
        uart_data_stream_tx_data((const uint8*)cmdRaw[cmdId].cmd,5);
        if(cmdId == statusPairingMode) /*for RST can't receive left earbud pairing */
            uart_data_stream_tx_data((const uint8*)cmdRaw[cmdId].cmd,5);            
    }
}
/*----------------------------------------------------------------------------*/

#endif
