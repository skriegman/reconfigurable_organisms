/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * The name if its contributors may not be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#include "QVX_Interfaces.h"
#include <QMessageBox>
#include <QTime>
#include "../Voxelyze/VXS_Voxel.h"
#include "../Voxelyze/VXS_Bond.h"
#include "../Voxelyze/VX_MeshUtil.h"
#include "Dlg_VideoCapture.h"

#ifdef linux
#include <unistd.h>
#endif

#ifdef DMU_ENABLED
#include "../../include/DMU.h"
#endif

#ifdef WIN32
#define LOCALSLEEP Sleep
#else
#define LOCALSLEEP sleep
#endif

#define DEFAULT_DISPLAY_UPDATE_MS 33 //normally updates the view at 30fps (33ms)

//QVX_Object:
void QVX_Object::Save(int Compression, bool NewLoc, QString* pNewFilenameOut) //saves the file, or prompts if not yet been saved
{
	QString tmpPath = QString(Path.c_str());
	if (tmpPath == "" || NewLoc){ //if file path is empty string (IE not been saved yet)
		tmpPath = QFileDialog::getSaveFileName(NULL, "Save VXC", GetLastDir(), "Voxel CAD Files (*.vxc)");
		if (tmpPath == "") return; //if we canceled the dialog...
		SetLastDir(tmpPath);
	}

	if (!tmpPath.isNull()){
		Path = tmpPath.toStdString();
		SaveVXCFile(Path, Compression);
		if (pNewFilenameOut) *pNewFilenameOut = QFileInfo(tmpPath).baseName();

	}
	else Path = "";
}

bool QVX_Object::Open(QString* pFilenameOut) //Brings up file dialog to open VXC file
{
#ifdef DMU_ENABLED
	QString tmpPath = QFileDialog::getOpenFileName(NULL, "Open VXC", GetLastDir(), "Voxel CAD Files (*.vxc *.dmf);;DMUnit Files (*.dmu)");
#else
	QString tmpPath = QFileDialog::getOpenFileName(NULL, "Open VXC", GetLastDir(), "Voxel CAD Files (*.vxc *.dmf)");
#endif
	
	if (!tmpPath.isNull()){
		Close();
		#ifdef DMU_ENABLED
		if (tmpPath.right(3).compare("dmu", Qt::CaseInsensitive) == 0) ImportDMU(tmpPath.toStdString(), this);
		else
		#endif
		LoadVXCFile(tmpPath.toStdString());

		if (pFilenameOut) *pFilenameOut = QFileInfo(tmpPath).baseName();
		SetLastDir(tmpPath);

		return true;
	}
	return false;
}

bool QVX_Object::OpenPal(void) //Open a palette
{
	QString TmpPath = QFileDialog::getOpenFileName(NULL, "Open Palette", GetLastDir(), "VoxCad Palette Files (*.vxp *.pal)");;
	
	if(!TmpPath.isNull()){
		LoadVXPFile(TmpPath.toStdString());
		SetLastDir(TmpPath);
		return true;
	}
	return false;
}


bool QVX_Object::SavePal(void) //save a palette
{
	QString TmpPath = QFileDialog::getSaveFileName(NULL, "Save Palette", GetLastDir(), "VoxCad Palette Files (*.vxp)");
	if(!TmpPath.isNull()){
		SaveVXPFile(TmpPath.toStdString()); //store only the palette
		SetLastDir(TmpPath);
		return true;
	}
	return false;
}

void QVX_Object::ExportSTL(void)
{
	QString TmpPath = QFileDialog::getSaveFileName(NULL, "Export STL", GetLastDir(), "Stereolithography Files (*.stl)");
	if(!TmpPath.isNull()){
		CVX_MeshUtil Obj;
		Obj.ToStl(TmpPath.toStdString(), this);
		SetLastDir(TmpPath);
	}
}

void QVX_Object::ExportXYZ(void)
{
	QString OutFilePath = QFileDialog::getSaveFileName(NULL, "Export XYZ Coordinates", GetLastDir(), "TXT Files (*.txt)");

	QFile File(OutFilePath);
		
	if (!File.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QMessageBox::warning(NULL, "File read error", "Could not open file. Aborting.");
		return;
	}
	QTextStream out(&File); 

	out << "MatIndex" << "\t" << "X (m)" << "\t" << "Y (m)" << "\t" << "Z (m)" << "\n";

	Vec3D<> Coord;
	int Mat;
	for (int i=0; i<GetStArraySize(); i++){
		Mat = GetMat(i);
		if (Mat != 0){
			Coord = GetXYZ(i);
			out << Mat << "\t" << Coord.x << "\t" << Coord.y << "\t" << Coord.z << "\n";
		}
	}

	File.close();
	SetLastDir(OutFilePath);

}

void QVX_Object::ExportKV6(void)
{
	QString TmpPath = QFileDialog::getSaveFileName(NULL, "Export KV6", GetLastDir(), "kv6 files (*.kv6)");
	if(!TmpPath.isNull()){
		ExportKV6File(TmpPath.toStdString());
		SetLastDir(TmpPath);
	}

}


//QIcon QVX_Object::GenerateMatIcon(int MatIndex)
//{
//
//}


//QVX_FEA:

void QVX_FEA::RequestSolveSystem(void)
{
	bool Success = true;
	QString RetMessage = "";

	Thread MyThread(&RetMessage);
	MyThread.LinkProgress(&CurProgTick, &CurProgMaxTick, &CurProgMsg, &CancelFlag);

	connect(&MyThread, SIGNAL(CallFunc(QString*)), this, SLOT(ExecuteSolveSystem(QString*)), Qt::DirectConnection);
	MyThread.Execute();

	//broadcast if we succesfully completed the simulation
	if (RetMessage != ""){
		QMessageBox::warning(NULL, "Warning", RetMessage);
		Success = false; //check if we were successful, first! (any return string is an error...)
	}
	emit SolveResult(Success);
}

void QVX_FEA::ExecuteSolveSystem(QString* Param)
{
	bool KeepGoing = true;
	std::string ParamString = Param->toStdString();

	if (!ImportObj(NULL, &ParamString)) KeepGoing = false; //import the linked VXC oject
	if (KeepGoing) Solve(&ParamString); //Solve the system
	*Param = ParamString.c_str();
}


//Environment
bool QVX_Environment::OpenBCs(void)
{
	QString TmpPath = QFileDialog::getOpenFileName(NULL, "Open Boundary Conditions", GetLastDir(), "VoxCad Boundary Condition Files (*.bcx)");;
	
	if(!TmpPath.isNull()){
		LoadBCXFile(TmpPath.toStdString());
		emit BCsChanged();
		SetLastDir(TmpPath);
		return true;
	}
	return false;
}

void QVX_Environment::SaveBCs(void)
{
	QString TmpPath = QFileDialog::getSaveFileName(NULL, "Save Boundary Conditions", GetLastDir(), "DM Boundary Condition Files (*.bcx)");
	if(!TmpPath.isNull()){
		SaveBCXFile(TmpPath.toStdString()); //store only the palette
		SetLastDir(TmpPath);
	}
}

//QVX_Sim::QVX_Sim(CVX_Environment* pSimEnvIn, CMesh* pSimMeshIn, QWidget *parent)
QVX_Sim::QVX_Sim(QWidget *parent)
{
//	pSimEnv = pSimEnvIn;
//	pSimMesh = pSimMeshIn;
	pGLWin = NULL;
	pSimView = new CVXS_SimGLView(this);

	SimMessage = ""; 
	LogEvery = false; 
	ApproxMSperLog = 1.0f; 
	
	LockCoMToCenter = false;
	HideBoundingBox = false;

	connect(&SimThread, SIGNAL(CallFunc(QString*)), this, SLOT(SimLoop(QString*)), Qt::DirectConnection);
	connect(this, SIGNAL(SimEndedInternally(QString)), this, SLOT(CatchInternalSimEnding(QString))); //, Qt::DirectConnection);
	
	Running = false;
	Paused = false;
	StopSim = false;
	Recording = false;
	GLUpdateEveryNFrame = -1;

	VideoOutputFolder = "";
	CurVideoFrameNumber = 0;
//	Dragging = false;
	DraggingVoxel = NULL;
	DraggingStiffness = 0;
	DraggingOffset = Vec3D<>(0,0,0);
	InputPoint = Vec3D<>(0,0,0);

	CurXSel = -1;

}

bool QVX_Sim::OpenVXA(QString* pFileNameOut)
{
	QString tmpPath = QFileDialog::getOpenFileName(NULL, "Open VoxCad Analysis", GetLastDir(), "VoxCad Analysis Files (*.vxa *.dmfea)");
	if (!tmpPath.isNull()){
		std::string ReturnString = "";
		LoadVXAFile(tmpPath.toStdString(), &ReturnString);
		if (ReturnString != "") QMessageBox::warning(NULL, "VXA Load", QString::fromStdString(ReturnString));
		emit BCsChanged();
		if (pFileNameOut) *pFileNameOut = QFileInfo(tmpPath).baseName();
		SetLastDir(tmpPath);
		return true;
	}
	return false;
}

void QVX_Sim::ExportDeformedSTL(void)
{
	bool WasRunning = false;
	if (Running && !Paused){
		WasRunning = true;
		SimPauseToggle();
	}

	QString TmpPath = QFileDialog::getSaveFileName(NULL, "Export STL", GetLastDir(), "Stereolithography Files (*.stl)");
	if (!TmpPath.isNull()){
		pSimView->VoxMesh.ToStl(TmpPath.toStdString(), pEnv->pObj, true);
		SetLastDir(TmpPath);
	}

	if (WasRunning) SimPauseToggle();
}

void QVX_Sim::SaveVXA(QString* pFilenameOut)
{
	QString TmpPath = QFileDialog::getSaveFileName(NULL, "Save VoxCad Analysis", GetLastDir(), "VoxCad Analysis Files (*.vxa)");
	if (!TmpPath.isNull()){
		SaveVXAFile(TmpPath.toStdString()); //store only the palette
		if (pFilenameOut) *pFilenameOut = QFileInfo(TmpPath).baseName();
		SetLastDir(TmpPath);
	}
}
void QVX_Sim::RequestBeginSim()
{
	SimMessage = ""; //reset the message
	SimThread.SArg1 = &SimMessage;
	SimThread.Execute(false);
}

void QVX_Sim::SimLoop(QString* pSimMessage)
{
	std::string tmp; //need to link this up to get info back...
	if (!Import(NULL, NULL, &tmp)){
		*pSimMessage += QString::fromStdString(tmp);
		emit SimEndedInternally(*pSimMessage);
		return;
	}

	pSimView->SmoothMesh.LinkSimSmooth(this, pSimView);
	pSimView->VoxMesh.LinkSimVoxels(this, pSimView);
	pSimView->VoxMesh.DefMesh.DrawSmooth=false;

	int Count = 0; //Possibly redundant with internal sim count.
	int LastCount = 0;
	bool IsStillDrawing;
	Vec3D<> CurSelPos;
	QString PosLabel;
	bool InternalEnding = false;
	QString Message, DispMessage;
	std::string RetMsg;
	QTime tLastPlot; //plot point add every...
	QTime tLastStatus; //status text box updated...
//	QTime tLastStatCalc; //simulation max/mins calculated every...
	tLastPlot.start();
	tLastStatus.start();
//	tLastStatCalc.start();

	emit ReqUiUpdate(); //for slider ranges that depend on dt or other sim params
	emit StartExternalGLUpdate(DEFAULT_DISPLAY_UPDATE_MS);
	StopSim = false;
	Running = true;
	Paused = false;

	//initialize the COM at the start of the simulation	
	IniCM=GetCM();
	Vec3D<> LastCM = IniCM;

	int StatusNumber = 1, PlotPointNumber = 1;
	double UpStatEv = 500; //Updates status pane every X ms
	int UpPlotEv = 30; //updates plot every X ms when not plotting every point.
	char PlotDataTypes;
	bool PlotVis, StatusVis;

 	bool usingGUI = true;
//	double volumeStart = pSimView->VoxMesh.computeAndStoreRobotVolumeStart();
//	double qhullStart = pSimView->VoxMesh.computeAndStoreQHullStart(usingGUI);

	bool alreadyAlteredGravity = false; 
	double defaultGravity = pEnv->GetGravityAccel();

	Vec3D<> normDistPhase1, normDistPhase2;

	while (true){ //do this step...
		if (StopConditionMet()){InternalEnding = true; StatToCalc=CALCSTAT_ALL; UpdateStats(); RetMsg+="Simulation stop condition reached.\n";break;}//if stop condition met...
		
		//figure out what stats we need to calculate
		StatToCalc=CALCSTAT_NONE;

		emit IsPlotVisible(&PlotVis);
		bool PlottingPoint = (LogEvery?true:tLastPlot.elapsed() > PlotPointNumber*UpPlotEv) && PlotVis;
		if (PlottingPoint){ //ensure we calculate the info we want to plot
			emit GetPlotRqdStats(&PlotDataTypes);
			StatToCalc |= PlotDataTypes;
		}
		emit IsStatusTextVisible(&StatusVis);
		bool UpdatingStatus = (tLastStatus.elapsed() > StatusNumber*UpStatEv) && StatusVis;
		if (UpdatingStatus){StatToCalc |= CALCSTAT_COM;} //calc any data we need in the text status box

		bool DrawingGLLocal = (GLUpdateEveryNFrame != -1 && Count%GLUpdateEveryNFrame==0);
		if (DrawingGLLocal || pSimView->NeedStatsUpdate){ //calc any data we need to draw the opengl view...
			StatToCalc |= pSimView->StatRqdToDraw();
			pSimView->NeedStatsUpdate = false;
			if (LockCoMToCenter) StatToCalc |= CALCSTAT_COM;
		}

		//input
		if (DraggingVoxel){
			Vec3D<> Dist = DraggingVoxel->GetCurPos() - InputPoint ;
			DraggingVoxel->SetInputForce(DraggingStiffness*Dist); //F=KX
		}

//		if (tLastStatCalc.elapsed() > StatCalcNumber*30){CalcStat=true; StatCalcNumber++; /*tLastStatCalc.restart();*/}

		if(pEnv->GetAlterGravityHalfway() != 1.0 && !alreadyAlteredGravity && GetStopConditionType() == SC_MAX_SIM_TIME && CurTime >= GetStopConditionValue()/2)
		{
			// Saving some stats before enacting the change...
			normDistPhase1 = getNormCOMdisplacement3D();
			fitPhase1 = normDistPhase1.Length();				
			avgStiffChange1 = getAverageStiffnessChange();

			// Altering gravity the second time, g = g*gravityMultiplier
			pEnv->SetGravityAccel( pEnv->GetGravityAccel()*pEnv->GetAlterGravityHalfway() );
			pSimView->ChangeSkyColor(0.75, 0.94, 1.0); // just to have a visual reference

			//std::cout << "t = " << CurTime <<"/"<< GetStopConditionValue() <<": altering gravity from now on: " << pEnv->GetGravityAccel() << std::endl;
			alreadyAlteredGravity = true;
			emit ReqUiUpdate(); // make sure to update the related GUI textfield to avoid inconsistencies
		}


		if (!TimeStep(&RetMsg)){InternalEnding = true; break;}//if something happened in this timestep

		if (DrawingGLLocal){
			IsStillDrawing = true;
			ReqGLDrawingStatus(&IsStillDrawing);
			while (IsStillDrawing){ //wait to finish drawing the previous timestep before continuing
				LOCALSLEEP(1);
				ReqGLDrawingStatus(&IsStillDrawing); //are 
			}
			emit ReqGLUpdate(); 
		} 

		if (StopSim) break;
		while(Paused){
			ActuallyPaused = true;
			if (StopSim) break; //kick out of the loop if we've stopped...
			LOCALSLEEP(100);
		}

		if(UpdatingStatus){
//			emit IsStatusTextVisible...
			if (CurXSel != -1){PosLabel = "Vox "+QString::number(XtoSIndexMap[CurXSel]) + " (sim)"; CurSelPos = VoxArray[XtoSIndexMap[CurXSel]].GetCurPos()*1000; }//position in mm
			else {PosLabel = "CoM "; CurSelPos = SS.CurCM*1000;} //center of mass in mm if nothing else selected
			Message = "Time " + QString::number(CurTime, 'g', 3) + " sec" + 
					"\nStep Num " + QString::number(CurStepCount) + 
					"\nTime Step = " + QString::number(dt, 'g', 3)+ " sec" + 
					"\nDisplay Rate  = "+ QString::number((Count-LastCount)/(UpStatEv/1000.0), 'g', 3)+" Steps/sec" +
					"\n" + PosLabel + " X:"+ QString::number(CurSelPos.x, 'g', 3)+"  Y:"+ QString::number(CurSelPos.y, 'g', 3)+"  Z:"+ QString::number(CurSelPos.z, 'g', 3)+ " mm" +
					"\nCoM Displacement = "+ QString::number((SS.CurCM-IniCM).Length()*1000, 'g', 3)+" mm" +
					"\nCoM Norm Displacement = "+ QString::number(getNormCOMdisplacement3D().Length(), 'g', 3)+" vox" +
					"\nCoM Velocity = "+ QString::number((SS.CurCM-LastCM).Length()*1000/(dt*(Count-LastCount)), 'g', 3)+" mm/s";
			
			//Boundary conditions
			Message += "\n";
			for (int i=0; i<pEnv->GetNumBCs(); i++){
				Vec3D<> ThisBCForce = GetSumForce(pEnv->GetBC(i));
				Vec3D<> ThisBCDisplace = GetAvgDisplace(pEnv->GetBC(i));
				Message += "\nBoundary Condition " + QString::number(i) + ":" +
					"\n  Force X:" + QString::number(ThisBCForce.x, 'g', 3) + "N" + 
					"\n  Force Y:" + QString::number(ThisBCForce.y, 'g', 3) + "N" + 
					"\n  Force Z:" + QString::number(ThisBCForce.z, 'g', 3) + "N" + 
					"\n  Displacement X:" + QString::number(ThisBCDisplace.x*1000, 'g', 3) + "  Y:" + QString::number(ThisBCDisplace.y*1000, 'g', 3) + "  Z:" + QString::number(ThisBCDisplace.z*1000, 'g', 3) + " mm"; 

			
			}
			if (CurXSel != -1){
				Vec3D<> Size = 1000*VoxArray[XtoSIndexMap[CurXSel]].GetSizeCurrent();
				Message += "\n\nSize X:" + QString::number(Size.x, 'g', 3)+" Y:" + QString::number(Size.y, 'g', 3)+" Z:" + QString::number(Size.z, 'g', 3) + " mm"
					"\nVolume = " + QString::number(Size.x*Size.y*Size.z, 'g', 3) + " mm^3";
			}

			if (IsFeatureEnabled(VXSFEAT_FLOOR)) Message += "\nVoxels touching ground: " + QString::number(GetNumTouchingFloor());

			Message += "\n";
//			Message += "Initial volume:"+QString::number(volumeStart)+" mm^3\n";
//			Message += "Initial chull:"+QString::number(qhullStart)+" mm^3\n";
//			Message += "Initial branching index: " + QString::number(1.0-(volumeStart/qhullStart), 'f', 3) + "\n\n";
			Message += "Average stiffness change: "+QString::number(getAverageStiffnessChange()) + "\n";
			Message += "NormDistPhase1: "+QString::number(fitPhase1) + "\n";
			Message += "NormDistPhase2: "+QString::number(fitPhase2) + "\n";	
			Message += "AvgStiffChangePhase1: "+QString::number(avgStiffChange1) + "\n";
			Message += "AvgStiffChangePhase2: "+QString::number(avgStiffChange2) + "\n";
			Message += "Gravity: "+QString::number(pEnv->GetGravityAccel()) + "\n";			

			if(alreadyAlteredGravity)
			{
				Message += "\n";
				Message += "** GRAVITY CHANGED HALFWAY, now is: "+QString::number(pEnv->GetGravityAccel()) + "\n";
			}

			LastCM = SS.CurCM;

			if (LogEvery) ApproxMSperLog = (double)tLastStatus.elapsed()/(Count-LastCount); //only update ms per step if its not fixed by the timer
			LastCount = Count;

			emit UpdateText(Message);
			StatusNumber++;
		}

		if (PlottingPoint){
			if (LogEvery) emit ReqAddPlotPoint(GetCurTime());
			else {
				ApproxMSperLog = UpPlotEv;
				emit ReqAddPlotPoint(GetCurTime());
				PlotPointNumber++;
			}
		}

//		if (StatCalcNumber == INT_MAX){StatCalcNumber=1; tLastStatCalc.restart();} //avoid int rollover for our counters
		if (StatusNumber == INT_MAX){StatusNumber=1; tLastStatus.restart();}
		if (PlotPointNumber == INT_MAX){PlotPointNumber=1; tLastPlot.restart();}


		Count++;


		///////////////////////////////////////////////////////////////////
		// DEBUG: print traces to command line
		// TODO: GUI checkbox to enable or disable print
		//Vec3D<> CM = GetCM();
		//std::cout << CurTime << "\t" << CM.x << "\t" << CM.y << "\t" << CM.z << "\t" << getAverageStiffnessChange() << std::endl;
		///////////////////////////////////////////////////////////////////		
	}

	normDistPhase2 = normDistPhase1 - getNormCOMdisplacement3D();
	fitPhase2 = normDistPhase2.Length();
	avgStiffChange2 = getAverageStiffnessChange() - avgStiffChange1;

	DraggingVoxel=NULL;
	emit StopExternalGLUpdate();

//	emit SetExternalGLUpdate(false);
	Running = false;
	Paused = false;

	Message = "Time " + QString::number(CurTime, 'g', 3) + " Sec" + 
		"\nStep Num " + QString::number(CurStepCount) + 
		"\nTime Step = " + QString::number(dt, 'g', 3)+ " Sec" + 
		"\n" + PosLabel + " X:"+ QString::number(CurSelPos.x, 'g', 3)+"  Y:"+ QString::number(CurSelPos.y, 'g', 3)+"  Z:"+ QString::number(CurSelPos.z, 'g', 3)+ " mm" +
		"\nCOM_Dist = "+ QString::number((SS.CurCM-IniCM).Length()*1000, 'g', 3)+" mm" + 
		"\nCoM Norm Displacement = "+ QString::number(getNormCOMdisplacement3D().Length(), 'g', 3)+" vox\n\n";

//	double qhullEnd = pSimView->VoxMesh.computeAndStoreQHullEnd(usingGUI);
//	double volumeEnd = pSimView->VoxMesh.computeAndStoreRobotVolumeEnd();
//
//	QString volumeInformation("Initial volume: " + QString::number(volumeStart, 'f', 3) + "  mm^3\n");
//	volumeInformation += "Initial chull volume: " + QString::number(qhullStart, 'f', 3) + "  mm^3\n";
//	volumeInformation += "Initial branching index: " + QString::number(volumeStart/qhullStart, 'f', 3) + "\n\n";
//	volumeInformation += "Final volume: " + QString::number(volumeEnd, 'f', 3) + " mm^3\n";
//	volumeInformation += "Final chull volume: " + QString::number(qhullEnd, 'f', 3) + "  mm^3\n";
//	volumeInformation += "Final branching index: " + QString::number(1.0-(volumeEnd/qhullEnd), 'f', 3) + "\n\n";

	RetMsg += "Simulation stopped.\n";
	Message += RetMsg.c_str();
	DispMessage = Message + "\nFinal Conditions:\nStep = " + QString::number(CurStepCount) + "\nTime Step = " + QString::number(dt)+"\nKinetic Energy = " + QString::number(SS.TotalObjKineticE);  // +"\n"+volumeInformation;
	DispMessage += "Average stiffness change: "+QString::number(getAverageStiffnessChange()) + "\n";

	DispMessage += "NormDistPhase1: "+QString::number(fitPhase1) + "\n";
	DispMessage += "NormDistPhase2: "+QString::number(fitPhase2) + "\n";	
	DispMessage += "AvgStiffChangePhase1: "+QString::number(avgStiffChange1) + "\n";
	DispMessage += "AvgStiffChangePhase2: "+QString::number(avgStiffChange2) + "\n";

	emit UpdateText(DispMessage);

	if (InternalEnding)	emit SimEndedInternally(RetMsg.c_str());

	alreadyAlteredGravity = false;
	pEnv->SetGravityAccel(defaultGravity);

	pSimView->ChangeSkyColor(1.0, 1.0, 1.0);

	emit ReqUiUpdate();
//	Running = false;



}

void QVX_Sim::CatchInternalSimEnding(QString Msg)
{
	EndRecording();
	ReqGLUpdate();
	if (Msg != "") QMessageBox::warning(NULL, "Simulation Error", Msg);
}

//bool QVX_Sim::TimeStepMT(std::string* pRetMessage)
//{
//	if(SelfColEnabled) UpdateCollisions(); //update self intersection lists if necessary
//	IntegrateMT(CurIntegrator);
//	return UpdateStats(pRetMessage);
//}
//
//void QVX_Sim::IntegrateMT(IntegrationType Integrator)
//{
//	switch (Integrator){
//		case I_EULER: {
//			QFuture<void> res = QtConcurrent::map(BondArrayInternal.begin(), BondArrayInternal.end(), QVX_Sim::BondCalcForce);
//			res.waitForFinished();
//	
//
//			////Update Forces...
//			//int iT = NumBond();
//			//for (int i=0; i<iT; i++){
//			//	QFuture<void> future = QtConcurrent::run(BondCalcForce, &BondArrayInternal[i]);
//			//	//BondArrayInternal[i].CalcForce();
//			//}
//
//			OptimalDt = CalcMaxDt();
//			dt = DtFrac*OptimalDt;
//
//			res = QtConcurrent::map(VoxArray.begin(), VoxArray.end(), QVX_Sim::VoxEulerStep);
//			res.waitForFinished();
//
//			//Update positions... need to do thises seperately if we're going to do damping in the summing forces stage.
//	//		int iT = NumVox();
//	//		for (int i=0; i<iT; i++) VoxArray[i].EulerStep();
//
//		}
//		break;
//	}
//}


void QVX_Sim::SimPauseToggle()
{
	bool WasRunning = false;
	if (Running && !Paused){
		WasRunning = true;
		ActuallyPaused = false; //if sim was running and we're pausing it, use this to wait until reached pause loop to return form the pause function
	}

	Paused = !Paused;
	ReqGLUpdate();

	if (WasRunning){
		while (!ActuallyPaused){QCoreApplication::processEvents(); LOCALSLEEP(10);}
	}
}

void QVX_Sim::EndSim()
{
	StopSim = true;
	SimThread.wait(); //wait until it actually exits...
	EndRecording(); //stops any recording we may have running
	pSimView->VoxMesh.Clear();
	ReqGLUpdate();
}

void QVX_Sim::ResetSim()
{
	EndRecording(); //stops any recording we may have running
	ResetSimulation();
	ReqGLUpdate();
}

void QVX_Sim::LMouseDown(Vec3D<> P)
{
	if (CurXSel != -1){
//		if (InputVoxSInd == -1) //if the input voxel has not yet been added to the voxel array...
//			CVoxelArray

		DraggingVoxel = &VoxArray[XtoSIndexMap[CurXSel]];

//		int SelSIndex = XtoSIndexMap[CurXSel]; //simulation index of currently selected voxel
//		CVXS_Voxel& CurSelVox = VoxArray[SelSIndex];


//		GetInputVoxel()->OverridePos(CurSelVox.GetCurPos()); //align the virtual input voxel to the selected one
//		GetInputVoxel()->SetMaterial(CurSelVox.GetMaterial()); //set to same material for reasonable stiffness

//		GetInputBond()->SetVoxels(SelSIndex, INPUT_VOX_INDEX);

		//UpdateBond(InputBondInd, SelSIndex, InputVoxSInd, false);

//		InputBond()->SetVox1SInd(SelSIndex);
//		InputBond()->SetVox2SInd(InputVoxSInd);
//		GetInputBond()->UpdateConstants();

//		VoxArray[SIndex1In].LinkBond(MyBondIndex);

		DraggingOffset = P - DraggingVoxel->GetCurPos();
		InputPoint = P-DraggingOffset;
		DraggingStiffness = DraggingVoxel->GetLinearStiffness(); //GetEMod()*DraggingVoxel->GetNominalSize(); //E*A/L = E*L since A=L^2
//		Dragging = true;
	}
	else {
//		Dragging = false;
		if (DraggingVoxel){
			DraggingVoxel->SetInputForce(Vec3D<>(0,0,0));
			DraggingVoxel = NULL;
			DraggingStiffness = 0;
		}
	}
}

void QVX_Sim::LMouseUp(Vec3D<> P)
{
	if (DraggingVoxel){
		DraggingVoxel->SetInputForce(Vec3D<>(0,0,0));
		DraggingVoxel = NULL;
		DraggingStiffness = 0;
	}
//	Dragging = false;

}

void QVX_Sim::LMouseDownMove(Vec3D<> P)
{
	if (DraggingVoxel){
		InputPoint = P-DraggingOffset;
	}
}


void QVX_Sim::DrawOverlay(void)
{
	ViewColor CurViewCol = pSimView->GetCurViewCol();
	if (CurViewCol == RVC_TYPE || CurViewCol == RVC_STIFFNESS || CurViewCol == RVC_KINETIC_EN || CurViewCol == RVC_DISP || CurViewCol == RVC_STRAIN_EN || CurViewCol == RVC_STRAIN || CurViewCol == RVC_STRESS || CurViewCol == RVC_PRESSURE){
		CColor Tmp;
		int XOff = 10;
		int YOff = 10;
		int XWidth = 30;
		int YHeight = 200;
		int NumChunks = 4;
		int TextXOff = 10;

		glBegin(GL_QUAD_STRIP);
		
		for (int i=0; i<=NumChunks; i++){
			double Perc = ((double)i)/NumChunks;
			Tmp = pSimView->GetJet(1.0-Perc);
			glColor4f(Tmp.r, Tmp.g, Tmp.b, Tmp.a);

			glVertex2f(XOff,YOff+Perc*YHeight);
			glVertex2f(XOff+XWidth,YOff+Perc*YHeight);

		}
		glEnd();

		//draw the labels...
		double MaxVal = 1.0, MinVal = 0.0;
		QString Units = "";
		switch(CurViewCol){
		    case RVC_TYPE: MaxVal = 0.25; Units = " Error" ; break;
			case RVC_KINETIC_EN: MaxVal = 1; MinVal = -1; Units = " Vmem" ; break;//MaxVal = SS.MaxVoxKinE*1000; Units = "mJ"; break;
			case RVC_DISP: MaxVal = 1; MinVal = -1; Units = " "; break;//MaxVal = SS.MaxVoxDisp*1000; Units = "mm"; break;
			case RVC_STRAIN_EN: MaxVal = PI/2; MinVal = -PI/2; Units = " Radians"; break; //SS.MaxBondStrainE*1000; Units = "mJ"; break;
			case RVC_STIFFNESS: MaxVal = pEnv->pObj->GetMaxElasticMod()/1e06; MinVal = pEnv->pObj->GetMinElasticMod()/1e06; Units = "MPa"; break;
			case RVC_STRAIN: MaxVal = SS.MaxBondStrain; break;
			case RVC_STRESS: MaxVal = SS.MaxBondStress/1000000; Units = "MPa"; break;
			case RVC_PRESSURE: {
				MaxVal = SS.MaxPressure/1000000;
				MinVal = SS.MinPressure/1000000;
				if (-MaxVal < MinVal) MinVal = -MaxVal;
				else MaxVal = -MinVal;
				Units = "MPa";
				break;
							   }
		}

		glColor4f(0, 0, 0, 1.0);
		QString ScaleNumber;

		for (int i=0; i<=NumChunks; i++){
			double Perc = ((double)i)/NumChunks;
			ScaleNumber = QString::number(MinVal + (1-Perc) * (MaxVal-MinVal), 'g', 3) + Units;
			pGLWin->renderText(XOff + XWidth + TextXOff, YOff + Perc*YHeight+5, ScaleNumber);
		}


		
	}


}

void QVX_Sim::BeginRecording(void) //opens record dialog and begins recording
{
	if (Recording) return; //if we're already recording, don't do anything!

	bool WasPaused = Paused; //pause the sim and remeber whether it was paused or not...
	if (Running && !WasPaused) SimPauseToggle();

	Dlg_VideoCapture VidCapDlg(this);
	if (StopConditionType == SC_MAX_TIME_STEPS) {VidCapDlg.StopEnabled=true; VidCapDlg.CurStopSettings = SS_TIME_STEPS; VidCapDlg.sNumStep = qRound(StopConditionValue);}
	if (StopConditionType == SC_MAX_SIM_TIME) {VidCapDlg.StopEnabled=true; VidCapDlg.CurStopSettings = SS_SIM_TIME; VidCapDlg.sSimTime = StopConditionValue;}
	if (StopConditionType == SC_TEMP_CYCLES) {VidCapDlg.StopEnabled=true; VidCapDlg.CurStopSettings = SS_TEMP_CYCLES; VidCapDlg.sNumCyc = qRound(StopConditionValue);}

	VidCapDlg.UpdateUI();
	VidCapDlg.exec();

	if (VidCapDlg.AcceptedDialog){
		if (VidCapDlg.ResetSimOnBegin) ResetSim();

		emit StopExternalGLUpdate(); //stop external update!
		GLUpdateEveryNFrame = -1; //make sure update every frame is turned off...

		double OutFps = VidCapDlg.OutputFps;
		double OutSpdFctr = VidCapDlg.OutputSpeedFactor;

		switch(VidCapDlg.CurVidSetting){
			case VS_DISPLAY: StartExternalGLUpdate(1000*OutSpdFctr/OutFps); break;
			case VS_SIMULATION: {
					DtFreeze(); //this could be bad for sims with non-linear materials that change dramatically in their stability
					double VideoDt = OutSpdFctr/OutFps; //what the dt step for video should be (in sec)
					if (dt > VideoDt){
						dt = VideoDt; //if simulation timesteps are greater than desired video framerate slow down simulation timesteps
						GLUpdateEveryNFrame=1;
					}
					else { //round dt down so that VideoDt is an even multiple...
						GLUpdateEveryNFrame = (int)(VideoDt/dt) + 1;
						dt = VideoDt/((double)GLUpdateEveryNFrame);
					}

				}
				break;
			case VS_EVERY: GLUpdateEveryNFrame = 1; break;
			default: break;
		}

		if (VidCapDlg.StopEnabled){
			switch (VidCapDlg.CurStopSettings){
				case SS_TIME_STEPS: StopConditionType = SC_MAX_TIME_STEPS; StopConditionValue = (vfloat)VidCapDlg.sNumStep; break;
				case SS_SIM_TIME: StopConditionType = SC_MAX_SIM_TIME; StopConditionValue = (vfloat)VidCapDlg.sSimTime; break;
				case SS_TEMP_CYCLES: StopConditionType = SC_TEMP_CYCLES; StopConditionValue = (vfloat)VidCapDlg.sNumCyc; break;
				default: StopConditionType = SC_NONE;
			}
		}
		else StopConditionType = SC_NONE;

		//reset naming for current files in the selected folder
		VideoOutputFolder=VidCapDlg.CurFolder;
		CurVideoFrameNumber = 0;

		emit ResizeGLWindow(VidCapDlg.WidthPix, VidCapDlg.HeightPix);

		Recording = true;

		if (!Running){ RequestBeginSim(); Running = true;}
		else SimPauseToggle(); //always unpause the simulation if we began recording
	}
	else { //rejected (canceled) return to running or not state.
		if (Running && !WasPaused) SimPauseToggle(); //unpause the simulation if it was running before
	}
}


void QVX_Sim::EndRecording(void) //stops recording
{
	if (!Recording) return; //if already not recording, don't do anything

	DtThaw();
	GLUpdateEveryNFrame = -1; //make sure update every frame is turned off...
	emit StopExternalGLUpdate(); //stop external display that might be at different speed
	emit StartExternalGLUpdate(DEFAULT_DISPLAY_UPDATE_MS); //start default display rate again

	emit ResetGLWindow();

	Recording = false;
}
