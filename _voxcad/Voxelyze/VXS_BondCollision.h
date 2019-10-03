/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef VXS_BONDCOLLISION_H
#define VXS_BONDCOLLISION_H

#include "VXS_Bond.h"

class CVXS_BondCollision : public CVXS_Bond
{
public:
	CVXS_BondCollision(CVX_Sim* p_SimIn);
	~CVXS_BondCollision(void);
	CVXS_BondCollision& operator=(const CVXS_BondCollision& Bond); //overload "=" 
	CVXS_BondCollision(const CVXS_BondCollision& Bond) : CVXS_Bond(Bond) {*this = Bond;} //copy constructor

	virtual void UpdateBond(void); //calculates force, positive for tension, negative for compression
	virtual void ResetBond(void); //resets this voxel to its default (imported) state.

private:
	void CalcContactForce();

};

#endif //VXS_BONDCOLLISION_H
