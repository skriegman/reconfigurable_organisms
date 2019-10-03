/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * The name if its contributors may not be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#ifndef DLG_WORKSPACE_H
#define DLG_WORKSPACE_H

#include "ui_vWorkSpace.h"
#include "../Voxelyze/VX_Object.h"


class Dlg_Workspace : public QWidget
{
	Q_OBJECT

public:
	Dlg_Workspace(CVX_Object* pObjIn, QWidget *parent = 0);
	~Dlg_Workspace();

	enum WorkspaceType {WSP_CUBES, WSP_FCCSPHERES, WSP_HCPSPHERES, WSP_DMU, WSP_CUSTOM};

	CVX_Object* pObj; //pointer to the digital object this palette editor is looking at...

	virtual QSize sizeHint () const {return QSize(100, 500);};

signals:
	void RequestUpdateGL(void);
//	void RequestZoomExtents(void);
	void WSDimChanged(void);

public slots:
	void UpdateUI(void);
	void IniUpdateUI(void); //need this to select a predefined scheme if the lattice matches it...

	void ChangedScheme(int NewWSScheme);
	void ChangedLatDim(double NewLatDim) {pObj->Lattice.SetLatticeDim(NewLatDim/1000.0); emit WSDimChanged();}
//	void ChangedVXDim(int NewVXDim) {pObj->Resize(NewVXDim, pObj->GetVYDim(), pObj->GetVZDim()); emit WSDimChanged();}
//	void ChangedVYDim(int NewVYDim) {pObj->Resize(pObj->GetVXDim(), NewVYDim, pObj->GetVZDim()); emit WSDimChanged();}
//	void ChangedVZDim(int NewVZDim) {pObj->Resize(pObj->GetVXDim(), pObj->GetVYDim(), NewVZDim); emit WSDimChanged();}
	void ChangedVXDim(void) {pObj->Resize(ui.XV_Spin->value(), pObj->GetVYDim(), pObj->GetVZDim()); emit WSDimChanged();}
	void ChangedVYDim(void) {pObj->Resize(pObj->GetVXDim(), ui.YV_Spin->value(), pObj->GetVZDim()); emit WSDimChanged();}
	void ChangedVZDim(void) {pObj->Resize(pObj->GetVXDim(), pObj->GetVYDim(), ui.ZV_Spin->value()); emit WSDimChanged();}

	//custom settings: Lattice
	void ChangedXDimAdj(double NewXDimAdj) {pObj->Lattice.SetXDimAdj(NewXDimAdj); emit RequestUpdateGL();}
	void ChangedYDimAdj(double NewYDimAdj) {pObj->Lattice.SetYDimAdj(NewYDimAdj); emit RequestUpdateGL();}
	void ChangedZDimAdj(double NewZDimAdj) {pObj->Lattice.SetZDimAdj(NewZDimAdj); emit RequestUpdateGL();}
	void ChangedXLiO(double NewXLiO) {pObj->Lattice.SetXLiO(NewXLiO); emit RequestUpdateGL();}
	void ChangedYLiO(double NewYLiO) {pObj->Lattice.SetYLiO(NewYLiO); emit RequestUpdateGL();}
	void ChangedXLaO(double NewXLaO) {pObj->Lattice.SetXLaO(NewXLaO); emit RequestUpdateGL();}
	void ChangedYLaO(double NewYLaO) {pObj->Lattice.SetYLaO(NewYLaO); emit RequestUpdateGL();}

	//custom settings: Voxel Display
	void ChangedVoxShape(int NewVoxShape) {pObj->Voxel.SetVoxName(NewVoxShape); emit RequestUpdateGL();}
	void ChangedXSqueeze(double NewXSqueeze) {pObj->Voxel.SetXSqueeze(NewXSqueeze); emit RequestUpdateGL();}
	void ChangedYSqueeze(double NewYSqueeze) {pObj->Voxel.SetYSqueeze(NewYSqueeze); emit RequestUpdateGL();}
	void ChangedZSqueeze(double NewZSqueeze) {pObj->Voxel.SetZSqueeze(NewZSqueeze); emit RequestUpdateGL();}



private:
	Ui::WorkspaceDialog ui;
};

#endif // DLG_WORKSPACE_H
