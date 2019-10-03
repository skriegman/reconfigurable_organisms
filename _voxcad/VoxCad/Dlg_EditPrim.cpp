/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * The name if its contributors may not be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#include "Dlg_EditPrim.h"
#include "../Voxelyze/VX_FRegion.h" 
#include "../Voxelyze/VX_Object.h"
#include <QFileDialog>

Dlg_EditPrim::Dlg_EditPrim(CVX_FRegion* pRegionIn, CVX_Object* pObjIn, QWidget *parent)
	: QWidget(parent)
{
	pVXRegion = pRegionIn;
	pObj = pObjIn;

	Snap = true;
	LockAspect = true;
	AutoUpdating = false;

	ui.setupUi(this);

	ui.SnapCheckBox->setChecked(Snap);
	ui.LockCheckBox->setChecked(Snap);

	connect(ui.SnapCheckBox, SIGNAL(stateChanged(int)), this, SLOT(ChangedSnap(int)));
	connect(ui.LockCheckBox, SIGNAL(stateChanged(int)), this, SLOT(ChangedLock(int)));


	connect(ui.BoxRadio, SIGNAL(clicked()), this, SLOT(ClickedBoxRadio()));
	connect(ui.SphereRadio, SIGNAL(clicked()), this, SLOT(ClickedSphereRadio()));
	connect(ui.CylinderRadio, SIGNAL(clicked()), this, SLOT(ClickedCylinderRadio()));
	connect(ui.MeshRadio, SIGNAL(clicked()), this, SLOT(ClickedMeshRadio()));
	connect(ui.LoadMeshButton, SIGNAL(clicked()), this, SLOT(ClickedLoadMesh()));
	
	
	connect(ui.XSlider, SIGNAL(valueChanged(int)), this, SLOT(ChangedX(int)));
	connect(ui.XSpin, SIGNAL(valueChanged(double)), this, SLOT(ChangedX(double)));
	connect(ui.YSlider, SIGNAL(valueChanged(int)), this, SLOT(ChangedY(int)));
	connect(ui.YSpin, SIGNAL(valueChanged(double)), this, SLOT(ChangedY(double)));
	connect(ui.ZSlider, SIGNAL(valueChanged(int)), this, SLOT(ChangedZ(int)));
	connect(ui.ZSpin, SIGNAL(valueChanged(double)), this, SLOT(ChangedZ(double)));

	connect(ui.DXSlider, SIGNAL(valueChanged(int)), this, SLOT(ChangedDX(int)));
	connect(ui.DXSpin, SIGNAL(valueChanged(double)), this, SLOT(ChangedDX(double)));
	connect(ui.DYSlider, SIGNAL(valueChanged(int)), this, SLOT(ChangedDY(int)));
	connect(ui.DYSpin, SIGNAL(valueChanged(double)), this, SLOT(ChangedDY(double)));
	connect(ui.DZSlider, SIGNAL(valueChanged(int)), this, SLOT(ChangedDZ(int)));
	connect(ui.DZSpin, SIGNAL(valueChanged(double)), this, SLOT(ChangedDZ(double)));
	connect(ui.RSlider, SIGNAL(valueChanged(int)), this, SLOT(ChangedRad(int)));
	connect(ui.RSpin, SIGNAL(valueChanged(double)), this, SLOT(ChangedRad(double)));


	connect(ui.RotXButton, SIGNAL(clicked()), this, SLOT(ClickedRotX()));
	connect(ui.RotYButton, SIGNAL(clicked()), this, SLOT(ClickedRotY()));
	connect(ui.RotZButton, SIGNAL(clicked()), this, SLOT(ClickedRotZ()));

}

Dlg_EditPrim::~Dlg_EditPrim()
{

}

void Dlg_EditPrim::UpdateUI(void)
{

	ui.SnapCheckBox->setChecked(Snap);
	ui.LockCheckBox->setChecked(LockAspect);

	//zero out everything
	ui.XSpin->setEnabled(false);	ui.XSlider->setEnabled(false);
	ui.YSpin->setEnabled(false);	ui.YSlider->setEnabled(false);
	ui.ZSpin->setEnabled(false);	ui.ZSlider->setEnabled(false);
	ui.DXSpin->setEnabled(false);	ui.DXSlider->setEnabled(false);
	ui.DYSpin->setEnabled(false);	ui.DYSlider->setEnabled(false);
	ui.DZSpin->setEnabled(false);	ui.DZSlider->setEnabled(false);
	ui.RSpin->setEnabled(false);	ui.RSlider->setEnabled(false);

	ui.BoxRadio->setEnabled(false);
	ui.CylinderRadio->setEnabled(false);
	ui.SphereRadio->setEnabled(false);
	ui.MeshRadio->setEnabled(false);

	ui.LoadMeshButton->setEnabled(false);

	ui.DegreeSpinBox->setEnabled(false);
	ui.RotXButton->setEnabled(false);
	ui.RotYButton->setEnabled(false);
	ui.RotZButton->setEnabled(false);


	if (pVXRegion){ //if something selected, then update
		CPrimitive* pPrim = pVXRegion->GetRegion();

		ui.BoxRadio->setEnabled(true);
		ui.CylinderRadio->setEnabled(true);
		ui.SphereRadio->setEnabled(true);
		ui.MeshRadio->setEnabled(true);

		if(pVXRegion->IsBox()) ui.BoxRadio->setChecked(true);
		else if(pVXRegion->IsCylinder()) ui.CylinderRadio->setChecked(true);
		else if(pVXRegion->IsSphere()) ui.SphereRadio->setChecked(true);
		else if(pVXRegion->IsMesh()){
			ui.MeshRadio->setChecked(true);
			ui.LoadMeshButton->setEnabled(true);
		}

		ui.XSpin->setValue(pPrim->X);
		ui.XSpin->setEnabled(true);
		ui.XSlider->setEnabled(true);
		ui.YSpin->setValue(pPrim->Y);
		ui.YSpin->setEnabled(true);
		ui.YSlider->setEnabled(true);
		ui.ZSpin->setValue(pPrim->Z);
		ui.ZSpin->setEnabled(true);
		ui.ZSlider->setEnabled(true);

		if (!pVXRegion->IsSphere()){
			ui.DXSpin->setValue(pPrim->dX);
			ui.DXSpin->setEnabled(true);
			ui.DXSlider->setEnabled(true);
			ui.DYSpin->setValue(pPrim->dY);
			ui.DYSpin->setEnabled(true);
			ui.DYSlider->setEnabled(true);
			ui.DZSpin->setValue(pPrim->dZ);
			ui.DZSpin->setEnabled(true);
			ui.DZSlider->setEnabled(true);
		}
		else {
			ui.DXSpin->setValue(0);
			ui.DYSpin->setValue(0);
			ui.DZSpin->setValue(0);
		}

		if (pVXRegion->IsCylinder() || pVXRegion->IsSphere()){
			ui.RSpin->setValue(pPrim->Radius);
			ui.RSpin->setEnabled(true);
			ui.RSlider->setEnabled(true);
		}
		else ui.RSpin->setValue(0);

		if (pVXRegion->IsMesh()){
			ui.DegreeSpinBox->setEnabled(true);
			ui.RotXButton->setEnabled(true);
			ui.RotYButton->setEnabled(true);
			ui.RotZButton->setEnabled(true);
		}
	}
}


void Dlg_EditPrim::ChangedX(double NewVal)
{
	ui.XSlider->setValue(NewVal*100);
	CPrimitive* pPrim = pVXRegion->GetRegion();

	if (!AutoUpdating){
		AutoUpdating = true;
		if (pVXRegion){
			double NewX = NewVal;

			if (pVXRegion->IsBox() && NewX + pPrim->dX > 1.0) NewX = 1.0 - pPrim->dX;//if this BC would be outside the workspace, keep it within
			if (Snap) NewX = GetSnapped(NewX, 1.0/pObj->GetVXDim());

			//Update variables/sliders
			pPrim->X = NewX; ChangedX(NewX);

		}
		emit RequestUpdateGL();
		AutoUpdating = false;
	}
}

void Dlg_EditPrim::ChangedY(double NewVal)
{
	ui.YSlider->setValue(NewVal*100);
	CPrimitive* pPrim = pVXRegion->GetRegion();

	if (!AutoUpdating){
		AutoUpdating = true;
		if (pVXRegion){
			double NewY = NewVal;

			if (pVXRegion->IsBox() && NewY + pPrim->dY > 1.0) NewY = 1.0 - pPrim->dY;//if this BC would be outside the workspace, keep it within
			if (Snap) NewY = GetSnapped(NewY, 1.0/pObj->GetVYDim());

			//Update variables/sliders
			pPrim->Y = NewY; ChangedY(NewY);
		}
		emit RequestUpdateGL();
		AutoUpdating = false;
	}
}

void Dlg_EditPrim::ChangedZ(double NewVal)
{
	ui.ZSlider->setValue(NewVal*100);
	CPrimitive* pPrim = pVXRegion->GetRegion();

	if (!AutoUpdating){
		AutoUpdating = true;
		if (pVXRegion){
			double NewZ = NewVal;

			if (pVXRegion->IsBox() && NewZ + pPrim->dZ > 1.0) NewZ = 1.0 - pPrim->dZ;//if this BC would be outside the workspace, keep it within
			if (Snap) NewZ = GetSnapped(NewZ, 1.0/pObj->GetVZDim());

			//Update variables/sliders
			pPrim->Z = NewZ; ChangedZ(NewZ);
		}
		emit RequestUpdateGL();
		AutoUpdating = false;
	}
}

void Dlg_EditPrim::ChangedDX(double NewVal)
{
	ui.DXSlider->setValue(NewVal*100); //keep the slider buddy-buddy
	CPrimitive* pPrim = pVXRegion->GetRegion();

	if (!AutoUpdating){
		AutoUpdating = true;
		if (pVXRegion){ //if we've got a current region
			double NewDX = NewVal;
			double NewDY = pPrim->dY;
			double NewDZ = pPrim->dZ;
			double NewX = pPrim->X;
			double NewY = pPrim->Y;
			double NewZ = pPrim->Z;
			double NewRad = pPrim->Radius;

			if (Snap) NewDX = GetSnapped(NewDX, 1.0/pObj->GetVXDim(), false); //if snapping to voxel is enabled
			
			if (pVXRegion->IsCylinder()){
				if(NewDX != 0) {
					NewDY=0; //keep cylinder on a principle axis!
					NewDZ=0;
					if (LockAspect) NewRad = NewDX*pPrim->_NomAspect.y/2/pPrim->_NomAspect.x;
				} 
			} 
			else if (pVXRegion->IsBox()){ // || pVXRegion->IsMesh()){
				if (LockAspect){
					NewDY = NewDX*pPrim->_NomAspect.y/pPrim->_NomAspect.x;
					NewDZ = NewDX*pPrim->_NomAspect.z/pPrim->_NomAspect.x;
					if (NewDY>1) {NewDX/=NewDY; NewDZ/=NewDY; NewDY=1.0;}//keep any from going above 1...
					if (NewDZ>1) {NewDX/=NewDZ; NewDY/=NewDZ; NewDZ=1.0;}//keep any from going above 1...
					if (NewDX + NewX > 1.0) NewX=1.0-NewDX;
					if (NewDY + NewY > 1.0) NewY=1.0-NewDY;
					if (NewDZ + NewZ > 1.0) NewZ=1.0-NewDZ;
				}
				else if (NewDX + NewX > 1.0) NewX=1.0-NewDX;  //if increasing the size pushes out the end, push location back
			}

			//Update variables/sliders
			pPrim->X = NewX; ChangedX(NewX);
			pPrim->Y = NewY; ChangedY(NewY);
			pPrim->Z = NewZ; ChangedZ(NewZ);
			pPrim->dX = NewDX; ChangedDX(NewDX);
			pPrim->dY = NewDY; ChangedDY(NewDY);
			pPrim->dZ = NewDZ; ChangedDZ(NewDZ);
			pPrim->Radius = NewRad; ChangedRad(NewRad);


			pPrim->UpdateAspect(); //This only does something if aspect ratio changes...

		}
		emit RequestUpdateGL();
		AutoUpdating = false;
	}

}

void Dlg_EditPrim::ChangedDY(double NewVal)
{
	ui.DYSlider->setValue(NewVal*100); //keep the slider buddy-buddy
	CPrimitive* pPrim = pVXRegion->GetRegion();

	if (!AutoUpdating){
		AutoUpdating = true;
		if (pVXRegion){ //if we've got a current region
			double NewDX = pPrim->dX;
			double NewDY = NewVal;
			double NewDZ = pPrim->dZ;
			double NewX = pPrim->X;
			double NewY = pPrim->Y;
			double NewZ = pPrim->Z;
			double NewRad = pPrim->Radius;

			
			if (Snap) NewDY = GetSnapped(NewDY, 1.0/pObj->GetVYDim(), false); //if snapping to voxel is enabled
			if (pVXRegion->IsCylinder()){
				if (NewDY != 0) {
					NewDX=0; //keep cylinder on a principle axis!
					NewDZ=0;
					if (LockAspect) NewRad = NewDY*pPrim->_NomAspect.y/2/pPrim->_NomAspect.x;
				} 
				
			}
			else if (pVXRegion->IsBox()){ // || pVXRegion->IsMesh()){
				if (LockAspect){
					NewDX = NewDY*pPrim->_NomAspect.x/pPrim->_NomAspect.y;
					NewDZ = NewDY*pPrim->_NomAspect.z/pPrim->_NomAspect.y;
					if (NewDX>1) {NewDY/=NewDX; NewDZ/=NewDX; NewDX=1.0;}//keep any from going above 1...
					if (NewDZ>1) {NewDX/=NewDZ; NewDY/=NewDZ; NewDZ=1.0;}//keep any from going above 1...
					if (NewDX + NewX > 1.0) NewX=1.0-NewDX;
					if (NewDY + NewY > 1.0) NewY=1.0-NewDY;
					if (NewDZ + NewZ > 1.0) NewZ=1.0-NewDZ;
				}
				else if (NewDY + NewY > 1.0) {NewY=1.0-NewDY;} //if increasing the size pushes out the end, push location back
			}

			//Update variables/sliders
			pPrim->X = NewX; ChangedX(NewX);
			pPrim->Y = NewY; ChangedY(NewY);
			pPrim->Z = NewZ; ChangedZ(NewZ);
			pPrim->dX = NewDX; ChangedDX(NewDX);
			pPrim->dY = NewDY; ChangedDY(NewDY);
			pPrim->dZ = NewDZ; ChangedDZ(NewDZ);
			pPrim->Radius = NewRad; ChangedRad(NewRad);

			pPrim->UpdateAspect(); //This only does something if aspect ratio changes...

		}
		emit RequestUpdateGL();
		AutoUpdating = false;
	}
}

void Dlg_EditPrim::ChangedDZ(double NewVal)
{
	ui.DZSlider->setValue(NewVal*100); //keep the slider buddy-buddy
	CPrimitive* pPrim = pVXRegion->GetRegion();

	if (!AutoUpdating){
		AutoUpdating = true;
		if (pVXRegion){ //if we've got a current region
			double NewDX = pPrim->dX;
			double NewDY = pPrim->dY;
			double NewDZ = NewVal;
			double NewX = pPrim->X;
			double NewY = pPrim->Y;
			double NewZ = pPrim->Z;
			double NewRad = pPrim->Radius;
			
			if (Snap) NewDZ = GetSnapped(NewDZ, 1.0/pObj->GetVZDim(), false); //if snapping to voxel is enabled
			if (pVXRegion->IsCylinder()){
				if(NewDZ != 0) {
					NewDX=0; //keep cylinder on a principle axis!
					NewDY=0;
					if (LockAspect) NewRad = NewDZ*pPrim->_NomAspect.y/2/pPrim->_NomAspect.x;
				} 
			}
			else if (pVXRegion->IsBox()){ // || pVXRegion->IsMesh()){
				if (LockAspect){
					NewDX = NewDZ*pPrim->_NomAspect.x/pPrim->_NomAspect.z;
					NewDY = NewDZ*pPrim->_NomAspect.y/pPrim->_NomAspect.z;
					if (NewDX>1) {NewDY/=NewDX; NewDZ/=NewDX; NewDX=1.0;}//keep any from going above 1...
					if (NewDY>1) {NewDX/=NewDY; NewDZ/=NewDY; NewDY=1.0;}//keep any from going above 1...
					if (NewDX + NewX > 1.0) NewX=1.0-NewDX;
					if (NewDY + NewY > 1.0) NewY=1.0-NewDY;
					if (NewDZ + NewZ > 1.0) NewZ=1.0-NewDZ;
				}
				else if (NewDZ + NewZ > 1.0) {NewZ=1.0-NewDZ;} //if increasing the size pushes out the end, push location back
			}

			//Update variables/sliders
			pPrim->X = NewX; ChangedX(NewX);
			pPrim->Y = NewY; ChangedY(NewY);
			pPrim->Z = NewZ; ChangedZ(NewZ);
			pPrim->dX = NewDX; ChangedDX(NewDX);
			pPrim->dY = NewDY; ChangedDY(NewDY);
			pPrim->dZ = NewDZ; ChangedDZ(NewDZ);
			pPrim->Radius = NewRad; ChangedRad(NewRad);

			pPrim->UpdateAspect(); //This only does something if aspect ratio changes...

		}
		emit RequestUpdateGL();
		AutoUpdating = false;
	}
}

void Dlg_EditPrim::ChangedRad(double NewVal) //as percentage of maximum dimension...
{
	CPrimitive* pPrim = pVXRegion->GetRegion();
	//this to get the maximum dimension
	int MaxDim = pObj->GetVXDim();
	if (pObj->GetVYDim()>MaxDim) MaxDim = pObj->GetVYDim();
	if (pObj->GetVZDim()>MaxDim) MaxDim = pObj->GetVZDim();


	ui.RSlider->setValue(NewVal*100); //keep the slider buddy-buddy

	if (!AutoUpdating){
		AutoUpdating = true;
		if (pVXRegion){ //if we've got a current region
			double NewDX = pPrim->dX;
			double NewDY = pPrim->dY;
			double NewDZ = pPrim->dZ;
			double NewRad = NewVal;

			if (Snap) NewRad = GetSnapped(NewRad, 1.0/MaxDim); //if snapping to voxel is enabled
			if (pVXRegion->IsCylinder()){
				if (LockAspect){
					if (NewDX != 0) NewDX = NewRad*pPrim->_NomAspect.x/(pPrim->_NomAspect.y/2.0);
					else if (NewDY != 0) NewDY = NewRad*pPrim->_NomAspect.x/(pPrim->_NomAspect.y/2.0);
					else if (NewDZ != 0) NewDZ = NewRad*pPrim->_NomAspect.x/(pPrim->_NomAspect.y/2.0);
				}
			}

			//Update variables/sliders
			pPrim->dX = NewDX; ChangedDX(NewDX);
			pPrim->dY = NewDY; ChangedDY(NewDY);
			pPrim->dZ = NewDZ; ChangedDZ(NewDZ);
			pPrim->Radius = NewRad; ChangedRad(NewRad);

			pPrim->UpdateAspect(); //This only does something if aspect ratio changes...

		}
		emit RequestUpdateGL();
		AutoUpdating = false;
	}
}


void Dlg_EditPrim::ClickedBoxRadio(void) 
{
	pVXRegion->CreateBoxRegion(Vec3D<>(0,0,0), Vec3D<>(0.1, 0.1, 0.1));
	UpdateUI();
	emit RequestUpdateGL();
}

void Dlg_EditPrim::ClickedSphereRadio(void)
{
	pVXRegion->CreateSphRegion(Vec3D<>(0,0,0), 0.1);
	UpdateUI();

	emit RequestUpdateGL();
}

void Dlg_EditPrim::ClickedCylinderRadio(void)
{
	pVXRegion->CreateCylRegion(Vec3D<>(0,0,0), Vec3D<>(0, 0, 0.1), 0.1);
	UpdateUI();

	emit RequestUpdateGL();
}

void Dlg_EditPrim::ClickedMeshRadio(void)
{
	pVXRegion->CreateMeshRegion();
	UpdateUI();

	emit RequestUpdateGL();
}

void Dlg_EditPrim::ClickedLoadMesh(void)
{
	QString TmpPath = QFileDialog::getOpenFileName(NULL, "Load STL", GetLastDir(), "Stereolithography Files (*.stl)");
	CMesh tmp;
	tmp.LoadSTL(TmpPath.toStdString());
	tmp.CalcFaceNormals();
	tmp.DrawSmooth = false;

	pVXRegion->CreateMeshRegion(&tmp, Vec3D<>(0,0,0), Vec3D<>(1.0, 1.0, 1.0));

	UpdateUI();
	emit RequestUpdateGL();
}

void Dlg_EditPrim::ClickedRotX(void)
{
	double RotAmt = ((double)ui.DegreeSpinBox->value())*2*3.14159265/360.0;
	if (pVXRegion->GetMesh()){
		pVXRegion->GetMesh()->ThisMesh.RotX(RotAmt);
	}
	emit RequestUpdateGL();

}

void Dlg_EditPrim::ClickedRotY(void)
{
	double RotAmt = ((double)ui.DegreeSpinBox->value())*2*3.14159265/360.0;
	if (pVXRegion->GetMesh()){
		pVXRegion->GetMesh()->ThisMesh.RotY(RotAmt);
	}
	emit RequestUpdateGL();

}

void Dlg_EditPrim::ClickedRotZ(void)
{
	double RotAmt = ((double)ui.DegreeSpinBox->value())*2*3.14159265/360.0;
	if (pVXRegion->GetMesh()){
		pVXRegion->GetMesh()->ThisMesh.RotZ(RotAmt);
	}
	emit RequestUpdateGL();

}
