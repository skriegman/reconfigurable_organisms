/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef VX_VOXEL_H
#define VX_VOXEL_H

#include "Utils/Vec3D.h"
#include <vector>
#include "VX_Enums.h"

#include <iostream>

class CVXS_BondInternal;
class CVX_Sim;
class CVXC_Material;

//info about a voxel and its relation to the overall simulated structure
class CVX_Voxel
{
public:
	CVX_Voxel(CVX_Sim* pSimIn, int SIndexIn, int XIndexIn, int MatIndexIn, Vec3D<>& NominalPositionIn, vfloat OriginalScaleIn);
	~CVX_Voxel(void);
	CVX_Voxel(const CVX_Voxel& VIn) {*this = VIn;} //copy constructor
	CVX_Voxel& operator=(const CVX_Voxel& VIn);

	//internal info
	bool LinkInternalBond(int SBondIndex, BondDir ThisBondDir); //Informs this voxel of it's participation in an internal bond to cache the link. (required to account for forces from this bond when summing for voxel)
	void UpdateInternalBondPtrs(); //Updates all the cached links (pointers) to bonds according to current p_Sim and InternalBondIndices[]
	CVXS_BondInternal* GetpInternalBond(BondDir BondDirection) {return InternalBondPointers[BondDirection];}
	inline bool IAmInternalVox1(const int BondDirIndex) const {return !IAmInternalVox2(BondDirIndex);} //returns true if this voxel is Vox1 of the specified bond
	inline bool IAmInternalVox2(const int BondDirIndex) const {return BondDirIndex%2;} //returns true if this voxel is Vox2 of the specified bond


	inline Vec3D<> GetNominalPosition(void) const {return NominalPosition;}
	inline vfloat GetNominalSize(void) const {return NominalSize;}
	inline int GetSimIndex(void) const {return MySIndex;} //returns the index of this voxel in the simulation voxel list


	inline void AddExternalForce(const Vec3D<>& ForceIn) {ExternalForce += ForceIn;} //Adds a constant external force to this voxel
	inline void AddExternalTorque(const Vec3D<>& TorqueIn) {ExternalTorque += TorqueIn;} //Adds a constant external torque to this voxel
	void SetExternalDisp(const Axis AxisIn, const vfloat DispIn); //Sets the displacement of this voxel in the specified direction
	void SetExternalTDisp(const Axis AxisIn, const vfloat TDispIn); //Sets the angular displacement of this voxel in the specified direction
	inline void SetExternalDisp(const Vec3D<>& DispIn) {ExternalDisp = DispIn;} //Sets the displacement of this voxel (only degrees of freedom set as "fixed" will observe this displacement)
	inline void SetExternalTDisp(const Vec3D<>& TDispIn) {ExternalTDisp = TDispIn;} //Sets the angular displacement of this voxel (only degrees of freedom set as "fixed" will observe this displacement)
	Vec3D<> GetExternalForce() {return ExternalForce;}
	Vec3D<> GetExternalTorque() {return ExternalTorque;}


	//Get parameters for this voxel
	inline vfloat GetMass(void) const {return Mass;}
	inline vfloat GetFirstMoment(void) const {return FirstMoment;}
	inline vfloat GetInertia(void) const {return Inertia;}
	inline vfloat GetEMod(void) const {return Vox_E;}

	// FC: Important: always use SetEMod when altering Vox_E, it's important to recompute a bunch of quantities that depend on the elastic mod.
	virtual void SetEMod(vfloat Vox_E_in){};
	
	inline vfloat GetPoisson(void) const  {return Vox_mu;}
	inline vfloat GetCTE(void) const  {return Vox_CTE;}
	inline vfloat GetLinearStiffness(void) const {vfloat linStiff = 2*Vox_E*NominalSize; /*std::cout << "[VX_Voxel.h] getLinearStiffness = "<< linStiff << std::endl; */ return linStiff;} //EA/L with L=NominalSize/2
	
	bool SetMaterial(const int MatIndexIn); //sets the material according to VX_Object material index
	int GetMaterialIndex(void) const {return MatIndex;} //returns the material index for VX_Object material list
	CVXC_Material* GetpMaterial(void) const {return _pMat;} //returns the material index for VX_Object material list


	bool IsLinear() {return IsLinearMaterial;} //returns true if the material model is linear

	//fixed degrees of freedom
	void FixDof(char DofFixedIn); //fixes any of the degrees of freedom indicated. Doesn't unfix any currently fixed ones
	inline char GetDofFixed(void) const {return DofFixed;}

	//Nearby voxel info information
	void CalcNearby(CVX_Sim* pSim, int NumHops); //populates NearbyVoxInds[] with all voxel within specified number of hops in the internal lattice. Does not jump gaps.
	inline bool IsNearbyVox(int GlobalSVoxInd) {int NumNear=NumNearbyVox(); for (int i=0; i<NumNear; i++) if (NearbyVoxInds[i] == GlobalSVoxInd) return true; return false;} //returns true if the requested voxel is in the NearbyVoxInds[] list
	bool IsSurfaceVoxel() {for (int i=0; i<6; i++){if (InternalBondIndices[i] == NO_BOND) return true;} return false;}; //returns true if any face of the voxel is exposed


protected:
	CVX_Sim* pSim;

	int MySIndex; //index in the simulation voxel Array
	char DofFixed; //which degrees of freedom are fixed (DOF_X, DOF_Y, etc. as defined in VX_FRegion.h)

	//internal connections
	int InternalBondIndices[6]; //bonds in the six ordinate directions according to BD_PX, BD_NX, etc.
	CVXS_BondInternal* InternalBondPointers[6]; //cached pointers to InternalBondIndices

	//Nearby voxel information
	int NumNearbyVox(void){return (int)NearbyVoxInds.size();} //how many voxels are nearby in the internal lattice according to last call of CalcNearby()
	std::vector<int> NearbyVoxInds; //which voxels are close by in the internal lattice according to last call of CalcNearby()

	//nominal (original) state...
	Vec3D<> NominalPosition; //Original position upon import. This will  never change in the course of the simulation.
	vfloat NominalSize; //Original size upon import. (assumes cubes!) Never change this.

	//Info about this voxel: (set via SetMaterial())
	int MatIndex; //Material associated with this voxel (pre blending)
	vfloat Mass; //The mass of this voxel
	vfloat FirstMoment; //1st moment "inertia" (needed for certain calculations)
	vfloat Inertia; //rotational mass
	vfloat Vox_E, Vox_mu, Vox_CTE; //Elastic modulus, poisson's ratio, and thermal expansion coefficient.
	bool IsLinearMaterial;

	//secondary calculated and cached info about this voxel (set via SetMaterial())
	vfloat _massInv; //1/Mass
	vfloat _inertiaInv; //1/Inertia
	vfloat _2xSqMxExS; //needed for quick damping calculations
	vfloat _2xSqIxExSxSxS; //needed for quick rotational damping calculations
	CVXC_Material* _pMat; //cached pointer to material.

	
	//Inputs affecting this local voxel
	Vec3D<> ExternalForce; //External force applied to this voxel in N if relevant DOF are unfixed
	Vec3D<> ExternalDisp; //Prescribed displaced position in meters, if DOF is fixed
	Vec3D<> ExternalTorque; //External torque applied to this voxel in N-m if relevant DOF are unfixed
	Vec3D<> ExternalTDisp; //Prescribed displaced in radians, if DOF is fixed



};

#endif //VX_VOXEL_H