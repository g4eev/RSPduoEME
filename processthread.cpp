// RSPduoEME Copyright 2020 David Warwick G4EEV
//
// This file is part of RSPduoEME.
//
// RSPduoEME is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, either version 3 of the License, or(at your option) any later version.
// RSPduoEME is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have
// received a copy of the GNU General Public License along with RSPduoEMRE, if not, see <https://www.gnu.org/licenses/>.



#include "processthread.h"
#include <unistd.h>  // for usleep

#include <QDebug>
#include <QThread>
#include <QAudioDeviceInfo>
#include <QtMath>

ProcessThread::ProcessThread(QObject *parent) : QObject(parent)
{
    // Start DSP Thread
    P_DSPthread = new DSPthread;
    P_DSPthread->moveToThread(&WorkerThread);
    WorkerThread.start();
    WorkerThread.setPriority(QThread::TimeCriticalPriority);

    // Initalise Timer object
    P_Timer = new QTimer(this);

    // connect Signals and Slots
    connect(P_Timer, SIGNAL(timeout()), this, SLOT(on_Timer()));
    connect(this, SIGNAL(StartDSP_A()),P_DSPthread, SLOT(ProcessBufferA()));
    connect(this, SIGNAL(StartDSP_AB()),P_DSPthread, SLOT(ProcessBufferAB()));
    connect(P_DSPthread, SIGNAL(StatusMessage(QString)), this, SLOT(SendStatusMessage(QString)));
    connect(this, SIGNAL(GenSinCosTable(double,double)), P_DSPthread, SLOT(GenerateSinCosTable(double,double)));
}


ProcessThread::~ProcessThread()
{
    // stop ProcessThreads
    WorkerThread.quit();
    WorkerThread.wait();
}


void ProcessThread::on_Timer()
{
    // process data in dual output mode on timer
    ProcessData();
}


void ProcessThread::SendStatusMessage(QString Message)
{
    emit StatusMessage(Message);
}


void ProcessThread::GenerateSinCosTable(double Aphase, double Bphase)
{
    // send command to DSP thread to recalculate LO phase
    emit GenSinCosTable(Aphase, Bphase);
}



void ProcessThread::Start(void)
{
     qDebug() << "Process Thread ID = "  << thread()->currentThreadId();

     // select audio output buffer size based upon output sample rate
     // needs to match processing block size (40mS) * 5 = 200mS of data in bytes (*4)
     if(SampleRate == 192000) AudioOutputBufferSize = 153600;
     else AudioOutputBufferSize = 76800;

     // Timer used for Dual O/P (IP) and also for Audio Output to supplement the Notify signal
     P_Timer->start(40); // 40mS


     // Open UDP Socket for UDP Output Modes

     if((TIMF2Output == 1) || (RAW16Output == 1))
     {
        P_UdpSocket = new QUdpSocket(this);
        connect(P_UdpSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(on_UdpSocket_StateChanged()));
        connect(P_UdpSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(on_UdpSocket_Error()));
     }


     // Create a TCP Listening Server Socket on port 49812 if Linrad RAW16 Output Mode

     if(RAW16Output == 1)
     {
         QString Message;
         P_TcpServer = new QTcpServer(this);
         connect(P_TcpServer, SIGNAL(newConnection()), this, SLOT(on_TcpNewConnection(void)));
         P_TcpServer->setMaxPendingConnections(1);
         if ((P_TcpServer->listen( QHostAddress::Any,49812)) == false)
         {
             Message = "TCP Server Failed " + P_TcpServer->errorString();
         }
         else
         {
             Message = "TCP Server Listening for Linrad on Port 49812";
         }
         emit StatusMessage(Message);
         qDebug() << Message;
     }


     // Open Audio Output Device if Sound Card Output Mode

     if(SoundCardOutput != 0)
     {
        // Define the Output Audio Format
        QAudioFormat outputformat;
        if(SoundCardOutput == 1) outputformat.setChannelCount(2);  // set 2 audio channels
        if(SoundCardOutput == 2) outputformat.setChannelCount(4);  // set 4 audio channels
        outputformat.setSampleRate(SampleRate);                    // Audio Sample rate (bits/sec)
        outputformat.setSampleSize(16);                            // Sample Width (bits)
        outputformat.setCodec("audio/pcm");
        outputformat.setByteOrder(QAudioFormat::LittleEndian);
        outputformat.setSampleType(QAudioFormat::SignedInt);

        // get list of availavle audio output devices
        QList<QAudioDeviceInfo> OutputDeviceInfoList;
        OutputDeviceInfoList = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);

        // Open Audio Device for Channel A

        for(int index = 0; index < OutputDeviceInfoList.length(); index++)
        {
            if (SelectedOutputDevice_A == (OutputDeviceInfoList[index].deviceName()))
            {
                if (!OutputDeviceInfoList[index].isFormatSupported(outputformat))
                {
                     QString message = SelectedOutputDevice_A + ": Error! - Format Not Supported";
                     emit StatusMessage(message);
                }
                // open Audio Output Device if not already opened
                if(P_AudioDevice_A == nullptr)
                {
                    qDebug() << "Opening:  " + SelectedOutputDevice_A;
                    P_AudioDevice_A = new QAudioOutput(OutputDeviceInfoList[index], outputformat, this);
                }
            }
        }

        // Connect QAudioOutput signals for channel A
        connect(P_AudioDevice_A, SIGNAL(notify()), this, SLOT(on_AudioOutput_A_Notify()));
        connect(P_AudioDevice_A, SIGNAL(stateChanged(QAudio::State)), this, SLOT(on_AudioOutput_A_StateChanged()));

        // Set Buffer Size and Start Audio Output
        P_AudioDevice_A->setBufferSize(AudioOutputBufferSize);

        // Start Audio Output
        P_OutputBuffer_A = P_AudioDevice_A->start();

        qDebug() << "(A)Output Error Status = " + QString::number(P_AudioDevice_A->error());
        qDebug() << "(A)Output Volume = " + QString::number(P_AudioDevice_A->volume());
        qDebug() << "(A)Output Buffer Size (Bytes) = " + QString::number(P_AudioDevice_A->bufferSize());
        qDebug() << "(A)Output Write Size (Bytes) = " + QString::number(P_AudioDevice_A->periodSize());

        // QAudioOutput emits a Notify signal when the buffer is ready for 'periodSize()' bytes, this
        // seems to be equal to 1/5 of 'bufferSize()'. The setNotifyInterval defaults to 1000mS, so must
        // be set to a suitable period. It has been found that setting the NotifyInterval to 1mS
        // will work for all sample rates as the Notify signal will only trigger at 'periodSize()' periods.

        P_AudioDevice_A->setNotifyInterval(1);
        qDebug() << "(A)Output Notify Period (mS) = " + QString::number(P_AudioDevice_A->notifyInterval());

        // fill the outbuffer with zeros at start
        QByteArray TempBuffer; qint8 Z = 0;
        TempBuffer.insert(0, AudioOutputBufferSize,Z);
        int BytesWritten = P_OutputBuffer_A->write(TempBuffer); // write bytes to output device buffer
     }

     // take copy of current input buffer
     P_DSPthread->PreviousLastBuffer = A_LastBuffer;

     P_DSPthread->Finished = 1; // set false finish to kick start processing

     //Set DSP Mode and start DSP processing
     P_DSPthread->SampleRate = SampleRate;                 // copy sample rate
     P_DSPthread->DuplicateA = DuplicateA;                 // copy duplicate flag
     P_DSPthread->TIMF2Output = TIMF2Output;               // copy TIMF2Output flag
     P_DSPthread->RAW16Output = RAW16Output;               // copy RAW16Output flag
     P_DSPthread->SoundCardOutput = SoundCardOutput;       // copy Sound Card output flag

     // Start Processing                                   // Remove DSPMode 0 (Stop)
     if(DualOP == 0) P_DSPthread->DSPMode = 1;             // Start Processing Channel A only
     if(DualOP == 1) P_DSPthread->DSPMode = 2;             // Satrt processing Channel A & B

}


void ProcessThread::on_AudioOutput_A_Notify(void)
{
    ProcessData(); // Process and o/p data if using Audio Device
}


void ProcessThread::ProcessData(void)
{

int datagrams = 0;

    if(SoundCardOutput == 1)  // Process Output for 2 Channel Sound Card Device
    {
        // Get size of latest DSP thread Output Buffer data size for Audio Output

        AFBufferSize = P_DSPthread->InPoint - AFOutPoint;
        if(AFBufferSize < 0) AFBufferSize = AFBufferSize + P_DSPthread->CircularOutputBufferSize;

        // limit data size to available data or audio device free buffer

        int OutputDeviceFree =  P_AudioDevice_A->bytesFree(); // note: 4 output bytes per processed buffer sample
        if((AFBufferSize*4) < OutputDeviceFree) OutputDeviceFree = AFBufferSize*4;

        // Output to Audio Device 96KHz/192KHz Signal as 16 bit integer I/Q (4 bytes)

        qint16 OutInt16;
        qint8 IOutLow, IOutHigh;
        qint8 QOutLow, QOutHigh;
        QByteArray TempBuffer;

        for(int loop = 0; loop < (OutputDeviceFree/4); loop++ )   // 4 Bytes per sample

        {
            if(SelectB == 1) // Output Channel B
            {
                OutInt16 = (qint16) P_DSPthread->I_CircularOutputBufferSCB[AFOutPoint]; // I channel float to 16 bit integer
                IOutLow = OutInt16 & 0x00ff;
                IOutHigh = OutInt16 >> 8;
                OutInt16 = (qint16) P_DSPthread->Q_CircularOutputBufferSCB[AFOutPoint]; // Q channel float to 16 bit integer
                QOutLow = OutInt16 & 0x00ff;
                QOutHigh = OutInt16 >> 8;
            }
            else  // Output Channel A
            {
                OutInt16 = (qint16) P_DSPthread->I_CircularOutputBufferSCA[AFOutPoint]; // I channel float to 16 bit integer
                IOutLow = OutInt16 & 0x00ff;
                IOutHigh = OutInt16 >> 8;
                OutInt16 = (qint16) P_DSPthread->Q_CircularOutputBufferSCA[AFOutPoint]; // Q channel float to 16 bit integer
                QOutLow = OutInt16 & 0x00ff;
                QOutHigh = OutInt16 >> 8;
            }

            TempBuffer.append(IOutLow); TempBuffer.append(IOutHigh);    // append I/Q data bytes to output
            TempBuffer.append(QOutLow); TempBuffer.append(QOutHigh);

            AFOutPoint++;  // incremet output pointer with wrap arround
            if(AFOutPoint >= P_DSPthread->CircularOutputBufferSize) AFOutPoint = 0;           
        }

        int BytesWritten = P_OutputBuffer_A->write(TempBuffer);         // write bytes to output device buffer

        //qDebug() << "--------- BF " + QString::number(P_AudioDevice_A->bytesFree());
    }


    if(SoundCardOutput == 2)  // Process Output for 4 Channel Sound Card Device
    {
        // Get size of latest DSP thread Output Buffer data size for Audio Output

        AFBufferSize = P_DSPthread->InPoint - AFOutPoint;
        if(AFBufferSize < 0) AFBufferSize = AFBufferSize + P_DSPthread->CircularOutputBufferSize;

        // limit data size to available data or audio device free buffer

        int OutputDeviceFree =  P_AudioDevice_A->bytesFree(); // note: 8 output bytes per processed buffer sample
        if((AFBufferSize*8) < OutputDeviceFree) OutputDeviceFree = AFBufferSize*8;

        // Output to Audio Device 96KHz/192KHz Signal as 16 bit integer I/Q (4 bytes)

        qint16 OutInt16;
        qint8 IOutLow, IOutHigh;
        qint8 QOutLow, QOutHigh;
        QByteArray TempBuffer;

        LatestOutputDataIndex = AFOutPoint;  // set index for data used by MainWindow Phase display

        for(int loop = 0; loop < (OutputDeviceFree/8); loop++ )    // 8 Bytes per sample

        {
            // For Channel A
            OutInt16 = (qint16) P_DSPthread->I_CircularOutputBufferSCA[AFOutPoint]; // I channel float to 16 bit integer
            IOutLow = OutInt16 & 0x00ff;
            IOutHigh = OutInt16 >> 8;
            OutInt16 = (qint16) P_DSPthread->Q_CircularOutputBufferSCA[AFOutPoint]; // Q channel float to 16 bit integer
            QOutLow = OutInt16 & 0x00ff;
            QOutHigh = OutInt16 >> 8;
            TempBuffer.append(IOutLow); TempBuffer.append(IOutHigh);    // append I/Q data bytes to output
            TempBuffer.append(QOutLow); TempBuffer.append(QOutHigh);

            // For Channel B
            OutInt16 = (qint16) P_DSPthread->I_CircularOutputBufferSCB[AFOutPoint]; // I channel float to 16 bit integer
            IOutLow = OutInt16 & 0x00ff;
            IOutHigh = OutInt16 >> 8;
            OutInt16 = (qint16) P_DSPthread->Q_CircularOutputBufferSCB[AFOutPoint]; // Q channel float to 16 bit integer
            QOutLow = OutInt16 & 0x00ff;
            QOutHigh = OutInt16 >> 8;
            TempBuffer.append(IOutLow); TempBuffer.append(IOutHigh);    // append I/Q data bytes to output
            TempBuffer.append(QOutLow); TempBuffer.append(QOutHigh);

            AFOutPoint++;  // incremet output pointer with wrap arround
            if(AFOutPoint >= P_DSPthread->CircularOutputBufferSize) AFOutPoint = 0;          
        }

        int BytesWritten = P_OutputBuffer_A->write(TempBuffer);         // write bytes to output device buffer

        //qDebug() << "--------- BF " + QString::number(P_AudioDevice_A->bytesFree());
    }


    // Output to Network if MAP65 (TIMF2) UDP Output Mode

    if(TIMF2Output == 1)
    {      
        static unsigned short int BlockNumber = 0;
        static int iptr = 0;
        static int PayloadSamples = 0;

        // UDP Head = 24 byte header
        // UDP Payload = 1392 bytes

        // Select payload samples based upon single or dual output mode
        if(DualOP == 1) PayloadSamples = PAYLOAD/16;  // 16 output bytes per sample
        else PayloadSamples = PAYLOAD/8;              //  8 output bytes per sample

        // calculate size of available data from circular buffer
        IPBufferSize = P_DSPthread->InPoint - IPOutPoint;
        if(IPBufferSize < 0) IPBufferSize = IPBufferSize + P_DSPthread->CircularOutputBufferSize;
        int IPBufferRemain = IPBufferSize;

        // Loop to send UDP packets when there is sufficent data available

        while(IPBufferRemain >= (PayloadSamples))
        {
            // set index for data used by MainWindow Phase display
            LatestOutputDataIndex = IPOutPoint;

            // build output datagram data
            QByteArray TempBuffer(HEAD,0); // initiate with 24 bytes for header
            NET_RX_STRUCT *Header = (NET_RX_STRUCT*)TempBuffer.data();
            Header->passband_center = (double)CentreFrequency/1000;
            Header->time = 1000*clock()/(CLOCKS_PER_SEC);
            Header->userx_freq = 96000;
            Header->ptr = iptr;
            Header->block_no = BlockNumber++;
            Header->passband_direction = -1;

            if(DualOP == 1) Header->userx_no = -2;  // -2 for two channel, float data format
            else Header->userx_no = -1;             // -1 for single channel, float data format

            iptr = iptr + 360;   // increment Linrad block pointer with 1016 wraparround
            if(iptr >= 1016) iptr = iptr - 1016;

            for(int loop = 0; loop < (PayloadSamples); loop++)
            {
                // send dual channel data, convert double to float
                floatUnion.f = (float)P_DSPthread->I_CircularOutputBufferA[IPOutPoint];
                TempBuffer.append(floatUnion.bytes[0]);
                TempBuffer.append(floatUnion.bytes[1]);
                TempBuffer.append(floatUnion.bytes[2]);
                TempBuffer.append(floatUnion.bytes[3]);
                floatUnion.f = (float)P_DSPthread->Q_CircularOutputBufferA[IPOutPoint];
                TempBuffer.append(floatUnion.bytes[0]);
                TempBuffer.append(floatUnion.bytes[1]);
                TempBuffer.append(floatUnion.bytes[2]);
                TempBuffer.append(floatUnion.bytes[3]);

                if(DualOP == 1)  // send channel B only in dual output mode
                {
                    floatUnion.f = (float)P_DSPthread->I_CircularOutputBufferB[IPOutPoint];
                    TempBuffer.append(floatUnion.bytes[0]);
                    TempBuffer.append(floatUnion.bytes[1]);
                    TempBuffer.append(floatUnion.bytes[2]);
                    TempBuffer.append(floatUnion.bytes[3]);
                    floatUnion.f = (float)P_DSPthread->Q_CircularOutputBufferB[IPOutPoint];
                    TempBuffer.append(floatUnion.bytes[0]);
                    TempBuffer.append(floatUnion.bytes[1]);
                    TempBuffer.append(floatUnion.bytes[2]);
                    TempBuffer.append(floatUnion.bytes[3]);
                }

                IPOutPoint++;  // incremet output pointer with wrap arround
                if(IPOutPoint >= P_DSPthread->CircularOutputBufferSize) IPOutPoint = 0;

                IPBufferRemain--;
            }

            int BytesSent = 0;
            int retry = 0;
            while(retry < 3) // try 3 times
            {
             // Send datagram, slow network will result in errors, data rate is at least 12.5Mbs
             BytesSent = P_UdpSocket->writeDatagram(TempBuffer,QHostAddress(IPAddress),50004);
             if(BytesSent == (HEAD+PAYLOAD)) break; // all sent correctly
             usleep(5000);  // if not sent correctly, wait and try again.
             retry++;
            }

            if(retry > 0) qDebug() << "retry = " + QString::number(retry);

            if(BytesSent == -1){QString Message = "Error Sending Datagram "; emit StatusMessage(Message);}

            // allow time for packets to send evenly
            usleep(700); // 900 (0.00090625 Sec per datagram) is theoretical max

            datagrams++;
        }

    }


    // Output to Network if Linrad (RAW16) UDP Output Mode

    if(RAW16Output == 1)
    {
        static unsigned short int BlockNumber = 0;
        static int Pointer = 0;

        // UDP Head = 24 byte header
        // UDP Payload = 1392 bytes

        // calculate size of available data from circular buffer
        IPBufferSize = P_DSPthread->InPoint - IPOutPoint;
        if(IPBufferSize < 0) IPBufferSize = IPBufferSize + P_DSPthread->CircularOutputBufferSize;
        int IPBufferRemain = IPBufferSize;

        // Loop to send UDP packets when there is sufficent data available

        while(IPBufferRemain >= (PAYLOAD/8))   // Note: 8 output bytes per sample for RAW16
        {
            // set index for data used by MainWindow Phase display
            LatestOutputDataIndex = AFOutPoint;

            // calculate pointer value
            Pointer+=PAYLOAD;
            if(Pointer >= Linrad_Block_Size) Pointer-=Linrad_Block_Size;

            // build output datagram data
            QByteArray TempBuffer(HEAD,0); // initiate with 24 bytes for header
            NET_RX_STRUCT *Header = (NET_RX_STRUCT*)TempBuffer.data();
            Header->passband_center = 0.048;
            Header->time = 1000*clock()/(CLOCKS_PER_SEC);
            Header->userx_freq = 96000;
            Header->ptr = Pointer;
            Header->block_no = BlockNumber++;
            Header->userx_no = 0xff;
            Header->passband_direction = 0x01;

            for(int loop = 0; loop < (PAYLOAD/8); loop++)  // Note: 8 output bytes per sample for RAW16
            {
                // send dual channel soundcard data, convert double to 16bit integer
                intUnion.i32 = (qint32) P_DSPthread->I_CircularOutputBufferSCA[AFOutPoint];
                TempBuffer.append(intUnion.bytes[0]);
                TempBuffer.append(intUnion.bytes[1]);
                intUnion.i32 = (qint32) P_DSPthread->Q_CircularOutputBufferSCA[AFOutPoint];
                TempBuffer.append(intUnion.bytes[0]);
                TempBuffer.append(intUnion.bytes[1]);
                intUnion.i32 = (qint32) P_DSPthread->I_CircularOutputBufferSCB[AFOutPoint];
                TempBuffer.append(intUnion.bytes[0]);
                TempBuffer.append(intUnion.bytes[1]);
                intUnion.i32 = (qint32) P_DSPthread->Q_CircularOutputBufferSCB[AFOutPoint];
                TempBuffer.append(intUnion.bytes[0]);
                TempBuffer.append(intUnion.bytes[1]);

                AFOutPoint++;  // incremet output pointer with wrap arround
                if(AFOutPoint >= P_DSPthread->CircularOutputBufferSize) AFOutPoint = 0;

                IPBufferRemain--;
            }

            int BytesSent = 0;
            int retry = 0;
            while(retry < 3) // try 3 times
            {
             // Send datagram, slow network will result in errors, data rate is at least 12.5Mbs
             BytesSent = P_UdpSocket->writeDatagram(TempBuffer,QHostAddress(IPAddress),50000);
             if(BytesSent == (HEAD+PAYLOAD)) break; // all sent correctly
             usleep(5000);  // if not sent correctly, wait and try again.
             retry++;
            }

            if(retry > 0) qDebug() << "retry = " + QString::number(retry);

            if(BytesSent == -1){QString Message = "Error Sending Datagram "; emit StatusMessage(Message);}

            // allow time for packets to send evenly
            usleep(700); // 900 (0.00090625 Sec per datagram) is theoretical max?
                         // Changing this to 1000 will output without network errors
                         // but not enough packets are sent within the time?
                         // Require 1 packet each 1.812 mS or 552 packest per sec.

            datagrams++;
        }

    }

    //qDebug() << "Datagrams Sent = " + QString::number(datagrams);

}


void ProcessThread::on_AudioOutput_A_StateChanged(void)
{
    int state =  P_AudioDevice_A->state();
    QString State, Message;
    if(state == 0) State = "Active";
    if(state == 1) State = "Suspended";
    if(state == 2) State = "Stopped";
    if(state == 3) State = "Idle";
    if(state == 4) State = "Interrupted";
    Message = "Soundcard Stream: " + State;
    emit StatusMessage(Message);
    qDebug() << Message;
}

void ProcessThread::on_UdpSocket_StateChanged(void)
{
    int state = P_UdpSocket->state();
    QString State, Message;
    if(state == 0) State = "Unconnected";
    if(state == 1) State = "HostLookup";
    if(state == 2) State = "Connecting";
    if(state == 3) State = "Connected";
    if(state == 4) State = "Bound";
    if(state == 5) State = "Listening";
    if(state == 6) State = "Closing";
    Message = ("UDP SocketState: " + State);
    emit StatusMessage(Message);
    qDebug() << Message;
}

void ProcessThread::on_UdpSocket_Error(void)
{
    QString Message;
    int error = P_UdpSocket->error();
    if(error == 7) Message = "UDP Socket: Network Error";
    else Message = "UDP SocketError: " + QString::number(error);
    emit StatusMessage(Message);
}

void ProcessThread::on_TcpNewConnection(void)
{
        P_TcpSocket = P_TcpServer->nextPendingConnection();
        connect(P_TcpSocket, SIGNAL(readyRead()), this, SLOT(on_TcpSocket_readReady(void)));
        qDebug() << "TCP New Connection";
}

void ProcessThread::on_TcpSocket_readReady(void)
{
  qDebug() << "Received Read Ready Signal!";
  QByteArray Data = P_TcpSocket->readAll();
  if(Data[0] == 0xb8)
  {
      qDebug() << "Rx Mode Request";

      // build output datagram data
      QByteArray TempBuffer(32,0);
      NETMSG_MODE_REQUEST_STRUCT *Datagram = (NETMSG_MODE_REQUEST_STRUCT*)TempBuffer.data();
      Datagram->sample_rate = 96000;                // Sample Rate
      Datagram->real_channels = 4;                  // Total Streams of Data
      Datagram->rf_channels = 2;                    // Channels
      Datagram->rx_input_mode = 6;                  // I/Q Format
      Datagram->block_bytes = Linrad_Block_Size;
      Datagram->fft1_size = 0x0800;                 // Not used?
      Datagram->fft1_n = 0x0b;                      // Not used?
      Datagram->First_FFT_Sinpow = 0x02;            // Not used?

      qint64 BytesWritten = P_TcpSocket->write(TempBuffer);
      P_TcpSocket->flush();
      QString Message = "Linrad Mode Request Acknowledged";
      emit StatusMessage(Message);
      qDebug() << Message;
  }

  // respond to linrad cal requests
  if((Data[0] == 0xb5) || (Data[0] == 0xb6))
  {
      QByteArray TempBuffer;
      TempBuffer.resize(1);
      TempBuffer[0] = 0x00;
      qint64 BytesWritten = P_TcpSocket->write(TempBuffer);
      qDebug() << "BytesWritten = " + QString::number(BytesWritten);
      P_TcpSocket->flush();

  }
}

void ProcessThread::Stop(void)
{
     P_Timer->stop();

     P_DSPthread->DSPMode = 0; // stop processing

     // stop and delete audio channel A if open
     if(P_AudioDevice_A != nullptr)
     {
         P_AudioDevice_A->stop();
         delete P_AudioDevice_A;
         P_AudioDevice_A = nullptr;
     }

     // close UDP Socket if open
     if(P_UdpSocket != nullptr)
     {
         P_UdpSocket->close();
         delete P_UdpSocket;
         P_UdpSocket = nullptr;
     }

     // close TCP Server if open
     if(P_TcpServer != nullptr)
     {
         P_TcpServer->close();
         delete P_TcpServer;
         P_TcpServer = nullptr;
     }

 }




