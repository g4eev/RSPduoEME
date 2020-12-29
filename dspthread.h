// RSPduoEME Copyright 2020 David Warwick G4EEV
//
// This file is part of RSPduoEME.
//
// RSPduoEME is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, either version 3 of the License, or(at your option) any later version.
// RSPduoEME is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have
// received a copy of the GNU General Public License along with RSPduoEMRE, if not, see <https://www.gnu.org/licenses/>.



#ifndef DSPTHREAD_H
#define DSPTHREAD_H

#include "rspduointerface.h"

#include <bits/stdc++.h> //for timimg

#include <QObject>
#include <QTimer>

extern short (*A_InputBuffer)[INPUT_BUFFER_SIZE];  // pointer to channel A input array [BufferNo][Buffer]
extern short (*B_InputBuffer)[INPUT_BUFFER_SIZE];  // pointer to channel B input array [BufferNo][Buffer]

extern volatile int A_LastBuffer;      // Last Input Buffer number for channel A
extern volatile int B_LastBuffer;      // Last Input Buffer number for channel B

class DSPthread : public QObject
{
    Q_OBJECT
public:
    explicit DSPthread(QObject *parent = nullptr);
    ~DSPthread();

    int SampleRate = 96000;      // selected sample rate, default to 96000
    int BufferNo = 0;            // current InputBuffer number, set by caller
    int PreviousLastBuffer = 0;  // copy of previous channel A input buffer number
    int DSPMode = 0;             // Mode for DSP process, 0=Off, 1=Channel A, 2=Channels A and B
    int DuplicateA = 0;          // duplicate channel A in channel B if = 1
    int TIMF2Output = 0;         // Linrad TIMF2 UDP output = 1, else 0
    int RAW16Output = 0;         // Linrad RAW16 UDP Output = 1, else 0
    int SoundCardOutput = 0;     // Sound Card Output, 1 = 2 Audio Channels, 2 = 4 Audio Channels, else 0
    volatile int Finished = 1;   // Flag = 1 to indicate processin has finished
    QList<double> *ProcessTimes; // pointer to list of process times for performance measurement


    int CircularOutputBufferSize = 96000;  // 500mS @ 192Khz rate (192000 samples / 2)
    double *I_CircularOutputBufferA;       // pointers to Circular Output Buffer for Ch A UDP Mode
    double *Q_CircularOutputBufferA;
    double *I_CircularOutputBufferB;       // pointers to Circular Output Buffer for Ch B UDP Mode
    double *Q_CircularOutputBufferB;
    double *I_CircularOutputBufferSCA;     // pointers to Circular Output Buffers for Sound Card
    double *Q_CircularOutputBufferSCA;
    double *I_CircularOutputBufferSCB;
    double *Q_CircularOutputBufferSCB;
    volatile int InPoint = 0;              // input pointer for Circular Output Buffers

public slots:

    void onTimer(void);
    void GenerateSinCosTable(double Aphase, double Bphase);
    void ProcessBufferA(void);
    void ProcessBufferAB(void);

signals:

    void StatusMessage(QString);

private:

    QTimer *Timer;                // pointer to Timer Object

    clock_t start, end;           // for interval time measurement

    double *I_BufferA;            // pointers to I buffer Ch A
    double *Q_BufferA;
    double *I_BufferB;            // pointers to I buffer Ch B
    double *Q_BufferB;
    double *I_D5FilterOutA;       // pointers to D5 filter output Ch A
    double *Q_D5FilterOutA;
    double *I_D5FilterOutB;       // pointers to D5 filter output Ch B
    double *Q_D5FilterOutB;
    double *I_US6FilterOutA;      // pointer to US6 filter output Ch A
    double *Q_US6FilterOutA;
    double *I_US6FilterOutB;      // pointer to US6 filter output Ch B
    double *Q_US6FilterOutB;
    double *I_US4FilterOutA;      // pointer to US4 filter output Ch A
    double *Q_US4FilterOutA;
    double *I_US4FilterOutB;      // pointer to US4 filter output Ch B
    double *Q_US4FilterOutB;
    double *I_SoundCardOutA;      // pointer to Sound Card device output Buffers
    double *Q_SoundCardOutA;
    double *I_SoundCardOutB;
    double *Q_SoundCardOutB;
    double *SinTableA;            // pointer to array of Sin values for tuning channel A
    double *CosTableA;            // pointer to array of Cos values for tuning channel A
    double *SinTableB;            // pointer to array of Sin values for tuning channel B
    double *CosTableB;            // pointer to array of Cos values for tuning channel B
    double PhaseAcc;              // phase accumulator for tuner oscillator
    double PhaseInc;              // phase increment for tuner oscillator
    int SinCosTableLength;        // length of Sin/Cos lookup tablse for tuner oscillator  

private slots:


};

#endif // DSPTHREAD_H
