// RSPduoEME Copyright 2020 David Warwick G4EEV
//
// This file is part of RSPduoEME.
//
// RSPduoEME is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, either version 3 of the License, or(at your option) any later version.
// RSPduoEME is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have
// received a copy of the GNU General Public License along with RSPduoEMRE, if not, see <https://www.gnu.org/licenses/>.


// Version 1-32 1/7/2022:  Added checkbox to allow Phase Display Test Mode which allows the Phase Error to be displayed
//                         continuously after initial lock to allow phase error calibration on live signal.

// Version 1-33 6/6/23:    Corrected TIMF2 single channel output mode to correctly send single channel data and
//                         corrected Centre Frequency value to TIMF2 header to display in Qmap. Moved the
//                         position of the Phase Test Mode tickbox and adjusted the main display height.
//              24/8/23    Added facility to independently change the IF gain of both tuners.


#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "rspduointerface.h"
#include "processthread.h"

#include <QDebug>
#include <QSettings>
#include <QPainter>
#include <QtMath>
#include <QPoint>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Start Process Thread
    P_ProcessThread = new ProcessThread;
    P_ProcessThread->moveToThread(&WorkerThread);
    WorkerThread.start();
    WorkerThread.setPriority(QThread::TimeCriticalPriority);

    // Initalise RSPduo interface object
    P_RSPduo = new RSPduoInterface;

    // Initalise Timer object
    P_Timer = new QTimer(this);

    // Connect Signals and Slots
    connect(P_RSPduo, SIGNAL(Status(QString)), this, SLOT(DisplayStatus(QString)));
    connect(P_Timer, SIGNAL(timeout()), this, SLOT(on_Timer()));
    connect(this, SIGNAL(StartProcessThread()), P_ProcessThread, SLOT(Start()));
    connect(this, SIGNAL(StopProcessThread()), P_ProcessThread, SLOT(Stop()));
    connect(P_ProcessThread, SIGNAL(StatusMessage(QString)), this, SLOT(DisplayStatus(QString)));
    connect(this, SIGNAL(GenerateSinCosTable(double,double)), P_ProcessThread, SLOT(GenerateSinCosTable(double,double)));

    // Read Saved Settings if available or use defaults provided
    QSettings settings("G4EEV", "RSPduoStream");
    SelectedOutputDevice_A = settings.value("SelectedOutputDevice_A",SelectedOutputDevice_A).toString();
    SelectedMode = settings.value("SelectedMode", SelectedMode).toString();
    CentreFrequency = settings.value("CentreFrequency",CentreFrequency).toInt();
    IFGainA = settings.value("IFGainA",IFGainA).toInt();
    IFGainB = settings.value("IFGainB",IFGainB).toInt();
    LNAGain = settings.value("LNAGain",LNAGain).toInt();
    IPAddress = settings.value("IPAddress",IPAddress).toString();
    RequiredPhase = settings.value("RequiredPhase",RequiredPhase).toInt();
    SelectedCalPort = settings.value("SelectedCalPort",SelectedCalPort).toString();
    AutoCal = settings.value("AutoCal", AutoCal).toInt();

    // Build list of available output audio devices
    OutputDeviceInfoList = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);

    // Fill Output Device Channel A combo box with available devices

    // loop first to find saved device and put on top if available
    for(int i=0; i < OutputDeviceInfoList.length(); i++)
    {
        if(OutputDeviceInfoList[i].deviceName() == SelectedOutputDevice_A)
        ui->OutputAcomboBox->addItem(OutputDeviceInfoList[i].deviceName());
    }
    // loop second time to fill remaining device names
    for(int i=0; i < OutputDeviceInfoList.length(); i++)
    {
        if(OutputDeviceInfoList[i].deviceName() != SelectedOutputDevice_A)
        ui->OutputAcomboBox->addItem(OutputDeviceInfoList[i].deviceName());
    }

    // Fill Mode combo box with available modes

    // loop first to find saved device and put on top if available
    for(int i=0; i < ModeList.length(); i++)
    {
        if(ModeList[i] == SelectedMode) ui->ModeComboBox->addItem(ModeList[i]);
    }
    // loop second time to fill remaining Modes
    for(int i=0; i < ModeList.length(); i++)
    {
        if(ModeList[i] != SelectedMode) ui->ModeComboBox->addItem(ModeList[i]);
    }

    // setup CentreFrequency inital value text edit with current value showing
    ui->FrequencyLineEdit->setInputMask("0000000");
    ui->FrequencyLineEdit->insert(QString::number(CentreFrequency));

    // setup IFgainA inital value spinbox with current value showing
    ui->GainSpinBoxA->setValue(IFGainA);
    ui->GainSpinBoxA->setRange(20,59);

    // setup IFgainB inital value spinbox with current value showing
    ui->GainSpinBoxB->setValue(IFGainB);
    ui->GainSpinBoxB->setRange(20,59);

    // setup LNAgain initial value spinbox with current value
    ui->LNAspinBox->setValue(LNAGain);
    ui->LNAspinBox->setRange(0,9);

    // set IPAddress line edit mask and initial value
    ui->IPLineEdit->setInputMask("000.000.000.000; ");
    ui->IPLineEdit->setText(IPAddress);

    // set Phase Spinbox values
    ui->PhaseSpinBox->setRange(-180,180); // degrees
    ui->PhaseSpinBox->setValue(RequiredPhase);

    // Fill Calibrator Port combo box with avaiable ports
    QList<QSerialPortInfo> PortList = QSerialPortInfo::availablePorts();

    // loop first to find saved port and put on top if available
    for(int i=0; i < PortList.length(); i++)
    {
        if(PortList[i].portName() == SelectedCalPort)
        ui->CalPortBox->addItem(PortList[i].portName());
    }
    if(SelectedCalPort == "None") ui->CalPortBox->addItem("None");

    // loop second time to fill remaining ports
    for(int i=0; i < PortList.length(); i++)
    {
        if(PortList[i].portName() != SelectedCalPort)
        ui->CalPortBox->addItem(PortList[i].portName());
    }
    if(SelectedCalPort != "None") ui->CalPortBox->addItem("None");

    // set Auto Calibration Checkbox to current value
    if(AutoCal == 1) ui->AutoCalCheckBox->setChecked(Qt::Checked);
    else ui->AutoCalCheckBox->setChecked(Qt::Unchecked);

    UpdateParameters(); // ensure processing parameters are set with initial values

    //qDebug() << "MainWindow Thread ID = "  << thread()->currentThreadId();
}

MainWindow::~MainWindow()
{
    // stop ProcessThread
    WorkerThread.quit();
    WorkerThread.wait();

    // ensure calibrator is left out of band and close port
    if(P_CalPort == nullptr) OpenCalibratorPort();
    CalFrequency = 4001000;
    QString buffer = QString::number(CalFrequency);
    if(P_CalPort != nullptr)
    {
        P_CalPort->write(buffer.toLatin1());
        P_CalPort->waitForReadyRead(2000);
        // wait for echo back before closing port
        CloseCalibratorPort();
    }

    // delete created objects
    delete ui;

}

void MainWindow::closeEvent(QCloseEvent *event)  // Save settings when closing MainWindow
{
    QSettings settings("G4EEV", "RSPduoStream");
    settings.setValue("SelectedOutputDevice_A", SelectedOutputDevice_A);
    settings.setValue("SelectedMode", SelectedMode);
    settings.setValue("CentreFrequency", CentreFrequency);
    settings.setValue("IFGainA", IFGainA);
    settings.setValue("IFGainB", IFGainB);
    settings.setValue("LNAGain",LNAGain);
    settings.setValue("IPAddress",IPAddress);
    settings.setValue("RequiredPhase", RequiredPhase);
    settings.setValue("SelectedCalPort", SelectedCalPort);
    settings.setValue("AutoCal",AutoCal);

}


void MainWindow::on_Timer()
{
    update(); // redraw phase display

    // timeout phase display after zeroing phase
    if(ui->PhasePushButton->isEnabled() == false) PhaseDisplayTimeout++;

    // verify process times are meeting performance requirements

    int loop = 0; double AverageTime = 0;
    while(!P_ProcessThread->P_DSPthread->ProcessTimes->isEmpty())
    {
        AverageTime += P_ProcessThread->P_DSPthread->ProcessTimes->takeFirst();
        loop++;
    }
    if(loop != 0) AverageTime /= loop;
    // 40ms is required for each DSP process, warn if getting close to this
    if(AverageTime > 0.035)
    {
        QString Message;
        Message = QString::asprintf("Warning: DSP Process Time = %2.0f%%",100*AverageTime/0.040);
        //ui->StatusTextEdit->appendPlainText(Message);
        qDebug() << Message;
    }
}


void MainWindow::reject(void)
{
    // if window is dismissed ensure RSP duo is stopped first
    ui->StartButton->setText("Stop");
    on_StartButton_clicked();
}


void MainWindow::UpdateParameters(void)
{
    // Update Processing Parameters (when inputs have changed)

    // allow update to SelectB while running
    if(SelectedMode == "6: Dual UDP (MAP65) + Ch A Soundcard") P_ProcessThread->SelectB = 0;
    if(SelectedMode == "7: Dual UDP (MAP65) + Ch B Soundcard") P_ProcessThread->SelectB = 1;

    // only update parameters when in stopped state
    if(Processing == 1) return;

    // copy variables to process thread
    P_ProcessThread->SelectedOutputDevice_A = SelectedOutputDevice_A;
    P_ProcessThread->IPAddress = IPAddress;
    P_ProcessThread->CentreFrequency = CentreFrequency;

    // select variables according to required mode

    if(SelectedMode == "1: Channel A, 96000 -> Soundcard")
    {
        P_ProcessThread->SampleRate = 96000;
        DualOP = 0;
        P_ProcessThread->DualOP = DualOP;
        DuplicateA = 0;
        P_ProcessThread->DuplicateA = DuplicateA;
        P_ProcessThread->TIMF2Output = 0;
        P_ProcessThread->RAW16Output = 0;
        P_ProcessThread->SoundCardOutput = 1;
        P_ProcessThread->SelectB = 0;
        // set MainWindow reduced height
        resize(InitWindowWidth,InitWindowHeight-PhaseDisplayHeight);
    }
    if(SelectedMode == "2: Channel A, 192000 -> Soundcard")
    {
        P_ProcessThread->SampleRate = 192000;
        DualOP = 0;
        P_ProcessThread->DualOP = DualOP;
        DuplicateA = 0;
        P_ProcessThread->DuplicateA = DuplicateA;
        P_ProcessThread->TIMF2Output = 0;
        P_ProcessThread->RAW16Output = 0;
        P_ProcessThread->SoundCardOutput = 1;
        P_ProcessThread->SelectB = 0;
        // set MainWindow reduced height
        resize(InitWindowWidth,InitWindowHeight-PhaseDisplayHeight);
    }
    if(SelectedMode == "3: Channel A, 96000 -> UDP (MAP65)")
    {
        P_ProcessThread->SampleRate = 96000;
        DualOP = 0;
        P_ProcessThread->DualOP = DualOP;
        DuplicateA = 0;
        P_ProcessThread->DuplicateA = DuplicateA;
        P_ProcessThread->TIMF2Output = 1;
        P_ProcessThread->RAW16Output = 0;
        P_ProcessThread->SoundCardOutput = 0;
        P_ProcessThread->SelectB = 0;
        // set MainWindow reduced height
        resize(InitWindowWidth,InitWindowHeight-PhaseDisplayHeight);
    }
    if(SelectedMode == "4: Dual A + A, 96000 -> UDP (MAP65)")
    {
        P_ProcessThread->SampleRate = 96000;
        DualOP = 1;
        P_ProcessThread->DualOP = DualOP;
        DuplicateA = 1;
        P_ProcessThread->DuplicateA = DuplicateA;
        P_ProcessThread->TIMF2Output = 1;
        P_ProcessThread->RAW16Output = 0;
        P_ProcessThread->SoundCardOutput = 0;
        P_ProcessThread->SelectB = 0;
        // set MainWindow reduced height
        resize(InitWindowWidth,InitWindowHeight-PhaseDisplayHeight);
    }
    if(SelectedMode == "5: Dual A & B, 96000 -> UDP (MAP65)")
    {
        P_ProcessThread->SampleRate = 96000;
        DualOP = 1;
        P_ProcessThread->DualOP = DualOP;
        DuplicateA = 0;
        P_ProcessThread->DuplicateA = DuplicateA;
        P_ProcessThread->TIMF2Output = 1;
        P_ProcessThread->RAW16Output = 0;
        P_ProcessThread->SoundCardOutput = 0;
        P_ProcessThread->SelectB = 0;
        // set MainWindow larger size for phase display
        resize(InitWindowWidth,InitWindowHeight);
    }
    if(SelectedMode == "6: Dual UDP (MAP65) + Ch A Soundcard")
    {
        P_ProcessThread->SampleRate = 96000;
        DualOP = 1;
        P_ProcessThread->DualOP = DualOP;
        DuplicateA = 0;
        P_ProcessThread->DuplicateA = DuplicateA;
        P_ProcessThread->TIMF2Output = 1;
        P_ProcessThread->RAW16Output = 0;
        P_ProcessThread->SoundCardOutput = 1;
        P_ProcessThread->SelectB = 0;
        // set MainWindow larger size for phase display
        resize(InitWindowWidth,InitWindowHeight);
    }
    if(SelectedMode == "7: Dual UDP (MAP65) + Ch B Soundcard")
    {
        P_ProcessThread->SampleRate = 96000;
        DualOP = 1;
        P_ProcessThread->DualOP = DualOP;
        DuplicateA = 0;
        P_ProcessThread->DuplicateA = DuplicateA;
        P_ProcessThread->TIMF2Output = 1;
        P_ProcessThread->RAW16Output = 0;
        P_ProcessThread->SoundCardOutput = 1;
        P_ProcessThread->SelectB = 1;
        // set MainWindow larger size for phase display
        resize(InitWindowWidth,InitWindowHeight);
    }
    if(SelectedMode == "8: Dual A & B -> UDP (Linrad RAW16)")
    {
        P_ProcessThread->SampleRate = 96000;
        DualOP = 1;
        P_ProcessThread->DualOP = DualOP;
        DuplicateA = 0;
        P_ProcessThread->DuplicateA = DuplicateA;
        P_ProcessThread->TIMF2Output = 0;
        P_ProcessThread->RAW16Output = 1;
        P_ProcessThread->SoundCardOutput = 0;
        P_ProcessThread->SelectB = 0;
        // set MainWindow larger size for phase display
        resize(InitWindowWidth,InitWindowHeight);
    }

    if(SelectedMode == "9: Dual A & B -> 4 Ch Soundcard")
    {
        P_ProcessThread->SampleRate = 96000;
        DualOP = 1;
        P_ProcessThread->DualOP = DualOP;
        DuplicateA = 0;
        P_ProcessThread->DuplicateA = DuplicateA;
        P_ProcessThread->TIMF2Output = 0;
        P_ProcessThread->RAW16Output = 0;
        P_ProcessThread->SoundCardOutput = 2;
        P_ProcessThread->SelectB = 0;
        // set MainWindow larger size for phase display
        resize(InitWindowWidth,InitWindowHeight);
    }

}


void MainWindow::on_StartButton_clicked()
{
    if(ui->StartButton->text() == "Start")
    {
        P_RSPduo->Start(CentreFrequency*1000, IFGainA, IFGainB, LNAGain);
        P_Timer->start(100); // update phase dispaly

        // ensure current parameters are set
        UpdateParameters();

        // reset oscillator phase to zero
        emit GenerateSinCosTable(0,0);

        // enable the phase correction button if dual mode
        // and open and set the calibration signal
        if(DualOP == 1)
        {
            ui->PhasePushButton->setEnabled(true);
            PhaseDisplayTimeout = 0;

            OpenCalibratorPort();
            CalFrequency = CentreFrequency + 45; // off tune by 45KHz for best diapley
            QString buffer = QString::number(CalFrequency);
            if(P_CalPort != nullptr)
            {
                P_CalPort->write(buffer.toLatin1());
                DisplayStatus("Calibrator Port " + SelectedCalPort + " set to " + buffer + "KHz");
            }
        }

        // change button text and set processing flag valid
        ui->StartButton->setText("Stop");
        Processing = 1;

        // Start the Process Thread
        emit StartProcessThread();
    }

    else if(ui->StartButton->text() == "Stop")
    {
        emit StopProcessThread();
        P_RSPduo->Stop();
        P_Timer->stop();
        ui->StartButton->setText("Start");
        Processing = 0; // reset processing flag
        CloseCalibratorPort();
    }
}


void MainWindow::DisplayStatus(QString MessageString)
{
    ui->StatusTextEdit->appendPlainText(MessageString);
}


void MainWindow::on_OutputAcomboBox_currentTextChanged(const QString &arg1)
{
        SelectedOutputDevice_A = arg1;
}


void MainWindow::on_FrequencyLineEdit_editingFinished()
{
        CentreFrequency = ui->FrequencyLineEdit->text().toInt();
}


void MainWindow::on_ModeComboBox_currentIndexChanged(const QString &arg1)
{
    SelectedMode = arg1;
    UpdateParameters();
}


void MainWindow::on_GainSpinBoxA_valueChanged(int arg1)
{
    IFGainA = arg1;
    // update IFgain on RSPduo if running
    if(Processing == 1) P_RSPduo->ChangeIFGainA(IFGainA);
}


void MainWindow::on_GainSpinBoxB_valueChanged(int arg1)
{
    IFGainB = arg1;
    // update IFgain on RSPduo if running
    if(Processing == 1) P_RSPduo->ChangeIFGainB(IFGainA,IFGainB);
}


void MainWindow::on_LNAspinBox_valueChanged(int arg1)
{
    LNAGain = arg1;
    // update LNAgain on RSPduo if running
    if(Processing == 1) P_RSPduo->ChangeLNAGain(LNAGain,IFGainA,IFGainB);
}


void MainWindow::on_IPLineEdit_textEdited(const QString &arg1)
{
     IPAddress = arg1;
}


void MainWindow::on_PhaseSpinBox_valueChanged(int arg1)
{
    RequiredPhase = arg1;
}


void MainWindow::on_PhasePushButton_clicked()
{
    emit GenerateSinCosTable(0,PhaseCorrection);
    ui->PhasePushButton->setEnabled(false);
    PhaseDisplayTimeout = 0;
}


void MainWindow::on_CalPortBox_currentTextChanged(const QString &arg1)
{
    SelectedCalPort = arg1;
}


void MainWindow::OpenCalibratorPort(void)
{
    // only open if closed and port selected
    if((P_CalPort == nullptr) && (SelectedCalPort != "None"))
    {
        P_CalPort = new QSerialPort;
        P_CalPort->setPortName(SelectedCalPort);
        P_CalPort->setBaudRate(QSerialPort::Baud9600);
        P_CalPort->setDataBits(QSerialPort::Data8);
        P_CalPort->setParity(QSerialPort::NoParity);
        P_CalPort->setStopBits(QSerialPort::TwoStop);
        P_CalPort->setFlowControl(QSerialPort::NoFlowControl);
        if(!P_CalPort->open(QIODevice::ReadWrite))
        {
        DisplayStatus("Calibration Port " + SelectedCalPort + " Failed to Open");
        P_CalPort = nullptr;
        }

    }
}


void MainWindow::CloseCalibratorPort(void)
{
    // close port if open
    if(P_CalPort != nullptr)
    {
        P_CalPort->close();
        P_CalPort = nullptr;
    }

}


void MainWindow::on_AutoCalCheckBox_clicked(bool checked)
{
    AutoCal = checked;
}


void MainWindow::on_PhaseTestModeCheckBox_clicked(bool checked)
{
    PhaseTestMode = checked;
}


void MainWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    const int BufferSize = PAYLOAD/16; // UDP payload(1392) / 4 bytes per float / 2 streams / 2 channels.

    // set top left and size of background display
    int BackgroundX = 22;
    int BackgroundY = 393;
    int BackgroundWidth = 106+(2*BufferSize);
    int BackgroundHeight = 86;

    // Set origin and radius of phase display
    int OriginX = 65;
    int OriginY = BackgroundY+43;
    int Radius = 35;

    // set top left and size of oscilloscope display
    int ScopeX = 120;
    int ScopeY = BackgroundY+43-35;
    int ScopeWidth = 2*BufferSize;
    int ScopeHeight = 35+35;
    int ScopeOrigin = BackgroundY+43;

    // draw phase and oscilloscope baskground
    painter.setPen(QPen(Qt::darkGray, 0, Qt::SolidLine, Qt::RoundCap));
    painter.setBrush(QBrush(Qt::white,Qt::SolidPattern));
    painter.drawRect(BackgroundX,BackgroundY,BackgroundWidth,BackgroundHeight);

    // draw background for phase display
    painter.setPen(QPen(Qt::black, 0, Qt::SolidLine, Qt::RoundCap));
    painter.drawEllipse(QPoint(OriginX,OriginY),Radius,Radius); // Circle   
    // draw phase zero line on display
    painter.setPen(QPen(Qt::red, 0, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(OriginX,OriginY,OriginX,OriginY-Radius);

    // draw background for Oscilloscope display
    painter.setPen(QPen(Qt::black, 0, Qt::SolidLine, Qt::RoundCap));
    painter.drawRect(ScopeX,ScopeY,ScopeWidth,ScopeHeight);

    // label the channels
    painter.setPen(QPen(Qt::red, 0, Qt::SolidLine, Qt::RoundCap));
    painter.drawText(OriginX+Radius+7,OriginY-Radius+9,"A");
    painter.setPen(QPen(Qt::blue, 0, Qt::SolidLine, Qt::RoundCap));
    painter.drawText(OriginX+Radius+7,OriginY+Radius,"B");

    if(Processing == 0) return;                      // don't display data until started
    if((DualOP == 0) || (DuplicateA == 1)) return;   // only display phase if Dual A & B Mode

    double phaseA, phaseB, phaseDif, X, Y;
    double phaseAv = 0;   // for average phase error
    double phaseMax = 0;  // for maximum phase error
    double phaseMin = 2*M_PI;  // for minimum phase error

    static double IAcopy[BufferSize];
    static double QAcopy[BufferSize];
    static double IBcopy[BufferSize];
    static double QBcopy[BufferSize];

    // loop to read new TIMF2 channel data until set is pressed
    // then continue until timeout is 10 (about 1 second)
    // unless PhaseTestMode is set.

    if((PhaseDisplayTimeout < 10) || (PhaseTestMode == 1))
    {
        int index = P_ProcessThread->LatestOutputDataIndex;
        for(int loop = 0; loop < BufferSize; loop++)
        {
            IAcopy[loop] = P_ProcessThread->P_DSPthread->I_CircularOutputBufferA[index];
            QAcopy[loop] = P_ProcessThread->P_DSPthread->Q_CircularOutputBufferA[index];
            IBcopy[loop] = P_ProcessThread->P_DSPthread->I_CircularOutputBufferB[index];
            QBcopy[loop] = P_ProcessThread->P_DSPthread->Q_CircularOutputBufferB[index];
            index++;
            if(index >= P_ProcessThread->P_DSPthread->CircularOutputBufferSize) index = 0;
        }
    }
    // stop in-band calibration signal
    if(PhaseDisplayTimeout > 10)
    {
        // if cal port is open, stop in-band cal signal by off tuning to 4GHz
        if((P_CalPort != nullptr) && (CalFrequency != 4000000))
         {
             CalFrequency = 4000000;
             QString buffer = QString::number(CalFrequency);
             P_CalPort->write(buffer.toLatin1());
         }
    }

    // loop to draw phase display
    for(int loop = 0; loop < BufferSize; loop++)
    {
        // calculate phase difference and display angle
        if(IAcopy[loop] >= 0) phaseA = qAtan(QAcopy[loop]/IAcopy[loop]);
        else phaseA = M_PI - (qAtan(QAcopy[loop]/(-1*IAcopy[loop])));

        if(IBcopy[loop] >= 0) phaseB = qAtan(QBcopy[loop]/IBcopy[loop]);
        else phaseB = M_PI - (qAtan(QBcopy[loop]/(-1*IBcopy[loop])));

        phaseDif = phaseA - phaseB;
        if(phaseDif < 0) phaseDif = (2*M_PI) + phaseDif;
        // update phase average, maximum and minimum
        phaseAv += phaseDif;  
        if(phaseDif > phaseMax) phaseMax = phaseDif;
        if(phaseDif < phaseMin) phaseMin = phaseDif;

    Y = (Radius-1) * qCos(phaseDif);  X = (Radius-1) * qSin(phaseDif);

    // detect "nan" value of X & Y (occurres on startup due to data = 0)
    if(Y != Y) Y=0; if(X != X) X=0;

    painter.setPen(QPen(Qt::blue, 0, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(OriginX,OriginY,OriginX+X,OriginY-Y);
    }

    // calculate phase correction value
    phaseAv /= BufferSize;
    PhaseCorrection = -1 * phaseAv; // reverse phase to correct
    // add in required phase relationship
    PhaseCorrection += qDegreesToRadians((double)RequiredPhase);
    // display Phase if PhaseTestMode
    if(PhaseTestMode == 1) DisplayStatus("Phase Error " + QString::number(qRadiansToDegrees(phaseAv)));

    // loop to calculate maximum signal for gain control

    float G=0;
    for(int loop=0; loop < BufferSize; loop++)
    {
        // test +ve values
        if(IAcopy[loop] > G) G = IAcopy[loop];
        if(IBcopy[loop] > G) G = IBcopy[loop];
        // test -ve values
        if(IAcopy[loop] < (-1*G)) G = (-1*IAcopy[loop]);
        if(IBcopy[loop] < (-1*G)) G = (-1*IBcopy[loop]);
    }

    G = 0.9*((ScopeHeight/2)/G); // max = 90% of display height

    // loop to draw oscilloscope display
    for(int loop=0; loop < BufferSize-1; loop++)
    {
        // plot channel A, I stream
        painter.setPen(QPen(Qt::red, 0, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(ScopeX+(2*loop),ScopeOrigin-(G*IAcopy[loop]),ScopeX+(2*(loop+1)),ScopeOrigin-(G*IAcopy[loop+1]));

        // plot channel B, I stream
        painter.setPen(QPen(Qt::blue, 0, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(ScopeX+(2*loop),ScopeOrigin-(G*IBcopy[loop]),ScopeX+(2*(loop+1)),ScopeOrigin-(G*IBcopy[loop+1]));
    }

    // draw status of Overload Flag
    painter.setPen(QPen(Qt::black, 0, Qt::SolidLine, Qt::RoundCap));
    if(OverloadFlag == 1) painter.drawText(37,90,"Overload");

    // set phase automatically if AutuCal enabled
    if(AutoCal == 1)
    {
        // check phase deviation and if low trigger synchronisation
        double phaseDeviation = phaseMax - phaseMin;
        double phaseMargin = M_PI / 36; // lock if within 0 - 5 degrees
        if((phaseDeviation > 0) && (phaseDeviation < phaseMargin))
        {
            // set phase if not previously set
            if(ui->PhasePushButton->isEnabled()) on_PhasePushButton_clicked();
        }
    }
}







