/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * The name if its contributors may not be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#include "Dlg_VideoCapture.h"
#include <QFileDialog>



Dlg_VideoCapture::Dlg_VideoCapture(QWidget *parent)
	: QDialog(parent) //, Settings("HillerLabs", "VoxCAD")
{
	ui.setupUi(this);
	AcceptedDialog = false;

	//enforce type-ins
	const QValidator* IEval = new QIntValidator(this);
	ui.WidthPixEdit->setValidator(IEval);
	ui.HeightPixEdit->setValidator(IEval);
	ui.TimeStepsEdit->setValidator(IEval);
	ui.TempCycleEdit->setValidator(IEval);
	const QValidator* DEval = new QDoubleValidator(this);
	ui.OutputFpsEdit->setValidator(DEval);
	ui.OutputSpeedFactorEdit->setValidator(DEval);
	ui.SimTimeEdit->setValidator(DEval);

	StopEnabled=false;
	CurStopSettings = SS_TIME_STEPS;
	sNumStep = 10000;
	sNumCyc = 10;
	sSimTime = 5.0;

	LoadSettings();

	connect(ui.SelectFolderButton, SIGNAL(clicked()), this, SLOT(ClickedSelectFolderButton()));

	connect(ui.WidthPixEdit, SIGNAL(editingFinished()), this, SLOT(WidthPixEditChanged()));
	connect(ui.HeightPixEdit, SIGNAL(editingFinished()), this, SLOT(HeightPixEditChanged()));

	connect(ui.r320x240Radio, SIGNAL(clicked()), this, SLOT(Clickedr320x240Radio()));
	connect(ui.r640x480Radio, SIGNAL(clicked()), this, SLOT(Clickedr640x480Radio()));
	connect(ui.r800x600Radio, SIGNAL(clicked()), this, SLOT(Clickedr800x600Radio()));
	connect(ui.r1024x768Radio, SIGNAL(clicked()), this, SLOT(Clickedr1024x768Radio()));
	connect(ui.r1280x720Radio, SIGNAL(clicked()), this, SLOT(Clickedr1280x720Radio()));
	connect(ui.r1920x1080Radio, SIGNAL(clicked()), this, SLOT(Clickedr1920x1080Radio()));

	connect(ui.OutputFpsEdit, SIGNAL(editingFinished()), this, SLOT(OutputFpsEditChanged()));
	connect(ui.OutputSpeedFactorEdit, SIGNAL(editingFinished()), this, SLOT(OutputSpdFctrEditChanged()));

	connect(ui.vsDisplayTimeRadio, SIGNAL(clicked()), this, SLOT(ClickedDisplayTimeRadio()));
	connect(ui.vsSimTimeRadio, SIGNAL(clicked()), this, SLOT(ClickedSimTimeRadio()));
	connect(ui.vsEveryFrameRadio, SIGNAL(clicked()), this, SLOT(ClickedEveryFrameRadio()));
	connect(ui.AutoStopEnabledCheck, SIGNAL(clicked(bool)), this, SLOT(ClickedAutoStopCheck(bool)));
	connect(ui.TimeStepsRadio, SIGNAL(clicked()), this, SLOT(ClickedTimeStepsRadio()));
	connect(ui.SimTimeRadio, SIGNAL(clicked()), this, SLOT(ClickedSimTimeRadio2()));
	connect(ui.TempCycleRadio, SIGNAL(clicked()), this, SLOT(ClickedTempCycleRadio()));

	connect(ui.TimeStepsEdit, SIGNAL(editingFinished()), this, SLOT(TimeStepsEditChanged()));
	connect(ui.SimTimeEdit, SIGNAL(editingFinished()), this, SLOT(SimTimeEditChanged()));
	connect(ui.TempCycleEdit, SIGNAL(editingFinished()), this, SLOT(TempCycleEditChanged()));

	connect(ui.ResetSimCheck, SIGNAL(clicked(bool)), this, SLOT(ClickedResetSimCheck(bool)));
	connect(ui.BeginButton, SIGNAL(clicked()), this, SLOT(ClickedBegin()));
	connect(ui.CancelButton, SIGNAL(clicked()), this, SLOT(ClickedCancel()));



	UpdateUI();
}

Dlg_VideoCapture::~Dlg_VideoCapture()
{

}

void Dlg_VideoCapture::LoadSettings(void)
{
	CurFolder = Settings.value("VideoCapture/Folder", "C:\\").toString();
	WidthPix = Settings.value("VideoCapture/WidthPix", 800).toInt();
	HeightPix = Settings.value("VideoCapture/HeightPix", 600).toInt();
	CurVidSetting = (VideoSpeedSetting) Settings.value("VideoCapture/VideoSpeed", (int)VS_DISPLAY).toInt();
	OutputFps = Settings.value("VideoCapture/OutputFps", 30.0).toDouble();
	OutputSpeedFactor = Settings.value("VideoCapture/OutputSpeedFactor", 1.0).toDouble();
	ResetSimOnBegin = Settings.value("VideoCapture/ResetSimOnBegin", false).toBool();
}


void Dlg_VideoCapture::UpdateUI(void)
{
	ui.OutputFolderEdit->setText(CurFolder);

	ui.WidthPixEdit->setText(QString::number(WidthPix));
	ui.HeightPixEdit->setText(QString::number(HeightPix));

	ui.r320x240Radio->setChecked(false);
	ui.r640x480Radio->setChecked(false);
	ui.r800x600Radio->setChecked(false);
	ui.r1024x768Radio->setChecked(false);
	ui.r1280x720Radio->setChecked(false);
	ui.r1920x1080Radio->setChecked(false);
	if (WidthPix==320 && HeightPix==240) ui.r320x240Radio->setChecked(true);
	else if (WidthPix==640 && HeightPix==480) ui.r640x480Radio->setChecked(true);
	else if (WidthPix==800 && HeightPix==600) ui.r800x600Radio->setChecked(true);
	else if (WidthPix==1024 && HeightPix==768) ui.r1024x768Radio->setChecked(true);
	else if (WidthPix==1280 && HeightPix==720) ui.r1280x720Radio->setChecked(true);
	else if (WidthPix==1920 && HeightPix==1080) ui.r1920x1080Radio->setChecked(true);

	ui.OutputFpsEdit->setEnabled(false);
	ui.OutputSpeedFactorEdit->setEnabled(false);
	if (CurVidSetting == VS_DISPLAY){ ui.vsDisplayTimeRadio->setChecked(true); ui.OutputFpsEdit->setEnabled(true); ui.OutputSpeedFactorEdit->setEnabled(true);}
	else if (CurVidSetting == VS_SIMULATION){ ui.vsSimTimeRadio->setChecked(true); ui.OutputFpsEdit->setEnabled(true); ui.OutputSpeedFactorEdit->setEnabled(true);}
	else if (CurVidSetting == VS_EVERY) ui.vsEveryFrameRadio->setChecked(true);

	ui.OutputFpsEdit->setText(QString::number(OutputFps));
	ui.OutputSpeedFactorEdit->setText(QString::number(OutputSpeedFactor));

	ui.AutoStopEnabledCheck->setChecked(StopEnabled);
	ui.TimeStepsRadio->setEnabled(StopEnabled);
	ui.TimeStepsEdit->setEnabled(StopEnabled);
	ui.SimTimeRadio->setEnabled(StopEnabled);
	ui.SimTimeEdit->setEnabled(StopEnabled);
	ui.TempCycleRadio->setEnabled(StopEnabled);
	ui.TempCycleEdit->setEnabled(StopEnabled);
	if (StopEnabled){
		switch (CurStopSettings){
		case SS_TIME_STEPS: ui.TimeStepsRadio->setChecked(true); break;
		case SS_SIM_TIME: ui.SimTimeRadio->setChecked(true); break;
		case SS_TEMP_CYCLES: ui.TempCycleRadio->setChecked(true); break;
		}
		ui.TimeStepsEdit->setText(QString::number(sNumStep));
		ui.SimTimeEdit->setText(QString::number(sSimTime));
		ui.TempCycleEdit->setText(QString::number(sNumCyc));
	}

	ui.ResetSimCheck->setChecked(ResetSimOnBegin);

}


void Dlg_VideoCapture::ClickedSelectFolderButton(void)
{
	QString dir = QFileDialog::getExistingDirectory(this, "Open Folder", CurFolder, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	//if (check for cancel)
	if (!dir.endsWith("\\")) dir += "\\";
	CurFolder = dir;
	Settings.setValue("VideoCapture/Folder", CurFolder);
	UpdateUI();
}
