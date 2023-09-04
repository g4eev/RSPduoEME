// RSPduoEME Copyright 2020 David Warwick G4EEV
//
// This file is part of RSPduoEME.
//
// RSPduoEME is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, either version 3 of the License, or(at your option) any later version.
// RSPduoEME is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have
// received a copy of the GNU General Public License along with RSPduoEMRE, if not, see <https://www.gnu.org/licenses/>.



#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "rspduointerface.h"
#include "processthread.h"

#include <QMainWindow>
#include <QTimer>
#include <QThread>
#include <QAudioDeviceInfo>
#include <QAudio>
#include <QList>
#include <QSerialPortInfo>
#include <QSerialPort>

extern int OverloadFlag;  // defined in rspduointerface.c

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void closeEvent(QCloseEvent *event);

public slots:

    void DisplayStatus(QString);

signals:

    void StartProcessThread(void);
    void StopProcessThread(void);
    void GenerateSinCosTable(double, double);

private slots:

    void reject(void);
    void UpdateParameters(void);
    void on_StartButton_clicked();
    void on_Timer();
    void on_OutputAcomboBox_currentTextChanged(const QString &arg1);
    void on_ModeComboBox_currentIndexChanged(const QString &arg1);
    void on_FrequencyLineEdit_editingFinished();
    void on_GainSpinBoxA_valueChanged(int arg1);
    void on_GainSpinBoxB_valueChanged(int arg1);
    void on_LNAspinBox_valueChanged(int arg1);
    void on_IPLineEdit_textEdited(const QString &arg1);
    void on_PhaseSpinBox_valueChanged(int arg1);
    void on_PhasePushButton_clicked();
    void paintEvent(QPaintEvent *event);
    void on_CalPortBox_currentTextChanged(const QString &arg1);
    void OpenCalibratorPort(void);
    void CloseCalibratorPort(void);
    void on_AutoCalCheckBox_clicked(bool checked);
    void on_PhaseTestModeCheckBox_clicked(bool checked);

private:
    Ui::MainWindow *ui;

    QThread WorkerThread;                          // worker thread for ProcessThread
    ProcessThread *P_ProcessThread;                // pointer to Process Thread
    RSPduoInterface *P_RSPduo;                     // pointer for RSPduoInterface class
    QTimer *P_Timer;                               // pointer for interval Timer
    QSerialPort *P_CalPort = nullptr;              // pointer to the calibrator serial port object
    QList<QAudioDeviceInfo> OutputDeviceInfoList;  // list of audio output devices
    QString SelectedOutputDevice_A;                // the selected audio output device
    QString IPAddress = "127.0.0.1";               // the selected IP Address
    int CentreFrequency = 144350;                  // current centre frequency (KHZ)
    int CalFrequency = 0;                          // for calibration frequency (KHz)
    int IFGainA = 40;                              // current IF gain setting for tuner A
    int IFGainB = 40;                              // current IF gain setting for tuner B
    int LNAGain = 5;                               // current LNA gain setting
    double PhaseCorrection = 0;                    // calculated value for dual mode phase correction (radians)
    int PhaseDisplayTimeout = 0;                   // used to count delay before phase display is stopped
    QString SelectedMode;                          // selected mode
    int DuplicateA = 0;                            // duplicate channel A in channel B if = 1
    int TIMF2Output = 0;                           // Linrad TIMF2 UDP Output = 1, els 0
    int RAW16Output = 0;                           // Linrad RAW16 UDP Output = 1, else 0
    int SoundCardOutput = 0;                       // Sound Card Output, 1 = 2 Audio Channels, 2 = 4 Audio Channels, else 0
    int DualOP = 0;                                // Dual O/P mode = 1, else 0
    int SelectB = 0;                               // Select Ch B o/p = 1, else Ch A (UDP+Soundcard Mode)
    int InitWindowHeight = 500;                    // set initial MainWindow height (full height including phase disply)
    int InitWindowWidth = 400;                     // set initial MainWindow width
    int PhaseDisplayHeight = 115;                  // height of Phase Display (subtracted from inital height for small display)
    int Processing = 0;                            // set to 1 when started, else 0
    int RequiredPhase = 0;                         // for required phase between A and B channels
    QString SelectedCalPort = "None";              // for the selected calibratior Com Port
    int AutoCal = 0;                               // auto calibration enabled = 1, else 0
    int PhaseTestMode = 0;                         // phase test mode enabled = 1, else 0

    QList<QString> ModeList = {"1: Channel A, 96000 -> Soundcard",    // List of Modes for selection
                               "2: Channel A, 192000 -> Soundcard",   // in Mode combo box
                               "3: Channel A, 96000 -> UDP (MAP65)",
                               "4: Dual A + A, 96000 -> UDP (MAP65)",
                               "5: Dual A & B, 96000 -> UDP (MAP65)",
                               "6: Dual UDP (MAP65) + Ch A Soundcard",
                               "7: Dual UDP (MAP65) + Ch B Soundcard",
                               "8: Dual A & B -> UDP (Linrad RAW16)",
                               "9: Dual A & B -> 4 Ch Soundcard"};

};

#endif // MAINWINDOW_H
