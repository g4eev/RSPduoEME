// RSPduoEME Copyright 2020 David Warwick G4EEV
//
// This file is part of RSPduoEME.
//
// RSPduoEME is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, either version 3 of the License, or(at your option) any later version.
// RSPduoEME is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have
// received a copy of the GNU General Public License along with RSPduoEMRE, if not, see <https://www.gnu.org/licenses/>.



#ifndef SDRPLAY_API_H
#define SDRPLAY_API_H



//Constant Definitions

#define SDRPLAY_API_VERSION      (float)(3.06)              // also compatible with version 3.07 (see below)

// Re: Hardware/API Compatibility [#140356] 21/08/2020
//
// Hello David,
//
// Where possible we do try to make the API releases backwards compatible. The difference from 3.06 to 3.07 on Windows was
// primarily to fix issues with Bias-T. The other updates were for non-Windows platforms and I think the DebugEnable setting
// changed from an integer to an enum. So basically if you are compatible with 3.06 then you should be for 3.07
//
// Some developers copy the sdrplay_api.dll into their application directory and this is where issues can arise, because that
// gets out of step with the other half of the API that is in the form of a service.
//
// If you use the registry to locate the API, then you should be more immune to API updates. Unless something fundamental changes,
// and then we'll try to give developers as much notice as possible to deal with that case.
//
// Hope that helps.
//
// Best regards,
//
// SDRplay Support


#define SDRPLAY_MAX_DEVICES             (16)                // Maximum devices supported by the API
#define SDRPLAY_MAX_TUNERS_PER_DEVICE   (2)                 // Maximum number of tuners available on one device
#define SDRPLAY_MAX_SER_NO_LEN          (64)                // Maximum length of device serial numbers
#define SDRPLAY_MAX_ROOT_NM_LEN         (128)               // Maximum length of device names

// Supported device IDs

#define SDRPLAY_RSP1_ID                 (1)
#define SDRPLAY_RSP1A_ID                (255)
#define SDRPLAY_RSP2_ID                 (2)
#define SDRPLAY_RSPduo_ID               (3)
#define SDRPLAY_RSPdx_ID                (4)



typedef void *HANDLE;

// Enum types
typedef enum
{
    sdrplay_api_Success               = 0,
    sdrplay_api_Fail                  = 1,
    sdrplay_api_InvalidParam          = 2,
    sdrplay_api_OutOfRange            = 3,
    sdrplay_api_GainUpdateError       = 4,
    sdrplay_api_RfUpdateError         = 5,
    sdrplay_api_FsUpdateError         = 6,
    sdrplay_api_HwError               = 7,
    sdrplay_api_AliasingError         = 8,
    sdrplay_api_AlreadyInitialised    = 9,
    sdrplay_api_NotInitialised        = 10,
    sdrplay_api_NotEnabled            = 11,
    sdrplay_api_HwVerError            = 12,
    sdrplay_api_OutOfMemError         = 13,
    sdrplay_api_ServiceNotResponding  = 14,
    sdrplay_api_StartPending          = 15,
    sdrplay_api_StopPending           = 16,
    sdrplay_api_InvalidMode           = 17,
    sdrplay_api_FailedVerification1   = 18,
    sdrplay_api_FailedVerification2   = 19,
    sdrplay_api_FailedVerification3   = 20,
    sdrplay_api_FailedVerification4   = 21,
    sdrplay_api_FailedVerification5   = 22,
    sdrplay_api_FailedVerification6   = 23,
    sdrplay_api_InvalidServiceVersion = 24
} sdrplay_api_ErrT;


typedef enum                            // Tuner Selected Enumerated Type:
{
    sdrplay_api_Tuner_Neither = 0,
    sdrplay_api_Tuner_A = 1,
    sdrplay_api_Tuner_B = 2,
    sdrplay_api_Tuner_Both = 3,
} sdrplay_api_TunerSelectT;


typedef enum                             // RSPduo Operating Mode Enumerated Type:
{
    sdrplay_api_RspDuoMode_Unknown = 0,
    sdrplay_api_RspDuoMode_Single_Tuner = 1,
    sdrplay_api_RspDuoMode_Dual_Tuner = 2,
    sdrplay_api_RspDuoMode_Master = 4,
    sdrplay_api_RspDuoMode_Slave = 8,
} sdrplay_api_RspDuoModeT;


typedef struct
{
char SerNo[SDRPLAY_MAX_SER_NO_LEN];     // Set by the API on return from
                                        // sdrplay_api_GetDevices() contains the serial
                                        // number of the device
unsigned char hwVer;                    // Set by the API on return from
                                        // sdrplay_api_GetDevices() contains the Hardware
                                        // version of the device
sdrplay_api_TunerSelectT tuner;         // Set by the API on return from
                                        // sdrplay_api_GetDevices() indicating which tuners
                                        // are available.
                                        // Set by the application and used during
                                        // sdrplay_api_SelectDevice() to indicate which
                                        // tuner(s) is to be used.
sdrplay_api_RspDuoModeT rspDuoMode;     // Set by the API on return from
                                        // sdrplay_api_GetDevices() for RSPduo devices
                                        // indicating which modes are available.
                                        // Set by the application and used during
                                        // sdrplay_api_SelectDevice() for RSPduo device to
                                        // indicate which mode is to be used.
double rspDuoSampleFreq;                // Set by the API on return from
                                        // sdrplay_api_GetDevices() for RSPduo slaves
                                        // indicating the sample rate previously set by the
                                        // master.
                                        // Set by the application and used during
                                        // sdrplay_api_SelectDevice() by RSPduo masters to
                                        // indicate required sample rate.
HANDLE dev;                             // Set by the API on return from
                                        // sdrplay_api_SelectDevice() for use in subsequent
                                        // calls to the API. Do not alter!
} sdrplay_api_DeviceT;


//ADC Sampling Frequency Parameters Structure:
typedef struct
{
double          fsHz;           // default: 2000000.0
unsigned char   syncUpdate;     // default: 0
unsigned char   reCal;          // default: 0
} sdrplay_api_FsFreqT;


//Synchronous Update Parameters Structure:
typedef struct
{
unsigned int   sampleNum;      // default: 0
unsigned int   period;         // default: 0
} sdrplay_api_SyncUpdateT;


//Reset Update Operations Structure:
typedef struct
{
unsigned char  resetGainUpdate; // default: 0
unsigned char  resetRfUpdate;   // default: 0
unsigned char  resetFsUpdate;   // default: 0
} sdrplay_api_ResetFlagsT;


//Transfer Mode Enumerated Type:
typedef enum
{
sdrplay_api_ISOCH = 0,
sdrplay_api_BULK = 1
} sdrplay_api_TransferModeT;


//RSP1A RF Notch Control Parameters Structure:
typedef struct
{
unsigned char rfNotchEnable;    // default: 0
unsigned char rfDabNotchEnable; // default: 0
} sdrplay_api_Rsp1aParamsT;


//RSP2 External Reference Control Parameters Structure:
typedef struct
{
unsigned char extRefOutputEn;  // default: 0
} sdrplay_api_Rsp2ParamsT;


//RSPduo External Reference Control Parameters Structure:
typedef struct
{
int extRefOutputEn;            // default: 0
} sdrplay_api_RspDuoParamsT;


//RSPdx Antenna Selection Enumerated Type:
typedef enum
{
sdrplay_api_RspDx_ANTENNA_A = 0,
sdrplay_api_RspDx_ANTENNA_B = 1,
sdrplay_api_RspDx_ANTENNA_C = 2,
} sdrplay_api_RspDx_AntennaSelectT;


//RSPdx Control Parameters Structure:
typedef struct
{
unsigned char                       hdrEnable;          // default: 0
unsigned char                       biasTEnable;        // default: 0
sdrplay_api_RspDx_AntennaSelectT    antennaSel;         // default: sdrplay_api_RspDx_ANTENNA_A
unsigned char                       rfNotchEnable;      // default: 0
unsigned char                       rfDabNotchEnable;   // default: 0
} sdrplay_api_RspDxParamsT;

//Bandwidth Enumerated Type:
typedef enum
{
sdrplay_api_BW_Undefined = 0,
sdrplay_api_BW_0_200 = 200,
sdrplay_api_BW_0_300 = 300,
sdrplay_api_BW_0_600 = 600,
sdrplay_api_BW_1_536 = 1536,
sdrplay_api_BW_5_000 = 5000,
sdrplay_api_BW_6_000 = 6000,
sdrplay_api_BW_7_000 = 7000,
sdrplay_api_BW_8_000 = 8000
} sdrplay_api_Bw_MHzT;


//IF Enumerated Type:
typedef enum
{
sdrplay_api_IF_Undefined = -1,
sdrplay_api_IF_Zero = 0,
sdrplay_api_IF_0_450 = 450,
sdrplay_api_IF_1_620 = 1620,
sdrplay_api_IF_2_048 = 2048
} sdrplay_api_If_kHzT;


//LO Enumerated Type:
typedef enum
{
sdrplay_api_LO_Undefined = 0,
sdrplay_api_LO_Auto = 1,
sdrplay_api_LO_120MHz = 2,
sdrplay_api_LO_144MHz = 3,
sdrplay_api_LO_168MHz = 4
} sdrplay_api_LoModeT;


//Minimum Gain Enumerated Type:
typedef enum
{
sdrplay_api_EXTENDED_MIN_GR = 0,
sdrplay_api_NORMAL_MIN_GR = 20
} sdrplay_api_MinGainReductionT;


//Current Gain Value Structure:
typedef struct
{
float curr;
float max;
float min;
} sdrplay_api_GainValuesT;


//Gain Setting Parameter Structure:
typedef struct
{
int                             gRdB;       // default: 50
unsigned char                   LNAstate;   // default: 0
unsigned char                   syncUpdate; // default: 0
sdrplay_api_MinGainReductionT   minGr;      // default: sdrplay_api_NORMAL_MIN_GR
sdrplay_api_GainValuesT         gainVals;   // output parameter
} sdrplay_api_GainT;


//RF Frequency Parameter Structure:
typedef struct
{
double                  rfHz; // default: 200000000.0
unsigned char           syncUpdate; // default: 0
} sdrplay_api_RfFreqT;


//DC Calibration Paramter Structure:
typedef struct
{
unsigned char                   dcCal;              // default: 3 (Periodic mode)
unsigned char                   speedUp;            // default: 0 (No speedup)
int                             trackTime;          // default: 1 (=> time in uSec = (dcCal * 3 * trackTime) = 9uSec)
int                             refreshRateTime;    // default: 2048 (=> time in uSec = (dcCal * 3 * refreshRateTime) = 18432uSec)
} sdrplay_api_DcOffsetTunerT;


//Tuner Parameter Structure:
typedef struct
{
sdrplay_api_Bw_MHzT         bwType;     // default: sdrplay_api_BW_0_200
sdrplay_api_If_kHzT         ifType;     // default: sdrplay_api_IF_Zero (master) or
                                        // sdrplay_api_IF_0_450 (slave)
sdrplay_api_LoModeT         loMode;     // default: sdrplay_api_LO_Auto
sdrplay_api_GainT           gain;
sdrplay_api_RfFreqT         rfFreq;
sdrplay_api_DcOffsetTunerT  dcOffsetTuner;
} sdrplay_api_TunerParamsT;


//DC Offset Control Parameters Structure:
typedef struct
{
unsigned char               DCenable;   // default: 1
unsigned char               IQenable;   // default: 1
} sdrplay_api_DcOffsetT;


//Decimation Control Parameters Structure:
typedef struct
{
unsigned char               enable;             // default: 0
unsigned char               decimationFactor;   // default: 1
unsigned char               wideBandSignal;     // default: 0
} sdrplay_api_DecimationT;


//AGC Loop Bandwidth Enumerated Type:
typedef enum
{
sdrplay_api_AGC_DISABLE = 0,
sdrplay_api_AGC_100HZ = 1,
sdrplay_api_AGC_50HZ = 2,
sdrplay_api_AGC_5HZ = 3,
sdrplay_api_AGC_CTRL_EN = 4
} sdrplay_api_AgcControlT;


//AGC Control Parameters Structure:
typedef struct
{
sdrplay_api_AgcControlT     enable;             // default: sdrplay_api_AGC_50HZ
int                         setPoint_dBfs;      // default: -60
unsigned short              attack_ms;          // default: 0
unsigned short              decay_ms;           // default: 0
unsigned short              decay_delay_ms;     // default: 0
unsigned short              decay_threshold_dB; // default: 0
int                         syncUpdate;         // default: 0
} sdrplay_api_AgcT;


//ADS-B Configuration Enumerated Type:
typedef enum
{
sdrplay_api_ADSB_DECIMATION = 0,
sdrplay_api_ADSB_NO_DECIMATION_LOWPASS = 1,
sdrplay_api_ADSB_NO_DECIMATION_BANDPASS_2MHZ = 2,
sdrplay_api_ADSB_NO_DECIMATION_BANDPASS_3MHZ = 3
} sdrplay_api_AdsbModeT;


//Control Parameters Structure:
typedef struct
{
sdrplay_api_DcOffsetT       dcOffset;
sdrplay_api_DecimationT     decimation;
sdrplay_api_AgcT            agc;
sdrplay_api_AdsbModeT       adsbMode; //default: sdrplay_api_ADSB_DECIMATION
} sdrplay_api_ControlParamsT;


//RSP1A Bias-T Control Parameters Structure:
typedef struct
{
unsigned char               biasTEnable; // default: 0
} sdrplay_api_Rsp1aTunerParamsT;


//RSP2 AM Port Enumerated Type:
typedef enum
{
sdrplay_api_Rsp2_AMPORT_1 = 1,
sdrplay_api_Rsp2_AMPORT_2 = 0,
} sdrplay_api_Rsp2_AmPortSelectT;


//RSP2 Antenna Selection Enumerated Type:
typedef enum
{
sdrplay_api_Rsp2_ANTENNA_A = 5,
sdrplay_api_Rsp2_ANTENNA_B = 6,
} sdrplay_api_Rsp2_AntennaSelectT;


//RSP2 Tuner Parameters Structure:
typedef struct
{
unsigned char                   biasTEnable;    // default: 0
sdrplay_api_Rsp2_AmPortSelectT  amPortSel;      // default: sdrplay_api_Rsp2_AMPORT_2
sdrplay_api_Rsp2_AntennaSelectT antennaSel;     // default: sdrplay_api_Rsp2_ANTENNA_A
unsigned char                   rfNotchEnable;  // default: 0
} sdrplay_api_Rsp2TunerParamsT;


//RSPduo AM Port Enumerated Type:
typedef enum
{
sdrplay_api_RspDuo_AMPORT_1 = 1,
sdrplay_api_RspDuo_AMPORT_2 = 0,
} sdrplay_api_RspDuo_AmPortSelectT;


//RSPduo Tuner Parameters Structure:
typedef struct
{
unsigned char                       biasTEnable;            // default: 0
sdrplay_api_RspDuo_AmPortSelectT    tuner1AmPortSel;        // default: sdrplay_api_RspDuo_AMPORT_2
unsigned char                       tuner1AmNotchEnable;    // default: 0
unsigned char                       rfNotchEnable;          // default: 0
unsigned char                       rfDabNotchEnable;       // default: 0
} sdrplay_api_RspDuoTunerParamsT;


//RSPdx HDR Mode Bandwidth Enumerated Type:
typedef enum
{
sdrplay_api_RspDx_HDRMODE_BW_0_200 = 0,
sdrplay_api_RspDx_HDRMODE_BW_0_500 = 1,
sdrplay_api_RspDx_HDRMODE_BW_1_200 = 2,
sdrplay_api_RspDx_HDRMODE_BW_1_700 = 3,
} sdrplay_api_RspDx_HdrModeBwT;


//RSPdx Tuner Parameters Structure:
typedef struct
{
sdrplay_api_RspDx_HdrModeBwT        hdrBw;      // default: sdrplay_api_RspDx_HDRMODE_BW_1_700
} sdrplay_api_RspDxTunerParamsT;


//Non-Receive Channel Related Device Parameters:
typedef struct
{
double                      ppm;            // default: 0.0
sdrplay_api_FsFreqT         fsFreq;
sdrplay_api_SyncUpdateT     syncUpdate;
sdrplay_api_ResetFlagsT     resetFlags;
sdrplay_api_TransferModeT   mode;           // default: sdrplay_api_ISOCH
unsigned int                samplesPerPkt;  // default: 0 (output param)
sdrplay_api_Rsp1aParamsT    rsp1aParams;
sdrplay_api_Rsp2ParamsT     rsp2Params;
sdrplay_api_RspDuoParamsT   rspDuoParams;
sdrplay_api_RspDxParamsT    rspDxParams;
} sdrplay_api_DevParamsT;

//Receive Channel Structure:
typedef struct
{
sdrplay_api_TunerParamsT        tunerParams;
sdrplay_api_ControlParamsT      ctrlParams;
sdrplay_api_Rsp1aTunerParamsT   rsp1aTunerParams;
sdrplay_api_Rsp2TunerParamsT    rsp2TunerParams;
sdrplay_api_RspDuoTunerParamsT  rspDuoTunerParams;
sdrplay_api_RspDxTunerParamsT   rspDxTunerParams;
} sdrplay_api_RxChannelParamsT;

//Device Parameters Structure:
typedef struct
{
sdrplay_api_DevParamsT       *devParams;    // All parameters for a single device (except tuner parameters)
sdrplay_api_RxChannelParamsT *rxChannelA;   // First tuner parameters for all devices
sdrplay_api_RxChannelParamsT *rxChannelB;   // Second tuner parameters for RSPduo
} sdrplay_api_DeviceParamsT;

//Events Enumerated Type :
typedef enum
{
    sdrplay_api_GainChange = 0,
    sdrplay_api_PowerOverloadChange = 1,
    sdrplay_api_DeviceRemoved = 2,
    sdrplay_api_RspDuoModeChange = 3,
} sdrplay_api_EventT;

//Streaming Data Parameter Callback Structure :
typedef struct
{
    unsigned int firstSampleNum;
    int grChanged;
    int rfChanged;
    int fsChanged;
    unsigned int numSamples;
} sdrplay_api_StreamCbParamsT;

//Event Callback Structure :
typedef struct
{
    unsigned int gRdB;
    unsigned int lnaGRdB;
    double currGain;
} sdrplay_api_GainCbParamT;

//Power Overload Event Enumerated Type :
typedef enum
{
    sdrplay_api_Overload_Detected = 0,
    sdrplay_api_Overload_Corrected = 1,
} sdrplay_api_PowerOverloadCbEventIdT;

//Power Overload Structure :
typedef struct
{
    sdrplay_api_PowerOverloadCbEventIdT powerOverloadChangeType;
} sdrplay_api_PowerOverloadCbParamT;

//RSPduo Event Enumerated Type :
typedef enum
{
    sdrplay_api_MasterInitialised = 0,
    sdrplay_api_SlaveAttached = 1,
    sdrplay_api_SlaveDetached = 2,
    sdrplay_api_SlaveInitialised = 3,
    sdrplay_api_SlaveUninitialised = 4,
    sdrplay_api_MasterDllDisappeared = 5,
    sdrplay_api_SlaveDllDisappeared = 6,
} sdrplay_api_RspDuoModeCbEventIdT;

//RSPduo Structure :
typedef struct
{
    sdrplay_api_RspDuoModeCbEventIdT modeChangeType;
} sdrplay_api_RspDuoModeCbParamT;

//Combination of Event Callback Structures :
typedef union
{
    sdrplay_api_GainCbParamT gainParams;
    sdrplay_api_PowerOverloadCbParamT powerOverloadParams;
    sdrplay_api_RspDuoModeCbParamT rspDuoModeParams;
} sdrplay_api_EventParamsT;

//2.10.3 Callback Function Prototypes
typedef void(*sdrplay_api_StreamCallback_t)(short *xi,
    short *xq,
    sdrplay_api_StreamCbParamsT *params,
    unsigned int numSamples,
    unsigned int reset,
    void *cbContext);
typedef void(*sdrplay_api_EventCallback_t)(sdrplay_api_EventT eventId,
    sdrplay_api_TunerSelectT tuner,
    sdrplay_api_EventParamsT *params,
    void *cbContext);


//Callback Function Definition Structure :
typedef struct
{
    sdrplay_api_StreamCallback_t StreamACbFn;
    sdrplay_api_StreamCallback_t StreamBCbFn;
    sdrplay_api_EventCallback_t EventCbFn;
} sdrplay_api_CallbackFnsT;

//Extended Error Message Structure
typedef struct
{
    char file[256];				// API file where the error occurred
    char function[256];			// API function that the error occurred in
    int line;					// line number that the error occurred on
    char message[1024];			// Readable API error message to display
} sdrplay_api_ErrorInfoT;

//Update Enumerated Type :
typedef enum
{
    sdrplay_api_Update_None =							0x00000000,
    // Reasons for master only mode
    sdrplay_api_Update_Dev_Fs =							0x00000001,
    sdrplay_api_Update_Dev_Ppm =						0x00000002,
    sdrplay_api_Update_Dev_SyncUpdate =					0x00000004,
    sdrplay_api_Update_Dev_ResetFlags =					0x00000008,
    sdrplay_api_Update_Rsp1a_BiasTControl =				0x00000010,
    sdrplay_api_Update_Rsp1a_RfNotchControl =			0x00000020,
    sdrplay_api_Update_Rsp1a_RfDabNotchControl =		0x00000040,
    sdrplay_api_Update_Rsp2_BiasTControl =				0x00000080,
    sdrplay_api_Update_Rsp2_AmPortSelect =				0x00000100,
    sdrplay_api_Update_Rsp2_AntennaControl =			0x00000200,
    sdrplay_api_Update_Rsp2_RfNotchControl =			0x00000400,
    sdrplay_api_Update_Rsp2_ExtRefControl =				0x00000800,
    sdrplay_api_Update_RspDuo_ExtRefControl =			0x00001000,
    sdrplay_api_Update_Master_Spare_1 =					0x00002000,
    sdrplay_api_Update_Master_Spare_2 =					0x00004000,
    // Reasons for master and slave mode
    // Note: sdrplay_api_Update_Tuner_Gr MUST be the first value defined in this section!
    sdrplay_api_Update_Tuner_Gr =						0x00008000,
    sdrplay_api_Update_Tuner_GrLimits =					0x00010000,
    sdrplay_api_Update_Tuner_Frf =						0x00020000,
    sdrplay_api_Update_Tuner_BwType =					0x00040000,
    sdrplay_api_Update_Tuner_IfType =					0x00080000,
    sdrplay_api_Update_Tuner_DcOffset =					0x00100000,
    sdrplay_api_Update_Tuner_LoMode =					0x00200000,
    sdrplay_api_Update_Ctrl_DCoffsetIQimbalance =		0x00400000,
    sdrplay_api_Update_Ctrl_Decimation =				0x00800000,
    sdrplay_api_Update_Ctrl_Agc =						0x01000000,
    sdrplay_api_Update_Ctrl_AdsbMode =					0x02000000,
    sdrplay_api_Update_Ctrl_OverloadMsgAck =			0x04000000,
    sdrplay_api_Update_RspDuo_BiasTControl =			0x08000000,
    sdrplay_api_Update_RspDuo_AmPortSelect =			0x10000000,
    sdrplay_api_Update_RspDuo_Tuner1AmNotchControl =	0x20000000,
    sdrplay_api_Update_RspDuo_RfNotchControl =			0x40000000,
    sdrplay_api_Update_RspDuo_RfDabNotchControl =		0x80000000,
} sdrplay_api_ReasonForUpdateT;

typedef enum
{
    sdrplay_api_Update_Ext1_None =						0x00000000,
    // Reasons for master only mode
    sdrplay_api_Update_RspDx_HdrEnable =				0x00000001,
    sdrplay_api_Update_RspDx_BiasTControl =				0x00000002,
    sdrplay_api_Update_RspDx_AntennaControl =			0x00000004,
    sdrplay_api_Update_RspDx_RfNotchControl =			0x00000008,
    sdrplay_api_Update_RspDx_RfDabNotchControl =		0x00000010,
    sdrplay_api_Update_RspDx_HdrBw =					0x00000020,
    // Reasons for master and slave mode
} sdrplay_api_ReasonForUpdateExtension1T;





// Function Prototypes:
extern "C"
{
sdrplay_api_ErrT       sdrplay_api_Open(void);
sdrplay_api_ErrT       sdrplay_api_Close(void);
sdrplay_api_ErrT       sdrplay_api_ApiVersion(float *apiVer);
sdrplay_api_ErrT       sdrplay_api_LockDeviceApi(void);
sdrplay_api_ErrT       sdrplay_api_UnlockDeviceApi(void);
sdrplay_api_ErrT       sdrplay_api_GetDevices(sdrplay_api_DeviceT *devices,unsigned int *numDevs, unsigned int maxDevs);
sdrplay_api_ErrT       sdrplay_api_SelectDevice(sdrplay_api_DeviceT *device);
sdrplay_api_ErrT       sdrplay_api_ReleaseDevice(sdrplay_api_DeviceT *device);
const char*            sdrplay_api_GetErrorString(sdrplay_api_ErrT err);
sdrplay_api_ErrorInfoT sdrplay_api_GetLastError(sdrplay_api_DeviceT *device);
sdrplay_api_ErrT       sdrplay_api_DebugEnable(HANDLE dev, unsigned int enable);
sdrplay_api_ErrT       sdrplay_api_GetDeviceParams(HANDLE dev, sdrplay_api_DeviceParamsT **deviceParams);
sdrplay_api_ErrT       sdrplay_api_Init(HANDLE dev, sdrplay_api_CallbackFnsT *callbackFns, void *cbContext);
sdrplay_api_ErrT       sdrplay_api_Uninit(HANDLE dev);
sdrplay_api_ErrT       sdrplay_api_Update(HANDLE dev,sdrplay_api_TunerSelectT tuner,sdrplay_api_ReasonForUpdateT reasonForUpdate,
                                                                     sdrplay_api_ReasonForUpdateExtension1T reasonForUpdateExt1);
sdrplay_api_ErrT       sdrplay_api_SwapRspDuoActiveTuner(HANDLE dev, sdrplay_api_TunerSelectT *currentTuner,
                                                           sdrplay_api_RspDuo_AmPortSelectT tuner1AmPortSel);
sdrplay_api_ErrT       sdrplay_api_SwapRspDuoDualTunerModeSampleRate(HANDLE dev, double *currentSampleRate);
}



#endif //SDRPLAY_API_H
