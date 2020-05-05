/*!
\copyright  Copyright (c) 2017 - 2019 TYMPHANY Technologies International, Ltd.
            All Rights Reserved.
            TYMPHANY Technologies International, Ltd. Confidential and Proprietary.
\version
\file       stanc3.c
\brief      Support for the ANC stanc3
*/
/* ------------------------ Include ------------------------ */
#include <panic.h>
#include <pio.h>
#include <stdlib.h>
#include <ps.h>
#include <byte_utils.h>
#include <logging.h>
#include "stanc3.h"
#include "tym_i2c_control.h"
#include "tym_power_control.h"
#include "tym_anc_config.h"
#include "earbud_tym_util.h"
#include "earbud_tym_psid.h"
#include "../../audio/kymera/kymera_private.h"

/* ------------------------ Defines ------------------------ */

#define xprint(x)            DEBUG_LOG(x)
#define xprintf(x, ...)      DEBUG_LOGF(x,  __VA_ARGS__)

#define ANC_CTRL_ADDR			(0x44)
#define STANC_CLOCK             (100) /*100 KHZ*/
/* Number of bytes used in the ps key for ANC config. */
#define SINK_ANC_DATA_BYTES      27  /*0x0 ~ 0x1A 27 units*/
/* Number of words */
#define SINK_ANC_PS_SIZE        ((SINK_ANC_DATA_BYTES + 1)/2)
/* ------------------------ Types -------------------------- */
typedef struct _ANC_CONFIG{
    uint8 regAddr;
    uint8 regData;
}ANC_CONFIG_S;

struct __anc_config anc_config = {
    .init = FALSE,
    .state = FALSE,
};
/* --------------- Local Function Prototypes --------------- */
void stanc3_init(ancConfig *config);
void stanc3_deinit(ancConfig *config);
/* --------------- Static Global Variables ----------------- */
tymAncTaskData app_tymanc = {
    .ambientLevel = 4,
    .speechLevel = 4,
};
/* ------------------ Local Function ----------------------- */
/*! \brief  stanc3 init function*/
void stanc3_init(ancConfig *config)
{
    const ANC_CONFIG_S ANCData[] = {
    {0x00, 0x00},
    {0x01, 0x01},
    {0x02, 0x10},
    {0x03, 0x3F},
    {0x04, 0x1E},
    {0x05, 0x00},
    {0x06, 0x00},
    {0x07, 0x00},
    {0x08, 0x00},
    {0x09, 0x02},
    {0x0A, 0x02},
    {0x0B, 0x0E},
    {0x0C, 0x02},
    {0x0D, 0x14},
    {0x0E, 0x00},
    {0x0F, 0x02},
    {0x10, 0x07},
    {0x11, 0x02},
    {0x12, 0x07},
    {0x13, 0x05},
    {0x14, 0x00},
    {0x15, 0x10},
    {0x16, 0xF8},
    {0x17, 0x8E},  /*origianl:0x8C,default: ANC mute:0x8E */
    {0x18, 0x09},
    {0x19, 0x0D},
    {0x1A, 0x02},
    {0x22, 0x01},
    {0x24, 0x04}
};
    uint8 write_data[2];
    uint8 config_len = sizeof(ANCData)/sizeof(ANC_CONFIG_S);
    int i;
    int ANCConfigData,retbytes;
    uint8 anc_data[SINK_ANC_DATA_BYTES];
    uint16 pskeydata[SINK_ANC_PS_SIZE];
    config->state = FALSE;
    if(config->init == TRUE)
        return;

    if (tym_i2cdevice_init(ANC_CTRL_ADDR,STANC_CLOCK, i2c_device_anc_id) == FALSE)
    {
        xprint("ANC i2c device init failed");
        return;
    }
    ANCConfigData = PsRetrieve(PSID_ANCTABLE,0,0);
    if(ANCConfigData != 0)
    {
        ANCConfigData = PsRetrieve(PSID_ANCTABLE,pskeydata,SINK_ANC_PS_SIZE);
        retbytes = ByteUtilsMemCpyUnpackString(anc_data, pskeydata, SINK_ANC_DATA_BYTES);
        xprintf("dump calibration value ret byte %d",retbytes);
        for(i = 0;i < retbytes;i++)
        {
            xprintf("0x%x data 0x%x\n",i,anc_data[i]);
            write_data[0] = i;
            write_data[1] = anc_data[i];
            if(!tym_i2c_write(ANC_CTRL_ADDR,write_data,2))
            {
                xprintf("return error tym_i2c_write seq %d",i);
                return; /* error */
            }
        }
        xprint("\n");
        write_data[0] = 0x22;
        write_data[1] = 0x01;
        if(!tym_i2c_write(ANC_CTRL_ADDR,write_data,2))
        {
            xprintf("return error tym_i2c_write seq %d",i);
            return; /* error */
        }
    }
    else
    {
        for(i = 0; i < config_len;i++)
        {
            write_data[0] = ANCData[i].regAddr;
            write_data[1] = ANCData[i].regData;
            if(!tym_i2c_write(ANC_CTRL_ADDR,write_data,2))
            {
                xprintf("return error tym_i2c_write seq %d",i);
                return; /* error */
            }
        }
    }
    stanc3_audiomute(FALSE);   
    config->init = TRUE;
}

/*! \brief  stanc3 clean init flag */
void stanc3_deinit(ancConfig *config)
{
    config->init = FALSE;
    stanc3_audiomute(TRUE);
}

/*! \brief  stanc3 audio mute/unmute */
void stanc3_audiomute(bool enable)
{
    uint8 read_addr;
    uint8 read_data[1];
    uint8 write_data[2];
    /* read address 0x17 */
    read_addr = 0x17;
    if(!tym_i2c_read(ANC_CTRL_ADDR,read_addr,read_data,1))
    {
        xprintf("return error tym_i2c_read %d",read_addr);
        return; /* error */
    }
    if(enable == 1)
    {
        /* mute */
        write_data[0] = 0x17;
        write_data[1] = (read_data[0] | (1 << 0));/*Set AUDIO_MUTE bit (0x17,D0) to 1*/
        xprintf("write 0x17 0x%x",write_data[1]);
        if(!tym_i2c_write(ANC_CTRL_ADDR,write_data,2))
        {
            xprintf("return error tym_i2c_write %d",write_data[0]);
            return; /* error */
        }
    }
    else
    {
    	/* unmute */
        write_data[0] = 0x17;
        write_data[1] = (read_data[0] & ~(1 << 0));/*Set AUDIO_MUTE bit (0x17,D0) to 0*/
        xprintf("write 0x17 0x%x",write_data[1]);
        if(!tym_i2c_write(ANC_CTRL_ADDR,write_data,2))
        {
            xprintf("return error tym_i2c_write %d",write_data[0]);
            return; /* error */
        }
    }
}

/* ----------------- Global Function ----------------------- */
/*! \brief  register ANC to phy*/
bool appAncClientRegister(Task task)
{
    tymAncTaskData *tymanc = TymAncGetTaskData();

    if (NULL == tymanc->clients)
    {
        ancConfig *config = appConfigAnc();
        tymanc->config = config;
        tymanc->clients = TaskList_Create();
        tym_power_on(i2c_device_anc_id);
        stanc3_init(config);
        tymanc->prevAncMode = amcinvalid;
        tymanc->curAncMode = ancoff;
    }

    return TaskList_AddTask(tymanc->clients, task);
}

/*! \brief  Unregister ANC to phy*/
void appAncClientUnregister(Task task)
{
    tymAncTaskData *tymanc = TymAncGetTaskData();
    if(tymanc->clients != NULL)
    {
        TaskList_RemoveTask(tymanc->clients, task);
        if (0 == TaskList_Size(tymanc->clients))
        {
            xprint("appAncClientUnregister");
            stanc3_deinit(tymanc->config);
            tym_i2cdevice_deinit(i2c_device_anc_id);
            tym_power_off(i2c_device_anc_id);
            TaskList_Destroy(tymanc->clients);
            tymanc->clients = NULL;
            tymanc->prevAncMode = amcinvalid;
            tymanc->curAncMode = amcinvalid;
        }
    }
}

void appAncPowerOn(void)
{
    tymAncTaskData *tymanc = TymAncGetTaskData();
    ancConfig *config = appConfigAnc(); 
    tym_power_on(i2c_device_anc_id);   
    stanc3_init(config);
    tymanc->prevAncMode = amcinvalid;
    tymanc->curAncMode = ancoff;     
}

void appAncPowerOff(void)
{
    tymAncTaskData *tymanc = TymAncGetTaskData();
    ancConfig *config = appConfigAnc();     
    stanc3_deinit(config);
    tym_power_off(i2c_device_anc_id);  
    tymanc->prevAncMode = amcinvalid;
    tymanc->curAncMode = amcinvalid;        
}

/*! \brief  stanc3 anc off*/
void stanc3_ancoff(void)
{
    uint8 write_data[2];
    uint8 read_addr;
    uint8 read_data[1];
    anc_config.state = FALSE;
    read_addr = 0x17;
    if(!tym_i2c_read(ANC_CTRL_ADDR,read_addr,read_data,1))
    {
        xprintf("return error tym_i2c_read %d",read_addr);
        return; /* error */
    }
    write_data[0] = 0x17;
    write_data[1] = (read_data[0] | (1 << 1));/*Set ANC_MUTE bit (0x17,D1) to 1*/
    if(!tym_i2c_write(ANC_CTRL_ADDR,write_data,2))
    {
        xprintf("return error tym_i2c_write %d",write_data[0]);
        return; /* error */
    }
    //xprintf("write 0x17 0x%x",write_data[1]);
    /* read address 0x24 */
    read_addr = 0x24;
    if(!tym_i2c_read(ANC_CTRL_ADDR,read_addr,read_data,1))
    {
        xprintf("return error tym_i2c_read %d",read_addr);
        return; /* error */
    }

    write_data[0] = 0x24;
    write_data[1] = (read_data[0] | (1 << 2));/*Set SWITCH_EQ_BANK bit (0x24,D2) set to 1 (audio compensation EQ to Bank 1)*/
    if(!tym_i2c_write(ANC_CTRL_ADDR,write_data,2))
    {
        xprintf("return error tym_i2c_write %d",write_data[0]);
        return; /* error */
    }

    //Swtich to SPK-EQ1 for ANC OFF
    appKymeraSpeakerEqOnOff(TRUE,FALSE);
}

/*! \brief  stanc3 anc on*/
void stanc3_ancon(void)
{
    uint8 write_data[2];
    uint8 read_addr;
    uint8 read_data[1];
    uint32 clock,cur_clock,timeout;
    anc_config.state = TRUE;
    /* read address 0x19 */
    read_addr = 0x19;
    if(!tym_i2c_read(ANC_CTRL_ADDR,read_addr,read_data,1))
    {
        xprintf("return error tym_i2c_read %d",read_addr);
        return; /* error */
    }
    write_data[0] = 0x19;
    write_data[1] = (read_data[0] | (1 << 0));/*Set MIC_BIAS bit (0x19,D0) to 1*/
    //xprintf("write 0x19 0x%x",write_data[1]);
    if(!tym_i2c_write(ANC_CTRL_ADDR,write_data,2))
    {
        xprintf("return error tym_i2c_write %d",write_data[0]);
        return; /* error */
    }
    /*wait 20 ms*/
    clock = VmGetTimerTime();
    timeout = (20*1000);
    while(1)
    {
        cur_clock = VmGetTimerTime();
        if(checktimeout(clock, cur_clock,timeout))
            break;
    }
    /* read address 0x17 */
    read_addr = 0x17;
    if(!tym_i2c_read(ANC_CTRL_ADDR,read_addr,read_data,1))
    {
        xprintf("return error tym_i2c_read %d",read_addr);
        return; /* error */
    }
    write_data[0] = 0x17;
    write_data[1] = (read_data[0] & ~(1 << 1));/*Set ANC_MUTE bit (0x17,D1) to 0*/
    //xprintf("write 0x17 0x%x",write_data[1]);
    if(!tym_i2c_write(ANC_CTRL_ADDR,write_data,2))
    {
        xprintf("return error tym_i2c_write %d",write_data[0]);
        return; /* error */
    }
    /*wait 100 ms*/
    clock = VmGetTimerTime();
    timeout = (100*1000);
    while(1)
    {
        cur_clock = VmGetTimerTime();
        if(checktimeout(clock, cur_clock,timeout))
            break;
    }
    /* read address 0x24 */
    read_addr = 0x24;
    if(!tym_i2c_read(ANC_CTRL_ADDR,read_addr,read_data,1))
    {
        xprintf("return error tym_i2c_read %d",read_addr);
        return; /* error */
    }
    write_data[0] = 0x24;
    write_data[1] = (read_data[0] & ~(2 << 1));/*Set SWITCH_EQ_BANK bit (0x24,D2) set to 0 (audio compensation EQ to Bank 0) */
    //xprintf("write 0x24 0x%x",write_data[1]);
    if(!tym_i2c_write(ANC_CTRL_ADDR,write_data,2))
    {
        xprintf("return error tym_i2c_write %d",write_data[0]);
        return; /* error */
    }

    //Swtich to SPK-EQ2 for ANC ON
    appKymeraSpeakerEqOnOff(FALSE,TRUE);
}

/*! \brief  stanc3 anc vol for factory test*/
uint8 stanc3_ancvol(void)
{
    uint8 read_addr;
    uint8 read_data[1];
    /* read address 0x24 */
    read_addr = 0x15;
    if(!tym_i2c_read(ANC_CTRL_ADDR,read_addr,read_data,1))
    {
        xprintf("return error tym_i2c_read %d",read_addr);
        return 0; /* error */
    }
    return read_data[0];
}

/*! \brief  stanc3 anc write ANC calibration value to PSKey */
void dumpANCWriteToPSKey(void)
{
    int ANCDataNum = SINK_ANC_DATA_BYTES;
    uint8 read_data[ANCDataNum];
    uint16 ps_key[SINK_ANC_PS_SIZE];
    uint8 read_addr;
    int i,retwords;
    for(i = 0;i < ANCDataNum;i++)
    {
        read_addr = i;
        if(!tym_i2c_read(ANC_CTRL_ADDR,read_addr,&read_data[i],1))
        {
            xprintf("return error tym_i2c_read %d",i);
            return; /* error */
        }
    }
    ByteUtilsMemCpyPackString(ps_key,read_data,ANCDataNum);
    xprint("dump calibration value");
    for(i = 0;i < ANCDataNum;i++)
        DEBUG_LOG("0x%x ",read_data[i]);
    retwords = PsStore(PSID_ANCTABLE,ps_key,SINK_ANC_PS_SIZE);
    xprintf("PsStore word %d",retwords);

}

/*! \brief stanc3 ANC calibration diable i2c */
void disable_i2c_for_cal(bool enable)
{
    tym_disable_i2c_control(enable);
}

/*! \brief get external ANC status enable return TRUE, disable return FALSE */
bool getExtAncEnableStatus(void)
{
    return anc_config.state;
}

/*! \brief set volume down and recovery for pop noise */
void stanc3_volumedown(bool enable)
{
    uint8 write_data[2];

    if(enable == 1)
    {
    	/* volume down */
    	write_data[0] = 0x15;
        write_data[1] = 0xF8;
        xprintf("write 0x15 0x%x",write_data[1]);
        if(!tym_i2c_write(ANC_CTRL_ADDR,write_data,2))
        {
            xprintf("return error tym_i2c_write %d",write_data[0]);
            return; /* error */
        }
    }
    else
    {    
        /* volume up */
        write_data[0] = 0x15;
        write_data[1] = 0x10;
        xprintf("write 0x15 0x%x",write_data[1]);
        if(!tym_i2c_write(ANC_CTRL_ADDR,write_data,2))
        {
            xprintf("return error tym_i2c_write %d",write_data[0]);
            return; /* error */
        }
    }	
}

