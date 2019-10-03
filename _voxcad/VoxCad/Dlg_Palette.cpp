/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * The name if its contributors may not be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#include "Dlg_Palette.h"
#include <QMessageBox>
#include <QFileDialog>

Dlg_Palette::Dlg_Palette(QVX_Object* pObjIn, QWidget *parent)
	: QWidget(parent)
{
	pObj = pObjIn;
	ui.setupUi(this);

	Palette_Model = new CPalModel(pObj, this);
//	Palette_Model.pObj = pObj; //connect our model to our object
	ui.MaterialList->setModel(Palette_Model);
	selectionModel = ui.MaterialList->selectionModel();
	SetCurMat(0);

	const QValidator* DEval = new QDoubleValidator(this); //constrain input to numbers only
	ui.ElasticEdit->setValidator(DEval);
	ui.PlasticEdit->setValidator(DEval);
	ui.YieldEdit->setValidator(DEval);
	ui.FailEdit->setValidator(DEval);
	ui.FailStrainEdit->setValidator(DEval);

	ui.PoissoEdit->setValidator(DEval);
	ui.DensityEdit->setValidator(DEval);
	ui.CTEEdit->setValidator(DEval);
	ui.TempPhaseEdit->setValidator(DEval);
	ui.UsEdit->setValidator(DEval);
	ui.UdEdit->setValidator(DEval);

	//Change selection
	connect(selectionModel, SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(UpdateFields()));

	//add/delete materials
	connect(ui.AddMatButton, SIGNAL(clicked(bool)), this, SLOT(AddMaterial()));
	connect(ui.DelMatButton, SIGNAL(clicked(bool)), this, SLOT(DeleteCurMaterial()));

	//Load or Save Palettes
	connect(ui.LoadPalButton, SIGNAL(clicked()), this, SLOT(LoadPalette()));
	connect(ui.SavePalButton, SIGNAL(clicked()), this, SLOT(SavePalette()));

	//Visible
	connect(ui.VisibleCheck, SIGNAL(clicked(bool)), this, SLOT(ChangedVisible(bool)));

	//Material mode Radios
	connect(ui.MTBasicRadio, SIGNAL(toggled(bool)), this, SLOT(ChangedBasic(bool)));
	connect(ui.MTDitherRadio, SIGNAL(toggled(bool)), this, SLOT(ChangedDither(bool)));
	connect(ui.MTStructureRadio, SIGNAL(toggled(bool)), this, SLOT(ChangedStructure(bool)));


	//color sliders
	connect(ui.RedSlider, SIGNAL(valueChanged(int)), this, SLOT(ChangedR(int)));
	connect(ui.GreenSlider, SIGNAL(valueChanged(int)), this, SLOT(ChangedG(int)));
	connect(ui.BlueSlider, SIGNAL(valueChanged(int)), this, SLOT(ChangedB(int)));
	connect(ui.AlphaSlider, SIGNAL(valueChanged(int)), this, SLOT(ChangedA(int)));

	//model properties
	connect(ui.MatModelCombo, SIGNAL(activated(int)), this, SLOT(MatModeChanged(int)));
	connect(ui.ElasticEdit, SIGNAL(textChanged(QString)), this, SLOT(ChangedElasticMod(QString)));
	connect(ui.PlasticEdit, SIGNAL(textChanged(QString)), this, SLOT(ChangedPlasticMod(QString)));
	connect(ui.YieldEdit, SIGNAL(textChanged(QString)), this, SLOT(ChangedYieldStress(QString)));

	connect(ui.StressFailRadio, SIGNAL(clicked()), this, SLOT(ClickedStressFail()));
	connect(ui.StrainFailRadio, SIGNAL(clicked()), this, SLOT(ClickedStrainFail()));
	connect(ui.FailEdit, SIGNAL(textChanged(QString)), this, SLOT(ChangedFailStress(QString)));
	connect(ui.FailStrainEdit, SIGNAL(textChanged(QString)), this, SLOT(ChangedFailStrain(QString)));

	connect(ui.LoadSSButton, SIGNAL(clicked()), this, SLOT(ClickedLoadSS()));

	

	//physical properties
	connect(ui.PoissoEdit, SIGNAL(textChanged(QString)), this, SLOT(ChangedPoisson(QString)));
	connect(ui.DensityEdit, SIGNAL(textChanged(QString)), this, SLOT(ChangedDens(QString)));
	connect(ui.CTEEdit, SIGNAL(textChanged(QString)), this, SLOT(ChangedCTE(QString)));
	connect(ui.TempPhaseEdit, SIGNAL(textChanged(QString)), this, SLOT(ChangedTempPhase(QString)));
	connect(ui.UsEdit, SIGNAL(textChanged(QString)), this, SLOT(ChangeduStatic(QString)));
	connect(ui.UdEdit, SIGNAL(textChanged(QString)), this, SLOT(ChangeduDynamic(QString)));


	//Dither
	connect(ui.Mat1SelCombo, SIGNAL(activated(int)), this, SLOT(Mat1IndexChanged(int)));
	connect(ui.Mat2SelCombo, SIGNAL(activated(int)), this, SLOT(Mat2IndexChanged(int)));
	connect(ui.MatPercSlider, SIGNAL(valueChanged(int)), this, SLOT(ChangedPercRand(int)));
	

	//Structure 
	connect(ui.ImportButton, SIGNAL(clicked()), this, SLOT(ClickedImport()));
	connect(ui.ExportButton, SIGNAL(clicked()), this, SLOT(ClickedExport()));
	connect(ui.XDimSpin, SIGNAL(valueChanged(int)), this, SLOT(ChangedXDim(int)));
	connect(ui.YDimSpin, SIGNAL(valueChanged(int)), this, SLOT(ChangedYDim(int)));
	connect(ui.ZDimSpin, SIGNAL(valueChanged(int)), this, SLOT(ChangedZDim(int)));
	connect(ui.XOffSpin, SIGNAL(valueChanged(int)), this, SLOT(ChangedXOff(int)));
	connect(ui.YOffSpin, SIGNAL(valueChanged(int)), this, SLOT(ChangedYOff(int)));
	connect(ui.ZOffSpin, SIGNAL(valueChanged(int)), this, SLOT(ChangedZOff(int)));
	connect(ui.RxRadio, SIGNAL(clicked(bool)), this, SLOT(ClickedRx(bool)));
	connect(ui.RyRadio, SIGNAL(clicked(bool)), this, SLOT(ClickedRy(bool)));
	connect(ui.RzRadio, SIGNAL(clicked(bool)), this, SLOT(ClickedRz(bool)));
	connect(ui.R0Radio, SIGNAL(clicked(bool)), this, SLOT(ClickedR0(bool)));
	connect(ui.R90Radio, SIGNAL(clicked(bool)), this, SLOT(ClickedR90(bool)));
	connect(ui.R180Radio, SIGNAL(clicked(bool)), this, SLOT(ClickedR180(bool)));
	connect(ui.R270Radio, SIGNAL(clicked(bool)), this, SLOT(ClickedR270(bool)));


	connect(ui.EditStructureButton, SIGNAL(clicked()), this, SLOT(ClickedEditStructure()));

	//set up editor window!!
	TmpEditDlg = new Dlg_StructureEditor(&TmpEdit, NULL); //shouldn't ever casue me leaks...

	TmpEditDlg->setWindowTitle("Structure Editor");
	TmpEditDlg->setWindowModality(Qt::ApplicationModal);
	connect(TmpEditDlg, SIGNAL(DoneEditing()), this, SLOT(DoneEditStructure()));
//	UpdateMaterialWindow(QItemSelection());
}

Dlg_Palette::~Dlg_Palette()
{

}

void Dlg_Palette::AddMaterial(void)
{
	Palette_Model->insertRows(0,0);
	UpdateFields();

}

void Dlg_Palette::DeleteCurMaterial(void)
{
	int CurMat = GetCurMat();
	if (CurMat > 0) Palette_Model->removeRows(CurMat,CurMat); //don't remove the null material, or try to remove -1
	emit RequestUpdateGL();
	UpdateFields();

	
}

void Dlg_Palette::LoadPalette(void)
{
	pObj->OpenPal();
	Palette_Model->UpdateList(); //to update the list...
	emit RequestUpdateGL();

}

void Dlg_Palette::SavePalette(void)
{
	pObj->SavePal();
}

void Dlg_Palette::GetCurMat(int* MatIndex)
{
	QModelIndexList indexes = selectionModel->selectedIndexes();
	if (!indexes.isEmpty())	*MatIndex = indexes.first().row();
	else *MatIndex = 0; //return erase if no material selected!
}

void Dlg_Palette::SetCurMat(int NewMatIndex)
{
	QModelIndex tmp = ui.MaterialList->model()->index(NewMatIndex, 0);
	ui.MaterialList->setCurrentIndex(tmp);
}

void Dlg_Palette::ChangedR(int RVal)
{
	int CurMat = GetCurMat();
	if (CurMat > 0) GetPCurMat()->SetRed(RVal/255.0);
	Palette_Model->UpdateList(); //to update the icon color
	emit RequestUpdateGL();
}

void Dlg_Palette::ChangedG(int GVal)
{
	int CurMat = GetCurMat();
	if (CurMat > 0) GetPCurMat()->SetGreen(GVal/255.0);
	Palette_Model->UpdateList(); //to update the icon color
	emit RequestUpdateGL();
}

void Dlg_Palette::ChangedB(int BVal)
{
	int CurMat = GetCurMat();
	if (CurMat > 0) GetPCurMat()->SetBlue(BVal/255.0);
	Palette_Model->UpdateList(); //to update the icon color
	emit RequestUpdateGL();
}

void Dlg_Palette::ChangedA(int AVal)
{
	int CurMat = GetCurMat();
	if (CurMat > 0) GetPCurMat()->SetAlpha(AVal/255.0);
	Palette_Model->UpdateList(); //to update the icon color
	emit RequestUpdateGL();
}

void Dlg_Palette::MatModeChanged(int NewMatMode)
{
	GetPCurMat()->SetMatModel(NewMatMode);
	if (NewMatMode == MDL_DATA) GetPCurMat()->ValidateSSData();
	UpdateFields();
}


void Dlg_Palette::Mat1IndexChanged(int NewMatIndex)
{
	if (NewMatIndex>=0 && NewMatIndex != GetCurMat() && MatIsEditable(NewMatIndex)){ //filter for selecting anything not allowed, since its a pain to disable in QComboBox
		GetPCurMat()->SetRandInd1(NewMatIndex);
		emit RequestUpdateGL();
	}
	UpdateFields();

}

void Dlg_Palette::Mat2IndexChanged(int NewMatIndex)
{
	if (NewMatIndex>=0 && NewMatIndex != GetCurMat() && MatIsEditable(NewMatIndex)){ //filter for selecting anything not allowed, since its a pain to disable in QComboBox
		GetPCurMat()->SetRandInd2(NewMatIndex);
		emit RequestUpdateGL();
	}
	UpdateFields();

}

void Dlg_Palette::ChangedPercRand(int RVal)
{
	int CurMat = GetCurMat();
	if (CurMat > 0) GetPCurMat()->SetRandPerc1(RVal/100.0);
	emit RequestUpdateGL();
}

void Dlg_Palette::ChangedXDim(int XDimVal)
{
	GetPCurMat()->SetSubXSize(XDimVal);
	if (GetPCurMat()->GetSubXOffset() >= GetPCurMat()->GetSubXSize()) ui.XOffSpin->setValue(XDimVal-1);
	ui.XOffSpin->setRange(0, XDimVal-1); //keep offsets within range...
	emit RequestUpdateGL();
}

void Dlg_Palette::ChangedXOff(int XOffVal)
{
	GetPCurMat()->SetSubXOffset(XOffVal);
	emit RequestUpdateGL();
}

void Dlg_Palette::ChangedYDim(int YDimVal)
{
	GetPCurMat()->SetSubYSize(YDimVal);
	if (GetPCurMat()->GetSubYOffset() >= GetPCurMat()->GetSubYSize()) ui.YOffSpin->setValue(YDimVal-1);
	ui.YOffSpin->setRange(0, YDimVal-1); //keep offsets within range...
	emit RequestUpdateGL();
}

void Dlg_Palette::ChangedYOff(int YOffVal)
{
	GetPCurMat()->SetSubYOffset(YOffVal);
	emit RequestUpdateGL();
}

void Dlg_Palette::ChangedZDim(int ZDimVal)
{
	GetPCurMat()->SetSubZSize(ZDimVal);
	if (GetPCurMat()->GetSubZOffset() >= GetPCurMat()->GetSubZSize()) ui.ZOffSpin->setValue(ZDimVal-1);
	ui.ZOffSpin->setRange(0, ZDimVal-1); //keep offsets within range...
	emit RequestUpdateGL();
}

void Dlg_Palette::ChangedZOff(int ZOffVal)
{
	GetPCurMat()->SetSubZOffset(ZOffVal);
	emit RequestUpdateGL();
}

void Dlg_Palette::ClickedEditStructure(void)
{
	if (GetPCurMat()->GetMatType() != INTERNAL) return;

	TmpEdit.Structure = GetPCurMat()->GetStructure();
	TmpEdit.Lattice = pObj->Lattice;
	TmpEdit.Palette = pObj->Palette; //needs work to avoid recusion!!
	std::string tmpName;
	for (int i=0; i<TmpEdit.GetNumMaterials(); i++){
		MatIsEditable(i, &tmpName);
		TmpEdit.Palette[i].SetName(tmpName);
//		if (i == GetCurMat()) TmpEdit.Palette[i].SetName(TmpEdit.Palette[i].GetName() + " (Editing)");
//		else if (TmpEdit.IsInRecursivePath(i, GetCurMat())) TmpEdit.Palette[i].SetName(TmpEdit.Palette[i].GetName() + " (Recursive)");
	}
	TmpEdit.Voxel = pObj->Voxel;

	TmpEditDlg->IniUpdateUI();
	TmpEditDlg->show();

}

bool Dlg_Palette::MatIsEditable(int MatIndex, std::string* ModName) //check for self reference, recursion...
{
	if (MatIndex == GetCurMat()){
		if (ModName) *ModName = pObj->Palette[MatIndex].GetName() + " (Editing)";
		return false;
	}
	else if (pObj->IsInRecursivePath(MatIndex, GetCurMat())){
		if (ModName) *ModName = pObj->Palette[MatIndex].GetName() + " (Recursive)";
		return false;
	}
	else {
		if (ModName) *ModName = pObj->Palette[MatIndex].GetName();
		return true;
	}
}

void Dlg_Palette::DoneEditStructure(void)
{
	GetPCurMat()->GetStructure() = TmpEdit.Structure;
	emit RequestUpdateGL();
}

void Dlg_Palette::ClickedImport(void) //does not currently support material recursion within the imported VXC (flattens it)
{
	QVX_Object tmpObj;
	std::string RetMsg;
	tmpObj.Open();
	int MatShiftNum = pObj->GetNumMaterials()-1; //amound to shift materials indices from imported VXC
	for (int i=1; i<tmpObj.GetNumMaterials(); i++){
		if (pObj->AddMat(tmpObj.Palette[i], true, &RetMsg) == -1){
			QMessageBox::warning(NULL, "Warning", "Could not add material. Aborting import.");
			return;
		}
	}

	for (int i=tmpObj.GetNumMaterials()-1; i>0; i--) //go backwards to avoid doubling up indices
		tmpObj.Structure.ReplaceMaterial(i, MatShiftNum+i); //temporarily creates bad indices, but they'll drop right in when we put this in our material structure

	GetPCurMat()->GetStructure() = tmpObj.Structure; //set the structure!
	UpdateFields(); //update the dimension sizes
	emit RequestUpdateGL();
}

void Dlg_Palette::ClickedExport(void) //does not currently support material recursion within the exported VXC (flattens it)
{
	QVX_Object tmp;
	tmp.Voxel = pObj->Voxel;
	tmp.Lattice = pObj->Lattice;
	tmp.Structure = GetPCurMat()->GetStructure();
	for (int i=0; i<pObj->GetNumMaterials(); i++){ //check each material if its used in this microstrucutre, add to palette if so...
		if (tmp.Structure.ContainsMatIndex(i)){ //if this material is in the substructure
			int ThisIndex = tmp.AddMat(pObj->Palette[i], true);
			tmp.Structure.ReplaceMaterial(i, ThisIndex);
		}
	}
	tmp.Save();
}

void Dlg_Palette::UpdateFields(void) //updates window based on current material selection, etc.
{
	int CurMat = GetCurMat();
	ui.VisibleCheck->setChecked(GetPCurMat()->IsVisible());
	
	if (CurMat == 0){ //if erase.nothing selected
		ui.VisibleCheck->setEnabled(false);
		ui.MatTypeGroup->setEnabled(false);
		ui.DelMatButton->setEnabled(false);
		
		//Appearance Tab
		ui.AppearanceTab->setEnabled(false);
		ui.RedSlider->setValue(0);
		ui.GreenSlider->setValue(0);
		ui.BlueSlider->setValue(0);
		ui.AlphaSlider->setValue(0);

		//Model Tab
		ui.ModelTab->setEnabled(false);
		ui.ElasticEdit->setText("");
		ui.PlasticEdit->setText("");
		ui.YieldEdit->setText("");
		ui.StressFailRadio->setChecked(false);
		ui.StrainFailRadio->setChecked(false);
		ui.FailEdit->setText("");
		ui.FailStrainEdit->setText("");

		//Physical Tab
		ui.PhysicalTab->setEnabled(false);
		ui.PoissoEdit->setText("");
		ui.DensityEdit->setText("");
		ui.CTEEdit->setText("");
		ui.TempPhaseEdit->setText("");
		ui.UsEdit->setText("");
		ui.UdEdit->setText("");

	}
	else { //if a valid material selected
		ui.VisibleCheck->setEnabled(true);
		ui.MatTypeGroup->setEnabled(true);
		ui.DelMatButton->setEnabled(true);

		//Appearance Tab
		ui.AppearanceTab->setEnabled(true);
		ui.RedSlider->setValue(GetPCurMat()->GetRedi());
		ui.GreenSlider->setValue(GetPCurMat()->GetGreeni());
		ui.BlueSlider->setValue(GetPCurMat()->GetBluei());
		ui.AlphaSlider->setValue(GetPCurMat()->GetAlphai());

		switch (GetPCurMat()->GetMatType()){
		case SINGLE:
			ui.MTBasicRadio->setChecked(true);
			//Model Tab:
			ui.ModelTab->setEnabled(true);

			ui.MatModelCombo->clear();
			ui.MatModelCombo->addItem("Linear");
			ui.MatModelCombo->addItem("Linear + Failure");
			ui.MatModelCombo->addItem("BiLinear");
			ui.MatModelCombo->addItem("Data");
			ui.MatModelCombo->setCurrentIndex(GetPCurMat()->GetMatModel());

			ui.ElasticEdit->setText(QString::number(GetPCurMat()->GetElasticMod()/1000000.0, 'g', 4));
			ui.PlasticEdit->setText(QString::number(GetPCurMat()->GetPlasticMod()/1000000.0, 'g', 4));
			ui.YieldEdit->setText(QString::number(GetPCurMat()->GetYieldStress()/1000000.0, 'g', 4));
			ui.FailEdit->setText(QString::number(GetPCurMat()->GetFailStress()/1000000.0, 'g', 4));
			ui.FailStrainEdit->setText(QString::number(GetPCurMat()->GetFailStrain(), 'g', 4));

			ui.ElasticEdit->setEnabled(false);
			ui.PlasticEdit->setEnabled(false);
			ui.YieldEdit->setEnabled(false);
			ui.FailEdit->setEnabled(false);
			ui.FailStrainEdit->setEnabled(false);
			ui.LoadSSButton->setEnabled(false);
			ui.StressFailRadio->setChecked(false);
			ui.StrainFailRadio->setChecked(false);
			ui.StressFailRadio->setEnabled(false);
			ui.StrainFailRadio->setEnabled(false);

			switch(GetPCurMat()->GetMatModel()){
			case MDL_LINEAR:
				ui.ElasticEdit->setEnabled(true);
				break;
			case MDL_LINEAR_FAIL:
				ui.ElasticEdit->setEnabled(true);
				break;
			case MDL_BILINEAR:
				ui.ElasticEdit->setEnabled(true);
				ui.PlasticEdit->setEnabled(true);
				ui.YieldEdit->setEnabled(true);
				break;
			case MDL_DATA:
				ui.LoadSSButton->setEnabled(true);
				break;
			}

			if (GetPCurMat()->GetMatModel() == MDL_LINEAR_FAIL || GetPCurMat()->GetMatModel() == MDL_BILINEAR){ 
				ui.StressFailRadio->setEnabled(true);
				ui.StrainFailRadio->setEnabled(true);
				if (GetPCurMat()->GetFailModel() == FM_MAXSTRAIN){
					ui.StrainFailRadio->setChecked(true);
					ui.FailStrainEdit->setEnabled(true);
				}
				else { //FailModel() == FM_MAXSTRESS
					ui.StressFailRadio->setChecked(true);
					ui.FailEdit->setEnabled(true);
				}
			}

			//Physical Tab:
			ui.PhysicalTab->setEnabled(true);
			ui.PoissoEdit->setText(QString::number(GetPCurMat()->GetPoissonsRatio(), 'g', 3));
			ui.DensityEdit->setText(QString::number(GetPCurMat()->GetDensity(), 'g', 4));
			ui.CTEEdit->setText(QString::number(GetPCurMat()->GetCTE(), 'g', 3));
			ui.TempPhaseEdit->setText(QString::number(GetPCurMat()->GetMatTempPhase(), 'g', 4));
			ui.UsEdit->setText(QString::number(GetPCurMat()->GetuStatic(), 'g', 3));
			ui.UdEdit->setText(QString::number(GetPCurMat()->GetuDynamic(), 'g', 3));


			break;
		case DITHER:
			ui.MTDitherRadio->setChecked(true);

			//Dither Tab:
			ui.MatPercSlider->setValue((int)(GetPCurMat()->GetRandPerc1()*100.0));

			ui.Mat1SelCombo->clear();
			ui.Mat2SelCombo->clear();

			for (int i=0; i<pObj->GetNumMaterials(); i++){
				std::string tmpName;
				MatIsEditable(i, &tmpName);
				ui.Mat1SelCombo->addItem(tmpName.c_str());
				ui.Mat2SelCombo->addItem(tmpName.c_str());
			}
			ui.Mat1SelCombo->setCurrentIndex(GetPCurMat()->GetRandInd1());
			ui.Mat2SelCombo->setCurrentIndex(GetPCurMat()->GetRandInd2());

			break;
		case INTERNAL:
			ui.MTStructureRadio->setChecked(true);

			//Internal tab:
			ui.XDimSpin->setValue(GetPCurMat()->GetSubXSize());
			ui.YDimSpin->setValue(GetPCurMat()->GetSubYSize());
			ui.ZDimSpin->setValue(GetPCurMat()->GetSubZSize());
			ui.XOffSpin->setValue(GetPCurMat()->GetSubXOffset());
			ui.YOffSpin->setValue(GetPCurMat()->GetSubYOffset());
			ui.ZOffSpin->setValue(GetPCurMat()->GetSubZOffset());
			switch (GetPCurMat()->GetSubRotateAxis()){
			case RAX_X: ui.RxRadio->setChecked(true); break;
			case RAX_Y: ui.RyRadio->setChecked(true); break;
			case RAX_Z: ui.RzRadio->setChecked(true); break;
			}
			switch (GetPCurMat()->GetSubRotateAmount()){
			case RAM_0: ui.R0Radio->setChecked(true); break;
			case RAM_90: ui.R90Radio->setChecked(true); break;
			case RAM_180: ui.R180Radio->setChecked(true); break;
			case RAM_270: ui.R270Radio->setChecked(true); break;
			}
			break;
		}
	}
	Palette_Model->UpdateList(); //probably already emitted, because setting color values calls this, but apart from performance concerns, this won't hurt anything
}

void Dlg_Palette::UpdateVisibleTabs(void)
{
	while (ui.tabProperties->count() != 0) //remove all the tabs (we'll add back the ones we need based on the material mode
		ui.tabProperties->removeTab(0);

	switch (GetPCurMat()->GetMatType()){
		case SINGLE:
			ui.tabProperties->insertTab(0, ui.PhysicalTab, "Physical");
			ui.tabProperties->insertTab(0, ui.ModelTab, "Model");
			ui.tabProperties->insertTab(0, ui.AppearanceTab, "Appearance");
			ui.tabProperties->setCurrentIndex(0);
			break;
		case DITHER:
			ui.tabProperties->insertTab(0, ui.DitherTab, "Dither");
			ui.tabProperties->insertTab(0, ui.AppearanceTab, "Appearance");
			break;
		case INTERNAL:
			ui.tabProperties->insertTab(0, ui.StructureTab, "Structure");
			ui.tabProperties->insertTab(0, ui.AppearanceTab, "Appearance");
			break;
	}
}

void Dlg_Palette::ImportSSData(void)
{
	QString tmpPath = QFileDialog::getOpenFileName(NULL, "Open Data", "", "Text (Tab delimited) (*.txt)");
	QString StrainInStr, StressInStr;
	std::vector<vfloat> StrainIn;
	std::vector<vfloat> StressIn;

	if (!tmpPath.isNull()){
		QFile File(tmpPath);
		if (!File.open(QIODevice::ReadOnly | QIODevice::Text)) return;
		QTextStream in(&File); 

		while(true){
			QString line = in.readLine();
			if (line == QString()) break;
			StrainInStr = line.section('\t', 0, 0);
			StressInStr = line.section('\t', 1, 1);
			StrainIn.push_back(StrainInStr.toDouble());
			StressIn.push_back(StressInStr.toDouble());

		}
	}

	std::string RetMsg;
	if (!GetPCurMat()->SetSSData(&StrainIn, &StressIn, &RetMsg)) QMessageBox::warning(NULL, "Import Error", RetMsg.c_str());
}



//MODEL

int CPalModel::rowCount(const QModelIndex &parent) const
{
     return pObj->GetNumMaterials();
}

QVariant CPalModel::data(const QModelIndex &index, int role) const
 {
	 if (!index.isValid())
         return QVariant();

	if (index.row() >= pObj->GetNumMaterials())
         return QVariant();

	if (role == Qt::DisplayRole){
		std::string tmp = (pObj->Palette[index.row()].GetName());
		if (pObj->Palette[index.row()].GetMatType()== DITHER) tmp += " (Dither)";
		else if (pObj->Palette[index.row()].GetMatType()== INTERNAL) tmp += " (Structure)";
		return QVariant(tmp.c_str());
	}
	if (role ==  Qt::EditRole){
		return QVariant((pObj->Palette[index.row()].GetName()).c_str());
	}
	else if (role == Qt::DecorationRole){
		int R, G, B;
		pObj->Palette[index.row()].GetColori(&R, &G, &B);
		return QColor(R,G,B);
	}
	else
		return QVariant();

 }

Qt::ItemFlags CPalModel::flags(const QModelIndex &index) const
{
	if (!index.isValid()) return Qt::ItemIsEnabled;

	return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

bool CPalModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (index.isValid() && role == Qt::EditRole) {

		pObj->Palette[index.row()].SetName(value.toString().toStdString());
		emit dataChanged(index, index);
		return true;
	}
	return false;
}

bool CPalModel::insertRows(int row, int count, const QModelIndex & parent)
{
	beginInsertRows(parent, rowCount(), rowCount()); //always only add one to the end
	pObj->AddMat("Default");
	endInsertRows();
	return true;
}

bool CPalModel::removeRows(int row, int count, const QModelIndex & parent)
{
	beginRemoveRows(parent, row, row); //removes just the one material
	if (pObj->GetBaseMat(row)->GetMatType() != SINGLE){ //if this is a compound material
		if (QMessageBox::question(NULL, "Warning", "Do you want to flatten (preserve) sub-voxels before delete?", QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel, QMessageBox::Yes) == QMessageBox::Yes){
			pObj->DeleteMat(row, true);
		}
		else pObj->DeleteMat(row);
	}	
	else pObj->DeleteMat(row);
	endRemoveRows();
	return true;
}



void CPalModel::UpdateList(void)
{
	emit dataChanged(index(0,0), index(pObj->GetNumMaterials(), 0));
}
