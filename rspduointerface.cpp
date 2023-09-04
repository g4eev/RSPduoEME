// RSPduoEME Copyright 2020 David Warwick G4EEV
//
// This file is part of RSPduoEME.
//
// RSPduoEME is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, either version 3 of the License, or(at your option) any later version.
// RSPduoEME is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have
// received a copy of the GNU General Public License along with RSPduoEMRE, if not, see <https://www.gnu.org/licenses/>.


// These API routinese are derived from the sample code provided by SDRplay for interfacing with the sdrplay_api DLL
// and are subject to the following licence / legal notices:

// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following
// conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the distribution.
// 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products
// derived from this software without specific prior written permission.
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
// FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
// OF SUCH DAMAGE.
// SDRPlay modules use a Mirics chipset and software. The information supplied hereunder is provided to you by SDRPlay
// under license from Mirics. Mirics hereby grants you a perpetual, worldwide, royalty free license to use the information herein
// for the purpose of designing software that utilizes SDRPlay modules, under the following conditions:
// There are no express or implied copyright licenses granted hereunder to design or fabricate any integrated circuits or
// integrated circuits based on the information in this document. Mirics reserves the right to make changes without further notice
// to any of its products. Mirics makes no warranty, representation or guarantee regarding the suitability of its products for any
// particular purpose, nor does Mirics assume any liability arising out of the application or use of any product or circuit, and
// specifically disclaims any and all liability, including without limitation consequential or incidental damages. Typical parameters
// that may be provided in Mirics data sheets and/or specifications can and do vary in different applications and actual
// performance may vary over time. All operating parameters must be validated for each customer application by the buyer’s
// technical experts. SDRPlay and Mirics products are not designed, intended, or authorized for use as components in systems
// intended for surgical implant into the body, or other applications intended to support or sustain life, or for any other application
// in which the failure of the Mirics product could create a situation where personal injury or death may occur. Should Buyer
// purchase or use SDRPlay or Mirics products for any such unintended or unauthorized application, Buyer shall indemnify and
// hold both SDRPlay and Mirics and their officers, employees, subsidiaries, affiliates, and distributors harmless against all
// claims, costs, damages, and expenses, and reasonable attorney fees arising out of, directly or indirectly, any claim of personal
// injury or death associated with such unintended or unauthorized use, even if such claim alleges that either SDRPlay or Mirics
// were negligent regarding the design or manufacture of the part. Mirics FlexiRF™, Mirics FlexiTV™ and Mirics™ are
// trademarks of Mirics.
// SDRPlay is the trading name of SDRPlay Limited a company registered in England # 09035244.
// Mirics is the trading name of Mirics Limited a company registered in England # 05046393





#include "rspduointerface.h"
#include "sdrplay_api.h"
#include "windows.h"

#include <QDebug>
#include <QString>
#include <QList>



// Global definitions outside of any class

void StreamACallback(short *xi, short *xq, sdrplay_api_StreamCbParamsT *params, unsigned int numSamples, unsigned int reset, void *cbContext);
void StreamBCallback(short *xi, short *xq, sdrplay_api_StreamCbParamsT *params, unsigned int numSamples, unsigned int reset, void *cbContext);
void EventCallback(sdrplay_api_EventT eventId, sdrplay_api_TunerSelectT tuner, sdrplay_api_EventParamsT *params, void *cbContext);


sdrplay_api_DeviceT devs[6];
sdrplay_api_DeviceParamsT *deviceParams = NULL;
sdrplay_api_CallbackFnsT cbFns;
sdrplay_api_RxChannelParamsT *chParams;
sdrplay_api_DeviceT *chosenDevice = NULL;
int masterInitialised = 0;
int slaveUninitialised = 0;


int Buffers = BUFFERS;
int InputBufferSize = INPUT_BUFFER_SIZE;

short (*B_InputBuffer)[INPUT_BUFFER_SIZE];      // for input buffer array pointers [BUFFERS][INPUT_BUFFER_SIZE]
short (*A_InputBuffer)[INPUT_BUFFER_SIZE];      // array is allocated with 'new' command in RSPduoInterface

int A_LastBuffer;                               // index to last updated input buffers
int B_LastBuffer;

QList<QString> MessageList;                     // Qlist to store messages for ststus display
int OverloadFlag = 0;                           // RSPduo Overload condition = 1, else 0

// Callback functions outside of any class

void StreamACallback(short *xi, short *xq, sdrplay_api_StreamCbParamsT *params, unsigned int numSamples, unsigned int reset, void *cbContext)
{
    QString MessageString;

    if (reset)
    {
        //MessageString = QString::asprintf("StreamACallback: numSamples=%d", numSamples);
        //qDebug() << MessageString;
        //MessageList.append(MessageString); // send to Status Display
    }
    // Process stream callback data here

    static int CurrentIndex = 0;  // current index into Buffer
    static int BufferNo = 0;      // current Buffer

    for(int loop = 0; loop < numSamples; loop++)
    {
        A_InputBuffer[BufferNo][CurrentIndex] = xi[loop];
        CurrentIndex++;
        if(CurrentIndex >= INPUT_BUFFER_SIZE)
        {
            CurrentIndex = 0;  // when current buffer is full, start another.
            A_LastBuffer = BufferNo;
            BufferNo++;
            if(BufferNo >= BUFFERS) BufferNo = 0;
        }

    }

    return;

}


void StreamBCallback(short *xi, short *xq, sdrplay_api_StreamCbParamsT *params, unsigned int numSamples, unsigned int reset, void *cbContext)
{
    QString MessageString;

    if (reset)
    {
       //MessageString = QString::asprintf("StreamBCallback: numSamples=%d", numSamples);
       //qDebug() << MessageString;
       //MessageList.append(MessageString); // send to Status Display
    }
    //     Process stream callback data here - this callback will only be used in dual tuner mode

    static int CurrentIndex = 0;  // current index into Buffer
    static int BufferNo = 0;      // current Buffer

    for(int loop = 0; loop < numSamples; loop++)
    {
        B_InputBuffer[BufferNo][CurrentIndex] = xi[loop];
        CurrentIndex++;
        if(CurrentIndex >= INPUT_BUFFER_SIZE)
        {
            CurrentIndex = 0;  // when current buffer is full, start another.
            B_LastBuffer = BufferNo;
            BufferNo++;
            if(BufferNo >= BUFFERS) BufferNo = 0;
        }

    }

    return;
}


void EventCallback(sdrplay_api_EventT eventId, sdrplay_api_TunerSelectT tuner, sdrplay_api_EventParamsT *params, void *cbContext)
{
    QString MessageString;

    switch (eventId)
    {
    case sdrplay_api_GainChange:
            MessageString = QString::asprintf("GainChange: tuner=%s gRdB=%d lnaGRdB=%d systemGain=%.2f",
                                    (tuner == sdrplay_api_Tuner_A) ? "A" : "B", params->gainParams.gRdB,
                                               params->gainParams.lnaGRdB, params->gainParams.currGain);
           qDebug() << MessageString;
           MessageList.append(MessageString); // send to Status Display

        break;
    case sdrplay_api_PowerOverloadChange:
            MessageString = QString::asprintf("PowerOverloadChange: %s %s",(tuner == sdrplay_api_Tuner_A) ?
                             "Tuner_A" : "Tuner_B", (params->powerOverloadParams.powerOverloadChangeType ==
                                sdrplay_api_Overload_Detected) ? "Overload_Detected" :"Overload_Corrected");

            qDebug() << MessageString;

            // set or reset Overload flag for Mainwindow display
            if(params->powerOverloadParams.powerOverloadChangeType == sdrplay_api_Overload_Detected) OverloadFlag = 1;
            else OverloadFlag = 0;

            // Send update message to acknowledge power overload message received
            sdrplay_api_Update(chosenDevice->dev, tuner, sdrplay_api_Update_Ctrl_OverloadMsgAck,
            sdrplay_api_Update_Ext1_None);
            break;

    case sdrplay_api_RspDuoModeChange:
            MessageString = QString::asprintf("sdrplay_api_EventCb: %s, tuner=%s modeChangeType=%s",
            "RspDuoModeChange", (tuner == sdrplay_api_Tuner_A) ?
            "Tuner_A" : "Tuner_B",
            (params->rspDuoModeParams.modeChangeType == sdrplay_api_MasterInitialised) ?
            "MasterInitialised" :
            (params->rspDuoModeParams.modeChangeType == sdrplay_api_SlaveAttached) ?
            "SlaveAttached" :
            (params->rspDuoModeParams.modeChangeType == sdrplay_api_SlaveDetached) ?
            "SlaveDetached" :
            (params->rspDuoModeParams.modeChangeType == sdrplay_api_SlaveInitialised) ?
            "SlaveInitialised" :
            (params->rspDuoModeParams.modeChangeType == sdrplay_api_SlaveUninitialised) ?
            "SlaveUninitialised" :
            (params->rspDuoModeParams.modeChangeType == sdrplay_api_MasterDllDisappeared) ?
            "MasterDllDisappeared" :
            (params->rspDuoModeParams.modeChangeType == sdrplay_api_SlaveDllDisappeared) ?
            "SlaveDllDisappeared" : "unknown type");

            qDebug() << MessageString;
            MessageList.append(MessageString); // send to Status Display

            if (params->rspDuoModeParams.modeChangeType == sdrplay_api_MasterInitialised)
                masterInitialised = 1;
            if (params->rspDuoModeParams.modeChangeType == sdrplay_api_SlaveUninitialised)
            slaveUninitialised = 1;
            break;

    case sdrplay_api_DeviceRemoved:
           MessageString = QString::asprintf("sdrplay_api_EventCb: %s", "sdrplay_api_DeviceRemoved");
           qDebug() << MessageString;
           MessageList.append(MessageString); // send to Status Display
           break;

    default:
           MessageString = QString::asprintf("sdrplay_api_EventCb: %d, unknown event", eventId);
           qDebug() << MessageString;
           MessageList.append(MessageString); // send to Status Display
           break;
    }
}


//*************************************************************************************************************************//

// RSPduoInterface class definitions below:


RSPduoInterface::RSPduoInterface(QWidget *parent) : QWidget(parent)
{

        A_InputBuffer = new short[BUFFERS][INPUT_BUFFER_SIZE];   // allocate Input Buffer arrays
        B_InputBuffer = new short[BUFFERS][INPUT_BUFFER_SIZE];

        Timer = new QTimer;
        connect(Timer, SIGNAL(timeout()), this, SLOT(on_Timeout(void)));
        Timer->start(100); // 100mS
}

RSPduoInterface::~RSPduoInterface()
{
    delete A_InputBuffer;
    delete B_InputBuffer;
    Timer->stop();
    delete Timer;
}

void RSPduoInterface::on_Timeout(void)
{
    // loop to output any status messages held in MessageList
    while(!MessageList.isEmpty())
    {
        emit Status(MessageList.takeFirst());
    }
}


void RSPduoInterface::Start(int LOfrequeccy, int gRdB_Gain_A, int gRdB_Gain_B,int LNAstate)
{

    int reqTuner = 0;                  // Set Tuner (A=0   B=1)
    int duoMode = 2;                   // Set Dual Mode (Master = 0  Slave = 1 Dual = 2)
    int SampleFrequency = 4000000;     // Set sample Frequency (Hz)

    char c;
    unsigned int chosenIdx = 0;
    unsigned int ndev;
    int i;
    float ver = 0.0;
    sdrplay_api_ErrT err;
    QString MessageString;


    // Open API

    if ((err = sdrplay_api_Open()) != sdrplay_api_Success)
    {
        MessageString = QString::asprintf("sdrplay_api_Open failed %s", sdrplay_api_GetErrorString(err));
        emit Status(MessageString);
    }
    else
    {
//        // Enable debug logging output
//        MessageString = QString::asprintf("Enabling debug logger output");
//        emit Status(MessageString);
//
//        if ((err = sdrplay_api_DebugEnable(NULL, 1)) != sdrplay_api_Success)
//        {
//            MessageString = QString::asprintf("sdrplay_api_DebugEnable failed %s", sdrplay_api_GetErrorString(err));
//            emit Status(MessageString);
//        }

        // Check API Versions Match

        if ((err = sdrplay_api_ApiVersion(&ver)) != sdrplay_api_Success)
        {
            MessageString = QString::asprintf("sdrplay_api_ApiVersion failed %s", sdrplay_api_GetErrorString(err));
            emit Status(MessageString);
        }

        if (ver < SDRPLAY_API_VERSION)
        {
            MessageString = QString::asprintf("SDRplay API versions don't match (local=%.2f dll=%.2f)", SDRPLAY_API_VERSION, ver);
            emit Status(MessageString);
            goto CloseApi;
        }
        else
        {
            MessageString = QString::asprintf("SDRplay API version %.2f found", ver);
            emit Status(MessageString);
        }

        MessageString = QString::asprintf("Searching for available RSP devices");
        emit Status(MessageString);

        // Lock API While Device Selection is Performed

        sdrplay_api_LockDeviceApi();

        // Fetch List of Available Devices

        if ((err = sdrplay_api_GetDevices(devs, &ndev, sizeof(devs) / sizeof(sdrplay_api_DeviceT))) != sdrplay_api_Success)
        {
            MessageString = QString::asprintf("sdrplay_api_GetDevices failed %s", sdrplay_api_GetErrorString(err));
            emit Status(MessageString);
            goto UnlockDeviceAndCloseApi;
        }

        // Check if any devices are found, exit if not
        if(ndev == 0)
        {
            MessageString = "No devices found!";
            emit Status(MessageString);
            goto CloseApi;
        }

        if (ndev > 0)
        {
            for (i = 0; i < (int)ndev; i++)
            {
                if (devs[i].hwVer == SDRPLAY_RSPduo_ID)
                {
                    MessageString = QString::asprintf("Found Dev%d: SerNo=%s hwVer=%d tuner=0x%.2x rspDuoMode=0x%.2x", i,
                                                       devs[i].SerNo, devs[i].hwVer , devs[i].tuner, devs[i].rspDuoMode);
                emit Status(MessageString);
                }
                else
                    {
                    MessageString = QString::asprintf("Found Dev%d: SerNo=%s hwVer=%d tuner=0x%.2x", i, devs[i].SerNo,
                                                                                        devs[i].hwVer, devs[i].tuner);
                    emit Status(MessageString);
                }
            }

            // Choose Device

            // Pick first RSPduo
            for (i = 0; i < (int)ndev; i++)
            {
                if (devs[i].hwVer == SDRPLAY_RSPduo_ID)
                {
                    chosenIdx = i;
                break;
                }
            }

            if (i == ndev)
            {
                MessageString = QString::asprintf("Couldn't find a suitable device to open - exiting");
                emit Status(MessageString);
                goto UnlockDeviceAndCloseApi;
            }

            chosenDevice = &devs[chosenIdx];

            // If chosen device is an RSPduo, assign additional fields
            if (chosenDevice->hwVer == SDRPLAY_RSPduo_ID)
            {
                // If master device is available, select device as master
                if (chosenDevice->rspDuoMode & sdrplay_api_RspDuoMode_Master)
                    {
                        // Select tuner based on user input (or default to TunerA)
                        chosenDevice->tuner = sdrplay_api_Tuner_A;
                        if (reqTuner == 1) chosenDevice->tuner = sdrplay_api_Tuner_B;
                        // Set operating mode
                        if (duoMode == 1) // Single tuner mode
                        {
                            chosenDevice->rspDuoMode = sdrplay_api_RspDuoMode_Single_Tuner;
                            MessageString = QString::asprintf("Selected Dev%d rspDuoMode=0x%.2x tuner=0x%.2x", chosenIdx,
                                                                          chosenDevice->rspDuoMode, chosenDevice->tuner);
                            emit Status(MessageString);
                        }
                        else if(duoMode == 0) // Master mode
                        {
                            chosenDevice->rspDuoMode = sdrplay_api_RspDuoMode_Master;
                            // Need to specify sample frequency in master/slave mode
                            chosenDevice->rspDuoSampleFreq = 6000000.0;
                            MessageString = QString::asprintf("Selected Dev%d: rspDuoMode=0x%.2x tuner=0x%.2x rspDuoSampleFreq=%.1f",
                                                                                                 chosenIdx, chosenDevice->rspDuoMode,
                            chosenDevice->tuner, chosenDevice->rspDuoSampleFreq);
                            emit Status(MessageString);
                        }
                        else if(duoMode == 2)  // Dual Tuner Mode
                        {
                            chosenDevice->tuner = sdrplay_api_Tuner_Both;
                            chosenDevice->rspDuoMode = sdrplay_api_RspDuoMode_Dual_Tuner;
                            // Need to specify sample frequency in master/slave mode
                            chosenDevice->rspDuoSampleFreq = SampleFrequency;
                            MessageString = QString::asprintf("Selected Dev%d: rspDuoMode=0x%.2x  tuner=0x%.2x  rspDuoSampleFreq=%7.0f",
                                                                                                    chosenIdx, chosenDevice->rspDuoMode,
                            chosenDevice->tuner, chosenDevice->rspDuoSampleFreq);
                            emit Status(MessageString);
                        }

                    }
                    else // Only slave device available
                    {
                        // Shouldn't change any parameters for slave device
                    }
            }
            // Select Chosen Device

            if ((err = sdrplay_api_SelectDevice(chosenDevice)) != sdrplay_api_Success)
            {
                MessageString = QString::asprintf("sdrplay_api_SelectDevice failed %s", sdrplay_api_GetErrorString(err));
                emit Status(MessageString);
                goto UnlockDeviceAndCloseApi;
            }

            // Unlock API now that device is selected
            sdrplay_api_UnlockDeviceApi();

            // Retrieve Device Parameters so they can be changed if wanted

            if ((err = sdrplay_api_GetDeviceParams(chosenDevice->dev, &deviceParams)) != sdrplay_api_Success)
            {
                MessageString = QString::asprintf("sdrplay_api_GetDeviceParams failed %s",
                                                         sdrplay_api_GetErrorString(err));
                emit Status(MessageString);
                goto CloseApi;
            }
            // Check for NULL pointers before changing settings
            if (deviceParams == NULL)
            {
                MessageString = QString::asprintf("sdrplay_api_GetDeviceParams returned NULL deviceParams pointer");
                emit Status(MessageString);
                goto CloseApi;
            }
            // Configure Device Parameters

            if (deviceParams->devParams != NULL)
            {
                // This will be NULL for slave devices, only the master can change these parameters
                // Only need to update non-default settings
                if ((duoMode == 0)||(duoMode == 2))
                {
                    // Change from default Fs
                    deviceParams->devParams->fsFreq.fsHz = SampleFrequency;
                }
                else
                {
                    // Can't change Fs in master/slave mode
                }
            }
            // Configure tuner parameters (depends on selected Tuner which parameters to use)
                chParams = (chosenDevice->tuner == sdrplay_api_Tuner_B) ? deviceParams->rxChannelB : deviceParams->rxChannelA;
            if (chParams != NULL)
                {
                chParams->tunerParams.rfFreq.rfHz = LOfrequeccy;              // set centre frequency
                chParams->tunerParams.bwType = sdrplay_api_BW_0_200;          // set IF bandwith
                chParams->tunerParams.ifType =  sdrplay_api_IF_0_450;         // set IF mode
                if (duoMode == 1) // Change single tuner mode to ZIF
                {
                    chParams->tunerParams.ifType = sdrplay_api_IF_Zero;
                }
                chParams->tunerParams.gain.gRdB = gRdB_Gain_A;                // set If gain reduction
                chParams->tunerParams.gain.LNAstate = LNAstate;               // set LNA state

                // Disable AGC
                chParams->ctrlParams.agc.enable = sdrplay_api_AGC_DISABLE;    // disable AGC
                chParams->ctrlParams.decimation.enable = 1;                   // enable decimation
                chParams->ctrlParams.decimation.decimationFactor = 2;         // by factor 2
            }
            else
            {
                printf("sdrplay_api_GetDeviceParams returned NULL chParams pointer");
                goto CloseApi;
            }

            // Assign callback functions to be passed to sdrplay_api_Init()

            cbFns.StreamACbFn = StreamACallback;
            cbFns.StreamBCbFn = StreamBCallback;
            cbFns.EventCbFn = EventCallback;

            // Now we're ready to start by calling the initialisation function
            // This will configure the device and start streaming

            if ((err = sdrplay_api_Init(chosenDevice->dev, &cbFns, NULL)) != sdrplay_api_Success)
            {
                MessageString = QString::asprintf("sdrplay_api_Init failed %s", sdrplay_api_GetErrorString(err));
                emit Status(MessageString);
                if (err == sdrplay_api_StartPending) // This can happen if we're starting in master/slave mode as a slave and the master is not yet running
                {
                    while (1)
                    {
                        Sleep(1000);
                        if (masterInitialised) // Keep polling flag set in event callback until the master is initialised
                        {
                            // Redo call - should succeed this time
                            if ((err = sdrplay_api_Init(chosenDevice->dev, &cbFns, NULL)) != sdrplay_api_Success)
                            {
                                MessageString = QString::asprintf("sdrplay_api_Init failed %s", sdrplay_api_GetErrorString(err));
                                emit Status(MessageString);
                            }
                        goto CloseApi;
                        }
                        MessageString = QString::asprintf("Waiting for master to initialise");
                        emit Status(MessageString);
                    }
                }
                else
                {
                    MessageString = QString::asprintf("Error initalising device");
                    emit Status(MessageString);
                    goto CloseApi;
                }
            }

        MessageString = QString::asprintf("Init: LO=%9.0f BW=%i  IF=%i Dec=%i IFgain=%i LNAgain=%i",
                        chParams->tunerParams.rfFreq.rfHz,chParams->tunerParams.bwType, chParams->tunerParams.ifType,
                        chParams->ctrlParams.decimation.decimationFactor, chParams->tunerParams.gain.gRdB,
                        chParams->tunerParams.gain.LNAstate);
        emit Status(MessageString);

        ChangeIFGainB(gRdB_Gain_A, gRdB_Gain_B); // set If gain values for both tuners

        return;
        }

    UnlockDeviceAndCloseApi:
        // Unlock API
        if ((err = sdrplay_api_UnlockDeviceApi()) != sdrplay_api_Success)
        {
            MessageString = QString::asprintf("sdrplay_api_UnlockDeviceApi failed %s", sdrplay_api_GetErrorString(err));
            emit Status(MessageString);
        }
        CloseApi:
        // Close API
        if ((err = sdrplay_api_Close()) != sdrplay_api_Success)
        {
            MessageString = QString::asprintf("sdrplay_api_Close failed %s", sdrplay_api_GetErrorString(err));
            emit Status(MessageString);
        }

    }

}


void RSPduoInterface::Stop(void)
{

    QString MessageString;
    sdrplay_api_ErrT err;

    // Finished with device so uninitialise it
    if ((err = sdrplay_api_Uninit(chosenDevice->dev)) != sdrplay_api_Success)
    {
        MessageString = QString::asprintf("sdrplay_api_Uninit failed %s\n", sdrplay_api_GetErrorString(err));
        emit Status(MessageString);
        if (err == sdrplay_api_StopPending)
        {
            // We’re stopping in master/slave mode as a master and the slave is still running
            while (1)
            {
                Sleep(1000);
                if (slaveUninitialised)
                {
                    // Keep polling flag set in event callback until the slave is uninitialised
                    // Repeat call - should succeed this time
                    if ((err = sdrplay_api_Uninit(chosenDevice->dev)) !=
                        sdrplay_api_Success)
                    {
                        MessageString = QString::asprintf("sdrplay_api_Uninit failed %s",
                                                        sdrplay_api_GetErrorString(err));
                        emit Status(MessageString);
                    }
                    slaveUninitialised = 0;
                    goto CloseApi;
                }
                MessageString = QString::asprintf("Waiting for slave to uninitialise");
                emit Status(MessageString);
            }
        }
        goto CloseApi;
    }
    // Release device (make it available to other applications)
    sdrplay_api_ReleaseDevice(chosenDevice);

    UnlockDeviceAndCloseApi:
    // Unlock API   
    if ((err = sdrplay_api_UnlockDeviceApi()) != sdrplay_api_Success)
    {
        MessageString = QString::asprintf("sdrplay_api_UnlockDeviceApi failed %s", sdrplay_api_GetErrorString(err));
        emit Status(MessageString);
    }
    CloseApi:
    // Close API
    if ((err = sdrplay_api_Close()) != sdrplay_api_Success)
    {
        MessageString = QString::asprintf("sdrplay_api_Close failed %s", sdrplay_api_GetErrorString(err));
        emit Status(MessageString);
    }


}


void RSPduoInterface::ChangeIFGainA(int gRdb_Gain)
{
    QString MessageString;
    sdrplay_api_ErrT err;
    chosenDevice->tuner = sdrplay_api_Tuner_A;

    chParams->tunerParams.gain.gRdB = gRdb_Gain;
        // Apply limits
        if (chParams->tunerParams.gain.gRdB > 59) chParams->tunerParams.gain.gRdB = 59;
        if (chParams->tunerParams.gain.gRdB < 20) chParams->tunerParams.gain.gRdB = 20;
    if ((err = sdrplay_api_Update(chosenDevice->dev, chosenDevice->tuner,
        sdrplay_api_Update_Tuner_Gr, sdrplay_api_Update_Ext1_None)) != sdrplay_api_Success)
    {
        MessageString = QString::asprintf("Update Tuner Failed %s", sdrplay_api_GetErrorString(err));
        emit Status(MessageString);
    }    

}

void RSPduoInterface::ChangeIFGainB(int gRdb_GainA, int gRdb_GainB)
{
    // To change tuner B both gains have to be changed and then tuner A reset.
    QString MessageString;
    sdrplay_api_ErrT err;
    chosenDevice->tuner = sdrplay_api_Tuner_Both;
qDebug() << "GainB = " + QString::number(gRdb_GainB);
    chParams->tunerParams.gain.gRdB = gRdb_GainB;
        // Apply limits
        if (chParams->tunerParams.gain.gRdB > 59) chParams->tunerParams.gain.gRdB = 59;
        if (chParams->tunerParams.gain.gRdB < 20) chParams->tunerParams.gain.gRdB = 20;
    if ((err = sdrplay_api_Update(chosenDevice->dev, chosenDevice->tuner,
        sdrplay_api_Update_Tuner_Gr, sdrplay_api_Update_Ext1_None)) != sdrplay_api_Success)
    {
        MessageString = QString::asprintf("Update Tuner Failed %s", sdrplay_api_GetErrorString(err));
        emit Status(MessageString);
    }

    ChangeIFGainA(gRdb_GainA); // reset tuner A
}


void RSPduoInterface::ChangeLNAGain(int LNAstate, int gRdb_GainA, int gRdb_GainB)
{

    // update new LNA gain value
    chParams->tunerParams.gain.LNAstate = LNAstate;
    // set new gain values for channels A and B
    ChangeIFGainB(gRdb_GainA,gRdb_GainB);

}
