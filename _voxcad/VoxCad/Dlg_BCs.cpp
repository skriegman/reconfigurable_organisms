/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * The name if its contributors may not be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/


#include "Dlg_BCs.h"

Dlg_BCs::Dlg_BCs(QVX_Environment* pEnvIn, QWidget *parent)
	: QWidget(parent)
{
	CurRegion = NULL;
	CurBcIndex = -1;

	pEnv = pEnvIn;	

	ui.setupUi(this);
	SetupPrimTab();


	ui.BCPresetsCombo->addItem("None"); //should be in same order as BCPresetType enum
	ui.BCPresetsCombo->addItem("X Cantilever"); //should be in same order as BCPresetType enum
	ui.BCPresetsCombo->addItem("Y Cantilever");
	ui.BCPresetsCombo->addItem("X Axial");
	ui.BCPresetsCombo->addItem("Y Axial");
	ui.BCPresetsCombo->addItem("Z Axial");
	ui.BCPresetsCombo->addItem("X Symmetry");
	ui.BCPresetsCombo->addItem("Y Symmetry");
	ui.BCPresetsCombo->addItem("Z Symmetry");


	const QValidator* DEval = new QDoubleValidator(this);
	ui.XForceEdit->setValidator(DEval);
	ui.YForceEdit->setValidator(DEval);
	ui.ZForceEdit->setValidator(DEval);
	ui.XDispEdit->setValidator(DEval);
	ui.YDispEdit->setValidator(DEval);
	ui.ZDispEdit->setValidator(DEval);
	ui.TXTorqueEdit->setValidator(DEval);
	ui.TYTorqueEdit->setValidator(DEval);
	ui.TZTorqueEdit->setValidator(DEval);
	ui.TXDispEdit->setValidator(DEval);
	ui.TYDispEdit->setValidator(DEval);
	ui.TZDispEdit->setValidator(DEval);

	ApplyPreset(BC_XCANT);

	connect(ui.BCPresetsCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(ApplyPreset(int)));
	//connect(ui.BCList, SIGNAL(currentRowChanged (int)), this, SLOT(BCrowChanged(int))); //itemClicked ?
	connect(ui.BCList, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(BCRowClicked())); //itemClicked ?

	//add or delete boundary conditions
	connect(ui.AddBCButton, SIGNAL(clicked(bool)), this, SLOT(AddBC()));
	connect(ui.DelBCButton, SIGNAL(clicked(bool)), this, SLOT(DelCurBC()));

	//Load or Save Boundary Conditions
	connect(ui.LoadBCButton, SIGNAL(clicked()), this, SLOT(LoadBCs()));
	connect(ui.SaveBCButton, SIGNAL(clicked()), this, SLOT(SaveBCs()));

	connect(ui.FixAllButton, SIGNAL(clicked()), this, SLOT(ClickedFixAll()));
	connect(ui.FixNoneButton, SIGNAL(clicked()), this, SLOT(ClickedFixNone()));

	//Translation
	connect(ui.XFixed, SIGNAL(clicked(bool)), this, SLOT(ChangedXFixed(bool)));
	connect(ui.YFixed, SIGNAL(clicked(bool)), this, SLOT(ChangedYFixed(bool)));
	connect(ui.ZFixed, SIGNAL(clicked(bool)), this, SLOT(ChangedZFixed(bool)));

	connect(ui.XForceEdit, SIGNAL(editingFinished()), this, SLOT(EditedXForce()));
	connect(ui.YForceEdit, SIGNAL(editingFinished()), this, SLOT(EditedYForce()));
	connect(ui.ZForceEdit, SIGNAL(editingFinished()), this, SLOT(EditedZForce()));

	connect(ui.XDispEdit, SIGNAL(editingFinished()), this, SLOT(EditedXDisp()));
	connect(ui.YDispEdit, SIGNAL(editingFinished()), this, SLOT(EditedYDisp()));
	connect(ui.ZDispEdit, SIGNAL(editingFinished()), this, SLOT(EditedZDisp()));

	//Rotation
	connect(ui.TXFixed, SIGNAL(clicked(bool)), this, SLOT(ChangedTXFixed(bool)));
	connect(ui.TYFixed, SIGNAL(clicked(bool)), this, SLOT(ChangedTYFixed(bool)));
	connect(ui.TZFixed, SIGNAL(clicked(bool)), this, SLOT(ChangedTZFixed(bool)));

	connect(ui.TXTorqueEdit, SIGNAL(editingFinished()), this, SLOT(EditedTXTorque()));
	connect(ui.TYTorqueEdit, SIGNAL(editingFinished()), this, SLOT(EditedTYTorque()));
	connect(ui.TZTorqueEdit, SIGNAL(editingFinished()), this, SLOT(EditedTZTorque()));

	connect(ui.TXDispEdit, SIGNAL(editingFinished()), this, SLOT(EditedTXDisp()));
	connect(ui.TXDispEdit, SIGNAL(textChanged(QString)), this, SLOT(ChangedTDisp(QString)));
	connect(ui.TYDispEdit, SIGNAL(editingFinished()), this, SLOT(EditedTYDisp()));
	connect(ui.TYDispEdit, SIGNAL(textChanged(QString)), this, SLOT(ChangedTDisp(QString)));
	connect(ui.TZDispEdit, SIGNAL(editingFinished()), this, SLOT(EditedTZDisp()));
	connect(ui.TZDispEdit, SIGNAL(textChanged(QString)), this, SLOT(ChangedTDisp(QString)));

	connect(ui.BCDonePushButton, SIGNAL(clicked()), this, SIGNAL(DoneEditing()));

}

Dlg_BCs::~Dlg_BCs()
{

}

void Dlg_BCs::SetupPrimTab()
{
	BCEditPrimDlg = new Dlg_EditPrim(CurRegion, pEnv->pObj, this); //setup the dialog with empty Region

	ui.ShapeScrollArea->setWidget(BCEditPrimDlg);

	connect(BCEditPrimDlg, SIGNAL(RequestUpdateGL()), this, SIGNAL(RequestUpdateGL()));
}


void Dlg_BCs::UpdateUI(bool UpdatingTextField)
{
	if (!CurRegion){ //if nothing selected:
		ui.FixAllButton->setEnabled(false);
		ui.FixNoneButton->setEnabled(false);
		ui.TranslationGroup->setEnabled(false);
		ui.RotationGroup->setEnabled(false);
	}
	else { //if something selected:
		ui.FixAllButton->setEnabled(true);
		ui.FixNoneButton->setEnabled(true);
		ui.TranslationGroup->setEnabled(true);
		ui.RotationGroup->setEnabled(true);

		char CurDofFixed = CurRegion->DofFixed;
		bool DofXFixed = IS_FIXED(DOF_X, CurDofFixed);
		bool DofYFixed = IS_FIXED(DOF_Y, CurDofFixed);
		bool DofZFixed = IS_FIXED(DOF_Z, CurDofFixed);
		bool DofTXFixed = IS_FIXED(DOF_TX, CurDofFixed);
		bool DofTYFixed = IS_FIXED(DOF_TY, CurDofFixed);
		bool DofTZFixed = IS_FIXED(DOF_TZ, CurDofFixed);

		ui.XFixed->setChecked(DofXFixed);
		ui.YFixed->setChecked(DofYFixed);
		ui.ZFixed->setChecked(DofZFixed);
		ui.XDispEdit->setEnabled(DofXFixed);
		ui.YDispEdit->setEnabled(DofYFixed);
		ui.ZDispEdit->setEnabled(DofZFixed);
		ui.XForceEdit->setEnabled(!DofXFixed);
		ui.YForceEdit->setEnabled(!DofYFixed);
		ui.ZForceEdit->setEnabled(!DofZFixed);

		ui.TXFixed->setChecked(DofTXFixed);
		ui.TYFixed->setChecked(DofTYFixed);
		ui.TZFixed->setChecked(DofTZFixed);
		ui.TXDispEdit->setEnabled(DofTXFixed);
		ui.TYDispEdit->setEnabled(DofTYFixed);
		ui.TZDispEdit->setEnabled(DofTZFixed);
		ui.TXTorqueEdit->setEnabled(!DofTXFixed);
		ui.TYTorqueEdit->setEnabled(!DofTYFixed);
		ui.TZTorqueEdit->setEnabled(!DofTZFixed);

		if (UpdatingTextField){
			ui.XDispEdit->setText(QString::number(CurRegion->Displace.x*1000, 'g', 3));
			ui.YDispEdit->setText(QString::number(CurRegion->Displace.y*1000, 'g', 3));
			ui.ZDispEdit->setText(QString::number(CurRegion->Displace.z*1000, 'g', 3));
			ui.XForceEdit->setText(QString::number(CurRegion->Force.x, 'g', 3));
			ui.YForceEdit->setText(QString::number(CurRegion->Force.y, 'g', 3));
			ui.ZForceEdit->setText(QString::number(CurRegion->Force.z, 'g', 3));

			ui.TXDispEdit->setText(QString::number(CurRegion->AngDisplace.x*RtoD, 'g', 3));
			ui.TYDispEdit->setText(QString::number(CurRegion->AngDisplace.y*RtoD, 'g', 3));
			ui.TZDispEdit->setText(QString::number(CurRegion->AngDisplace.z*RtoD, 'g', 3));
			ui.TXTorqueEdit->setText(QString::number(CurRegion->Torque.x*1000, 'g', 3));
			ui.TYTorqueEdit->setText(QString::number(CurRegion->Torque.y*1000, 'g', 3));
			ui.TZTorqueEdit->setText(QString::number(CurRegion->Torque.z*1000, 'g', 3));
		}
	}
}

void Dlg_BCs::ChangedFixed(char DofToChange, bool State)
{
	if (CurRegion){
		SET_FIXED(DofToChange, CurRegion->DofFixed, State);
		if (DOF_X&DofToChange){State ? CurRegion->Force.x=0 : CurRegion->Displace.x=0; } //if we're changing the X DOF
		if (DOF_Y&DofToChange){State ? CurRegion->Force.y=0 : CurRegion->Displace.y=0; } //if we're changing the Y DOF
		if (DOF_Z&DofToChange){State ? CurRegion->Force.z=0 : CurRegion->Displace.z=0; } //if we're changing the Z DOF
		if (DOF_TX&DofToChange){State ? CurRegion->Torque.x=0 : CurRegion->AngDisplace.x=0; } //if we're changing the TX DOF
		if (DOF_TY&DofToChange){State ? CurRegion->Torque.y=0 : CurRegion->AngDisplace.y=0; } //if we're changing the TY DOF
		if (DOF_TZ&DofToChange){State ? CurRegion->Torque.z=0 : CurRegion->AngDisplace.z=0; } //if we're changing the TZ DOF
	
	}
}


void Dlg_BCs::ApplySelection(const int BcIndex) //portal to handle whenever the selected boundary conditionchanges. -1 is clear selection (none selected)
{
	if (BcIndex != CurBcIndex){ //only handle this if the selection has changed
		CurBcIndex = BcIndex;
		UpdateBCLists();   //this should be first to ensure the list has all the current rows in it

		if(BcIndex >= 0 && BcIndex < ui.BCList->count()){ui.BCList->setCurrentRow(BcIndex);		}
		else {ui.BCList->clearSelection();}

		emit RequestGLSelect(BcToGlIndex(BcIndex));
		if (BCEditPrimDlg) BCEditPrimDlg->UpdateRegion(CurRegion);
		UpdateUI();

	}
	UpdateCurRegion();
}

void Dlg_BCs::UpdateBCLists(void)
{
	int PrevCurRow = CurBcIndex;
	ui.BCList->clear();

	for (int i=0; i<pEnv->GetNumBCs(); i++){
		char ThisDofFixed = pEnv->GetBC(i)->DofFixed;
		QString MyName = "BC" + QString::number(i) + " : (";
		if (IS_FIXED(DOF_X, ThisDofFixed)) MyName += "*"; else MyName += "_";
		if (IS_FIXED(DOF_Y, ThisDofFixed)) MyName += "*"; else MyName += "_";
		if (IS_FIXED(DOF_Z, ThisDofFixed)) MyName += "*"; else MyName += "_";
		if (IS_FIXED(DOF_TX, ThisDofFixed)) MyName += "*"; else MyName += "_";
		if (IS_FIXED(DOF_TY, ThisDofFixed)) MyName += "*"; else MyName += "_";
		if (IS_FIXED(DOF_TZ, ThisDofFixed)) MyName += "*"; else MyName += "_";
		MyName += ")";

		new QListWidgetItem(MyName, ui.BCList);
	}

	ApplySelection(PrevCurRow);
//	ui.BCList->setCurrentRow(PrevCurRow);

	UpdateUI();
}

void Dlg_BCs::ChangedTXFixed(bool State)
{
	ChangedFixed(DOF_TX, State);
	if (!State) CurRegion->AngDisplace = Vec3D<>(0,0,0);

	UpdateBCLists();
	emit RequestUpdateGL();
}

void Dlg_BCs::ChangedTYFixed(bool State)
{
	ChangedFixed(DOF_TY, State);
	if (!State) CurRegion->AngDisplace = Vec3D<>(0,0,0);
	UpdateBCLists();
	emit RequestUpdateGL();
}

void Dlg_BCs::ChangedTZFixed(bool State)
{
	ChangedFixed(DOF_TZ, State);
	if (!State) CurRegion->AngDisplace = Vec3D<>(0,0,0);
	UpdateBCLists();
	emit RequestUpdateGL();
}


void Dlg_BCs::EditedTXDisp()
{
	QString NewText=ui.TXDispEdit->text();
	if (CurRegion){
		double NewVal = NewText.toDouble()*DtoR;
		CurRegion->AngDisplace.x = NewVal;
		UpdateBCLists();
		emit RequestUpdateGL();
	}
}

void Dlg_BCs::EditedTYDisp()
{
	QString NewText=ui.TYDispEdit->text();
	if (CurRegion){
		double NewVal = NewText.toDouble()*DtoR;
		CurRegion->AngDisplace.y = NewVal;
		UpdateBCLists();
		emit RequestUpdateGL();
	}
}

void Dlg_BCs::EditedTZDisp()
{
	QString NewText=ui.TZDispEdit->text();
	if (CurRegion){
		double NewVal = NewText.toDouble()*DtoR;
		CurRegion->AngDisplace.z = NewVal;
		UpdateBCLists();
		emit RequestUpdateGL();
	}
}

void Dlg_BCs::ApplyPreset(int NewPreset)
{
	pEnv->ClearBCs();
	BCEditPrimDlg->ChangedSnap(false);

	char ThisDof = DOF_NONE;
	switch (NewPreset){
	case BC_XCANT:
		pEnv->AddFixedBc(Vec3D<>(0,0,0), Vec3D<>(0.01, 1.0, 1.0));
		pEnv->AddForcedBc(Vec3D<>(0.99,0,0), Vec3D<>(0.01, 1.0, 1.0), Vec3D<>(0,0,-1.0), Vec3D<>(0,0,0));
		break;
	case BC_YCANT:
		pEnv->AddFixedBc(Vec3D<>(0,0,0), Vec3D<>(1.0, 0.01, 1.0));
		pEnv->AddForcedBc(Vec3D<>(0,0.99,0), Vec3D<>(1.0, 0.01, 1.0), Vec3D<>(0,0,-1.0), Vec3D<>(0,0,0));
		break;
	case BC_XAXIAL:
		pEnv->AddFixedBc(Vec3D<>(0,0,0), Vec3D<>(0.01, 1.0, 1.0));
		pEnv->AddForcedBc(Vec3D<>(0.99,0,0), Vec3D<>(0.01, 1.0, 1.0), Vec3D<>(1.0,0,0), Vec3D<>(0,0,0));
		break;
	case BC_YAXIAL:
		pEnv->AddFixedBc(Vec3D<>(0,0,0), Vec3D<>(1.0, 0.01, 1.0));
		pEnv->AddForcedBc(Vec3D<>(0,0.99,0), Vec3D<>(1.0, 0.01, 1.0), Vec3D<>(0,1.0,0), Vec3D<>(0,0,0));
		break;
	case BC_ZAXIAL:
		pEnv->AddFixedBc(Vec3D<>(0,0,0), Vec3D<>(1.0, 1.0, 0.01));
		pEnv->AddForcedBc(Vec3D<>(0,0,0.99), Vec3D<>(1.0, 1.0, 0.01), Vec3D<>(0,0,1.0), Vec3D<>(0,0,0));
		break;
	case BC_XSYM:
		SET_FIXED(DOF_X, ThisDof, true);
		SET_FIXED(DOF_TY, ThisDof, true);
		SET_FIXED(DOF_TZ, ThisDof, true);
		pEnv->AddFixedBc(Vec3D<>(0.99,0,0), Vec3D<>(0.01, 1.0, 1.0), ThisDof);
		break;
	case BC_YSYM:
		SET_FIXED(DOF_Y, ThisDof, true);
		SET_FIXED(DOF_TX, ThisDof, true);
		SET_FIXED(DOF_TZ, ThisDof, true);
		pEnv->AddFixedBc(Vec3D<>(0,0.99,0), Vec3D<>(1.0, 0.01, 1.0), ThisDof);
		break;
	case BC_ZSYM:
		SET_FIXED(DOF_Z, ThisDof, true);
		SET_FIXED(DOF_TX, ThisDof, true);
		SET_FIXED(DOF_TY, ThisDof, true);
		pEnv->AddFixedBc(Vec3D<>(0,0,0.99), Vec3D<>(1.0, 1.0, 0.01), ThisDof);
		break;

	}

	ui.BCList->setCurrentRow(0); //set the current elected to this one
	BCrowChanged(0); //need to call this explicitly sometimes, if new index happens to be the same as the old one...

	UpdateBCLists();
	emit RequestUpdateGL();
}

void Dlg_BCs::AddBC(void)
{
	pEnv->AddFixedBc(Vec3D<>(0,0,0), Vec3D<>(0.1, 0.1, 0.1));
	int MyNewIndex = pEnv->GetNumBCs()-1;
	CurRegion = pEnv->GetBC(MyNewIndex); 
	ui.BCList->setCurrentRow(MyNewIndex); //set the current elected to this one
	BCrowChanged(MyNewIndex); //need to call this explicitly sometimes, if new index happens to be the same as the old one...

	UpdateBCLists();
	emit RequestUpdateGL();
}

void Dlg_BCs::DelCurBC(void)
{
	pEnv->DelBC(CurBcIndex);

	UpdateBCLists();
	emit RequestUpdateGL();
}

void Dlg_BCs::SaveBCs(void)
{
	pEnv->SaveBCs();
}

void Dlg_BCs::LoadBCs(void)
{
	pEnv->OpenBCs();

	ui.BCList->setCurrentRow(0); //set the current elected to this one
	BCrowChanged(0); //need to call this explicitly sometimes, if new index happens to be the same as the old one...

	UpdateBCLists();
	emit RequestUpdateGL();
}


////MODEL
//
//int CBCModel::rowCount(const QModelIndex &parent) const
//{
//     return pEnv->GetNumMaterials();
//}
//
//QVariant CBCModel::data(const QModelIndex &index, int role) const
// {
//	 if (!index.isValid())
//         return QVariant();
//
//	if (index.row() >= pObj->GetNumMaterials())
//         return QVariant();
//
//	if (role == Qt::DisplayRole || role ==  Qt::EditRole)
//		return QVariant((pObj->Palette[index.row()].GetName()).c_str());
//	else if (role == Qt::DecorationRole){
//		int R, G, B;
//		pObj->Palette[index.row()].GetColori(&R, &G, &B);
//		return QColor(R,G,B);
//	}
//	else
//		return QVariant();
//
// }
//
//Qt::ItemFlags CBCModel::flags(const QModelIndex &index) const
//{
//	if (!index.isValid()) return Qt::ItemIsEnabled;
//
//	return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
//}
//
//bool CBCModel::setData(const QModelIndex &index, const QVariant &value, int role)
//{
//	if (index.isValid() && role == Qt::EditRole) {
//
//		pObj->Palette[index.row()].SetName(value.toString().toStdString());
//		emit dataChanged(index, index);
//		return true;
//	}
//	return false;
//}
//
//bool CBCModel::insertRows(int row, int count, const QModelIndex & parent)
//{
//	beginInsertRows(parent, rowCount(), rowCount()); //always only add one to the end
//	pObj->AddMat("Default");
//	endInsertRows();
//	return true;
//}
//
//bool CBCModel::removeRows(int row, int count, const QModelIndex & parent)
//{
//	beginRemoveRows(parent, row, row); //removes just the one material
//	pObj->DeleteMat(row);
//	endRemoveRows();
//	return true;
//}
//
//
//
//void CBCModel::UpdateList(void)
//{
//	emit dataChanged(index(0,0), index(pObj->GetNumMaterials(), 0));
//}
