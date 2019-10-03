/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * The name if its contributors may not be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#ifndef DLG_STRUCTUREEDITOR_H
#define DLG_STRUCTUREEDITOR_H

#include <ui_vStructureEditor.h>
#include "QVX_Edit.h"
#include "../QTUtils/QOpenGL.h"
#include <QCloseEvent> //why this isn't defined in QWidget I have no idea...

class Dlg_StructureEditor : public QWidget
{
	Q_OBJECT

public:
	Dlg_StructureEditor(CQDM_Edit* pEditIn, QWidget *parent);
	~Dlg_StructureEditor();

	CQDM_Edit* pEdit;
	CQOpenGL* pGLWin;

	void SetupRef3DWindow(void);
	QDialog* RefWin; //so we can make it a child...
	CQOpenGL* GLRef3DWin;

signals:
	void DoneEditing(void); //emitted to commit changes to the source

public slots:
	void IsEditMode(bool* YN) {*YN=true;}; //here, we are always in edit mode...

	void ClickedPencil(void);
	void ClickedBox(void);
	void ClickedEllipse(void);
	void ClickedLayerBack(void);
	void ClickedLayerForward(void);
	void ClickedRefView(bool State);

	//it is unfortunate to duplicate these here (from voxcad.h), but we need them to keep things hierachical
	void HoverMove(float X, float Y, float Z) {pEdit->HoverMove(Vec3D<>(X, Y, Z));};
	void LMouseDown(float X, float Y, float Z, bool IsCtrl) {pEdit->LMouseDown(Vec3D<>(X, Y, Z), IsCtrl);};
	void LMouseUp(float X, float Y, float Z) {pEdit->LMouseUp(Vec3D<>(X, Y, Z));};
	void LMouseDownMove(float X, float Y, float Z) {pEdit->LMouseDownMove(Vec3D<>(X, Y, Z));};
	void PressedEscape(void) {pEdit->PressedEscape();};
	void CtrlMouseRoll(bool Positive) {pEdit->CtrlMouseRoll(Positive);};


	void IniUpdateUI();
	void UpdateUI();
	void UpdateGLWins(void) {pGLWin->updateGL(); GLRef3DWin->updateGL();};
	void CurMaterial(int* pMat) {*pMat = ui.MatList->currentRow();};

	void WantGLIndex(bool* YN) {*YN=true;}
	void WantCoord3D(bool* YN) {*YN=true;}


protected:
	void closeEvent(QCloseEvent *event) {RefWin->close(); emit DoneEditing(); event->accept();};

private:
	Ui::StructEdDlg ui;
};

#endif // DLG_STRUCTUREEDITOR_H
