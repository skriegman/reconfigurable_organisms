/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * The name if its contributors may not be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#ifndef DLG_EDITPRIM_H
#define DLG_EDITPRIM_H

#include "ui_vPrimEdit.h"
#include <QWidget>
#include <qsettings.h>
class CVX_FRegion;
class CVX_Object;

class Dlg_EditPrim : public QWidget
{
	Q_OBJECT

public:
	Dlg_EditPrim(CVX_FRegion* pRegionIn, CVX_Object* pObjIn, QWidget *parent = 0);
	~Dlg_EditPrim();

signals:
	void RequestUpdateGL(void);

public slots:
	void UpdateRegion(CVX_FRegion* pNewRegion) {pVXRegion = pNewRegion; UpdateUI();}

	void ChangedSnap(int NewState) {if(NewState == Qt::Checked) Snap = true; else Snap = false;}
	void ChangedLock(int NewState) {if(NewState == Qt::Checked) LockAspect = true; else LockAspect = false;}

	void ChangedX(int NewVal) {ui.XSpin->setValue(NewVal/100.0);}
	void ChangedX(double NewVal);
	void ChangedY(int NewVal) {ui.YSpin->setValue(NewVal/100.0);}
	void ChangedY(double NewVal);
	void ChangedZ(int NewVal) {ui.ZSpin->setValue(NewVal/100.0);}
	void ChangedZ(double NewVal);

	void ChangedDX(int NewVal) {ui.DXSpin->setValue(NewVal/100.0);}
	void ChangedDX(double NewVal);
	void ChangedDY(int NewVal) {ui.DYSpin->setValue(NewVal/100.0);}
	void ChangedDY(double NewVal);
	void ChangedDZ(int NewVal) {ui.DZSpin->setValue(NewVal/100.0);}
	void ChangedDZ(double NewVal);
	void ChangedRad(int NewVal) {ui.RSpin->setValue(NewVal/100.0);};
	void ChangedRad(double NewVal);

	void ClickedBoxRadio(void);
	void ClickedSphereRadio(void);
	void ClickedCylinderRadio(void);
	void ClickedMeshRadio(void);
	void ClickedLoadMesh(void);

	void ClickedRotX(void);
	void ClickedRotY(void);
	void ClickedRotZ(void);



	void UpdateUI(void);

private:
	Ui::PrimEditDlg ui;
	CVX_FRegion* pVXRegion; //boundary condition to edit
	CVX_Object* pObj;

	bool Snap;
	bool LockAspect;
	bool AutoUpdating; //flag to not do calculation for auto-updating other sliders... 



	double GetSnapped(double Dim, double Intvl, bool CanRetZero = true){double RetVal = Intvl * (((int)(Dim/Intvl+0.5))); if(RetVal == 0) return Intvl; else return RetVal;} //returns

	QString GetLastDir(){return QSettings().value("CurrentDir", "").toString();}

};

#endif // DLG_EDITPRIM_H
