/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * The name if its contributors may not be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#ifndef DLG_FEAINFO_H
#define DLG_FEAINFO_H

#include "ui_vFEAInfo.h"
#include "QVX_Interfaces.h"
#include <QWidget>

class Dlg_FEAInfo : public QWidget
{
	Q_OBJECT

public:
	Dlg_FEAInfo(QVX_FEA* pFEA, QWidget *parent);
	~Dlg_FEAInfo();

	QVX_FEA* pFEA;
	virtual QSize sizeHint () const {return QSize(100, 500);};

signals:
	void RequestUpdateGL(void);
	void GetCurIndex(int* CurIndex);
	void GetFEAInfoString(QString* Info);
	void GetFEAInfoString(int Index, QString* Info);
	void DoneAnalyzing(void);

public slots:
	void ApplyPreset(int NewPreset);
	void ChangedDeflection(int NewDef) {pFEA->ViewDefPerc = NewDef/200.0; emit RequestUpdateGL();};
	void ChangedSection(int NewSec) {pFEA->ViewZChop = NewSec/1000.0; emit RequestUpdateGL();};
	void ChangedIso(int NewIso) {pFEA->ViewThresh = NewIso/1000.0; emit RequestUpdateGL();};

	void SetDirToX(void) {pFEA->ViewModeDir = XDIR; emit RequestUpdateGL();};
	void SetDirToY(void) {pFEA->ViewModeDir = YDIR; emit RequestUpdateGL();};
	void SetDirToZ(void) {pFEA->ViewModeDir = ZDIR; emit RequestUpdateGL();};
	void SetDirToMax(void) {pFEA->ViewModeDir = MAXDIR; emit RequestUpdateGL();};

	void UpdateUI(void);
	void UpdateText(void); //updates just the info text

	void DoneButtonPressed(void) {emit DoneAnalyzing();};

private:

	Ui::FEAInfoDlg ui;
	
};

#endif // DLG_FEAINFO_H
