/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * The name if its contributors may not be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#ifndef DLG_BCS_H
#define DLG_BCS_H

#include "ui_vBCs.h"
#include "QVX_Interfaces.h"
#include "Dlg_EditPrim.h"
#include <QWidget>

#define RtoD (180/3.14159265359)
#define DtoR (3.14159265359/180)


class Dlg_BCs : public QWidget
{
	Q_OBJECT

public:
	Dlg_BCs(QVX_Environment* pEnvIn, QWidget *parent = 0);
	~Dlg_BCs();

	QVX_Environment* pEnv;
	QWidget EditPrimCont;


	enum BCPresetType {BC_NONE, BC_XCANT, BC_YCANT, BC_XAXIAL, BC_YAXIAL, BC_ZAXIAL, BC_XSYM, BC_YSYM, BC_ZSYM};

	virtual QSize sizeHint () const {return QSize(100, 500);};

//	std::vector <CVX_FRegion> PercBCs; //holds the percentage values for the BC (independant of LatDim and Workspace size)

signals:
	void RequestUpdateGL(void);
	void RequestGLSelect(int NewGLIndex);
	void DoneEditing(void); //emitted to exit the boundary condition editting mode

public slots:
	void AddBC(void);
	void DelCurBC(void);
	void SaveBCs(void);
	void LoadBCs(void);
	void BCrowChanged(int NewRow) {ApplySelection(NewRow);}
	void BCRowClicked() {ApplySelection(ui.BCList->currentRow());}

	void ApplyExtSelection(int NewGLIndex) {ApplySelection(GlToBcIndex(NewGLIndex));} //called when we select a BC external to the window (IE GL picking)

	
	void ClickedFixAll(void) {ChangedFixed(DOF_X | DOF_Y | DOF_Z | DOF_TX | DOF_TY | DOF_TZ, true); UpdateBCLists(); emit RequestUpdateGL();}
	void ClickedFixNone(void) {ChangedFixed(DOF_X | DOF_Y | DOF_Z | DOF_TX | DOF_TY | DOF_TZ, false); UpdateBCLists(); emit RequestUpdateGL();}
	
//	void ChangedFixed(bool State);
//	void ChangedForced(bool State);
	
	void ChangedXFixed(bool State) {ChangedFixed(DOF_X, State); UpdateBCLists(); emit RequestUpdateGL();}
	void ChangedYFixed(bool State) {ChangedFixed(DOF_Y, State); UpdateBCLists(); emit RequestUpdateGL();}
	void ChangedZFixed(bool State) {ChangedFixed(DOF_Z, State); UpdateBCLists(); emit RequestUpdateGL();}
	void ChangedTXFixed(bool State); // {ChangedFixed(DOF_TX, State);}
	void ChangedTYFixed(bool State); // {ChangedFixed(DOF_TY, State);}
	void ChangedTZFixed(bool State); // {ChangedFixed(DOF_TZ, State);}


	void EditedXForce() {if (CurRegion){QString NewText=ui.XForceEdit->text(); CurRegion->Force.x = NewText.toDouble(); emit RequestUpdateGL();}}
	void EditedYForce() {if (CurRegion){QString NewText=ui.YForceEdit->text(); CurRegion->Force.y = NewText.toDouble(); emit RequestUpdateGL();}}
	void EditedZForce() {if (CurRegion){QString NewText=ui.ZForceEdit->text(); CurRegion->Force.z = NewText.toDouble(); emit RequestUpdateGL();}}
	void EditedXDisp() {if (CurRegion){QString NewText=ui.XDispEdit->text(); CurRegion->Displace.x = NewText.toDouble()/1000.0; emit RequestUpdateGL();}}
	void EditedYDisp() {if (CurRegion){QString NewText=ui.YDispEdit->text(); CurRegion->Displace.y = NewText.toDouble()/1000.0; emit RequestUpdateGL();}}
	void EditedZDisp() {if (CurRegion){QString NewText=ui.ZDispEdit->text(); CurRegion->Displace.z = NewText.toDouble()/1000.0; emit RequestUpdateGL();}}

	void EditedTXTorque() {if (CurRegion){QString NewText=ui.TXTorqueEdit->text(); CurRegion->Torque.x = NewText.toDouble()/1000.0; emit RequestUpdateGL();}}
	void EditedTYTorque() {if (CurRegion){QString NewText=ui.TYTorqueEdit->text(); CurRegion->Torque.y = NewText.toDouble()/1000.0; emit RequestUpdateGL();}}
	void EditedTZTorque() {if (CurRegion){QString NewText=ui.TZTorqueEdit->text(); CurRegion->Torque.z = NewText.toDouble()/1000.0; emit RequestUpdateGL();}}
	void EditedTXDisp(); 
	void EditedTYDisp(); 
	void EditedTZDisp(); 
	void ChangedTDisp(QString NewText) {if (CurRegion){double NewVal=NewText.toDouble();if(NewVal != 0){ChangedFixed(DOF_TX | DOF_TY | DOF_TZ, true);UpdateUI(false);}}}
	void ApplyPreset(int NewPreset);

	void DoneButtonClicked(void){emit DoneEditing();};
	void EditLocationClicked(void){if (EditPrimCont.isVisible()) EditPrimCont.hide(); else EditPrimCont.show();}
	

	void UpdateBCLists(void);

private:
	Ui::BCDlg ui;

	void SetupPrimTab();
	Dlg_EditPrim* BCEditPrimDlg;

	void UpdateUI(bool UpdatingTextField = true);
	void ChangedFixed(char DofToChange, bool State);

	int CurBcIndex; //currently selected row number
	CVX_FRegion* CurRegion; //currently selected boundary condition
	void UpdateCurRegion(){if (pEnv && CurBcIndex >=0 && CurBcIndex < pEnv->GetNumBCs()){CurRegion = pEnv->GetBC(CurBcIndex);} else CurRegion=NULL;}

//	int CombToFixedIndex(int CombInd) {if (CombInd >= 0 && CombInd < pEnv->GetNumFixed()) return CombInd; else return -1;}; //returns the index of the fixed array from the master index
//	int CombToForcedIndex(int CombInd) {if (CombInd >= pEnv->GetNumFixed() && CombInd < pEnv->GetNumFixed()+pEnv->GetNumForced()) return CombInd-pEnv->GetNumFixed(); else return -1;}; //returns the index of the forced array from the master index
//	int FixedToCombIndex(int FixedInd) {return FixedInd;};
//	int ForcedToCombIndex(int ForcedInd) {return pEnv->GetNumFixed() + ForcedInd;};
	//int CombToGLIndex(int CombInd) {
	//	int tmpInd = CombToFixedIndex(CombInd); 
	//	if (tmpInd != -1) return BCFIXED_GLIND_OFF+tmpInd;
	//	tmpInd = CombToForcedIndex(CombInd); 
	//	if (tmpInd != -1) return BCFORCE_GLIND_OFF+tmpInd;
	//	return -1;
	//};
	//int GLToCombIndex(int GLIndex){
	//	int tmpInd = GLIndex-BCFIXED_GLIND_OFF;
	//	if (tmpInd>=0 && tmpInd<pEnv->GetNumFixed()) return FixedToCombIndex(tmpInd);
	//	tmpInd = GLIndex-BCFORCE_GLIND_OFF;
	//	if (tmpInd>=0 && tmpInd<pEnv->GetNumForced()) return ForcedToCombIndex(tmpInd);
	//	return -1;
	//}
	inline int BcToGlIndex(const int BcIndexIn) const {return BC_GLIND_OFF+BcIndexIn;}
	inline int GlToBcIndex(const int GlIndexIn) const {int tmpInd = GlIndexIn-BC_GLIND_OFF; if (tmpInd>=0) return tmpInd; else return -1;}

	void ApplySelection(const int BcIndex); //portal to handle whenever the selected boundary conditionchanges

protected:
//	void closeEvent(QCloseEvent* event);
//	int GLToForcedIndex(int CombInd) {int tmpInd = CombToForcedIndex(CombInd); if (tmpInd == -1) return -1; else return BCFORCE_GLIND_OFF+tmpInd;};
	

};

////THE LINK between qt's model framework and my CVX_Object palette object
//
//class CBCModel : public QAbstractListModel
//{
//	Q_OBJECT
//
//public:
//	CBCModel() {pObj = NULL;};
//	CBCModel(CVX_FEA* pFEAIn, QObject *parent = 0) : QAbstractListModel(parent), pEnv(pFEAIn) {} //calls default constructor, sets pObj to pVXC
//
//public slots:
//	void UpdateList(void);
//
//public:
//	CVX_FEA* pEnv; //VXC object we are analyzing
//
//	int rowCount(const QModelIndex &parent = QModelIndex()) const;
//	QVariant data(const QModelIndex &index, int role) const;
//
//	Qt::ItemFlags flags(const QModelIndex &index) const;
//	bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
//
//	bool insertRows(int row, int count, const QModelIndex & parent = QModelIndex()); 
//	bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex()); 
//};
//
#endif // DLG_BCS_H
