/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * The name if its contributors may not be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#include "Dlg_StructureEditor.h"
#include <QString>

Dlg_StructureEditor::Dlg_StructureEditor(CQDM_Edit* pEditIn, QWidget *parent)
	: QWidget(parent)
{
	pEdit = pEditIn;
	ui.setupUi(this);

	QGLFormat format; // double buffering and depth buffering is turned on in the default format, so no need to reset those
	pGLWin = new CQOpenGL(format);
	ui.horizontalLayout->addWidget(pGLWin);
	resize(500, 300);
	SetupRef3DWindow();

////
//	connect(GLWindow, SIGNAL(FindDims(Vec3D*)), &MainObj, SLOT(GetDim(Vec3D*)));
//	connect(GLWindow, SIGNAL(DrawGL()), this, SLOT(DrawCurScene()));
//	connect(GLWindow, SIGNAL(MousePressIndex(int)), this, SLOT(SetGLSelected(int)));
//
//	//mouse handling
//	connect(GLWindow, SIGNAL(WantGLIndex(bool*)), this, SLOT(WantGLIndex(bool*)));
//	connect(GLWindow, SIGNAL(WantCoord3D(bool*)), this, SLOT(WantCoord3D(bool*)));
//
//	connect(GLWindow, SIGNAL(MouseMoveHover(float, float, float)), this, SLOT(HoverMove(float, float, float)));
//	connect(GLWindow, SIGNAL(LMouseMovePressed(float, float, float)), this, SLOT(LMouseDownMove(float, float, float)));
//	connect(GLWindow, SIGNAL(LMouseDown(float, float, float)), this, SLOT(LMouseDown(float, float, float)));
//	connect(GLWindow, SIGNAL(LMouseUp(float, float, float)), this, SLOT(LMouseUp(float, float, float)));
//	connect(GLWindow, SIGNAL(PressedEscape()), this, SLOT(PressedEscape()));
//	connect(GLWindow, SIGNAL(CtrlWheelRoll(bool)), this, SLOT(CtrlMouseRoll(bool)));
////

	connect(pGLWin, SIGNAL(FindDims(Vec3D<>*, Vec3D<>*)), pEdit, SLOT(GetDim(Vec3D<>*, Vec3D<>*)));
	connect(pGLWin, SIGNAL(DrawGL(bool)), pEdit, SLOT(DrawSceneEdit(bool)));

	connect(pGLWin, SIGNAL(WantGLIndex(bool*)), this, SLOT(WantGLIndex(bool*)));
	connect(pGLWin, SIGNAL(WantCoord3D(bool*)), this, SLOT(WantCoord3D(bool*)));
//	connect(pGLWin, SIGNAL(IsExtScene2D(bool*)), this, SLOT(IsEditMode(bool*)));

	connect(pGLWin, SIGNAL(MouseMoveHover(float, float, float)), this, SLOT(HoverMove(float, float, float)));
	connect(pGLWin, SIGNAL(LMouseMovePressed(float, float, float)), this, SLOT(LMouseDownMove(float, float, float)));
	connect(pGLWin, SIGNAL(LMouseDown(float, float, float, bool)), this, SLOT(LMouseDown(float, float, float, bool)));
	connect(pGLWin, SIGNAL(LMouseUp(float, float, float)), this, SLOT(LMouseUp(float, float, float)));
	connect(pGLWin, SIGNAL(PressedEscape()), this, SLOT(PressedEscape()));
	connect(pGLWin, SIGNAL(CtrlWheelRoll(bool)), this, SLOT(CtrlMouseRoll(bool)));

	connect(pEdit, SIGNAL(UpdateGLWindows()), this, SLOT(UpdateGLWins()));
	connect(pEdit, SIGNAL(GetCurMaterial(int*)), this, SLOT(CurMaterial(int*)));

	//connect this! void GetCurMaterial(int* MatIndex);
	
	//UI handlers:
	connect(ui.pencilButton, SIGNAL(clicked()), this, SLOT(ClickedPencil()));
	connect(ui.squareButton, SIGNAL(clicked()), this, SLOT(ClickedBox()));
	connect(ui.ellipseButton, SIGNAL(clicked()), this, SLOT(ClickedEllipse()));
	connect(ui.layerbackButton, SIGNAL(clicked()), this, SLOT(ClickedLayerBack()));
	connect(ui.layerforwardButton, SIGNAL(clicked()), this, SLOT(ClickedLayerForward()));
	connect(ui.refviewButton, SIGNAL(clicked(bool)), this, SLOT(ClickedRefView(bool)));

	IniUpdateUI();


}

Dlg_StructureEditor::~Dlg_StructureEditor()
{
}

void Dlg_StructureEditor::IniUpdateUI()
{
	pGLWin->SetViewTop();
	pGLWin->GLCenterView(); //necessary because view is already top...
	pEdit->SetV2DTop();
	pEdit->SetSectionView();
	
	//initial ui update (things that won't change...)
	ui.MatList->clear();
	for (int i=0; i<pEdit->GetNumMaterials(); i++){
		QString Name = QString(pEdit->Palette[i].GetName().c_str());
		ui.MatList->addItem(Name);
		QPixmap tmpMap = QPixmap(16, 16);
		tmpMap.fill(QColor(pEdit->Palette[i].GetRedi(), pEdit->Palette[i].GetGreeni(), pEdit->Palette[i].GetBluei()));
		ui.MatList->item(i)->setIcon(QIcon(tmpMap));
		if (Name.endsWith("(Editing)", Qt::CaseInsensitive)) ui.MatList->item(i)->setFlags(NULL);
		if (Name.endsWith("(Recursive)", Qt::CaseInsensitive)) ui.MatList->item(i)->setFlags(NULL);
	}
	ui.MatList->setCurrentRow(0); //set to erase material
	UpdateUI();
}

void Dlg_StructureEditor::ClickedPencil(void)
{
	pEdit->SetDrawPencil();
	UpdateUI();

}

void Dlg_StructureEditor::ClickedBox(void)
{
	pEdit->SetDrawRectangle();
	UpdateUI();
}

void Dlg_StructureEditor::ClickedEllipse(void)
{
	pEdit->SetDrawCircle();
	UpdateUI();
}

void Dlg_StructureEditor::ClickedLayerBack(void)
{
	pEdit->LayerBack();
	UpdateUI();

}

void Dlg_StructureEditor::ClickedLayerForward(void)
{
	pEdit->LayerForward();
	UpdateUI();

}

void Dlg_StructureEditor::ClickedRefView(bool State)
{
	if (RefWin->isVisible()) RefWin->hide();
	else {
		GLRef3DWin->GLCenterView();
		RefWin->show();
	}
}

void Dlg_StructureEditor::UpdateUI()
{
	ui.pencilButton->setChecked(false);
	ui.ellipseButton->setChecked(false);
	ui.squareButton->setChecked(false);

	if (pEdit->GetCurDrawTool() == DT_PEN) ui.pencilButton->setChecked(true);
	if (pEdit->GetCurDrawTool() == DT_BOX) ui.squareButton->setChecked(true);
	if (pEdit->GetCurDrawTool() == DT_ELLIPSE) ui.ellipseButton->setChecked(true);

	UpdateGLWins();
}

void Dlg_StructureEditor::SetupRef3DWindow(void)
{
	RefWin = new QDialog(this);
	QHBoxLayout* HorLayout = new QHBoxLayout(RefWin);

	QGLFormat format; // double buffering and depth buffering is turned on in the default format, so no need to reset those
	GLRef3DWin = new CQOpenGL(format);
	HorLayout->addWidget(GLRef3DWin);
	GLRef3DWin->SetViewCustom1();
	GLRef3DWin->setWindowTitle("3D View");

	connect(GLRef3DWin, SIGNAL(FindDims(Vec3D<>*, Vec3D<>*)), pEdit, SLOT(GetDim(Vec3D<>*, Vec3D<>*)));
	connect(GLRef3DWin, SIGNAL(DrawGL(bool)), pEdit, SLOT(DrawSceneView(bool)));

	QRect GLFrame = pGLWin->geometry();
	RefWin->setGeometry(GLFrame.x()+400, GLFrame.y()+50, 300, 200);

	//GLRef3DWin->
//	GLRef3DWin->hide();
//	GLRef3DWin->setFloating(true);
//	GLRef3DWin->setGeometry (Ref3DDockWidget->parentWidget()->geometry().x()+10,Ref3DDockWidget->parentWidget()->geometry().y()+80,340,280 );

}