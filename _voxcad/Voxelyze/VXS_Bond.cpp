/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "VXS_Bond.h"
#include "VXS_Voxel.h"
#include "VX_Sim.h"

CVXS_Bond::CVXS_Bond(CVX_Sim* p_SimIn) : CVX_Bond(p_SimIn)
{
	ResetBond(); //Zeroes out all state variables
}

CVXS_Bond::~CVXS_Bond(void)
{
}

CVXS_Bond& CVXS_Bond::operator=(const CVXS_Bond& Bond)
{
	CVX_Bond::operator=(Bond);

	//State variables
	Force1 = Bond.Force1;
	Force2 = Bond.Force2;
	Moment1 = Bond.Moment1;
	Moment2 = Bond.Moment2;
	StrainEnergy = Bond.StrainEnergy;
	_Pos2=Bond._Pos2; 
	_Angle1=Bond._Angle1;
	_Angle2=Bond._Angle2;
	_LastPos2=Bond._LastPos2; 
	_LastAngle1=Bond._LastAngle1;
	_LastAngle2=Bond._LastAngle2; 
	CurStrainTot = Bond.CurStrainTot;
	CurStrainV1 = Bond.CurStrainV1;
	CurStrainV2 = Bond.CurStrainV2;
	CurStress = Bond.CurStress;
	MaxStrain = Bond.MaxStrain;
	StrainOffset = Bond.StrainOffset;
	Yielded = Bond.Yielded;
	Broken = Bond.Broken;
	
	TStrainSum1 = Bond.TStrainSum1;
	TStrainSum2 = Bond.TStrainSum2;
	CSArea1 = Bond.CSArea1;
	CSArea2 = Bond.CSArea2;

	return *this;
}

void CVXS_Bond::ResetBond(void) //resets this voxel to its default (imported) state.
{
	Force1 = Vec3D<>(0,0,0);
	Force2 = Vec3D<>(0,0,0);
	Moment1 = Vec3D<>(0,0,0);
	Moment2 = Vec3D<>(0,0,0);
	StrainEnergy = 0; 

	CurStrainTot = 0;
	CurStrainV1 = 0;
	CurStrainV2 = 0;
	CurStress = 0;
	MaxStrain = 0;
	StrainOffset = 0;

	TStrainSum1 = 0;
	TStrainSum2 = 0;
	CSArea1 = CSArea2 = L.y*L.z;

	Yielded = false;
	Broken = false;

	_Pos2 = Vec3D<>(0,0,0);
	_Angle1 = Vec3D<>(0,0,0);
	_Angle2 = Vec3D<>(0,0,0);
	_LastPos2 = Vec3D<>(0,0,0);
	_LastAngle1 = Vec3D<>(0,0,0);
	_LastAngle2 = Vec3D<>(0,0,0);

}


vfloat CVXS_Bond::GetMaxVoxKinE(){
	vfloat Ke1 = pVox1->GetCurKineticE(), Ke2 = pVox2->GetCurKineticE();
	return Ke1>Ke2?Ke1:Ke2;
}

vfloat CVXS_Bond::GetMaxVoxDisp(){
	vfloat D1 = pVox1->GetCurAbsDisp(), D2 = pVox2->GetCurAbsDisp();
	return D1>D2?D1:D2;
}

void CVXS_Bond::SetYielded(void)
{
	Yielded = true;
	pVox1->SetYielded(true);
	pVox2->SetYielded(true);
}

void CVXS_Bond::SetBroken(void)
{
	if (p_Sim->IsFeatureEnabled(VXSFEAT_FAILURE)){ // FailureEnabled()){
		Broken = true; 
		pVox1->SetBroken(true);
		pVox2->SetBroken(true);
	}
}

vfloat CVXS_Bond::CalcStrainEnergy() const
{
	return	_2xA1Inv*Force1.x*Force1.x + //Tensile strain
			_2xA2Inv*Moment1.x*Moment1.x + //Torsion strain
			_3xB3zInv*(Moment1.z*Moment1.z - Moment1.z*Moment2.z +Moment2.z*Moment2.z) + //Bending Z
			_3xB3yInv*(Moment1.y*Moment1.y - Moment1.y*Moment2.y +Moment2.y*Moment2.y); //Bending Y
}
