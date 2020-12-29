// RSPduoEME Copyright 2020 David Warwick G4EEV
//
// This file is part of RSPduoEME.
//
// RSPduoEME is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation, either version 3 of the License, or(at your option) any later version.
// RSPduoEME is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details. You should have
// received a copy of the GNU General Public License along with RSPduoEMRE, if not, see <https://www.gnu.org/licenses/>.



#ifndef RSPDUOINTERFACE_H
#define RSPDUOINTERFACE_H

#include <QWidget>
#include <QTimer>


#define BUFFERS 10                          // Number of input buffers.
#define INPUT_BUFFER_SIZE 80000             // Size of each input buffer, which should equal block processing
                                            // size for 40mS at 2MHz input rate. (0.04 * 2000000 = 80000)


class RSPduoInterface : public QWidget
{
    Q_OBJECT
public:
    explicit RSPduoInterface(QWidget *parent = nullptr);
    ~RSPduoInterface();

    void Start(int LOfrequency, int gRdb_Gain, int LNAstate);
    void Stop (void);
    void ChangeIFGain(int gRdB_Gain);
    void ChangeLNAGain(int LNAstate);

signals:

    Status(QString);

private slots:

    void on_Timeout(void);

private:

    QTimer *Timer;

};

#endif // RSPDUOINTERFACE_H


