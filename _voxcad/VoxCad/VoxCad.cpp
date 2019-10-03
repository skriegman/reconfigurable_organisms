/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * The name if its contributors may not be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/


#include "VoxCad.h"
#include "../QTUtils/QOpenGL.h"
#include <QMessageBox>
#include <QClipboard>
#include <QCoreApplication>

//#include "Dlg_EditPrim.h"
#include "Dlg_Tensile.h" 

//temp for cout experiementsL:
//#include <ostream>
//#include <iostream> 

VoxCad::VoxCad(QWidget *parent, Qt::WFlags flags)
: QMainWindow(parent, flags) //, MainSim(&MainEnv, &MainMesh)
{
	QCoreApplication::setOrganizationName("HillerLabs");
	QCoreApplication::setOrganizationDomain("www.VoxCAD.com");
	QCoreApplication::setApplicationName("VoxCAD");

	//setup main object and FEA
	MainEnv.pObj = &MainObj; //connect environment to object
	MainFEA.pEnv = &MainEnv; //connect FEA to envirnment
	MainSim.pEnv = &MainEnv; //connect Simulation to envirnment
	connect(&MainObj, SIGNAL(UpdateGLWindows()), this, SLOT(ReqGLUpdateAll()));
	connect(&MainObj, SIGNAL(GetCurGLSelected(int*)), this, SLOT(GetCurGLSelected(int*)));
	connect(&MainFEA, SIGNAL(GetCurGLSelected(int*)), this, SLOT(GetCurGLSelected(int*)));
	connect(&MainFEA, SIGNAL(SolveResult(bool)), this, SLOT(FEAMode(bool))); //allows FEA solve to call back, decide whether to enter mode or not

	CurGLSelected = -1;
	GraphicsEnabled = true;

	ui.setupUi(this);
	SetupGLWindow();
	SetupVoxInfoWindow();
	SetupPaletteWindow();
	SetupWorkspaceWindow();
	SetupBCWindow();
	SetupFEAInfoWindow();
	SetupRef3DWindow();
	SetupPhysicsWindow();
	SetupTensileWindow();
	SetupBrushWindow();

	//set up initial application toolbox configuration
	tabifyDockWidget(BrushDockWidget, PhysicsDockWidget);
	tabifyDockWidget(PhysicsDockWidget, TensileDockWidget);
	tabifyDockWidget(TensileDockWidget, FEAInfoDockWidget);
	tabifyDockWidget(FEAInfoDockWidget, BCDockWidget);
	tabifyDockWidget(BCDockWidget, WorkspaceDockWidget);
	tabifyDockWidget(WorkspaceDockWidget, PaletteDockWidget);

	//Connect Menus:
	//File
	connect(ui.actionNew, SIGNAL(triggered()), this, SLOT(New()));
	connect(ui.actionOpen, SIGNAL(triggered()), this, SLOT(OpenVXC()));
	connect(ui.actionSave, SIGNAL(triggered()), this, SLOT(SaveZLib()));
	connect(ui.actionSave_As, SIGNAL(triggered()), this, SLOT(SaveAsZLib()));
	//..Import
	connect(ui.actionVXA_IN, SIGNAL(triggered()), this, SLOT(ImportVXA()));
	//..Export
	connect(ui.actionVXA_OUT, SIGNAL(triggered()), this, SLOT(SaveVXA()));
	#ifdef DMU_ENABLED
	connect(ui.actionDMU_Out, SIGNAL(triggered()), this, SLOT(ExportDMU()));
	#endif
	connect(ui.actionVoxelCoordinates, SIGNAL(triggered()), &MainObj, SLOT(ExportXYZ()));
	connect(ui.actionKV6, SIGNAL(triggered()), &MainObj, SLOT(ExportKV6()));
	


	//....Mesh
	connect(ui.actionSTL, SIGNAL(triggered()), this, SLOT(ExportSTL()));
	//....Alternate VXC
	connect(ui.actionASCII_VXC, SIGNAL(triggered()), &MainObj, SLOT(SaveAsAsciiReadable()));
	connect(ui.actionBASE64_VXC, SIGNAL(triggered()), &MainObj, SLOT(SaveAsBase64()));

	//Edit
	connect(ui.actionCopy, SIGNAL(triggered()), this, SLOT(Copy()));
	connect(ui.actionCut, SIGNAL(triggered()), this, SLOT(Cut()));
	connect(ui.actionPaste, SIGNAL(triggered()), this, SLOT(Paste()));
	connect(ui.actionWorkspace, SIGNAL(toggled(bool)), this, SLOT(ViewWorkspaceWindow(bool)));
	connect(ui.actionPalette, SIGNAL(toggled(bool)), this, SLOT(ViewPaletteWindow(bool)));

	//View
	connect(ui.actionShow_Triad, SIGNAL(triggered(bool)), GLWindow, SLOT(SetAxesVisible(bool)));
	connect(ui.actionCenter_View, SIGNAL(triggered()), GLWindow, SLOT(ZoomExtents()));
	connect(ui.actionLarge_Mode, SIGNAL(triggered(bool)), GLWindow, SLOT(EnterFastMode(bool)));
	connect(ui.actionView_Tiled, SIGNAL(triggered(bool)), this, SLOT(ViewTiled(bool)));
	connect(ui.actionGraphics_Enabled, SIGNAL(triggered(bool)), this, SLOT(EnableGraphics(bool)));

	
	
	//..Standard Views
	connect(ui.actionPerspective, SIGNAL(triggered()), this, SLOT(ViewPerspective()));
	connect(ui.actionTop, SIGNAL(triggered()), this, SLOT(ViewTop()));
	connect(ui.actionBottom, SIGNAL(triggered()), this, SLOT(ViewBottom()));
	connect(ui.actionLeft, SIGNAL(triggered()), this, SLOT(ViewLeft()));
	connect(ui.actionRight, SIGNAL(triggered()), this, SLOT(ViewRight()));
	connect(ui.actionFront, SIGNAL(triggered()), this, SLOT(ViewFront()));
	connect(ui.actionBack, SIGNAL(triggered()), this, SLOT(ViewBack()));
	//..Section View
	connect(ui.actionSection_View, SIGNAL(toggled(bool)), this, SLOT(SetSectionView(bool)));
	connect(ui.actionLayer_Back, SIGNAL(triggered()), &MainObj, SLOT(LayerBack()));
	connect(ui.actionLayer_Forward, SIGNAL(triggered()), &MainObj, SLOT(LayerForward()));
	//--
	connect(ui.actionReference_View, SIGNAL(toggled(bool)), this, SLOT(ViewRef3DWindow(bool)));
	connect(ui.actionInfo, SIGNAL(toggled(bool)), this, SLOT(ViewVoxInfoWindow(bool)));
	
	//Tools
	connect(ui.actionEdit_Voxels, SIGNAL(toggled(bool)), this, SLOT(EditMode(bool)));
	connect(ui.action3D_Brush, SIGNAL(toggled(bool)), this, SLOT(Brush3DMode(bool)));

	

	//..Drawing
	connect(ui.actionPencil, SIGNAL(triggered()), &MainObj, SLOT(SetDrawPencil()));
	connect(ui.actionRectangle, SIGNAL(triggered()), &MainObj, SLOT(SetDrawRectangle()));
	connect(ui.actionCircle, SIGNAL(triggered()), &MainObj, SLOT(SetDrawCircle()));
	connect(ui.actionPaint_Bucket, SIGNAL(triggered()), &MainObj, SLOT(SetDrawBucket()));

	//Analyze
	connect(ui.actionBCs, SIGNAL(triggered(bool)), this, SLOT(BCsMode(bool))); //triggered (vs toggled) avoids getting called when setcheck is called.
	connect(ui.actionSolve, SIGNAL(triggered(bool)), this, SLOT(RequestFEAMode(bool))); //triggered (vs toggled) avoids getting called when setcheck is called.
	connect(ui.actionPhysics, SIGNAL(triggered(bool)), this, SLOT(PhysicsMode(bool))); //triggered (vs toggled) avoids getting called when setcheck is called.
	connect(ui.actionTensile, SIGNAL(triggered(bool)), this, SLOT(TensileMode(bool))); //triggered (vs toggled) avoids getting called when setcheck is called.
//	connect(ui.actionTensile, SIGNAL(triggered()), this, SLOT(TensileTest())); //triggered (vs toggled) avoids getting called when setcheck is called.
	
	//Menu groups:
	DrawGroup = new QActionGroup(this);
	DrawGroup->addAction(ui.actionPencil);
	DrawGroup->addAction(ui.actionRectangle);
	DrawGroup->addAction(ui.actionCircle);
	DrawGroup->addAction(ui.actionPaint_Bucket);
	ui.actionPencil->setChecked(true);


	//initialize other random stuff
	SetSectionView(false);
	New(); //creates new object to start with
	ui.actionGraphics_Enabled->setChecked(GraphicsEnabled);


	CurViewMode = VM_NONE; //so 3DView will be a new mode!
	EnterVMMode(VM_3DVIEW);


	//redirect cout:
	//	http://www.qtcentre.org/threads/17880-Display-content-wirtten-to-STL-ostream-dynamically
	//std::ostream myWidget(;
	//std::streambuf *oldbuf = std::cout.rdbuf(myWidget.rdbuf());
}

VoxCad::~VoxCad()
{
}

void VoxCad::SetupGLWindow(void)
{
	QGLFormat format; // double buffering and depth buffering is turned on in the default format, so no need to reset those
	GLWindow = new CQOpenGL(format);
	ui.horizontalLayout->addWidget(GLWindow);

	//opengl info
	connect(GLWindow, SIGNAL(FindDims(Vec3D<>*, Vec3D<>*)), &MainObj, SLOT(GetDim(Vec3D<>*, Vec3D<>*)));
	connect(GLWindow, SIGNAL(DrawGL(bool)), this, SLOT(DrawCurScene(bool)));
	connect(GLWindow, SIGNAL(DrawGLOverlay()), this, SLOT(DrawCurSceneOverlay()));
	connect(GLWindow, SIGNAL(MousePressIndex(int)), this, SLOT(SetGLSelected(int)));
	connect(GLWindow, SIGNAL(FastModeChanged(bool)), this, SLOT(FastModeChanged(bool)));

	//mouse handling
	connect(GLWindow, SIGNAL(WantGLIndex(bool*)), this, SLOT(WantGLIndex(bool*)));
	connect(GLWindow, SIGNAL(WantCoord3D(bool*)), this, SLOT(WantCoord3D(bool*)));

	connect(GLWindow, SIGNAL(MouseMoveHover(float, float, float)), this, SLOT(HoverMove(float, float, float)));
	connect(GLWindow, SIGNAL(LMouseMovePressed(float, float, float)), this, SLOT(LMouseDownMove(float, float, float)));
	connect(GLWindow, SIGNAL(LMouseDown(float, float, float, bool)), this, SLOT(LMouseDown(float, float, float, bool)));
	connect(GLWindow, SIGNAL(LMouseUp(float, float, float)), this, SLOT(LMouseUp(float, float, float)));
	connect(GLWindow, SIGNAL(PressedEscape()), this, SLOT(PressedEscape()));
	connect(GLWindow, SIGNAL(CtrlWheelRoll(bool)), this, SLOT(CtrlMouseRoll(bool)));
	
	MainSim.pGLWin = GLWindow;
}

void VoxCad::SetupRef3DWindow(void)
{
	Ref3DDockWidget = new QDockWidget(this);

	QGLFormat format; // double buffering and depth buffering is turned on in the default format, so no need to reset those
	GLRef3DWin = new CQOpenGL(format);
	GLRef3DWin->SetViewCustom1();

	Ref3DDockWidget->setWidget(GLRef3DWin);
    Ref3DDockWidget->setWindowTitle("3D View");

	connect(Ref3DDockWidget->toggleViewAction(), SIGNAL(toggled(bool)), this, SLOT(ViewRef3DWindow(bool)));
	connect(GLRef3DWin, SIGNAL(FindDims(Vec3D<>*, Vec3D<>*)), &MainObj, SLOT(GetDim(Vec3D<>*, Vec3D<>*)));
	connect(GLRef3DWin, SIGNAL(DrawGL(bool)), &MainObj, SLOT(DrawSceneView(bool)));

	Ref3DDockWidget->hide();
	Ref3DDockWidget->setFloating(true);
	Ref3DDockWidget->setGeometry (Ref3DDockWidget->parentWidget()->geometry().x()+10,Ref3DDockWidget->parentWidget()->geometry().y()+80,340,280 );

}

void VoxCad::SetupBrushWindow(void)
{
	BrushDockWidget = new QDockWidget(this);
	BrushDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

	if (MainSim.ImportSurfMesh)	Brush3DDlg = new Dlg_3DBrush(MainEnv.pObj, MainSim.ImportSurfMesh, BrushDockWidget);
	else Brush3DDlg = new Dlg_3DBrush(MainEnv.pObj, NULL, BrushDockWidget);

	BrushDockWidget->setWidget(Brush3DDlg);
    BrushDockWidget->setWindowTitle("3D Brush");
	BrushDockWidget->setVisible(false);

	connect(BrushDockWidget->toggleViewAction(), SIGNAL(triggered(bool)), this, SLOT(Brush3DMode(bool)));
	connect(Brush3DDlg, SIGNAL(RequestUpdateGL()), this, SLOT(ReqGLUpdateAll()));
	connect(Brush3DDlg, SIGNAL(DoneAdding()), this, SLOT(ViewMode(void))); 
	
	addDockWidget(Qt::RightDockWidgetArea, BrushDockWidget);

}

void VoxCad::SetupPaletteWindow(void)
{
	PaletteDockWidget = new QDockWidget(this);
	PaletteDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	PaletteDlg = new Dlg_Palette(&MainObj, PaletteDockWidget);
	PaletteDockWidget->setWidget(PaletteDlg);
    PaletteDockWidget->setWindowTitle("Palette");

	addDockWidget(Qt::RightDockWidgetArea, PaletteDockWidget);

	connect(PaletteDockWidget->toggleViewAction(), SIGNAL(toggled(bool)), this, SLOT(ViewPaletteWindow(bool)));
	connect(PaletteDlg, SIGNAL(RequestUpdateGL()), this, SLOT(ReqGLUpdateAll()));
	connect(&MainObj, SIGNAL(GetCurMaterial(int*)), PaletteDlg, SLOT(GetCurMat(int*)));
}

void VoxCad::SetupWorkspaceWindow(void)
{
	WorkspaceDockWidget = new QDockWidget(this);
	WorkspaceDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	WorkspaceDlg = new Dlg_Workspace(&MainObj, WorkspaceDockWidget);
	WorkspaceDockWidget->setWidget(WorkspaceDlg);
    WorkspaceDockWidget->setWindowTitle("Workspace");

	addDockWidget(Qt::RightDockWidgetArea, WorkspaceDockWidget);

	WorkspaceDlg->IniUpdateUI();
	connect(WorkspaceDockWidget->toggleViewAction(), SIGNAL(toggled(bool)), this, SLOT(ViewWorkspaceWindow(bool)));
	connect(WorkspaceDlg, SIGNAL(RequestUpdateGL()), this, SLOT(ReqGLUpdateAll()));
	connect(WorkspaceDlg, SIGNAL(WSDimChanged()), this, SLOT(WSDimChanged()));

	connect(&MainObj, SIGNAL(GetCurMaterial(int*)), PaletteDlg, SLOT(GetCurMat(int*)));
}

void VoxCad::SetupVoxInfoWindow(void)
{
	VoxInfoDockWidget = new QDockWidget(this);
	VoxInfoDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	VoxInfoDlg = new Dlg_VoxInfo(VoxInfoDockWidget);
	VoxInfoDockWidget->setWidget(VoxInfoDlg);
    VoxInfoDockWidget->setWindowTitle("Voxel Info");

	connect(VoxInfoDockWidget->toggleViewAction(), SIGNAL(toggled(bool)), this, SLOT(ViewVoxInfoWindow(bool)));
	connect(VoxInfoDlg, SIGNAL(GetCurIndex(int*)), this, SLOT(GetCurGLSelected(int*)));
	connect(VoxInfoDlg, SIGNAL(GetDMInfoString(QString*)), &MainObj, SLOT(GetVXCInfoString(QString*)));
	connect(VoxInfoDlg, SIGNAL(GetVoxInfoString(int, QString*)), &MainObj, SLOT(GetVoxInfoString(int, QString*)));
	connect(&MainObj, SIGNAL(ModelChanged()), VoxInfoDlg, SLOT(UpdateUI()));


	VoxInfoDlg->UpdateUI();
	addDockWidget(Qt::RightDockWidgetArea, VoxInfoDockWidget);

}

void VoxCad::SetupBCWindow(void)
{
	BCDockWidget = new QDockWidget(this);
	BCDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	BCsDlg = new Dlg_BCs(&MainEnv, BCDockWidget);
	BCDockWidget->setWidget(BCsDlg);
    BCDockWidget->setWindowTitle("BCs");
	BCDockWidget->setVisible(false);

	connect(BCDockWidget->toggleViewAction(), SIGNAL(triggered(bool)), this, SLOT(BCsMode(bool)));
	connect(BCsDlg, SIGNAL(RequestUpdateGL()), this, SLOT(ReqGLUpdateAll()));
	connect(BCsDlg, SIGNAL(RequestGLSelect(int)), this, SLOT(SetGLSelected(int)));
	connect(BCsDlg, SIGNAL(DoneEditing()), this, SLOT(ViewMode(void))); 
	
	connect(&MainEnv, SIGNAL(BCsChanged()), BCsDlg, SLOT(UpdateBCLists(void))); //when we load BC's update the dlg!
	connect(&MainSim, SIGNAL(BCsChanged()), BCsDlg, SLOT(UpdateBCLists(void))); //when we load BC's update the dlg!

	addDockWidget(Qt::RightDockWidgetArea, BCDockWidget);

}

void VoxCad::SetupFEAInfoWindow(void)
{
	FEAInfoDockWidget = new QDockWidget(this);
	FEAInfoDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	FEAInfoDlg = new Dlg_FEAInfo(&MainFEA, FEAInfoDockWidget);
	FEAInfoDockWidget->setWidget(FEAInfoDlg);
    FEAInfoDockWidget->setWindowTitle("FEA Info");
	FEAInfoDockWidget->setVisible(false);

	connect(FEAInfoDockWidget->toggleViewAction(), SIGNAL(triggered(bool)), this, SLOT(RequestFEAMode(bool)));
	connect(FEAInfoDlg, SIGNAL(RequestUpdateGL()), this, SLOT(ReqGLUpdateAll()));
	connect(FEAInfoDlg, SIGNAL(GetCurIndex(int*)), this, SLOT(GetCurGLSelected(int*)));
	connect(FEAInfoDlg, SIGNAL(GetFEAInfoString(QString*)), &MainFEA, SLOT(GetFEAInfoString(QString*)));
	connect(FEAInfoDlg, SIGNAL(GetFEAInfoString(int, QString*)), &MainFEA, SLOT(GetFEAInfoString(int, QString*)));
	connect(FEAInfoDlg, SIGNAL(DoneAnalyzing()), this, SLOT(ForceViewMode(void))); 

	addDockWidget(Qt::RightDockWidgetArea, FEAInfoDockWidget);

}

void VoxCad::SetupPhysicsWindow(void)
{
	PhysicsDockWidget = new QDockWidget(this);
	PhysicsDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	PhysicsDlg = new Dlg_Physics(&MainSim, PhysicsDockWidget);
	PhysicsDockWidget->setWidget(PhysicsDlg);
    PhysicsDockWidget->setWindowTitle("Physics Settings");
	PhysicsDockWidget->setVisible(false);


	connect(PhysicsDockWidget->toggleViewAction(), SIGNAL(triggered(bool)), this, SLOT(PhysicsMode(bool)));
	connect(&MainSim, SIGNAL(UpdateText(QString)), PhysicsDlg, SLOT(SetStatusText(QString)));
	connect(&MainSim, SIGNAL(ReqGLUpdate()), this, SLOT(ReqGLUpdateAll()));
	connect(&MainSim, SIGNAL(ReqUiUpdate()), PhysicsDlg, SLOT(UpdateUI()));
	connect(&MainSim, SIGNAL(ReqGLDrawingStatus(bool*)), GLWindow, SLOT(IsDrawing(bool*)));


	connect(&MainSim, SIGNAL(IsStatusTextVisible(bool*)), PhysicsDlg, SLOT(IsOutputVisible(bool*)), Qt::DirectConnection); 
	connect(&MainSim, SIGNAL(IsPlotVisible(bool*)), PhysicsDlg, SLOT(IsPlotVisible(bool*)), Qt::DirectConnection); 
	connect(&MainSim, SIGNAL(GetPlotRqdStats(char*)), this, SLOT(GetPlotRqdDataType(char*)), Qt::DirectConnection); 
	connect(&MainSim, SIGNAL(ReqAddPlotPoint(double)), PhysicsDlg, SLOT(AddPlotPoint(double)), Qt::DirectConnection); 
	connect(&MainSim, SIGNAL(SimEndedInternally(QString)), PhysicsDlg, SLOT(UpdateUI())); 
//	connect(&MainSim, SIGNAL(SimEndedInternally(QString)), this, SLOT(ReqGLUpdateAll())); 


	connect(&MainSim, SIGNAL(StartExternalGLUpdate(int)), GLWindow, SLOT(StartAutoRedraw(int)));
	connect(&MainSim, SIGNAL(StopExternalGLUpdate()), GLWindow, SLOT(StopAutoRedraw()));
	connect(GLWindow, SIGNAL(FindCamTarget(Vec3D<>*)), &MainSim, SLOT(GetCoM(Vec3D<>*)));
	connect(GLWindow, SIGNAL(WantAutoSaveFrames(bool*)), &MainSim, SLOT(WantFramesAutoSaved(bool*)));
	connect(GLWindow, SIGNAL(GetFrameFilePath(QString*)), &MainSim, SLOT(AutoSavePath(QString*)));

	connect(&MainSim, SIGNAL(ResizeGLWindow(int, int)), this, SLOT(ResizeGlWindowArea(int, int)));
	connect(&MainSim, SIGNAL(ResetGLWindow()), this, SLOT(ResetGlWindowArea()));


	//connect(FEAInfoDlg, SIGNAL(RequestUpdateGL()), this, SLOT(ReqGLUpdateAll()));
	//connect(FEAInfoDlg, SIGNAL(GetCurIndex(int*)), this, SLOT(GetCurGLSelected(int*)));
	//connect(FEAInfoDlg, SIGNAL(GetFEAInfoString(QString*)), &MainFEA, SLOT(GetFEAInfoString(QString*)));
	//connect(FEAInfoDlg, SIGNAL(GetFEAInfoString(int, QString*)), &MainFEA, SLOT(GetFEAInfoString(int, QString*)));
	//connect(FEAInfoDlg, SIGNAL(DoneAnalyzing()), this, SLOT(ForceViewMode(void))); 

	addDockWidget(Qt::RightDockWidgetArea, PhysicsDockWidget);
}


void VoxCad::SetupTensileWindow(void)
{
	TensileDockWidget = new QDockWidget(this);
	TensileDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
//	TensileDlg = new Dlg_Tensile(&MainEnv, PhysicsDockWidget);
	TensileDlg = new Dlg_Tensile(&MainSim, PhysicsDockWidget);
	TensileDockWidget->setWidget(TensileDlg);
    TensileDockWidget->setWindowTitle("Tensile Testing");
	TensileDockWidget->setVisible(false);

	connect(TensileDlg, SIGNAL(DoneTensileTesting()), this, SLOT(ViewMode()));
	connect(&TensileDlg->Tester, SIGNAL(StartExternalGLUpdate(int)), GLWindow, SLOT(StartAutoRedraw(int)));
	connect(&TensileDlg->Tester, SIGNAL(StopExternalGLUpdate()), GLWindow, SLOT(StopAutoRedraw()));

	addDockWidget(Qt::RightDockWidgetArea, TensileDockWidget);
}


void VoxCad::EnterVMMode(AppViewMode NewMode, bool Force) //switch between viewing modes!
{
	//check to see if we want to abandom analysis!
	if (CurViewMode == NewMode) return;

	//handle any de-initialization of the old mode
	switch (CurViewMode){
	case VM_3DVIEW:
		break;
	case VM_EDITLAYER:
		ViewRef3DWindow(false);
		SetSectionView(false);

		ui.actionEdit_Voxels->setChecked(false);
		ui.actionPerspective->setEnabled(true);
		ui.actionSection_View->setEnabled(true);

		ui.actionPencil->setEnabled(false);
		ui.actionRectangle->setEnabled(false);
		ui.actionCircle->setEnabled(false);

		break;
	case VM_EDITBCS:
		ViewBCsWindow(false);
		ui.actionBCs->setChecked(false);
		break;
	case VM_FEA:
		//if we don't want to abandon the results, intervene here
		if (!Force && QMessageBox::question(NULL, "Exiting simulation", "Do you want to exit the simulation and discard results?", QMessageBox::Ok | QMessageBox::Cancel)==QMessageBox::Cancel){
			ui.actionSolve->setChecked(true); //reset the button
			ViewFEAInfoWindow(true); 
			return;
		}

		ViewFEAInfoWindow(false);
		ui.actionSolve->setChecked(false); //reset the button
		WorkspaceDlg->setEnabled(true);
		PaletteDlg->setEnabled(true);
		MainFEA.ResetFEA();

		break;
	case VM_PHYSICS:
		//stop the simulation if it hasn't been alraedy...
		MainSim.EndSim();

		ViewPhysicsWindow(false);
		WorkspaceDlg->setEnabled(true);
		ui.actionPhysics->setChecked(false);

		GLWindow->bDrawBounds = true;

		break;

	case VM_TENSILE:
		WorkspaceDlg->setEnabled(true);
		PaletteDlg->setEnabled(true);

		ViewTensileWindow(false);
		TensileDlg->setEnabled(true);
		ui.actionTensile->setChecked(false);

		break;
	case VM_3DBRUSH:
		ViewBrushWindow(false);
		ui.action3D_Brush->setChecked(false);
		break;
	}
	
	//enter the new mode!
	switch (NewMode){
	case VM_3DVIEW:
		CurViewMode = VM_3DVIEW;
		ViewPerspective(false);
		break;
	case VM_EDITLAYER:
		CurViewMode = VM_EDITLAYER;
		ViewTop(false);
		ui.actionEdit_Voxels->setChecked(true);

///		ui.actionPerspective->setEnabled(false);
		ui.actionSection_View->setEnabled(false);
		SetSectionView(true);

		ui.actionPencil->setEnabled(true);
		ui.actionRectangle->setEnabled(true);
		ui.actionCircle->setEnabled(true);

		//open the correct windowa:
		ViewPaletteWindow(true);
		ViewRef3DWindow(true);

		break;
	case VM_EDITBCS:
		CurViewMode = VM_EDITBCS;
		ViewPerspective(false);
		
		ui.actionBCs->setChecked(true);

		ViewBCsWindow(true);

		break;
	case VM_FEA:
		CurViewMode = VM_FEA;
		ViewPerspective(false);
		
		ui.actionSolve->setChecked(true);

		FEAInfoDlg->UpdateUI();
		WorkspaceDlg->setEnabled(false);
		PaletteDlg->setEnabled(false);
		ViewFEAInfoWindow(true);
		FEAInfoDlg->UpdateUI();

		break;
	case VM_PHYSICS:
		CurViewMode = VM_PHYSICS;
		ViewPerspective(false);
		ui.actionPhysics->setChecked(true);

		//update ui of dialog?
		GLWindow->bDrawBounds = false;

		WorkspaceDlg->setEnabled(false);

		PhysicsDlg->DeletePlotPoints();
		PhysicsDlg->UpdateUI();
		PhysicsDlg->SetStatusButtonText(true); //A little hacky, but otherwise we'd need to call UpdateUI after we _know_ the thread has started.
		ViewPhysicsWindow(true);
		MainSim.RequestBeginSim();

		break;
	case VM_TENSILE:
		CurViewMode = VM_TENSILE;
		ViewPerspective(false);
		ui.actionTensile->setChecked(true);

		WorkspaceDlg->setEnabled(false);
		PaletteDlg->setEnabled(false);

//		TensileDlg->UpdateUI();
		ViewTensileWindow(true);

		break;
	case VM_3DBRUSH:
		CurViewMode = VM_3DBRUSH;
		ViewPerspective(false);
		
		ui.action3D_Brush->setChecked(true);
		Brush3DDlg->UpdateUI();
		ViewBrushWindow(true);

		break;
	}

	//Things to do regardless
	SetGLSelected(); //reset selection
	ZoomExtAll();
}

void VoxCad::DrawCurScene(bool FastMode)
{
	if (GraphicsEnabled){
		switch (CurViewMode){
		case VM_3DVIEW:
			MainObj.DrawSceneView(FastMode);
			break;
		case VM_EDITLAYER:
			MainObj.DrawSceneEdit(FastMode);
			break;
		case VM_EDITBCS:
			MainEnv.DrawBCs(CurGLSelected);
			break;
		case VM_FEA:
			MainFEA.DrawScene();
			break;
		case VM_PHYSICS:
			MainSim.pSimView->Draw(CurGLSelected, MainObj.ViewSection, MainObj.CurSecLayer);

			if (MainObj.ViewSection){
				MainObj.CurSecAxis = AXIS_Z; //put these somewhere better!
				MainObj.CurSecFromNeg = false;
				MainObj.DrawSectionPlane();
			}
			break;
		case VM_TENSILE:
			if (TensileDlg->Tester.TestRunning){TensileDlg->Tester.pTensileView->Draw();}
			else MainObj.DrawSceneView(FastMode);
			break;
		case VM_3DBRUSH:
			MainObj.DrawSceneView();
			Brush3DDlg->DrawBrush();
		}
	}
}

void VoxCad::DrawCurSceneOverlay(void)
{
	switch (CurViewMode){
	case VM_FEA:
//		MainFEA.DrawScene();
		break;
	case VM_PHYSICS:
		MainSim.DrawOverlay();
		break;

	}
}


void VoxCad::SetSectionView(bool ViewSec)
{
	if (ViewSec){
		MainObj.SetSectionView();
		ui.actionSection_View->setChecked(true);
		ui.actionLayer_Back->setEnabled(true);
		ui.actionLayer_Forward->setEnabled(true);
	}
	else {
		MainObj.SetSectionView(false);
		ui.actionSection_View->setChecked(false);
		ui.actionLayer_Back->setEnabled(false);
		ui.actionLayer_Forward->setEnabled(false);
	}
	ReqGLUpdateAll();
}

void VoxCad::Copy(void)
{
	if (CurViewMode == VM_EDITLAYER){
		CVXC_Structure Layer;
		MainObj.ExtractCurLayer(&Layer);

		CXML_Rip LocXML;
		Layer.WriteXML(&LocXML);
		std::string Text;
		LocXML.toXMLText(&Text);

		QClipboard *clipboard = QApplication::clipboard();
		clipboard->setText(Text.c_str());
	}
}

void VoxCad::Cut(void)
{
	if (CurViewMode == VM_EDITLAYER){
		Copy();
		CVXC_Structure BlankLayer = CVXC_Structure(1000, 1000, 1);
		MainObj.ImposeLayerCur(&BlankLayer);
		ReqGLUpdateAll(); 
	}
}

void VoxCad::Paste(void)
{
	if (CurViewMode == VM_EDITLAYER){
		CVXC_Structure Layer;
		CXML_Rip LocXML;

		QClipboard *clipboard = QApplication::clipboard();
		std::string Text = clipboard->text().toStdString();
		LocXML.fromXMLText(&Text);
		Layer.ReadXML(&LocXML);

		MainObj.ImposeLayerCur(&Layer);
		ReqGLUpdateAll(); 
	}
}

//void VoxCad::TensileTest(void)
//{
//	Dlg_Tensile TensDlg(&MainEnv, this);
//	TensDlg.exec();
//}

void VoxCad::GetPlotRqdDataType(char* pRqdDataOut)
{
	switch (PhysicsDlg->CurPlotType){
	case Dlg_Physics::PL_DISP: *pRqdDataOut = CALCSTAT_DISP; break;
	case Dlg_Physics::PL_KINETIC: *pRqdDataOut = CALCSTAT_KINE; break; 
	case Dlg_Physics::PL_STRAINE: *pRqdDataOut = CALCSTAT_STRAINE; break; 
	case Dlg_Physics::PL_TOTALENERGY: *pRqdDataOut = CALCSTAT_STRAINE | CALCSTAT_KINE; break; 
	case Dlg_Physics::PL_MAXDISP: *pRqdDataOut = CALCSTAT_VEL; break; 

		
	}
}
