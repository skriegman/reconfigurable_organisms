/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * The name if its contributors may not be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/


#ifndef DLG_VIDEOCAPTURE_H
#define DLG_VIDEOCAPTURE_H

#include <QDialog>
#include <QSettings>
#include "ui_vVideoGen.h"

enum VideoSpeedSetting {VS_DISPLAY, VS_SIMULATION, VS_EVERY};
enum StopSettings {SS_TIME_STEPS, SS_SIM_TIME, SS_TEMP_CYCLES};

class Dlg_VideoCapture : public QDialog
{
	Q_OBJECT

public:
	Dlg_VideoCapture(QWidget *parent);
	~Dlg_VideoCapture();

	void UpdateUI(void);
	void LoadSettings(void);

	QString CurFolder;
	int WidthPix, HeightPix;
	bool AcceptedDialog;
	VideoSpeedSetting CurVidSetting;
	double OutputFps, OutputSpeedFactor;
	bool StopEnabled;
	StopSettings CurStopSettings;
	int sNumStep, sNumCyc;
	double sSimTime;
	bool ResetSimOnBegin;

public slots:
	void ClickedSelectFolderButton(void);

	void WidthPixEditChanged(void) {int Val=ui.WidthPixEdit->text().toInt(); Val=Val<0?0:Val; Val=Val>4096?4096:Val; SetHRes(Val);}
	void HeightPixEditChanged(void) {int Val=ui.HeightPixEdit->text().toInt(); Val=Val<0?0:Val; Val=Val>3072?3072:Val; SetVRes(Val);}
	void Clickedr320x240Radio(void) {SetHRes(320); SetVRes(240);}
	void Clickedr640x480Radio(void) {SetHRes(640); SetVRes(480);}
	void Clickedr800x600Radio(void) {SetHRes(800); SetVRes(600);}
	void Clickedr1024x768Radio(void) {SetHRes(1024); SetVRes(768);}
	void Clickedr1280x720Radio(void) {SetHRes(1280); SetVRes(720);}
	void Clickedr1920x1080Radio(void) {SetHRes(1920); SetVRes(1080);}

	void OutputFpsEditChanged(void) {double Val=ui.OutputFpsEdit->text().toDouble(); Val=Val<1.0?1.0:Val; OutputFps=Val; Settings.setValue("VideoCapture/OutputFps", OutputFps); UpdateUI();}
	void OutputSpdFctrEditChanged(void) {double Val=ui.OutputSpeedFactorEdit->text().toDouble(); Val=Val<0.0001?0.0001:Val; OutputSpeedFactor=Val;  Settings.setValue("VideoCapture/OutputSpeedFactor", OutputSpeedFactor); UpdateUI();}

	void ClickedDisplayTimeRadio(void){CurVidSetting=VS_DISPLAY; Settings.setValue("VideoCapture/VideoSpeed", (int)CurVidSetting); UpdateUI();}
	void ClickedSimTimeRadio(void){CurVidSetting=VS_SIMULATION; Settings.setValue("VideoCapture/VideoSpeed", (int)CurVidSetting); UpdateUI();}
	void ClickedEveryFrameRadio(void){CurVidSetting=VS_EVERY; Settings.setValue("VideoCapture/VideoSpeed", (int)CurVidSetting); UpdateUI();}

	void ClickedAutoStopCheck(bool Checked){StopEnabled = Checked; UpdateUI();}
	void ClickedTimeStepsRadio(void){CurStopSettings=SS_TIME_STEPS; UpdateUI();}
	void TimeStepsEditChanged(void) {int Val=ui.TimeStepsEdit->text().toInt(); Val=Val<0?0:Val; sNumStep=Val; UpdateUI();}
	void ClickedSimTimeRadio2(void){CurStopSettings=SS_SIM_TIME; UpdateUI();}
	void SimTimeEditChanged(void) {double Val=ui.SimTimeEdit->text().toDouble(); Val=Val<0.0001?0.0001:Val; sSimTime=Val; UpdateUI();}
	void ClickedTempCycleRadio(void){CurStopSettings=SS_TEMP_CYCLES; UpdateUI();}
	void TempCycleEditChanged(void) {int Val=ui.TempCycleEdit->text().toInt(); Val=Val<0?0:Val; sNumCyc=Val; UpdateUI();}

	void ClickedResetSimCheck(bool Checked){ResetSimOnBegin = Checked; Settings.setValue("VideoCapture/ResetSimOnBegin", ResetSimOnBegin);}
	void ClickedBegin(void) {AcceptedDialog=true; close();}
	void ClickedCancel(void) {AcceptedDialog=false; close();}

private:

	QSettings Settings;

	void SetHRes(int HResIn) {WidthPix=HResIn; Settings.setValue("VideoCapture/WidthPix", HResIn); UpdateUI();}
	void SetVRes(int VResIn) {HeightPix=VResIn; Settings.setValue("VideoCapture/HeightPix", VResIn); UpdateUI();}


	Ui::VideoDialog ui;

};

#endif // DLG_VIDEOCAPTURE_H
