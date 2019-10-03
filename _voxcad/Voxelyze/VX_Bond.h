/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef VX_BOND_H
#define VX_BOND_H

#include "Utils/Vec3D.h"
#include "VX_Enums.h"
class CVXS_Voxel;
class CVX_Sim;


class CVX_Bond
{
public:
	CVX_Bond(CVX_Sim* p_SimIn);
	~CVX_Bond(void);
	CVX_Bond& operator=(const CVX_Bond& Bond); //overload "=" 
	CVX_Bond(const CVX_Bond& Bond) {*this = Bond;} //copy constructor

	//Bond setup
	bool LinkVoxels(const int V1SIndIn, const int V2SIndIn); 
	bool UpdateVoxelPtrs(); //call whenever VoxArray may have been reallocated

	//Get information about this bond
	int GetVox1SInd() const {return Vox1SInd;}
	int GetVox2SInd() const {return Vox2SInd;}
	CVXS_Voxel* GetpV1() const {return pVox1;}
	CVXS_Voxel* GetpV2() const {return pVox2;}

	Axis GetBondAxis() const {return ThisBondAxis;}

	vfloat GetLinearStiffness(void) const {return a1;}
	vfloat GetDampingFactorM1() const {return _2xSqA1xM1;}
	vfloat GetDampingFactorM2() const {return _2xSqA1xM2;}

	//unwind a coordinate as if the bond was in the the positive X direction (and back...)
	template <typename T> void ToXDirBond(Vec3D<T>* const pVec) const {switch (ThisBondAxis){case AXIS_Y: {T tmp = pVec->x; pVec->x=pVec->y; pVec->y = -tmp; break;} case AXIS_Z: {T tmp = pVec->x; pVec->x=pVec->z; pVec->z = -tmp; break;}}}; //transforms a vec3D in the original orientation of the bond to that as if the bond was in +X direction
	template <typename T> void ToXDirBond(CQuat<T>* const pQuat) const {switch (ThisBondAxis){case AXIS_Y: {T tmp = pQuat->x; pQuat->x=pQuat->y; pQuat->y = -tmp; break;} case AXIS_Z: {T tmp = pQuat->x; pQuat->x=pQuat->z; pQuat->z = -tmp; break;}}}
	template <typename T> void ToOrigDirBond(Vec3D<T>* const pVec) const {switch (ThisBondAxis){case AXIS_Y: {vfloat tmp = pVec->y; pVec->y=pVec->x; pVec->x = -tmp; break;} case AXIS_Z: {vfloat tmp = pVec->z; pVec->z=pVec->x; pVec->x = -tmp; break;}}};
	template <typename T> void ToOrigDirBond(CQuat<T>* const pQuat) const {switch (ThisBondAxis){case AXIS_Y: {vfloat tmp = pQuat->y; pQuat->y=pQuat->x; pQuat->x = -tmp; break;} case AXIS_Z: {vfloat tmp = pQuat->z; pQuat->z=pQuat->x; pQuat->x = -tmp; break;}}};

	
protected:
	CVX_Sim* p_Sim; //pointer back the the simulator

	//Everything below set by LinkVoxels
	int Vox1SInd, Vox2SInd; //USE get/set functions
	CVXS_Voxel *pVox1, *pVox2; //pointers to the two voxels that make up this bond (call UpdateVoxelPtrs whenever voxels added to simulation)
	bool HomogenousBond, LinearBond; //true if both materials are the same / both materials are linear
	Axis ThisBondAxis;
	vfloat E, u, CTE, Eh; //Eh is the effective modulus accounting for poissons ratio
	vfloat E1, E2, u1, u2, CTE1, CTE2; //remember the original paramters
	Vec3D<> L;
	
	bool UpdateConstants(void); //fills in the constant parameters for the bond... returns false if unsensible material properties
	//Everything below updated by UpdateConstants().
	vfloat G, A, Iy, Iz, J, a1, a2, b1y, b2y, b3y, b1z, b2z, b3z;
	vfloat _2xA1Inv, _2xA2Inv, _3xB3yInv, _3xB3zInv; 
	vfloat _2xSqA1xM1, _2xSqA1xM2, _2xSqA2xI1, _2xSqA2xI2; 
	vfloat _2xSqB1YxM1, _2xSqB1YxM2, _2xSqB1ZxM1, _2xSqB1ZxM2;
	vfloat _2xSqB2YxFM1, _2xSqB2YxFM2, _2xSqB2ZxFM1, _2xSqB2ZxFM2;
	vfloat _2xSqB2YxI1, _2xSqB2YxI2, _2xSqB2ZxI1, _2xSqB2ZxI2, _2xSqB2YxM1, _2xSqB2YxM2, _2xSqB2ZxM1, _2xSqB2ZxM2; //b2
	vfloat _2xSqB3YxI1, _2xSqB3YxI2, _2xSqB3ZxI1, _2xSqB3ZxI2; //b3

};


#endif //VX_BOND_H
