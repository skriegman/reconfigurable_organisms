/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * The name if its contributors may not be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#include "Dlg_3DBrush.h"
#include "../Voxelyze/VX_FRegion.h" 
#include "../Voxelyze/VX_Object.h"
#include "../Voxelyze/VX_MeshUtil.h"

Dlg_3DBrush::Dlg_3DBrush(CVX_Object* pObjIn, CMesh* pMeshIn, QWidget *parent)
	: QWidget(parent)
{
	pObj = pObjIn;
	pSurfMesh = NULL;
	if (pMeshIn) pSurfMesh = pMeshIn;
	BrushShape.CreateBoxRegion(Vec3D<>(0,0,0), Vec3D<>(0.1, 0.1, 0.1));

	//set up the dialog...
	BrushEditPrimDlg = new Dlg_EditPrim(&BrushShape, pObjIn, this);

	QVBoxLayout *verticalLayout_1 = new QVBoxLayout(this);
	QHBoxLayout *horizontalLayout_1 = new QHBoxLayout();
	QHBoxLayout *horizontalLayout_2 = new QHBoxLayout();

	verticalLayout_1->setObjectName(QString::fromUtf8("verticalLayout_1"));
	
	QLabel* MatLabel;
	MatLabel = new QLabel(BrushEditPrimDlg);
	MatLabel->setText("Material:");


	MatToRip = new QComboBox(BrushEditPrimDlg);
	ApplyButton = new QPushButton(BrushEditPrimDlg);
	ApplyButton->setText("Apply");
	connect(ApplyButton, SIGNAL(clicked()), this, SLOT(ApplyBrush()));

	DoneButton = new QPushButton(BrushEditPrimDlg);
	DoneButton->setText("Done");
	connect(DoneButton, SIGNAL(clicked()), this, SLOT(ClickedDone()));

	horizontalLayout_2->addWidget(MatLabel);
	horizontalLayout_2->addWidget(MatToRip);

	horizontalLayout_1->addWidget(ApplyButton);
	horizontalLayout_1->addWidget(DoneButton);


	verticalLayout_1->addWidget(BrushEditPrimDlg);
	verticalLayout_1->addLayout(horizontalLayout_2);
	verticalLayout_1->addLayout(horizontalLayout_1);

	//QSpacerItem *verticalSpacer;
	//verticalSpacer = new QSpacerItem(20, 136, QSizePolicy::Minimum, QSizePolicy::Expanding);
	//verticalLayout_1->addSpacerItem(verticalSpacer);

	connect(BrushEditPrimDlg, SIGNAL(RequestUpdateGL()), this, SIGNAL(RequestUpdateGL()));

}

Dlg_3DBrush::~Dlg_3DBrush()
{

}

void Dlg_3DBrush::ApplyBrush(void)
{
	Vec3D<> Loc;
	Vec3D<> WS = pObj->GetWorkSpace();
	int MatIndex = MatToRip->currentIndex();
	for (int i=0; i<pObj->GetStArraySize(); i++){
		Loc = pObj->GetXYZ(i); //.ScaleInv(WS); //[0->1]
		if(BrushShape.GetRegion()->IsIn(&Loc, &WS)) pObj->SetMat(i, MatIndex);
	}

	//set most recent brush to current surface mesh... (only for mesh option now, not primitives...
	if (pSurfMesh && BrushShape.IsMesh()){
		*pSurfMesh = BrushShape.GetMesh()->ThisMesh;
		//rotate and scale...

		Vec3D<> BrushPos(BrushShape.GetRegion()->X, BrushShape.GetRegion()->Y, BrushShape.GetRegion()->Z);
		Vec3D<> BrushSize(BrushShape.GetRegion()->dX, BrushShape.GetRegion()->dY, BrushShape.GetRegion()->dZ);
		Vec3D<> MeshSize = BrushShape.GetMesh()->ThisMesh.GetBBSize();

		Vec3D<> ScaleFacV = BrushSize.Scale(WS.ScaleInv(MeshSize)); //= WS/Size
		double MinScaleFac = ScaleFacV.Min();
		Vec3D<> v1 = BrushPos.Scale(WS);

		pSurfMesh->Scale(Vec3D<>(MinScaleFac, MinScaleFac, MinScaleFac));
		pSurfMesh->Translate(v1);

	}

	emit RequestUpdateGL();
}

void Dlg_3DBrush::UpdateUI()
{
	BrushEditPrimDlg->UpdateUI(); 
	MatToRip->clear();
	for (int i=0; i<pObj->GetNumMaterials(); i++){
		MatToRip->addItem(pObj->GetBaseMat(i)->GetName().c_str());
	}

	if (pObj->GetNumMaterials()>1) MatToRip->setCurrentIndex(1);

}

void Dlg_3DBrush::DrawBrush(void)
{
	Vec3D<> WS = pObj->GetWorkSpace();
	BrushShape.DrawScaled(&WS);
}
