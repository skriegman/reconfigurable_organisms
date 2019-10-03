/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * The name if its contributors may not be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#ifndef DLG_3DBRUSH_H
#define DLG_3DBRUSH_H

#include <QWidget>
#include "Dlg_EditPrim.h"
#include "../Voxelyze/VX_FRegion.h"
#include <QComboBox>

class CVX_FRegion;
class CVX_Object;

class Dlg_3DBrush : public QWidget
{
	Q_OBJECT

public:
	Dlg_3DBrush(CVX_Object* pObjIn, CMesh* pMeshIn = 0, QWidget *parent = 0);
	~Dlg_3DBrush();

signals:
	void RequestUpdateGL(void);
	void DoneAdding(void); //exit brush mode...
	

public slots:
	void ApplyBrush(void);
	void UpdateUI(void);
	void DrawBrush(void);
	void ClickedDone(void) {emit DoneAdding();}

private:
	CVX_Object* pObj;
	CMesh* pSurfMesh; //! Add to surface mesh for later simulation...
	CVX_FRegion BrushShape;


	QWidget BrushDlg;
	Dlg_EditPrim* BrushEditPrimDlg;	
	QComboBox *MatToRip;
	QPushButton *ApplyButton;
	QPushButton *DoneButton;



};

#endif // DLG_3DBRUSH_H
