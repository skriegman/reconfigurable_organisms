/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef VXS_BOND_H
#define VXS_BOND_H

#include "VX_Bond.h"

class CVXS_Bond : public CVX_Bond
{
public:
	CVXS_Bond(CVX_Sim* p_SimIn);
	~CVXS_Bond(void);

	CVXS_Bond& operator=(const CVXS_Bond& Bond); //overload "=" 
	CVXS_Bond(const CVXS_Bond& Bond) : CVX_Bond(Bond) {*this = Bond;} //copy constructor


	//Bond Calculation
	virtual void UpdateBond(void) {}; //calculates force, positive for tension, negative for compression
	virtual void ResetBond(void); //resets this bond to its default (imported) state.

	//Get information about this bond
	vfloat GetStrainEnergy(void) const {return StrainEnergy;}
	vfloat GetEngStrain(void) const {return CurStrainTot;}
	vfloat GetEngStress(void) const {return CurStress;}
	vfloat GetEffectiveStiffness(void) const {return Eh*(CSArea1+CSArea2)/(2*(CurStrainTot*L.x+L.x));} //EA/L: accounting for volume effects
	Vec3D<> GetForce1(void) const {return Force1;}
	Vec3D<> GetForce2(void) const {return Force2;}
	Vec3D<> GetMoment1(void) const {return Moment1;}
	Vec3D<> GetMoment2(void) const {return Moment2;}
	vfloat GetMaxVoxKinE();
	vfloat GetMaxVoxDisp();

	bool const IsYielded(void) const {return Yielded;}
	bool const IsBroken(void) const {return Broken;}

	//POISSONS
	Vec3D<> NormForce1; //only the normal component
	vfloat GetStrainV1() {return CurStrainV1;}
	vfloat GetStrainV2() {return CurStrainV2;}
	vfloat TStrainSum1, TStrainSum2; //strain Y + strain Z (in local CS)
	vfloat CSArea1, CSArea2;


	//Vec3D<> GetAngle1(){return _Angle1;}
protected:
	//state variables for this bond
	Vec3D<> Force1, Force2, Moment1, Moment2; //The variables we really care about
	Vec3D<> _Pos2, _Angle1, _Angle2;
	Vec3D<> _LastPos2, _LastAngle1, _LastAngle2;

	//secondary calculated values
	vfloat StrainEnergy; //closely related to Forces and moments for this bond
	vfloat CurStrainTot, CurStrainV1, CurStrainV2, CurStress;
	vfloat MaxStrain, StrainOffset; //the most strain this bond has undergone (needed for plastic deformation...). The horizontal offset on the stress-strain plot representing the new rest distance.
	bool Yielded, Broken; //has the bond yielded or broken? (only update these after relaxation cycle!

	vfloat CalcStrainEnergy() const; //calculates the strain energy in the bond according to current forces and moments.

	void SetYielded(void);
	void SetBroken(void);
	
};

#endif //VXS_BOND_H
