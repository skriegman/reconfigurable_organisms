/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * The name if its contributors may not be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#include "Dlg_FEAInfo.h"

Dlg_FEAInfo::Dlg_FEAInfo(QVX_FEA* pFEAIn, QWidget *parent)
	: QWidget(parent)
{
	pFEA = pFEAIn;
	ui.setupUi(this);

	ui.ViewTypeCombo->addItem("Displacement"); //Must be in same order as FeaViewMode enum
	ui.ViewTypeCombo->addItem("Force");
	ui.ViewTypeCombo->addItem("Strain");
	ui.ViewTypeCombo->addItem("Reaction");




	connect(ui.ViewTypeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(ApplyPreset(int)));

	connect(ui.DefSlider, SIGNAL(valueChanged(int)), this, SLOT(ChangedDeflection(int)));
	connect(ui.SectionSlider, SIGNAL(valueChanged(int)), this, SLOT(ChangedSection(int)));
	connect(ui.IsoThreshSlider, SIGNAL(valueChanged(int)), this, SLOT(ChangedIso(int)));

	connect(ui.DirXRadio, SIGNAL(clicked()), this, SLOT(SetDirToX()));
	connect(ui.DirYRadio, SIGNAL(clicked()), this, SLOT(SetDirToY()));
	connect(ui.DirZRadio, SIGNAL(clicked()), this, SLOT(SetDirToZ()));
	connect(ui.DirMaxRadio, SIGNAL(clicked()), this, SLOT(SetDirToMax()));

	connect(ui.DoneButton, SIGNAL(clicked()), this, SLOT(DoneButtonPressed()));

	UpdateUI();
}

Dlg_FEAInfo::~Dlg_FEAInfo()
{

}

void Dlg_FEAInfo::ApplyPreset(int NewPreset)
{
	switch (NewPreset){
	case VIEW_DISP: pFEA->SetViewModeDisplacement(); break;
	case VIEW_FORCE: pFEA->SetViewModeForce(); break;
	case VIEW_STRAIN: pFEA->SetViewModeStrain(); break;
	case VIEW_REACTION: pFEA->SetViewModeReaction(); break;
	}

	UpdateUI();
	emit RequestUpdateGL();
}

void Dlg_FEAInfo::UpdateUI(void)
{
	if (isVisible()){
		ui.ViewTypeCombo->setCurrentIndex((int)pFEA->ViewMode);

		ui.DefSlider->setValue(pFEA->ViewDefPerc*200.0); //set initial value of slider...
		ui.SectionSlider->setValue(pFEA->ViewZChop*1000.0); //set initial value of slider...
		ui.IsoThreshSlider->setValue(pFEA->ViewThresh*1000.0); //set initial value of slider...

		switch(pFEA->ViewModeDir){
			case XDIR: ui.DirXRadio->setChecked(true); break;
			case YDIR: ui.DirYRadio->setChecked(true); break;
			case ZDIR: ui.DirZRadio->setChecked(true); break;
			case MAXDIR: ui.DirMaxRadio->setChecked(true); break;
		}
		UpdateText();
	}
}

void Dlg_FEAInfo::UpdateText(void)
{
	QString FullText;
	int CurSel = -1;
	emit GetCurIndex(&CurSel);
	if (CurSel <0) emit GetFEAInfoString(&FullText);
	else emit GetFEAInfoString(CurSel, &FullText);

	ui.FEAInfoLabel->setText(FullText);
}