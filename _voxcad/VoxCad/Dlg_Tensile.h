/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * The name if its contributors may not be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/


#ifndef DLG_TENSILE_H
#define DLG_TENSILE_H

#include <QDialog>
#include "ui_vTensile.h"
#include "QVX_TensileTest.h"

//class QVX_Environment;
class QVX_Sim;

enum ConvScheme {CVS_FAST, CVS_BALANCED, CVS_ACCURATE, CVS_MANUAL};

class Dlg_Tensile : public QWidget
{
	Q_OBJECT

public:
	Dlg_Tensile(QVX_Sim* pSimIn, QWidget *parent);
//	Dlg_Tensile(QVX_Environment* pEnvIn, QWidget *parent);
	~Dlg_Tensile();
	QVX_TensileTest Tester;

public slots:
	void StartTest(void);
	void UpdateUI(void);
	
	void ClickedFastRadio() {CurConvScheme = CVS_FAST; UpdateUI();}
	void ClickedBalancedRadio() {CurConvScheme = CVS_BALANCED; UpdateUI();}
	void ClickedAccurateRadio() {CurConvScheme = CVS_ACCURATE; UpdateUI();}
	void ClickedManualRadio() {CurConvScheme = CVS_MANUAL; UpdateUI();}

signals:
	void DoneTensileTesting();

private:
	Ui::TensileDlg ui;



	ConvScheme CurConvScheme;
//	double ConvThresh;

//	QVX_Environment* pEnv;
	QVX_Sim* pSim;
};

#endif // DLG_TENSILE_H
