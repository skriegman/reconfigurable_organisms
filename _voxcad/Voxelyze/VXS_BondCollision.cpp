/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "VXS_BondCollision.h"
#include "VXS_Voxel.h"



CVXS_BondCollision::CVXS_BondCollision(CVX_Sim* p_SimIn) : CVXS_Bond(p_SimIn)
{
	ResetBond(); //Zeroes out all state variables
}

CVXS_BondCollision::~CVXS_BondCollision(void)
{
}

CVXS_BondCollision& CVXS_BondCollision::operator=(const CVXS_BondCollision& Bond)
{
	CVXS_Bond::operator=(Bond);
	return *this;
}

void CVXS_BondCollision::UpdateBond() //calculates force, positive for tension, negative for compression
{
	CalcContactForce();
}

void CVXS_BondCollision::ResetBond() //calculates force, positive for tension, negative for compression
{
	CVXS_Bond::ResetBond();
}

void CVXS_BondCollision::CalcContactForce() 
{
	//just basic sphere envelope, repel with the stiffness of the material... (assumes UpdateConstants has been called)
	Vec3D<> Pos2 = pVox2->GetCurPos() - pVox1->GetCurPos();
	vfloat NomDist = (pVox1->GetCurScale() + pVox2->GetCurScale())*0.75; //effective diameter of 1.5 voxels...
	vfloat RelDist = NomDist - Pos2.Length(); //positive for overlap!
	if (RelDist > 0){ //if we're overlapping
		Force2 = Pos2/Pos2.Length() *a1*(RelDist); 
		Force1 = -Force2; 
	}
	else {
		Force2 = Vec3D<>(0,0,0);
		Force1 = Vec3D<>(0,0,0);
	}

	Moment1 = Vec3D<>(0,0,0);
	Moment2 = Vec3D<>(0,0,0);

}
