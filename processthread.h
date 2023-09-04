// RSPduoEME Copyright 2020 David Warwick G4EEV
//
// This file is part of RSPduoEME.
//
// RSPduoEME is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, either version 3 of the License, or(at your option) any later version.
// RSPduoEME is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have
// received a copy of the GNU General Public License along with RSPduoEMRE, if not, see <https://www.gnu.org/licenses/>.



#ifndef PROCESSTHREAD_H
#define PROCESSTHREAD_H

#include "rspduointerface.h"
#include "dspthread.h"
#include <bits/stdc++.h> //for timimg

#include <QObject>
#include <QTimer>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QUdpSocket>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>

#define HEAD 24                        // UDP Header = 24 bytes
#define PAYLOAD 1392                   // UDP Payload = 1392 bytes

#define Linrad_Block_Size  4096        // Size of Linrad Buffer as sent in RAW16 Mode Request

extern volatile int A_LastBuffer;      // Last Input Buffer number for channel A
extern volatile int B_LastBuffer;      // Last Input Buffer number for channel B

extern short (*A_InputBuffer)[INPUT_BUFFER_SIZE];  // pointer to channel A input array [BufferNo][Buffer]
extern short (*B_InputBuffer)[INPUT_BUFFER_SIZE];  // pointer to channel B input array [BufferNo][Buffer]


class ProcessThread : public QObject
{
    Q_OBJECT
public:
    explicit ProcessThread(QObject *parent = nullptr);
    ~ProcessThread();
                                        // Copy of variables from MainWindow
    QString SelectedOutputDevice_A;     // the selected audio output device for channel A
    int SampleRate = 96000;             // selected sample rate, default to 96000
    int DualOP = 0;                     // current Dual O/P mode
    int DuplicateA = 0;                 // duplicate channel A in channel B if = 1
    int TIMF2Output = 0;                // Linrad TIMF2 UDP Output = 1, else 0
    int RAW16Output = 0;                // Linrad RAW16 UDP Output = 1, else 0
    int SoundCardOutput = 0;            // Sound Card Output, 1 = 2 Audio Channels, 2 = 4 Audio Channels, else 0
    int SelectB = 0;                    // Select Ch B o/p = 1, else Ch A (Soundcard Mode)
    DSPthread *P_DSPthread = nullptr;   // pointer to DSPthread
    QString IPAddress;                  // for copy of IP address set by MainWindow
    int LatestOutputDataIndex = 0;      // set to start of latest output index of CircularInputBuffers (for Phase display data)
    int CentreFrequency = 0;            // selected Centre Frequency in KHz as set by mainwindow

signals:

    void StatusMessage(QString);
    void StartDSP_A(void);
    void StartDSP_AB(void);
    void GenSinCosTable(double, double);

public slots:

     void Start(void);
     void Stop(void);
     void SendStatusMessage(QString);
     void GenerateSinCosTable(double, double);

private:

    QThread WorkerThread;                     // worker thread for DSPthread
    QTimer *P_Timer = nullptr;                // pointer for interval Timer
    QAudioOutput *P_AudioDevice_A = nullptr;  // pointer for Channel A Audio Output Device
    QAudioOutput *P_AudioDevice_B = nullptr;  // pointer for Channel B Audio Output Device
    QIODevice *P_OutputBuffer_A = nullptr;    // pointer for Audio Output Buffer Channel A
    QIODevice *P_OutputBuffer_B = nullptr;    // pointer for Audio Output Buffer Channel B
    QUdpSocket *P_UdpSocket = nullptr;        // pointer to UDP Socket
    QTcpServer *P_TcpServer = nullptr;        // pointer to Server Socket
    QTcpSocket *P_TcpSocket = nullptr;        // pointer to TCP Socket
    clock_t start, end;                       // for interval time measurement
    int AFBufferSize = 0;                     // Size of new data in Circular Output Buffer for Audio Device
    int IPBufferSize = 0;                     // Size of new data in Circular Output Buffer for IP output
    int IPOutPoint = 0;                       // output pointer for Circular Output Buffer (IP output)
    int AFOutPoint = 0;                       // output pointer for Circular Output buffer (Audio output)

    int AudioOutputBufferSize = 76800;        // for current output buffer size, changse swith SampleRate

    // Linrad format UDP Header Structure
    typedef struct {
    double passband_center;                 //  8
    int time;                               //  4
    float userx_freq;                       //  4
    int ptr;                                //  4
    unsigned short int block_no;            //  2
    signed char userx_no;                   //  1
    signed char passband_direction;         //  1
    } NET_RX_STRUCT;

    // Linrad NetMsg_Mode_Request Structure
    typedef struct {
    int sample_rate;
    int real_channels;
    int rf_channels;
    int rx_input_mode;
    int block_bytes;
    int fft1_size;
    int fft1_n;
    int First_FFT_Sinpow;
    } NETMSG_MODE_REQUEST_STRUCT;

    // Union to convert float to bytes
    union floatUnion_t
    {
        float f;
        char bytes[4];
    } floatUnion;

    // Union to extract bytes from qint32 data
    union intUnion_t
    {
        qint16 i32;
        char bytes[4];
    } intUnion;

private slots:

    void on_Timer();
    void on_AudioOutput_A_Notify(void);
    void on_AudioOutput_A_StateChanged(void);
    void ProcessData(void);
    void on_UdpSocket_StateChanged(void);
    void on_UdpSocket_Error(void);
    void on_TcpNewConnection(void);
    void on_TcpSocket_readReady(void);

};

#endif // PROCESSTHREAD_H
