/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "VX_Bond.h"
#include "VXS_Voxel.h"
#include "VX_Sim.h"

CVX_Bond::CVX_Bond(CVX_Sim* p_SimIn)
{
	p_Sim = p_SimIn;

	Vox1SInd = -1;
	Vox2SInd = -1;
	pVox1 = NULL;
	pVox2 = NULL;
	HomogenousBond = false;
	LinearBond = false;
	ThisBondAxis = AXIS_NONE;
	E=0; u=0; CTE=0; Eh=0;
	E1=0; E2=0; u1=0; u2=0; CTE1=0; CTE2=0;
	L = Vec3D<>(0, 0, 0);
	
	UpdateConstants(); //updates all the dependent variables based on zeros above.
}

CVX_Bond::~CVX_Bond(void)
{
}

CVX_Bond& CVX_Bond::operator=(const CVX_Bond& Bond)
{
	p_Sim = Bond.p_Sim;

	Vox1SInd = Bond.Vox1SInd;
	Vox2SInd = Bond.Vox2SInd;
	if (!UpdateVoxelPtrs()){Vox1SInd = Vox2SInd = -1;}
	HomogenousBond = Bond.HomogenousBond;
	LinearBond = Bond.LinearBond;
	ThisBondAxis = Bond.ThisBondAxis;
	L = Bond.L;
	E = Bond.E;
	//std::cout << "[VX_Bond.cpp] debugmsg : operator=() (1) E = " << E << std::endl;
	u = Bond.u;
	CTE = Bond.CTE;
	Eh = Bond.Eh;
	E1 = Bond.E1;
	u1 = Bond.u1;
	CTE1 = Bond.CTE1;
	E2 = Bond.E2;
	u2 = Bond.u2;
	CTE2 = Bond.CTE2;
	
	UpdateConstants();

	return *this;
}


bool CVX_Bond::LinkVoxels(const int V1SIndIn, const int V2SIndIn)
{
	if (!p_Sim || V1SIndIn >= p_Sim->VoxArray.size() || V2SIndIn >= p_Sim->VoxArray.size()) return false;

	Vox1SInd=V1SIndIn;
	Vox2SInd=V2SIndIn;
	if (Vox1SInd == Vox2SInd) return true; //Equal voxel indices is a flag to disable a bond (?? still in use?)
	if (!UpdateVoxelPtrs()){Vox1SInd = Vox2SInd = -1; return false;}

	Vec3D<> OrigDist = pVox2->GetNominalPosition() - pVox1->GetNominalPosition(); //original distance (world coords)
	HomogenousBond = (pVox1->GetMaterialIndex() == pVox2->GetMaterialIndex() && pVox1->GetEMod() == pVox2->GetEMod());
	LinearBond = (pVox1->IsLinear() && pVox2->IsLinear());

	if (OrigDist.x == 0 && OrigDist.y == 0) ThisBondAxis = AXIS_Z;
	else if (OrigDist.x == 0 && OrigDist.z == 0) ThisBondAxis = AXIS_Y;
	else if (OrigDist.y == 0 && OrigDist.z == 0) ThisBondAxis = AXIS_X;
	else ThisBondAxis = AXIS_NONE;

	E1 = pVox1->GetEMod(); E2 = pVox2->GetEMod();

	//std::cout << "[VX_Bond.cpp] debugmsg : linkVoxels() (2) E1 = " << E1 << " E2 = " << E2 << std::endl;

	u1 = pVox1->GetPoisson(); u2 = pVox2->GetPoisson();
	CTE1 = pVox1->GetCTE(); CTE2 = pVox2->GetCTE();

	if (E1 == 0 || E2 == 0) {return false;}
	if (u1 < 0 || u1 > 0.5 || u2 < 0 || u2 > 0.5 ) {return false;} //bad poissons ratio;

	E = (E1*E2/(E1+E2))*2; //x2 derived from case of equal stiffness: E1*E1/(E1+E1) = 0.5*E1 

	//std::cout << "[VX_Bond.cpp] debugmsg : linkVoxels() (3) E = " << E << std::endl;

	if (u1==0 && u2==0) u=0;
	else u = (u1*u2/(u1+u2))*2; //Poissons ratio: todo: keep separate? I don't think this is correct
	CTE = (CTE1/2+CTE2/2); //thermal expansion
//	Eh = E/((1-2*u)*(1+u)); //effective modulus, accounting for volume effects
	vfloat E1h = E1/((1-2*u1)*(1+u1)); //effective modulus, accounting for volume effects
	vfloat E2h = E2/((1-2*u2)*(1+u2)); //effective modulus, accounting for volume effects
	Eh = (E1h*E2h/(E1h+E2h))*2; //x2 derived from case of equal stiffness: E1*E1/(E1+E1) = 0.5*E1 


	//for now we are only using the nominal size of the voxel, although we could change this later if needed
	vfloat NominalSize = (pVox1->GetNominalSize() + pVox2->GetNominalSize())*0.5;
	L = Vec3D<>(NominalSize, NominalSize, NominalSize);


	if (!UpdateConstants()) return false;


	return true;
}

bool CVX_Bond::UpdateVoxelPtrs()
{
	if (!p_Sim) return false;
	pVox1 = pVox2 = NULL;

	if (Vox1SInd>=0 && Vox1SInd < p_Sim->VoxArray.size()) pVox1 = &(p_Sim->VoxArray[Vox1SInd]);
	if (Vox2SInd>=0 && Vox2SInd < p_Sim->VoxArray.size()) pVox2 = &(p_Sim->VoxArray[Vox2SInd]);
	
	return pVox1 && pVox2;
}


bool CVX_Bond::UpdateConstants(void) //fills in the constant parameters for the bond...
{
	G = E/(2*(1+u)); //Shear modulus
	A = L.y * L.z;
	Iy = L.z*L.y*L.y*L.y / 12; //BHHH/12
	Iz = L.y*L.z*L.z*L.z / 12;
	J = L.y*L.z*(L.y*L.y + L.z*L.z)/12; //torsional MOI: BH/12*(BB+HH)

	if (L.x == 0){a1=0; a2=0; b1y=0; b1z=0; b2y=0; b2z=0; b3y=0; b3z=0;}
	else {
		a1 = E * A / L.x; //Units of N/m
		//std::cout << "[VX_Bond.cpp] debugmsg : UpdateConstants() a1 = " << a1 << std::endl;
		a2 = G * J / L.x; //Units of N-m
		b1y = 12 * E * Iy / (L.x*L.x*L.x); // + G * A / L; //Units of N/m
		b1z = 12 * E * Iz / (L.x*L.x*L.x); // + G * A / L; //Units of N/m
		b2y = 6 * E * Iy / (L.x*L.x); //Units of N (or N-m/m: torque related to linear distance)
		b2z = 6 * E * Iz / (L.x*L.x); //Units of N (or N-m/m: torque related to linear distance)
		b3y = 2 * E * Iy / L.x; //Units of N-m
		b3z = 2 * E * Iz / L.x; //Units of N-m
	}

	//for strain energy calculations
	_2xA1Inv = 1.0/(a1*2.0);
	_2xA2Inv = 1.0/(a2*2.0);
	_3xB3yInv = 1.0/(b3y*3.0);
	_3xB3zInv = 1.0/(b3z*3.0);

	//cached pre-multiplied values
	vfloat M1=0, M2=0;
	vfloat FM1=0, FM2=0;
	vfloat I1=0, I2=0;
	if (pVox1 && pVox2){	
		M1 = pVox1->GetMass(), M2 = pVox2->GetMass();
		FM1 = pVox1->GetFirstMoment(), FM2 = pVox2->GetFirstMoment(), 
		I1 = pVox1->GetInertia(), I2 = pVox2->GetInertia();
	}

	_2xSqA1xM1 = 2.0*sqrt(a1*M1); _2xSqA1xM2 = 2.0*sqrt(a1*M2);
	_2xSqA2xI1 = 2.0*sqrt(a2*I1); _2xSqA2xI2 = 2.0*sqrt(a2*I2);
	_2xSqB1YxM1 = 2.0*sqrt(b1y*M1); _2xSqB1YxM2 = 2.0*sqrt(b1y*M2);
	_2xSqB1ZxM1 = 2.0*sqrt(b1z*M1); _2xSqB1ZxM2 = 2.0*sqrt(b1z*M2);
	_2xSqB2YxFM1 = 2.0*sqrt(b2y*FM1); _2xSqB2YxFM2 = 2.0*sqrt(b2y*FM2);
	_2xSqB2ZxFM1 = 2.0*sqrt(b2z*FM1); _2xSqB2ZxFM2 = 2.0*sqrt(b2z*FM2);
	_2xSqB2YxI1 = 2.0*sqrt(b2y*I1); _2xSqB2YxI2 = 2.0*sqrt(b2y*I2);
	_2xSqB2ZxI1 = 2.0*sqrt(b2z*I1); _2xSqB2ZxI2 = 2.0*sqrt(b2z*I2);
	_2xSqB2YxM1 = 2.0*sqrt(b2y*M1); _2xSqB2YxM2 = 2.0*sqrt(b2y*M2);
	_2xSqB2ZxM1 = 2.0*sqrt(b2z*M1); _2xSqB2ZxM2 = 2.0*sqrt(b2z*M2);
	_2xSqB3YxI1 = 2.0*sqrt(b3y*I1); _2xSqB3YxI2 = 2.0*sqrt(b3y*I2);
	_2xSqB3ZxI1 = 2.0*sqrt(b3z*I1); _2xSqB3ZxI2 = 2.0*sqrt(b3z*I2);

	return true;
}