/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * The name if its contributors may not be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#include "Dlg_Physics.h"
#include "../QTUtils/QSimplePlot.h"

Dlg_Physics::Dlg_Physics(QVX_Sim* pSimIn, QWidget *parent)
	: QWidget(parent)
{
	pSim = pSimIn;
	ui.setupUi(this);

	//these should be in the order of the stop condition enum
	ui.StopSelectCombo->addItem("None");
	ui.StopSelectCombo->addItem("Time Steps");
	ui.StopSelectCombo->addItem("Simulation Time");
	ui.StopSelectCombo->addItem("Temperature cycles");
	ui.StopSelectCombo->addItem("Constant Energy");
	ui.StopSelectCombo->addItem("Kinetic Floor");
	ui.StopSelectCombo->addItem("Motion Floor");

	//these should match the order of the PlotType enum
	ui.VariableCombo->addItem("Displacement");
	ui.VariableCombo->addItem("Angle");
	ui.VariableCombo->addItem("Kinetic Energy");
	ui.VariableCombo->addItem("Potential Energy");
	ui.VariableCombo->addItem("Total Energy");
	ui.VariableCombo->addItem("Max Voxel Movement");


	//these should match the order of the PlotDir enum
	ui.DirectionCombo->addItem("Maximum");
	ui.DirectionCombo->addItem("X Direction");
	ui.DirectionCombo->addItem("Y Direction");
	ui.DirectionCombo->addItem("Z Direction");

	//enforce type-ins
	const QValidator* DEval = new QDoubleValidator(this);
	ui.dtEdit->setValidator(DEval);

	CurPlotType = PL_DISP;
	CurPlotDir = MAXDIR;

	connect(ui.PauseButton, SIGNAL(clicked()), this, SLOT(ClickedPause()));
	connect(ui.ResetButton, SIGNAL(clicked()), this, SLOT(ClickedReset()));
	connect(ui.RecordButton, SIGNAL(clicked(bool)), this, SLOT(ClickedRecord(bool)));

	
	connect(ui.UseEquilibriumCheck, SIGNAL(clicked(bool)), this, SLOT(UseEquilibriumCheckChanged(bool)));
	connect(ui.StopSelectCombo, SIGNAL(activated(int)), this, SLOT(StopSelectChanged(int)));
	connect(ui.StopValueEdit, SIGNAL(editingFinished()), this, SLOT(StopValueEditChanged()));

	connect(ui.dtSlider, SIGNAL(valueChanged(int)), this, SLOT(dtSliderChanged(int)));
	connect(ui.dtEdit, SIGNAL(editingFinished()), this, SLOT(dtEditChanged()));
	connect(ui.BondDampSlider, SIGNAL(valueChanged(int)), this, SLOT(BondDampSliderChanged(int)));
	connect(ui.BondDampEdit, SIGNAL(editingFinished()), this, SLOT(BondDampEditChanged()));
	connect(ui.GNDDampSlider, SIGNAL(valueChanged(int)), this, SLOT(GNDDampSliderChanged(int)));
	connect(ui.GNDDampEdit, SIGNAL(editingFinished()), this, SLOT(GNDDampEditChanged()));
	connect(ui.ColDampSlider, SIGNAL(valueChanged(int)), this, SLOT(ColDampSliderChanged(int)));
	connect(ui.ColDampEdit, SIGNAL(editingFinished()), this, SLOT(ColDampEditChanged()));
	connect(ui.MaxVelLimitSlider, SIGNAL(valueChanged(int)), this, SLOT(MaxVelLimitSliderChanged(int)));
	connect(ui.UseSelfColCheck, SIGNAL(clicked(bool)), this, SLOT(UseSelfColCheckChanged(bool)));
	connect(ui.UseMaxVelLimitCheck, SIGNAL(clicked(bool)), this, SLOT(UseMaxVelLimitCheckChanged(bool)));

	connect(ui.UseVolumeEffectsCheck, SIGNAL(clicked(bool)), this, SLOT(UseVolEffectsCheckChanged(bool)));

	connect(ui.UseTempCheck, SIGNAL(clicked(bool)), this, SLOT(UseTempCheckChanged(bool)));
	connect(ui.TempSlider, SIGNAL(valueChanged(int)), this, SLOT(TempSliderChanged(int)));
	connect(ui.TempEdit, SIGNAL(editingFinished()), this, SLOT(TempEditChanged()));
	connect(ui.VaryTempCheck, SIGNAL(clicked(bool)), this, SLOT(VaryTempCheckChanged(bool)));
	connect(ui.TempPerSlider, SIGNAL(valueChanged(int)), this, SLOT(TempPerSliderChanged(int)));
	connect(ui.TempPerEdit, SIGNAL(editingFinished()), this, SLOT(TempPerEditChanged()));
	connect(ui.UseGravCheck, SIGNAL(clicked(bool)), this, SLOT(UseGravCheckChanged(bool)));
	connect(ui.GravSlider, SIGNAL(valueChanged(int)), this, SLOT(GravSliderChanged(int)));
	connect(ui.GravEdit, SIGNAL(editingFinished()), this, SLOT(GravEditChanged()));

	connect(ui.UseFloorCheck, SIGNAL(clicked(bool)), this, SLOT(UseFloorCheckChanged(bool)));
	connect(ui.UseFloorSlopeCheck, SIGNAL(clicked(bool)), this, SLOT(UseFloorSlopeCheckChanged(bool)));	

	connect(ui.DispDisableRadio, SIGNAL(clicked(bool)), this, SLOT(DisplayDisableChanged(bool)));
	connect(ui.DispVoxelsRadio, SIGNAL(clicked(bool)), this, SLOT(DisplayVoxChanged(bool)));
	connect(ui.DispConnRadio, SIGNAL(clicked(bool)), this, SLOT(DisplayConChanged(bool)));


	connect(ui.ViewDiscreteRadio, SIGNAL(clicked(bool)), this, SLOT(VoxDiscreteChanged(bool)));
	connect(ui.ViewDeformedRadio, SIGNAL(clicked(bool)), this, SLOT(VoxDeformedChanged(bool)));
	connect(ui.ViewSmoothRadio, SIGNAL(clicked(bool)), this, SLOT(VoxSmoothChanged(bool)));
	connect(ui.ForcesCheck, SIGNAL(clicked(bool)), this, SLOT(ForcesCheckChanged(bool)));
	connect(ui.LocalCoordCheck, SIGNAL(clicked(bool)), this, SLOT(LCsCheckChanged(bool)));

	connect(ui.TypeRadio, SIGNAL(clicked(bool)), this, SLOT(CTypeChanged(bool)));
	connect(ui.KineticERadio, SIGNAL(clicked(bool)), this, SLOT(CKinEChanged(bool)));



	connect(ui.PlotDragRadio, SIGNAL(clicked(bool)), this, SLOT(PlotDragRadioChanged(bool)));
	connect(ui.PlotSpeedsRadio, SIGNAL(clicked(bool)), this, SLOT(PlotSpeedsRadioChanged(bool)));
	connect(ui.PlotNormalsRadio, SIGNAL(clicked(bool)), this, SLOT(PlotNormalsRadioChanged(bool)));
	connect(ui.PlotForcesCheck, SIGNAL(clicked(bool)), this, SLOT(PlotForcesCheckChanged(bool)));
	connect(ui.ForcesScalingEdit, SIGNAL(editingFinished()), this, SLOT(ForcesScalingEditChanged()));


	connect(ui.DisplacementRadio, SIGNAL(clicked(bool)), this, SLOT(CDispChanged(bool)));
	connect(ui.StateRadio, SIGNAL(clicked(bool)), this, SLOT(CStateChanged(bool)));
	connect(ui.StiffnessRadio, SIGNAL(clicked(bool)), this, SLOT(CStiffnessChanged(bool)));
	connect(ui.StrainERadio, SIGNAL(clicked(bool)), this, SLOT(CStrainEChanged(bool)));
	connect(ui.StrainRadio, SIGNAL(clicked(bool)), this, SLOT(CStrainChanged(bool)));
	connect(ui.StressRadio, SIGNAL(clicked(bool)), this, SLOT(CStressChanged(bool)));
	connect(ui.PressureRadio, SIGNAL(clicked(bool)), this, SLOT(CPressureChanged(bool)));

	connect(ui.CoMCheck, SIGNAL(clicked(bool)), this, SLOT(CoMCheckChanged(bool)));


	connect(ui.VariableCombo, SIGNAL(activated(int)), this, SLOT(VarComboChanged(int)));
	connect(ui.DirectionCombo, SIGNAL(activated(int)), this, SLOT(DirComboChanged(int)));
	connect(ui.LogEachCheck, SIGNAL(clicked(bool)), this, SLOT(LogEachCheckChanged(bool)));
	connect(ui.SaveDataButton, SIGNAL(clicked()), this, SLOT(ClickedSaveData()));

	//set up plot for real-time display...
	
	pPlot = new QSimplePlot();
	ui.verticalLayout_3->insertWidget(1, pPlot);
	pPlot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	PlotUpdateTimer = new QTimer(this);
	connect(PlotUpdateTimer, SIGNAL(timeout()), this, SLOT(UpdatePlot()));

	PlotUpdateRate = 33;
	PlotUpdateTimer->start(PlotUpdateRate); //just run continuously...

	// DEFAULT: stiffness view.
	pSim->pSimView->SetCurViewCol(RVC_STIFFNESS);

	UpdateUI();
	
}



Dlg_Physics::~Dlg_Physics()
{
	PlotUpdateTimer->stop();
}

void Dlg_Physics::UpdateUI(void)
{
	CVX_Environment* ptEnv = pSim->pEnv; //pointer to current environment object
	bool EqMode = pSim->IsFeatureEnabled(VXSFEAT_EQUILIBRIUM_MODE); // pSim->IsEquilibriumEnabled();

	//Sim
	if (!pSim->Paused && pSim->Running) SetStatusButtonText(true);
	else SetStatusButtonText(false);
	ui.RecordButton->setChecked(pSim->Recording);

	ui.UseEquilibriumCheck->setChecked(EqMode);
	StopCondition CurStopCond = pSim->GetStopConditionType();
	ui.StopSelectCombo->setCurrentIndex(CurStopCond);
	ui.StopValueEdit->setEnabled(CurStopCond != SC_NONE);
	ui.StopValueEdit->setText(QString::number(pSim->GetStopConditionValue()));
	switch (CurStopCond){
	case SC_NONE: ui.StopValueLabel->setText(""); break;
	case SC_MAX_TIME_STEPS: ui.StopValueLabel->setText("#"); break;
	case SC_MAX_SIM_TIME: ui.StopValueLabel->setText("Sec"); break;
	case SC_TEMP_CYCLES: ui.StopValueLabel->setText("#"); break;
	case SC_CONST_MAXENERGY: ui.StopValueLabel->setText("Avg mJ/Vox/500 ts"); break;
	case SC_MIN_KE: ui.StopValueLabel->setText("Avg mJ/Vox/500 ts"); break;
	case SC_MIN_MAXMOVE: ui.StopValueLabel->setText("Max mm/timestep"); break;
	}

	ui.dtSlider->setRange(0, 1000);
	ui.dtSlider->setValue(qRound(pSim->DtFrac*500)); 
	ui.dtEdit->setText(QString::number(pSim->DtFrac, 'g', 3));
	ui.ForcesScalingEdit->setText(QString::number(pSim->pSimView->GetVectorsScalingView(), 'g', 3));

	ui.BondDampSlider->setEnabled(!EqMode);
	ui.BondDampEdit->setEnabled(!EqMode);
	ui.BondDampSlider->setRange(0, 100);
	ui.BondDampSlider->setValue(qRound(pSim->GetBondDampZ()*50)); 
	ui.BondDampEdit->setText(QString::number(pSim->GetBondDampZ(), 'g', 3));

	//from .00001 to .1
	ui.GNDDampSlider->setEnabled(!EqMode);
	ui.GNDDampEdit->setEnabled(!EqMode);
	ui.GNDDampSlider->setRange(0, 100);
	if (pSim->GetSlowDampZ() == 0) ui.GNDDampSlider->setValue(0);
	else ui.GNDDampSlider->setValue(qRound((log10(pSim->GetSlowDampZ())+5)*25.0)); 
	ui.GNDDampEdit->setText(QString::number(pSim->GetSlowDampZ(), 'g', 3));

	ui.UseSelfColCheck->setChecked(pSim->IsFeatureEnabled(VXSFEAT_COLLISIONS)); //pSim->IsSelfColEnabled());
	ui.ColDampSlider->setEnabled(pSim->IsFeatureEnabled(VXSFEAT_COLLISIONS));
	ui.ColDampSlider->setRange(0, 100);
	ui.ColDampSlider->setValue(qRound(pSim->GetCollisionDampZ()*50)); 
	ui.ColDampEdit->setText(QString::number(pSim->GetCollisionDampZ(), 'g', 3));

	ui.UseMaxVelLimitCheck->setEnabled(!EqMode);
	ui.MaxVelLimitSlider->setEnabled(!EqMode);
	ui.UseMaxVelLimitCheck->setChecked(pSim->IsFeatureEnabled(VXSFEAT_MAX_VELOCITY)); //pSim->IsMaxVelLimitEnabled());
	ui.MaxVelLimitSlider->setEnabled(pSim->IsFeatureEnabled(VXSFEAT_MAX_VELOCITY));
	ui.MaxVelLimitSlider->setRange(0, 100);
	ui.MaxVelLimitSlider->setValue(qRound(pSim->GetMaxVoxVelLimit()*400)); 

	//Env
	ui.UseTempCheck->setChecked(pSim->IsFeatureEnabled(VXSFEAT_TEMPERATURE)); //ptEnv->IsTempEnabled());
	ui.TempSlider->setRange(0, 50); //+/- 25 degrees from TempBase
	ui.TempSlider->setValue(qRound(25 + ptEnv->GetTempAmplitude())); 
	ui.TempEdit->setText(QString::number(ptEnv->GetTempBase() + ptEnv->GetTempAmplitude(), 'g', 3));

	ui.VaryTempCheck->setChecked(pSim->IsFeatureEnabled(VXSFEAT_TEMPERATURE_VARY)); //ptEnv->IsTempVaryEnabled());
	ui.TempPerSlider->setRange(0, 10000);
	ui.TempPerSlider->setValue(qRound(ptEnv->GetTempPeriod()/pSim->OptimalDt)); //slider range of 0-10,000 timesteps
	ui.TempPerEdit->setText(QString::number(ptEnv->GetTempPeriod(), 'g', 3));

	ui.UseGravCheck->setChecked(pSim->IsFeatureEnabled(VXSFEAT_GRAVITY)); //ptEnv->IsGravityEnabled());
	ui.GravSlider->setRange(0, 10000);
	ui.GravSlider->setValue(qRound(-ptEnv->GetGravityAccel()/0.00981)); //1e-5 takes care for float -> int rounding...
	ui.GravEdit->setText(QString::number(ptEnv->GetGravityAccel(), 'g', 4));

	ui.UseFloorCheck->setChecked(pSim->IsFeatureEnabled(VXSFEAT_FLOOR)); //ptEnv->IsFloorEnabled());
	ui.UseFloorSlopeCheck->setChecked(pSim->pEnv->GetFloorSlopeEnabled()); //ptEnv->IsFloorEnabled());


	//View
	bool ViewEnabled = true;
	switch (pSim->pSimView->GetCurViewMode()){
		case RVM_NONE: ui.DispDisableRadio->setChecked(true); ViewEnabled = false; break;
		case RVM_VOXELS: ui.DispVoxelsRadio->setChecked(true); break;
		case RVM_BONDS: ui.DispConnRadio->setChecked(true); break;
	}
	ui.ViewOptionsGroup->setEnabled(ViewEnabled);
	ui.ColorGroup->setEnabled(ViewEnabled);
	ui.PlotForcesGroup->setEnabled(ViewEnabled);
	ui.CoMCheck->setEnabled(ViewEnabled);

	switch (pSim->pSimView->GetCurViewVox()){
		case RVV_DISCRETE: ui.ViewDiscreteRadio->setChecked(true); break;
		case RVV_DEFORMED: ui.ViewDeformedRadio->setChecked(true); break;
		case RVV_SMOOTH: ui.ViewSmoothRadio->setChecked(true); break;

	}
	ui.ForcesCheck->setChecked(pSim->pSimView->GetViewForce());
	ui.LocalCoordCheck->setChecked(pSim->pSimView->GetViewAngles());

	switch (pSim->pSimView->GetCurViewCol()){
		case RVC_TYPE: ui.TypeRadio->setChecked(true); break;
		case RVC_KINETIC_EN: ui.KineticERadio->setChecked(true); break;
		case RVC_DISP: ui.DisplacementRadio->setChecked(true); break;
		case RVC_STATE: ui.StateRadio->setChecked(true); break;
		case RVC_STRAIN_EN: ui.StrainERadio->setChecked(true); break;
		case RVC_STRAIN: ui.StrainRadio->setChecked(true); break;
		case RVC_STRESS: ui.StressRadio->setChecked(true); break;
		case RVC_PRESSURE: ui.PressureRadio->setChecked(true); break;
		case RVC_STIFFNESS: ui.StiffnessRadio->setChecked(true); break;

	}

	ui.CoMCheck->setChecked(pSim->LockCoMToCenter);

	ui.VariableCombo->setCurrentIndex(CurPlotType);
	ui.DirectionCombo->setCurrentIndex(CurPlotDir);

	ui.UseVolumeEffectsCheck->setChecked(pSim->IsFeatureEnabled(VXSFEAT_VOLUME_EFFECTS)); //IsVolEffectsEnabled());

}

void Dlg_Physics::ApplyVoxSelection(int NewSel)
{
	DeletePlotPoints();
	pSim->CurXSel = NewSel; //TMP!!! don't make cpoies of selection, or it will get out of sync
}

void Dlg_Physics::AddPlotPoint(double Time)
{
	double ToAppend = 0;

	switch (CurPlotType){
	case PL_DISP:
		if (pSim->CurXSel == -1){ //if a voxel is not selected:
			switch (CurPlotDir){
			case MAXDIR: ToAppend = pSim->SS.NormObjDisp; break;
			case XDIR: ToAppend = pSim->SS.TotalObjDisp.x; break;
			case YDIR: ToAppend = pSim->SS.TotalObjDisp.y; break;
			case ZDIR: ToAppend = pSim->SS.TotalObjDisp.z; break;
			}
		}
		else {
			int ThisVox = pSim->CurXSel==-1?0:pSim->CurXSel; //solves weird threading issue
			CVXS_Voxel& CurVox = pSim->VoxArray[pSim->XtoSIndexMap[ThisVox]];
			switch (CurPlotDir){
			case MAXDIR: ToAppend = 0; break; //NOT USED
			case XDIR: ToAppend = CurVox.GetCurPos().x-CurVox.GetNominalPosition().x; break;
			case YDIR: ToAppend = CurVox.GetCurPos().y-CurVox.GetNominalPosition().y; break;
			case ZDIR: ToAppend = CurVox.GetCurPos().z-CurVox.GetNominalPosition().z; break;
			}
		}
		break;
	case PL_ANGLE:
		if (pSim->CurXSel != -1){ //if a voxel is not selected:
			int ThisVox = pSim->CurXSel==-1?0:pSim->CurXSel; //solves weird threading issue
			CVXS_Voxel& CurVox = pSim->VoxArray[pSim->XtoSIndexMap[ThisVox]];
			switch (CurPlotDir){
			case MAXDIR: ToAppend = 0; break; //NOT USED
			case XDIR: ToAppend = CurVox.GetCurAngle().ToRotationVector().x; break;
			case YDIR: ToAppend = CurVox.GetCurAngle().ToRotationVector().y; break;
			case ZDIR: ToAppend = CurVox.GetCurAngle().ToRotationVector().z; break;
			}
		}
		break;
	case PL_KINETIC: 
		ToAppend = pSim->SS.TotalObjKineticE;
		break;
	case PL_STRAINE: 
		ToAppend = pSim->SS.TotalObjStrainE;
		break;
	case PL_TOTALENERGY: 
		ToAppend = pSim->SS.TotalObjKineticE + pSim->SS.TotalObjStrainE;
		break;
	case PL_MAXDISP:
		ToAppend = pSim->SS.MaxVoxVel*pSim->dt;
		break;
	}

	pPlot->AddPoint(Time, ToAppend);

}

void Dlg_Physics::UpdatePlot(void)
{
	if (pPlot->isVisible()){ //only add points if the plot is visible
		double dTStep = pSim->ApproxMSperLog;
		if (dTStep == 0) dTStep = 1;
		pPlot->SetMaxToShow(5000/dTStep); //approximate 5 second window
		pPlot->update();
	}
}


void Dlg_Physics::DeletePlotPoints(void)
{
	pPlot->Reset();
	pPlot->update();
}

void Dlg_Physics::ClickedReset(void)
{
	pSim->ResetSim(); 
	DeletePlotPoints();
	UpdateUI();
}

void Dlg_Physics::ClickedRecord(bool State)
{
	if (State) pSim->BeginRecording();
	else pSim->EndRecording();

	UpdateUI();
}

void Dlg_Physics::ClickedSaveData(void)
{
	pPlot->SaveData();
}

void Dlg_Physics::IsPlotVisible(bool* pVisible)
{
	*pVisible =  pPlot->isVisible();
}
