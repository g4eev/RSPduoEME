// RSPduoEME Copyright 2020 David Warwick G4EEV
//
// This file is part of RSPduoEME.
//
// RSPduoEME is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, either version 3 of the License, or(at your option) any later version.
// RSPduoEME is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have
// received a copy of the GNU General Public License along with RSPduoEMRE, if not, see <https://www.gnu.org/licenses/>.



#include "dspthread.h"
#include "filters.h"
#include <QThread>
#include <QDebug>

#include <QtMath>

DSPthread::DSPthread(QObject *parent) : QObject(parent)
{

    Timer = new QTimer;  // Initalise Timer Object
    connect (Timer, SIGNAL(timeout()), this, SLOT(onTimer()));
    Timer->start(20);  //20 mS

    // initiaate list of process times for performance measurement by MainWindow
    ProcessTimes = new QList<double>;

    // Initalise Circular Buffers and Filter Arrays

    I_BufferA = new double[INPUT_BUFFER_SIZE];            // allocate buffer arrays
    Q_BufferA = new double[INPUT_BUFFER_SIZE];
    I_BufferB = new double[INPUT_BUFFER_SIZE];
    Q_BufferB = new double[INPUT_BUFFER_SIZE];

    I_D5FilterOutA = new double[INPUT_BUFFER_SIZE];       // allocate D5 output arrays
    Q_D5FilterOutA = new double[INPUT_BUFFER_SIZE];
    I_D5FilterOutB = new double[INPUT_BUFFER_SIZE];
    Q_D5FilterOutB = new double[INPUT_BUFFER_SIZE];

    I_US6FilterOutA = new double[INPUT_BUFFER_SIZE * 6];  // allocate US6 output arrays
    Q_US6FilterOutA = new double[INPUT_BUFFER_SIZE * 6];  // (larger than needed!)
    I_US6FilterOutB = new double[INPUT_BUFFER_SIZE * 6];
    Q_US6FilterOutB = new double[INPUT_BUFFER_SIZE * 6];

    I_US4FilterOutA = new double[INPUT_BUFFER_SIZE * 6];  // allocate US4 output arrays
    Q_US4FilterOutA = new double[INPUT_BUFFER_SIZE * 6];  // (larger than needed!)
    I_US4FilterOutB = new double[INPUT_BUFFER_SIZE * 6];
    Q_US4FilterOutB = new double[INPUT_BUFFER_SIZE * 6];

    I_SoundCardOutA = new double[INPUT_BUFFER_SIZE * 6];   // allocate Sound Card output arrays
    Q_SoundCardOutA = new double[INPUT_BUFFER_SIZE * 6];   // (larger than needed!)
    I_SoundCardOutB = new double[INPUT_BUFFER_SIZE * 6];
    Q_SoundCardOutB = new double[INPUT_BUFFER_SIZE * 6];

    I_CircularOutputBufferA = new double[CircularOutputBufferSize];
    Q_CircularOutputBufferA = new double[CircularOutputBufferSize];
    I_CircularOutputBufferB = new double[CircularOutputBufferSize];
    Q_CircularOutputBufferB = new double[CircularOutputBufferSize];
    I_CircularOutputBufferSCA = new double[CircularOutputBufferSize];
    Q_CircularOutputBufferSCA = new double[CircularOutputBufferSize];
    I_CircularOutputBufferSCB = new double[CircularOutputBufferSize];
    Q_CircularOutputBufferSCB = new double[CircularOutputBufferSize];

    // initialise Sin / Cos lookup table arrays

    PhaseInc = 0.45*M_PI;     // to tune to 0.45 bandwith
    SinCosTableLength = 40;   // for this frequency repeats after 40

    SinTableA = new double[SinCosTableLength];
    CosTableA = new double[SinCosTableLength];
    SinTableB = new double[SinCosTableLength];
    CosTableB = new double[SinCosTableLength];

    GenerateSinCosTable(0,0); // initalise arrays with 0 starting phases


    // initalise D2A Ch A arrays and filters (polyphase decimation by 2)
    I_D2AA = new double[2][D2A_Order/2];
    Q_D2AA = new double[2][D2A_Order/2];
    for(int filter = 0; filter < 2; filter++) // zero all 2 sub filters
    { for(int loop = 0;loop < D2A_Order/2; loop++) { I_D2AA[filter][loop]=0; Q_D2AA[filter][loop]=0; }}

    // initalise D2A Ch B arrays and filters (polyphase decimation by 2)
    I_D2AB = new double[2][D2A_Order/2];
    Q_D2AB = new double[2][D2A_Order/2];
    for(int filter = 0; filter < 2; filter++) // zero all 2 sub filters
    { for(int loop = 0;loop < D2A_Order/2; loop++) { I_D2AB[filter][loop]=0; Q_D2AB[filter][loop]=0; }}


    // initalise D2B Ch A arrays and filters (polyphase decimation by 2)
    I_D2BA = new double[2][D2B_Order/2];
    Q_D2BA = new double[2][D2B_Order/2];
    for(int filter = 0; filter < 2; filter++) // zero all 2 sub filters
    { for(int loop = 0;loop < D2B_Order/2; loop++) { I_D2BA[filter][loop]=0; Q_D2BA[filter][loop]=0; }}

    // initalise D2B Ch B arrays and filters (polyphase decimation by 2)
    I_D2BB = new double[2][D2B_Order/2];
    Q_D2BB = new double[2][D2B_Order/2];
    for(int filter = 0; filter < 2; filter++) // zero all 2 sub filters
    { for(int loop = 0;loop < D2B_Order/2; loop++) { I_D2BB[filter][loop]=0; Q_D2BB[filter][loop]=0; }}


    // initalise D5 Ch A arrays and filters (polyphase decimation by 5)
    I_D5A = new double[5][D5_Order/5];
    Q_D5A = new double[5][D5_Order/5];
    for(int filter = 0; filter < 5; filter++) // zero all 5 sub filters
    { for(int loop = 0;loop < D5_Order/5; loop++) { I_D5A[filter][loop]=0; Q_D5A[filter][loop]=0; }}

    // initalise D5 Ch B arrays and filters (polyphase decimation by 5)
    I_D5B = new double[5][D5_Order/5];
    Q_D5B = new double[5][D5_Order/5];
    for(int filter = 0; filter < 5; filter++) // zero all 5 sub filters
    { for(int loop = 0;loop < D5_Order/5; loop++) { I_D5B[filter][loop]=0; Q_D5B[filter][loop]=0; }}


    // initalise US6 Ch A filters (polyphase upsample by 6, hence / 6)
    I_US6A = new double[US6_Order/6];
    Q_US6A = new double[US6_Order/6];
    for(int loop = 0;loop < US6_Order/6; loop++) { I_US6A[loop]=0; Q_US6A[loop]=0; }

    // initalise US6 Ch B filters (polyphase upsample by 6, hence / 6)
    I_US6B = new double[US6_Order/6];
    Q_US6B = new double[US6_Order/6];
    for(int loop = 0;loop < US6_Order/6; loop++) { I_US6B[loop]=0; Q_US6B[loop]=0; }


    // initalise US4 Ch A filters (polyphase upsample by 4, hence / 4)
    I_US4A = new double[US4_Order/4];
    Q_US4A = new double[US4_Order/4];
    for(int loop = 0;loop < US4_Order/4; loop++) { I_US4A[loop]=0; Q_US4A[loop]=0; }

    // initalise US4 Ch B filters (polyphase upsample by 4, hence / 4)
    I_US4B = new double[US4_Order/4];
    Q_US4B = new double[US4_Order/4];
    for(int loop = 0;loop < US4_Order/4; loop++) { I_US4B[loop]=0; Q_US4B[loop]=0; }

}

DSPthread::~DSPthread()
{
    // Stop timer and delete Timer object
    Timer->stop();
    delete Timer;

    // delete filter buffers and tables
    delete I_BufferA;
    delete Q_BufferA;
    delete I_BufferB;
    delete Q_BufferB;
    delete I_D5A;
    delete Q_D5A;
    delete I_D5B;
    delete Q_D5B;
    delete I_US6A;
    delete Q_US6A;
    delete I_US6B;
    delete Q_US6B;
    delete I_US4A;
    delete Q_US4A;
    delete I_US4B;
    delete Q_US4B;
    delete I_D5FilterOutA;
    delete Q_D5FilterOutA;
    delete I_D5FilterOutB;
    delete Q_D5FilterOutB;
    delete I_US6FilterOutA;
    delete Q_US6FilterOutA;
    delete I_US6FilterOutB;
    delete Q_US6FilterOutB;
    delete I_US4FilterOutA;
    delete Q_US4FilterOutA;
    delete I_US4FilterOutB;
    delete Q_US4FilterOutB;
    delete I_SoundCardOutA;
    delete Q_SoundCardOutA;
    delete I_SoundCardOutB;
    delete Q_SoundCardOutB;
    delete I_CircularOutputBufferA;
    delete Q_CircularOutputBufferA;
    delete I_CircularOutputBufferB;
    delete Q_CircularOutputBufferB;
    delete I_CircularOutputBufferSCA;
    delete Q_CircularOutputBufferSCA;
    delete I_CircularOutputBufferSCB;
    delete Q_CircularOutputBufferSCB;
    delete SinTableA;
    delete CosTableA;
    delete SinTableB;
    delete CosTableB;

}


void DSPthread::GenerateSinCosTable(double Aphase, double Bphase)
{
    // Calculate sin/cos table for Tuners

    PhaseAcc = Aphase; // set Channel A phase start
    for(int loop = 0; loop < SinCosTableLength; loop++)
    {
        SinTableA[loop] = qSin(PhaseAcc);
        CosTableA[loop] = -1 * qCos(PhaseAcc);
        PhaseAcc += PhaseInc;
        if(PhaseAcc >= (2 * M_PI)) PhaseAcc -= (2 * M_PI);
        //qDebug() << "Phase Table A " + QString::number(loop) + "  Value = " + QString::number(SinTableA[loop]);
    }

    PhaseAcc = Bphase; // set Channel B phase start
    for(int loop = 0; loop < SinCosTableLength; loop++)
    {
        SinTableB[loop] = qSin(PhaseAcc);
        CosTableB[loop] = -1 * qCos(PhaseAcc);
        PhaseAcc += PhaseInc;
        if(PhaseAcc >= (2 * M_PI)) PhaseAcc -= (2 * M_PI);
        //qDebug() << "Phase Table B " + QString::number(loop) + "  Value = " + QString::number(SinTableB[loop]);
    }

}


void DSPthread::onTimer(void)
{
    // Check if previous input buffer is processed and if so start another

    if(DSPMode == 0) return; // 0 = no processing

    if(Finished != 1) return; // previous process not finished

    // return if no new input buffer available
    if(A_LastBuffer == PreviousLastBuffer) return;

    // Incremnet previous buffer pointer and use
    PreviousLastBuffer++;
    if(PreviousLastBuffer >= BUFFERS) PreviousLastBuffer = 0;

    // flag any missmatch in buffer numbers
    if(A_LastBuffer != B_LastBuffer)
    {
        QString Message = "Buffer Missmatch";
        //emit StatusMessage(Message);
        qDebug() << Message;
    }


    // Set next buffer number and initate a new process by DSPthread

    BufferNo = PreviousLastBuffer;              // set bufer number
    Finished = 0;                               // reset Finished flag
    if(DSPMode == 1) ProcessBufferA();          // process channel A only
    if (DSPMode == 2) ProcessBufferAB();        // process channels A and B

}


// *****************************  Process Buffer A only  ****************************** //


void DSPthread::ProcessBufferA(void)
{

        start = clock();

        // Tune to centre of IF i.e. 450KHz. (sample rate is 2MHz, Signal Real)

        int LookupTablePointer = 0;
        for(int loop = 0; loop < INPUT_BUFFER_SIZE; loop++)
        {
            // multiply input with Tuner Oscillator
            I_BufferA[loop] = SinTableA[LookupTablePointer] * A_InputBuffer[BufferNo][loop];
            Q_BufferA[loop] = CosTableA[LookupTablePointer] * A_InputBuffer[BufferNo][loop];
            LookupTablePointer ++;  // increment and loop
            if(LookupTablePointer >= SinCosTableLength) LookupTablePointer = 0;
         }

        // signal here is at 2MHz sample rate complex, bandwith 1MHz, in I/Q_Buffer


        // Decimation by 2 polyphase filter (D2A)

        double I_D2AsubAccA[2] {0,0};
        double Q_D2AsubAccA[2] {0,0};
        int D2Astage = 0;
        int D2A_OP = 0;
        for(int loop = 0; loop < INPUT_BUFFER_SIZE; loop++)
        {

            // shift in a new sample to subfilter
            for(int index = (D2A_Order/2)-1; index >= 0 ; index--)   // read in every index if I/Q_Buffer
            {
                if(index != 0)
                {
                    I_D2AA[D2Astage][index] = I_D2AA[D2Astage][index-1];
                    Q_D2AA[D2Astage][index] = Q_D2AA[D2Astage][index-1];
                }
                else { I_D2AA[D2Astage][0] = I_BufferA[loop]; Q_D2AA[D2Astage][0] = Q_BufferA[loop];}
            }
            // multiply and accumulate subfilter
            I_D2AsubAccA[D2Astage] = 0; Q_D2AsubAccA[D2Astage] = 0;
            for(int index = 0; index < (D2A_Order/2); index++)
            {
                I_D2AsubAccA[D2Astage] += (I_D2AA[D2Astage][index] * D2ACoef[(1-D2Astage)+(index*2)]);
                Q_D2AsubAccA[D2Astage] += (Q_D2AA[D2Astage][index] * D2ACoef[(1-D2Astage)+(index*2)]);
            }
            D2Astage += 1;
            if(D2Astage >= 2)
            {
                // accumulate all subfilters and output
                I_BufferA[loop-1] = I_D2AsubAccA[0] + I_D2AsubAccA[1];  // write out every 2nd index of I/Q_Buffer
                Q_BufferA[loop-1] = Q_D2AsubAccA[0] + Q_D2AsubAccA[1];
                D2Astage = 0;
                D2A_OP++;  // inc D2A filter O/P size
            }

        }


        if(SampleRate == 96000)  // extra decimation stage for 96000 Sample Rate
        {

            // Decimation by 2 polyphase filter (D2B)

            double I_D2BsubAccA[2] {0,0};
            double Q_D2BsubAccA[2] {0,0};
            int D2Bstage = 0;
            int D2B_OP = 0;
            for(int loop = 0; loop < INPUT_BUFFER_SIZE; loop+=2)  // read in every 2nd index of I/Q_Buffer
            {

                // shift in a new sample to subfilter
                for(int index = (D2B_Order/2)-1; index >= 0 ; index--)
                {
                    if(index != 0)
                    {
                        I_D2BA[D2Bstage][index] = I_D2BA[D2Bstage][index-1];
                        Q_D2BA[D2Bstage][index] = Q_D2BA[D2Bstage][index-1];
                    }
                    else { I_D2BA[D2Bstage][0] = I_BufferA[loop]; Q_D2BA[D2Bstage][0] = Q_BufferA[loop];}
                }
                // multiply and accumulate subfilter
                I_D2BsubAccA[D2Bstage] = 0; Q_D2BsubAccA[D2Bstage] = 0;
                for(int index = 0; index < (D2B_Order/2); index++)
                {
                    I_D2BsubAccA[D2Bstage] += (I_D2BA[D2Bstage][index] * D2BCoef[(1-D2Bstage)+(index*2)]);
                    Q_D2BsubAccA[D2Bstage] += (Q_D2BA[D2Bstage][index] * D2BCoef[(1-D2Bstage)+(index*2)]);
                }
                D2Bstage += 1;
                if(D2Bstage >= 2)
                {
                    // accumulate all subfilters and output
                    I_BufferA[loop-2] = I_D2BsubAccA[0] + I_D2BsubAccA[1];  // write out every 4th index of I/Q_Buffer
                    Q_BufferA[loop-2] = Q_D2BsubAccA[0] + Q_D2BsubAccA[1];
                    D2Bstage = 0;
                    D2B_OP++;  // inc D2A filter O/P size
                }

            }

        }

        // Decimated by 4 output (500KHz) now in I/Q_Buffer index 4  (for 96KHz output rate)
        // decimated by 2 output (1000KHz) now in I/Q_Buffer index 2 (for 192KHz output rate)

        // Select buffer index dependant on output rate 96KHz v 192KHz

        int RateIndex;
        if(SampleRate == 192000) RateIndex = 2;  // for 192KHz Sample Rate
        else RateIndex = 4;                      // default for 96KHz Sample Rate

        // Decimation by 5 polyphase filter (D5)

        double I_D5subAccA[5] {0,0,0,0,0,};
        double Q_D5subAccA[5] {0,0,0,0,0,};
        int D5stage = 0;
        int D5_OP = 0;
        for(int loop = 0; loop < INPUT_BUFFER_SIZE/RateIndex; loop++)
        {

            // shift in a new sample to subfilter
            for(int index = (D5_Order/5)-1; index >= 0 ; index--)
            {
                if(index != 0)
                {
                    I_D5A[D5stage][index] = I_D5A[D5stage][index-1];
                    Q_D5A[D5stage][index] = Q_D5A[D5stage][index-1];
                }
                else { I_D5A[D5stage][0] = I_BufferA[loop*RateIndex]; Q_D5A[D5stage][0] = Q_BufferA[loop*RateIndex];}
            }
            // multiply and accumulate subfilter
            I_D5subAccA[D5stage] = 0; Q_D5subAccA[D5stage] = 0;
            for(int index = 0; index < (D5_Order/5); index++)
            {
                I_D5subAccA[D5stage] += (I_D5A[D5stage][index] * D5Coef[(4-D5stage)+(index*5)]);
                Q_D5subAccA[D5stage] += (Q_D5A[D5stage][index] * D5Coef[(4-D5stage)+(index*5)]);
            }
            D5stage += 1;
            if(D5stage >= 5)
            {
                // accumulate all subfilters and output
                I_D5FilterOutA[(loop/5)] = I_D5subAccA[0] + I_D5subAccA[1] + I_D5subAccA[2] + I_D5subAccA[3] + I_D5subAccA[4];
                Q_D5FilterOutA[(loop/5)] = Q_D5subAccA[0] + Q_D5subAccA[1] + Q_D5subAccA[2] + Q_D5subAccA[3] + Q_D5subAccA[4];
                D5stage = 0;
                D5_OP++;  // inc D5 filter O/P size
            }

        }

        // Decimated by 5 output (100KHz/200KHz) now in I/Q_D5FilterOut


        // Perform Polyphase upsample by 6 filter (US6)

        double I_AccumulatorA = 0;
        double Q_AccumulatorA = 0;
        int US6_OP = 0;

        for(int x = 0; x < D5_OP; x++)  // loop D5 output size
        {
            // shift in new sample to filter register (size US6_Order/6)
            for(int SR = (US6_Order/6)-1; SR >= 0; SR--)
            {
                if(SR != 0) { I_US6A[SR] = I_US6A[SR-1]; Q_US6A[SR] = Q_US6A[SR-1]; }
                else { I_US6A[0] = I_D5FilterOutA[x]; Q_US6A[0] = Q_D5FilterOutA[x]; }
            }
            //loop 6 times to multiply / accumulate and output upsampled samples
            for(int loop = 0; loop < 6; loop++)
            {
                I_AccumulatorA = 0; Q_AccumulatorA = 0;
                for(int index = 0; index < (US6_Order/6); index++)
                {
                    I_AccumulatorA += (I_US6A[index] * US6Coef[loop+(index*6)]);
                    Q_AccumulatorA += (Q_US6A[index] * US6Coef[loop+(index*6)]);
                }
                I_US6FilterOutA[US6_OP+loop] = I_AccumulatorA * 6; // adjust gain
                Q_US6FilterOutA[US6_OP+loop] = Q_AccumulatorA * 6;
            }
            US6_OP += 6; // inc US6 filter output size
        }


        // Upsample by 6 Decimate 5 (120KHz/240KHz) output now in I/Q_US6FilterOut index * 5;


        // Perform Polyphase upsample by 4 filter (US4)

        I_AccumulatorA = 0;
        Q_AccumulatorA = 0;
        int US4_OP = 0;

        for(int x = 0; x < US6_OP; x += 5)  // decimate by 5, using US6 filter output size
        {

            // shift in new sample to filter register (size US4_Order/4)
            for(int SR = (US4_Order/4)-1; SR >= 0; SR--)
            {
                if(SR != 0) { I_US4A[SR] = I_US4A[SR-1]; Q_US4A[SR] = Q_US4A[SR-1]; }
                else { I_US4A[0] = I_US6FilterOutA[x]; Q_US4A[0] = Q_US6FilterOutA[x]; }
            }
            //loop 4 times to multiply / accumulate and output upsampled samples
            for(int loop = 0; loop < 4; loop++)
            {
                I_AccumulatorA = 0; Q_AccumulatorA = 0;
                for(int index = 0; index < (US4_Order/4); index++)
                {
                    I_AccumulatorA += (I_US4A[index] * US4Coef[loop+(index*4)]);
                    Q_AccumulatorA += (Q_US4A[index] * US4Coef[loop+(index*4)]);
                }
                I_US4FilterOutA[US4_OP+loop] = I_AccumulatorA * 4; // adjust gain
                Q_US4FilterOutA[US4_OP+loop] = Q_AccumulatorA * 4;
            }
            US4_OP += 4; // inc output pointer
        }

        // Upsampled by 4 Decimated by 5 (96KHz/192KHz) output in I/Q_US4FilterOut index * 5


        if(SoundCardOutput != 0)
        {
            // Save US4FilterOut (Soundcard Format Spectrum) in SoundCardOut buffer

            for(int loop = 0; loop < US4_OP; loop+=5)  // (decimating by five)
            {
                I_SoundCardOutA[loop] = I_US4FilterOutA[loop];
                Q_SoundCardOutA[loop] = Q_US4FilterOutA[loop];
            }
        }


        if(TIMF2Output == 1)
        {
            // Rotate spectrum 180 degrees (pi) to MAP65 format (in-place using US4 buffer index 5)

            double IoutA, QoutA, IoutB, QoutB;
            static double PhaseAcc = 0;
            double PhaseInc = M_PI;

            for(int loop = 0; loop < US4_OP; loop+=5)
            {
                // incremet the tuner oscillator each sample period
                PhaseAcc += PhaseInc;
                if(PhaseAcc >= (2 * M_PI)) PhaseAcc -= (2 * M_PI);

                // Complex multiply I & Q with Tuner Oscillator
                IoutA = (I_US4FilterOutA[loop] * qSin(PhaseAcc)) - (Q_US4FilterOutA[loop] * qCos(PhaseAcc));
                QoutA = (I_US4FilterOutA[loop] * qCos(PhaseAcc)) + (Q_US4FilterOutA[loop] * qSin(PhaseAcc));
                I_US4FilterOutA[loop] = QoutA;  Q_US4FilterOutA[loop] = IoutA;
            }
        }

        // Move Output Data to Circular Output Buffers, decimated by 5

        for(int loop = 0; loop < US4_OP; loop+=5)
        {
            I_CircularOutputBufferA[InPoint] = I_US4FilterOutA[loop]; // UDP format data
            Q_CircularOutputBufferA[InPoint] = Q_US4FilterOutA[loop];
            I_CircularOutputBufferB[InPoint] = 0; // zero unused channel in case
            Q_CircularOutputBufferB[InPoint] = 0; // UDP Mode is used for single channel

            I_CircularOutputBufferSCA[InPoint] = I_SoundCardOutA[loop]; // Sound Card format data
            Q_CircularOutputBufferSCA[InPoint] = Q_SoundCardOutA[loop];

            InPoint++;
            if(InPoint >= CircularOutputBufferSize) InPoint = 0;
        }

        Finished = 1; // Flag completion of DSP processing for this buffer

        end = clock();
        double time_taken = double(end - start) / double(CLOCKS_PER_SEC);
        ProcessTimes->append(time_taken);

        //qDebug() << "DSP1 time = " + QString::number(time_taken);

}


// *****************************  Process Both Buffers A & B  ****************************** //


void DSPthread::ProcessBufferAB(void)
{

    start = clock();

    // Tune to centre of IF i.e. 450KHz. (sample rate is 2MHz, Signal Real)

    int LookupTablePointer = 0;
    for(int loop = 0; loop < INPUT_BUFFER_SIZE; loop++)
    {
        // multiply input with Tuner Oscillator
        I_BufferA[loop] = SinTableA[LookupTablePointer] * A_InputBuffer[BufferNo][loop];
        Q_BufferA[loop] = CosTableA[LookupTablePointer] * A_InputBuffer[BufferNo][loop];
        I_BufferB[loop] = SinTableB[LookupTablePointer] * B_InputBuffer[BufferNo][loop];
        Q_BufferB[loop] = CosTableB[LookupTablePointer] * B_InputBuffer[BufferNo][loop];
        LookupTablePointer ++;  // increment and loop
        if(LookupTablePointer >= SinCosTableLength) LookupTablePointer = 0;
     }

    // signal here is at 2MHz sample rate complex, bandwith 1MHz, in I/Q_Buffer


    // Decimation by 2 polyphase filter (D2A)

    double I_D2AsubAccA[2] {0,0};
    double Q_D2AsubAccA[2] {0,0};
    double I_D2AsubAccB[2] {0,0};
    double Q_D2AsubAccB[2] {0,0};
    int D2Astage = 0;
    int D2A_OP = 0;
    for(int loop = 0; loop < INPUT_BUFFER_SIZE; loop++)
    {

        // shift in a new sample to subfilter
        for(int index = (D2A_Order/2)-1; index >= 0 ; index--)   // read in every index if I/Q_Buffer
        {
            if(index != 0)
            {
                I_D2AA[D2Astage][index] = I_D2AA[D2Astage][index-1];
                Q_D2AA[D2Astage][index] = Q_D2AA[D2Astage][index-1];
                I_D2AB[D2Astage][index] = I_D2AB[D2Astage][index-1];
                Q_D2AB[D2Astage][index] = Q_D2AB[D2Astage][index-1];
            }
            else
            {   I_D2AA[D2Astage][0] = I_BufferA[loop];
                Q_D2AA[D2Astage][0] = Q_BufferA[loop];
                I_D2AB[D2Astage][0] = I_BufferB[loop];
                Q_D2AB[D2Astage][0] = Q_BufferB[loop];
            }
        }
        // multiply and accumulate subfilter
        I_D2AsubAccA[D2Astage] = 0; Q_D2AsubAccA[D2Astage] = 0;
        I_D2AsubAccB[D2Astage] = 0; Q_D2AsubAccB[D2Astage] = 0;
        for(int index = 0; index < (D2A_Order/2); index++)
        {
            I_D2AsubAccA[D2Astage] += (I_D2AA[D2Astage][index] * D2ACoef[(1-D2Astage)+(index*2)]);
            Q_D2AsubAccA[D2Astage] += (Q_D2AA[D2Astage][index] * D2ACoef[(1-D2Astage)+(index*2)]);
            I_D2AsubAccB[D2Astage] += (I_D2AB[D2Astage][index] * D2ACoef[(1-D2Astage)+(index*2)]);
            Q_D2AsubAccB[D2Astage] += (Q_D2AB[D2Astage][index] * D2ACoef[(1-D2Astage)+(index*2)]);
        }
        D2Astage += 1;
        if(D2Astage >= 2)
        {
            // accumulate all subfilters and output
            I_BufferA[loop-1] = I_D2AsubAccA[0] + I_D2AsubAccA[1];  // write out every 2nd index of I/Q_Buffer
            Q_BufferA[loop-1] = Q_D2AsubAccA[0] + Q_D2AsubAccA[1];
            I_BufferB[loop-1] = I_D2AsubAccB[0] + I_D2AsubAccB[1];
            Q_BufferB[loop-1] = Q_D2AsubAccB[0] + Q_D2AsubAccB[1];
            D2Astage = 0;
            D2A_OP++;  // inc D2A filter O/P size
        }

    }


    if(SampleRate == 96000)  // extra decimation stage for 96000 Sample Rate
    {

        // Decimation by 2 polyphase filter (D2B)

        double I_D2BsubAccA[2] {0,0};
        double Q_D2BsubAccA[2] {0,0};
        double I_D2BsubAccB[2] {0,0};
        double Q_D2BsubAccB[2] {0,0};
        int D2Bstage = 0;
        int D2B_OP = 0;
        for(int loop = 0; loop < INPUT_BUFFER_SIZE; loop+=2)  // read in every 2nd index of I/Q_Buffer
        {

            // shift in a new sample to subfilter
            for(int index = (D2B_Order/2)-1; index >= 0 ; index--)
            {
                if(index != 0)
                {
                    I_D2BA[D2Bstage][index] = I_D2BA[D2Bstage][index-1];
                    Q_D2BA[D2Bstage][index] = Q_D2BA[D2Bstage][index-1];
                    I_D2BB[D2Bstage][index] = I_D2BB[D2Bstage][index-1];
                    Q_D2BB[D2Bstage][index] = Q_D2BB[D2Bstage][index-1];
                }
                else
                {
                    I_D2BA[D2Bstage][0] = I_BufferA[loop];
                    Q_D2BA[D2Bstage][0] = Q_BufferA[loop];
                    I_D2BB[D2Bstage][0] = I_BufferB[loop];
                    Q_D2BB[D2Bstage][0] = Q_BufferB[loop];
                }
            }
            // multiply and accumulate subfilter
            I_D2BsubAccA[D2Bstage] = 0; Q_D2BsubAccA[D2Bstage] = 0;
            I_D2BsubAccB[D2Bstage] = 0; Q_D2BsubAccB[D2Bstage] = 0;
            for(int index = 0; index < (D2B_Order/2); index++)
            {
                I_D2BsubAccA[D2Bstage] += (I_D2BA[D2Bstage][index] * D2BCoef[(1-D2Bstage)+(index*2)]);
                Q_D2BsubAccA[D2Bstage] += (Q_D2BA[D2Bstage][index] * D2BCoef[(1-D2Bstage)+(index*2)]);
                I_D2BsubAccB[D2Bstage] += (I_D2BB[D2Bstage][index] * D2BCoef[(1-D2Bstage)+(index*2)]);
                Q_D2BsubAccB[D2Bstage] += (Q_D2BB[D2Bstage][index] * D2BCoef[(1-D2Bstage)+(index*2)]);
            }
            D2Bstage += 1;
            if(D2Bstage >= 2)
            {
                // accumulate all subfilters and output
                I_BufferA[loop-2] = I_D2BsubAccA[0] + I_D2BsubAccA[1];  // write out every 4th index of I/Q_Buffer
                Q_BufferA[loop-2] = Q_D2BsubAccA[0] + Q_D2BsubAccA[1];
                I_BufferB[loop-2] = I_D2BsubAccB[0] + I_D2BsubAccB[1];
                Q_BufferB[loop-2] = Q_D2BsubAccB[0] + Q_D2BsubAccB[1];
                D2Bstage = 0;
                D2B_OP++;  // inc D2A filter O/P size
            }

        }

    }

    // Decimated by 4 output (500KHz) now in I/Q_Buffer index 4  (for 96KHz output rate)
    // decimated by 2 output (1000KHz) now in I/Q_Buffer index 2 (for 192KHz output rate)

    // Select buffer index dependant on output rate 96KHz v 192KHz

    int RateIndex;
    if(SampleRate == 192000) RateIndex = 2;  // for 192KHz Sample Rate
    else RateIndex = 4;                      // default for 96KHz Sample Rate

    // Decimation by 5 polyphase filter (D5)

    double I_D5subAccA[5] {0,0,0,0,0,};
    double Q_D5subAccA[5] {0,0,0,0,0,};
    double I_D5subAccB[5] {0,0,0,0,0,};
    double Q_D5subAccB[5] {0,0,0,0,0,};
    int D5stage = 0;
    int D5_OP = 0;
    for(int loop = 0; loop < INPUT_BUFFER_SIZE/RateIndex; loop++)
    {

        // shift in a new sample to subfilter
        for(int index = (D5_Order/5)-1; index >= 0 ; index--)
        {
            if(index != 0)
            {
                I_D5A[D5stage][index] = I_D5A[D5stage][index-1];
                Q_D5A[D5stage][index] = Q_D5A[D5stage][index-1];
                I_D5B[D5stage][index] = I_D5B[D5stage][index-1];
                Q_D5B[D5stage][index] = Q_D5B[D5stage][index-1];
            }
            else
            {
                I_D5A[D5stage][0] = I_BufferA[loop*RateIndex];
                Q_D5A[D5stage][0] = Q_BufferA[loop*RateIndex];
                I_D5B[D5stage][0] = I_BufferB[loop*RateIndex];
                Q_D5B[D5stage][0] = Q_BufferB[loop*RateIndex];
            }
        }
        // multiply and accumulate subfilter
        I_D5subAccA[D5stage] = 0; Q_D5subAccA[D5stage] = 0;
        I_D5subAccB[D5stage] = 0; Q_D5subAccB[D5stage] = 0;
        for(int index = 0; index < (D5_Order/5); index++)
        {
            I_D5subAccA[D5stage] += (I_D5A[D5stage][index] * D5Coef[(4-D5stage)+(index*5)]);
            Q_D5subAccA[D5stage] += (Q_D5A[D5stage][index] * D5Coef[(4-D5stage)+(index*5)]);
            I_D5subAccB[D5stage] += (I_D5B[D5stage][index] * D5Coef[(4-D5stage)+(index*5)]);
            Q_D5subAccB[D5stage] += (Q_D5B[D5stage][index] * D5Coef[(4-D5stage)+(index*5)]);
        }
        D5stage += 1;
        if(D5stage >= 5)
        {
            // accumulate all subfilters and output
            I_D5FilterOutA[(loop/5)] = I_D5subAccA[0] + I_D5subAccA[1] + I_D5subAccA[2] + I_D5subAccA[3] + I_D5subAccA[4];
            Q_D5FilterOutA[(loop/5)] = Q_D5subAccA[0] + Q_D5subAccA[1] + Q_D5subAccA[2] + Q_D5subAccA[3] + Q_D5subAccA[4];
            I_D5FilterOutB[(loop/5)] = I_D5subAccB[0] + I_D5subAccB[1] + I_D5subAccB[2] + I_D5subAccB[3] + I_D5subAccB[4];
            Q_D5FilterOutB[(loop/5)] = Q_D5subAccB[0] + Q_D5subAccB[1] + Q_D5subAccB[2] + Q_D5subAccB[3] + Q_D5subAccB[4];
            D5stage = 0;
            D5_OP++;  // inc D5 filter O/P size
        }
    }

    // Decimated by 5 output (100KHz/200KHz) now in I/Q_D5FilterOut


    // Perform Polyphase upsample by 6 filter (US6)

    double I_AccumulatorA = 0;
    double Q_AccumulatorA = 0;
    double I_AccumulatorB = 0;
    double Q_AccumulatorB = 0;
    int US6_OP = 0;

    for(int x = 0; x < D5_OP; x++)  // loop D5 output size
    {
        // shift in new sample to filter register (size US6_Order/6)
        for(int SR = (US6_Order/6)-1; SR >= 0; SR--)
        {
            if(SR != 0)
            {
                I_US6A[SR] = I_US6A[SR-1]; Q_US6A[SR] = Q_US6A[SR-1];
                I_US6B[SR] = I_US6B[SR-1]; Q_US6B[SR] = Q_US6B[SR-1];
            }
            else
            {
                I_US6A[0] = I_D5FilterOutA[x]; Q_US6A[0] = Q_D5FilterOutA[x];
                I_US6B[0] = I_D5FilterOutB[x]; Q_US6B[0] = Q_D5FilterOutB[x];
            }
        }
        //loop 6 times to multiply / accumulate and output upsampled samples
        for(int loop = 0; loop < 6; loop++)
        {
            I_AccumulatorA = 0; Q_AccumulatorA = 0;
            I_AccumulatorB = 0; Q_AccumulatorB = 0;
            for(int index = 0; index < (US6_Order/6); index++)
            {
                I_AccumulatorA += (I_US6A[index] * US6Coef[loop+(index*6)]);
                Q_AccumulatorA += (Q_US6A[index] * US6Coef[loop+(index*6)]);
                I_AccumulatorB += (I_US6B[index] * US6Coef[loop+(index*6)]);
                Q_AccumulatorB += (Q_US6B[index] * US6Coef[loop+(index*6)]);
            }
            I_US6FilterOutA[US6_OP+loop] = I_AccumulatorA * 6; // adjust gain
            Q_US6FilterOutA[US6_OP+loop] = Q_AccumulatorA * 6;
            I_US6FilterOutB[US6_OP+loop] = I_AccumulatorB * 6;
            Q_US6FilterOutB[US6_OP+loop] = Q_AccumulatorB * 6;
        }
        US6_OP += 6; // inc US6 filter output size
    }


    // Upsample by 6 Decimate 5 (120KHz/240KHz) output now in I/Q_US6FilterOut index * 5;


    // Perform Polyphase upsample by 4 filter (US4)

    I_AccumulatorA = 0;
    Q_AccumulatorA = 0;
    I_AccumulatorB = 0;
    Q_AccumulatorB = 0;
    int US4_OP = 0;

    for(int x = 0; x < US6_OP; x += 5)  // decimate by 5, using US6 filter output size
    {

        // shift in new sample to filter register (size US4_Order/4)
        for(int SR = (US4_Order/4)-1; SR >= 0; SR--)
        {
            if(SR != 0)
            {
                I_US4A[SR] = I_US4A[SR-1]; Q_US4A[SR] = Q_US4A[SR-1];
                I_US4B[SR] = I_US4B[SR-1]; Q_US4B[SR] = Q_US4B[SR-1];
            }
            else
            {
                I_US4A[0] = I_US6FilterOutA[x]; Q_US4A[0] = Q_US6FilterOutA[x];
                I_US4B[0] = I_US6FilterOutB[x]; Q_US4B[0] = Q_US6FilterOutB[x];
            }
        }
        //loop 4 times to multiply / accumulate and output upsampled samples
        for(int loop = 0; loop < 4; loop++)
        {
            I_AccumulatorA = 0; Q_AccumulatorA = 0;
            I_AccumulatorB = 0; Q_AccumulatorB = 0;
            for(int index = 0; index < (US4_Order/4); index++)
            {
                I_AccumulatorA += (I_US4A[index] * US4Coef[loop+(index*4)]);
                Q_AccumulatorA += (Q_US4A[index] * US4Coef[loop+(index*4)]);
                I_AccumulatorB += (I_US4B[index] * US4Coef[loop+(index*4)]);
                Q_AccumulatorB += (Q_US4B[index] * US4Coef[loop+(index*4)]);
            }
            I_US4FilterOutA[US4_OP+loop] = I_AccumulatorA * 4; // adjust gain
            Q_US4FilterOutA[US4_OP+loop] = Q_AccumulatorA * 4;
            I_US4FilterOutB[US4_OP+loop] = I_AccumulatorB * 4;
            Q_US4FilterOutB[US4_OP+loop] = Q_AccumulatorB * 4;
        }
        US4_OP += 4; // inc output pointer
    }

    // Upsampled by 4 Decimated by 5 (96KHz/192KHz) output in I/Q_US4FilterOut index * 5


    // Save US4FilterOut (Soundcard Format Spectrum) in SoundCardOut buffers

    for(int loop = 0; loop < US4_OP; loop+=5)  // (decimating by five)
        {
            I_SoundCardOutA[loop] = I_US4FilterOutA[loop];
            Q_SoundCardOutA[loop] = Q_US4FilterOutA[loop];
            I_SoundCardOutB[loop] = I_US4FilterOutB[loop];
            Q_SoundCardOutB[loop] = Q_US4FilterOutB[loop];
        }


    // Rotate spectrum 180 degrees (pi) to MAP65 (TIMF2) format (in-place using US4 buffer index 5)

    double IoutA, QoutA, IoutB, QoutB;
    static double PhaseAcc = 0;
    double PhaseInc = M_PI;

    for(int loop = 0; loop < US4_OP; loop+=5)
    {
        // incremet the tuner oscillator each sample period
        PhaseAcc += PhaseInc;
        if(PhaseAcc >= (2 * M_PI)) PhaseAcc -= (2 * M_PI);

        // Complex multiply I & Q with Tuner Oscillator
        IoutA = (I_US4FilterOutA[loop] * qSin(PhaseAcc)) - (Q_US4FilterOutA[loop] * qCos(PhaseAcc));
        QoutA = (I_US4FilterOutA[loop] * qCos(PhaseAcc)) + (Q_US4FilterOutA[loop] * qSin(PhaseAcc));
        IoutB = (I_US4FilterOutB[loop] * qSin(PhaseAcc)) - (Q_US4FilterOutB[loop] * qCos(PhaseAcc));
        QoutB = (I_US4FilterOutB[loop] * qCos(PhaseAcc)) + (Q_US4FilterOutB[loop] * qSin(PhaseAcc));
        I_US4FilterOutA[loop] = QoutA;  Q_US4FilterOutA[loop] = IoutA;
        I_US4FilterOutB[loop] = QoutB;  Q_US4FilterOutB[loop] = IoutB;
    }


    // Move Output Data to Circular Output Buffers, decimated by 5

    for(int loop = 0; loop < US4_OP; loop+=5)
    {
        I_CircularOutputBufferA[InPoint] = I_US4FilterOutA[loop];
        Q_CircularOutputBufferA[InPoint] = Q_US4FilterOutA[loop];

        if(DuplicateA == 1) // Duplicate channle A in channel B
        {
            I_CircularOutputBufferB[InPoint] = I_US4FilterOutA[loop];
            Q_CircularOutputBufferB[InPoint] = Q_US4FilterOutA[loop];
        }
        else
        {
            I_CircularOutputBufferB[InPoint] = I_US4FilterOutB[loop];
            Q_CircularOutputBufferB[InPoint] = Q_US4FilterOutB[loop];
        }

        I_CircularOutputBufferSCA[InPoint] = I_SoundCardOutA[loop];
        Q_CircularOutputBufferSCA[InPoint] = Q_SoundCardOutA[loop];
        I_CircularOutputBufferSCB[InPoint] = I_SoundCardOutB[loop];
        Q_CircularOutputBufferSCB[InPoint] = Q_SoundCardOutB[loop];

        InPoint++;
        if(InPoint >= CircularOutputBufferSize) InPoint = 0;
    }

    Finished = 1; // Flag completion of DSP processing for this buffer

    end = clock();
    double time_taken = double(end - start) / double(CLOCKS_PER_SEC);
    ProcessTimes->append(time_taken);

    //qDebug() << "DSP2 time = " + QString::number(time_taken);

}




