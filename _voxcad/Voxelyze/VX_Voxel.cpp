/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "VX_Voxel.h"
#include "VXS_BondInternal.h"
#include "VX_Sim.h"

CVX_Voxel::CVX_Voxel(CVX_Sim* pSimIn, int SIndexIn, int XIndexIn, int MatIndexIn, Vec3D<>& NominalPositionIn, vfloat OriginalScaleIn) 
{
	pSim = pSimIn;

	MySIndex = SIndexIn;
	DofFixed = DOF_NONE;

	for (int i=0; i<6; i++){
		InternalBondIndices[i] = NO_BOND;
		InternalBondPointers[i] = NULL;
	}
	NearbyVoxInds.clear();

	NominalPosition = NominalPositionIn; //nominal position, if this is fixed
	NominalSize = OriginalScaleIn;

	SetMaterial(MatIndexIn); //Sets a bunch of variables, depends on NominalSize

	//constraints
	ExternalForce = Vec3D<>(0,0,0);
	ExternalDisp = Vec3D<>(0,0,0);
	ExternalTorque = Vec3D<>(0,0,0);
	ExternalTDisp = Vec3D<>(0,0,0);

}

CVX_Voxel::~CVX_Voxel(void)
{
}

CVX_Voxel& CVX_Voxel::operator=(const CVX_Voxel& VIn)
{
	pSim = VIn.pSim; //not sure if we want this or not...

	MySIndex = VIn.MySIndex;
	DofFixed = VIn.DofFixed;

	for (int i=0; i<6; i++){
		InternalBondIndices[i] = VIn.InternalBondIndices[i];
		InternalBondPointers[i] = VIn.InternalBondPointers[i];		
	}

	NearbyVoxInds = VIn.NearbyVoxInds;

	NominalPosition = VIn.NominalPosition;
	NominalSize = VIn.NominalSize;

	MatIndex = VIn.MatIndex;
	SetMaterial(VIn.MatIndex);

	// Make sure to preserve the current Vox_E, possibly different from the material one
	Vox_E = VIn.GetEMod();

	ExternalForce = VIn.ExternalForce;
	ExternalDisp = VIn.ExternalDisp;
	ExternalTorque = VIn.ExternalTorque;
	ExternalTDisp = VIn.ExternalTDisp;

	return *this;
}

void CVX_Voxel::SetExternalDisp(const Axis AxisIn, const vfloat DispIn) //Sets the displacement of this voxel in the specified direction
{
	switch (AxisIn){
	case AXIS_X: ExternalDisp.x = DispIn; return;
	case AXIS_Y: ExternalDisp.y = DispIn; return;
	case AXIS_Z: ExternalDisp.z = DispIn; return;
	}
}

void CVX_Voxel::SetExternalTDisp(const Axis AxisIn, const vfloat TDispIn) //Sets the angular displacement of this voxel in the specified direction
{
	switch (AxisIn){
	case AXIS_X: ExternalTDisp.x = TDispIn; return;
	case AXIS_Y: ExternalTDisp.y = TDispIn; return;
	case AXIS_Z: ExternalTDisp.z = TDispIn; return;
	}
}

bool CVX_Voxel::SetMaterial(const int MatIndexIn) {
	MatIndex = MatIndexIn;

	if (MatIndexIn < 0 || MatIndexIn >= pSim->LocalVXC.Palette.size()){
//		CacheMaterial(NULL); //ensure everything is set to zero
		_pMat = NULL;
		Mass = Inertia = FirstMoment = _massInv = _inertiaInv = _2xSqMxExS = _2xSqIxExSxSxS = 0;
		return false; //enforce index range
	}
	else {
//		CacheMaterial(pSim->LocalVXC.GetBaseMat(MatIndex));
		_pMat = pSim->LocalVXC.GetBaseMat(MatIndex);
	}

	//Update material depended parameters
	vfloat Volume = NominalSize*NominalSize*NominalSize;
	Mass = Volume * _pMat->GetDensity(); 
	Inertia = Mass * (NominalSize*NominalSize)/6; //simple 1D approx
	FirstMoment = Mass*NominalSize/2;

	if (Volume==0 || Mass==0 || Inertia==0){_massInv=_inertiaInv=_2xSqMxExS=_2xSqIxExSxSxS=0; return false;}

	Vox_E = _pMat->GetElasticMod();
	Vox_mu = _pMat->GetPoissonsRatio();
	Vox_CTE = _pMat->GetCTE();

	IsLinearMaterial = (_pMat->GetMatModel() == MDL_LINEAR || _pMat->GetMatModel() == MDL_LINEAR_FAIL);

	_massInv = 1/Mass; //cache inverses for FAST division
	_inertiaInv = 1/Inertia;
	_2xSqMxExS = 2*sqrt(Mass*GetEMod()*NominalSize);
	_2xSqIxExSxSxS = 2*sqrt(Inertia*GetEMod()*NominalSize*NominalSize*NominalSize);

	return true;
}


bool CVX_Voxel::LinkInternalBond(int SBondIndex, BondDir ThisBondDir) //simulation bond index...
{
	if (!pSim || SBondIndex >= pSim->BondArrayInternal.size()) return false;
	
	InternalBondIndices[(int)ThisBondDir] = SBondIndex;
	InternalBondPointers[(int)ThisBondDir] = &(pSim->BondArrayInternal[SBondIndex]);

	return true;
}


void CVX_Voxel::UpdateInternalBondPtrs() //updates all links (pointers) to bonds according to pSim
{
	for (int i=0; i<6; i++){
		if (InternalBondIndices[i] == NO_BOND) InternalBondPointers[i] = NULL;
		else InternalBondPointers[i] = &(pSim->BondArrayInternal[InternalBondIndices[i]]);
	}
}


//CVX_Voxel* CVX_Voxel::pNearbyVox(int LocalNearbyInd) //To CVXS_Voxel?
//{
////	return &(p_Sim->VoxArray[NearbyVoxInds[LocalNearbyInd]]);
//	return (CVX_Voxel*) &(p_Sim->VoxArray[NearbyVoxInds[LocalNearbyInd]]);
//
//}



void CVX_Voxel::FixDof(char DofFixedIn) //fixes any of the degrees of freedom indicated. Doesn't unfix any currently fixed ones
{
	if (IS_FIXED(DOF_X, DofFixedIn)) SET_FIXED(DOF_X, DofFixed, true);
	if (IS_FIXED(DOF_Y, DofFixedIn)) SET_FIXED(DOF_Y, DofFixed, true);
	if (IS_FIXED(DOF_Z, DofFixedIn)) SET_FIXED(DOF_Z, DofFixed, true);
	if (IS_FIXED(DOF_TX, DofFixedIn)) SET_FIXED(DOF_TX, DofFixed, true);
	if (IS_FIXED(DOF_TY, DofFixedIn)) SET_FIXED(DOF_TY, DofFixed, true);
	if (IS_FIXED(DOF_TZ, DofFixedIn)) SET_FIXED(DOF_TZ, DofFixed, true);

}

void CVX_Voxel::CalcNearby(CVX_Sim* pSim, int NumHops) //populates pNearbyVox
{
	NearbyVoxInds.clear();
	int StartPoint = 0; //our enter and exit point (so we don't repeat for each iteration
	int StopPoint = 1;

	NearbyVoxInds.push_back(MySIndex);

	for (int i=0; i<NumHops; i++){
		for (int j=StartPoint; j<StopPoint; j++){ //go through the list from the most recent interation...
			CVX_Voxel* pThisVox = &(pSim->VoxArray[NearbyVoxInds[j]]);

			for (int k=0; k<6; k++){ //look at all the potential (permanent) bonds of this voxel
//				if (pNearbyVox(j)->InternalBondIndices[k] != NO_BOND){
				if (pThisVox->InternalBondIndices[k] != NO_BOND){
//					int OtherSIndex = (pNearbyVox(j)->IAmInternalVox2(k)) ? pNearbyVox(j)->InternalBondPointers[k]->GetpV1()->MySIndex  : pNearbyVox(j)->InternalBondPointers[k]->GetpV2()->MySIndex;
					int OtherSIndex = (pThisVox->IAmInternalVox2(k)) ? pThisVox->InternalBondPointers[k]->GetpV1()->MySIndex  : pThisVox->InternalBondPointers[k]->GetpV2()->MySIndex;

					//get the other voxel in this bond...
					//int OtherSIndex = pNearbyVox(j)->GetBond(k)->GetpV1()->MySIndex;
					//if (pNearbyVox(j)->IsMe(pNearbyVox(j)->GetBond(k)->GetpV1()))  OtherSIndex = pNearbyVox(j)->GetBond(k)->GetpV2()->MySIndex; //if this voxel 1

					//Add it to the list if its not already on it.
					if (!IsNearbyVox(OtherSIndex)) NearbyVoxInds.push_back(OtherSIndex);
				}
			}
			//for (int k=0; k<pNearbyVox(j)->GetNumLocalBonds(); k++){ //look at all the bonds of this voxel
			//	//get the other voxel in this bond...
			//	int OtherSIndex = pNearbyVox(j)->GetBond(k)->GetpV1()->MySIndex;
			//	if (pNearbyVox(j)->IsMe(pNearbyVox(j)->GetBond(k)->GetpV1()))  OtherSIndex = pNearbyVox(j)->GetBond(k)->GetpV2()->MySIndex; //if this voxel 1

			//	//Add it to the list if its not already on it.
			//	if (!IsNearbyVox(OtherSIndex)) NearbyVoxInds.push_back(OtherSIndex);
			//}
		}
		StartPoint = StopPoint;
		StopPoint = NumNearbyVox();
	}
}

//bool CVX_Voxel::IsLinear() //returns true if the material model is linear
//{
//	if (_pMat->GetMatModel() == MDL_LINEAR || _pMat->GetMatModel() == MDL_LINEAR_FAIL) return true;
//	else return false;
//}


