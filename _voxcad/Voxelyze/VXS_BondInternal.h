/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef VXS_BONDINTERNAL_H
#define VXS_BONDINTERNAL_H

#include "VXS_Bond.h"

#ifdef PREC_LOW
	static const vfloat SA_BOND_BEND_RAD = 0.1; //Amount for small angle bond calculations
#elif defined PREC_HIGH
	static const vfloat SA_BOND_BEND_RAD = 0.02; //Amount for small angle bond calculations
#elif defined PREC_MAX 
	static const vfloat SA_BOND_BEND_RAD = 0.002; //Amount for small angle bond calculations
#else //defined PREC_MED 
	static const vfloat SA_BOND_BEND_RAD = 0.05; //Amount for small angle bond calculations
#endif

static const vfloat SA_BOND_EXT_PERC = 1.30; //Amount for small angle bond calculations

class CVXS_BondInternal : public CVXS_Bond
{
public:
	CVXS_BondInternal(CVX_Sim* p_SimIn);
	~CVXS_BondInternal(void);
	CVXS_BondInternal& operator=(const CVXS_BondInternal& Bond); //overload "=" 
	CVXS_BondInternal(const CVXS_BondInternal& Bond) : CVXS_Bond(Bond) {*this = Bond;} //copy constructor

	virtual void UpdateBond(void); //calculates force, positive for tension, negative for compression
	virtual void ResetBond(void); //resets this voxel to its default (imported) state.

	bool const IsSmallAngle(void) const {return SmallAngle;}

	//For debugging
	Vec3D<> AxialForce1, AxialForce2, ShearForce1, ShearForce2, BendingForce1, BendingForce2;

private:
	void CalcLinForce();

	bool SmallAngle; //based on compiled precision setting
	vfloat MidPoint; //percent (0 to 1) of the material interface between vox 1 and vox 2

	bool UpdateBondStrain(vfloat CurStrainIn); //Updates yielded, brokem, CurStrainTot, and CurStress based on CurStrainIn
	void AddDampForces(); //Adds damping forces IN LOCAL BOND COORDINATES (with bond pointing in +x direction, pos1 = 0,0,0
	bool UpdateConstants(void); //fills in the constant parameters for the bond... returns false if unsensible material properties

	
};

//RESOURCES

//quaternion properties (for reference)
//1) To rotate a vector V, form a quaternion with w = 0; To rotate by Quaternion Q, do Q*V*Q.Conjugate() and trim off the w component.
//2) To do multiple rotations: To Rotate by Q1 THEN Q2, Q2*Q1*V*Q1.Conjugate*Q2.Conjugate(), or make a Qtot = Q2*Q1 and do Qtot*V*Qtot.Conjugate()
//3) Q1*Q1.Conjugate - Identity
//4) To do a reverse rotation Q1, just do Q1.conjugate*V*Q1
//5) An orientation quaternion is really just the relative rotation from the identity quaternion (1,0,0,0) to this orientation.
//6) If an orientation is represented by Q1, to rotate that orientation by Q2 the new orientation is Q2*Q1
//http://www.cprogramming.com/tutorial/3d/quaternions.html


//The giant stiffness matrix VoxCAD uses to model the connections between beams: (Forces, torques) = K*(displacements, angles)
//Sources:
//http://www.google.com/url?sa=t&rct=j&q=&esrc=s&source=web&cd=3&ved=0CDkQFjAC&url=http%3A%2F%2Fsteel.sejong.ac.kr%2Fdown%2Fpaper%2Fi-paper-13.pdf&ei=fmFuUP_kAeOGyQGIrIDYDQ&usg=AFQjCNG3YZI0bd9OO69VQqV7PTO3KIsEyQ&cad=rja
//http://www.colorado.edu/engineering/cas/courses.d/IFEM.d/
//http://www.google.com/url?sa=t&rct=j&q=&esrc=s&source=web&cd=2&ved=0CCcQFjAB&url=http%3A%2F%2Fwww.eng.fsu.edu%2F~chandra%2Fcourses%2Feml4536%2FChapter4.ppt&ei=919uUKOnDamryQGc_oEI&usg=AFQjCNER15oW2k4KtNrNR3FdvxNnUbnRVw&cad=rja
/*
F = |k| x
				1	2	3	4	5	6	7	8	9	10	11	12
|F1x|		|	a1						-a1						 |	|X1 |
|F1y|		|		b1y				b2z		-b1y			b2z	 | 	|Y1 |
|F1z|		|			b1z		-b2y			-b1z	-b2y	 | 	|Z1 |
|M1x|		|				a2						-a2			 | 	|TX1|
|M1y|		|			-b2z	2*b3y			b2z		b3y		 | 	|TY1|
|M1z| = 	|		b2y				2*b3z	-b2y			b3z	 |*	|TZ1|
|F2x|		|	-a1						a1						 | 	|X2 |
|F2y|		|		-b1y			-b2z	b1y				-b2z | 	|Y2 |
|F2z|		|			-b1z	b2y				b1z		b2y		 | 	|Z2 |
|M2x|		|				-a2						a2			 | 	|TX2|
|M2y|		|			-b2z	b3y				b2z		2*b3y	 | 	|TY2|
|M2z|		|		b2y				b3z		-b2y			2*b3z| 	|TZ2|

//Confirmed at http://www.eng-tips.com/viewthread.cfm?qid=14924&page=97
k[ 1][ 1] =  E*area/L;				k[ 1][ 7] = -k[1][1];
k[ 2][ 2] =  12.0*E*Iz/(L*L*L);		k[ 2][ 6] =  6.0*E*Iz/(L*L);	k[ 2][ 8] = -k[2][2];		k[ 2][12] =  k[2][6];		
k[ 3][ 3] =  12.0*E*Iy/(L*L*L);		k[ 3][ 5] = -6.0*E*Iy/(L*L);	k[ 3][ 9] = -k[3][3];		k[ 3][11] =  k[3][5];
k[ 4][ 4] =  G*J/L;					k[ 4][10] = -k[4][4];
k[ 5][ 5] =  4.0*E*Iy/L;			k[ 5][ 9] =  6.0*E*Iy/(L*L);	k[ 5][11] =  2.0*E*Iy/L;
k[ 6][ 6] =  4.0*E*Iz/L;			k[ 6][ 8] = -6.0*E*Iz/(L*L);	k[ 6][12] =  2.0*E*Iz/L;
k[ 7][ 7] =  k[1][1];
k[ 8][ 8] =  k[2][2];				k[ 8][12] = -k[2][6];
k[ 9][ 9] =  k[3][3];				k[ 9][11] =  k[5][9];
k[10][10] =  k[4][4];
k[11][11] =  k[5][5];
k[12][12] =  k[6][6];

Likewise, for caculating local damping forces
F=-zeta*2*s(MK)*v or -zeta*2*s(I*wk)*w

		||M1|				|
|M| =	|	|I1|			|	(|M1| is 3x3 identity * mass of voxel 1, etc.)
		|		|M2|		|
		|			|I2|	| (12x12)

					1		2		3		4		5		6		7		8		9		10		11		12
|F1x|			|	a1M1											-a1M1											|	|V1x|
|F1y|			|			b1yM1							b2zI1			-b1yM1							b2zI1	|	|V1y|
|F1z|			|					b1zM1			-b2yI1							-b1zM1			-b2yI1			|	|V1z|
|M1x|			|							a2I1											-a2I1					|	|w1x|
|M1y|			|					-b2zM1			2*b3yI1							b2zM1			b3yI1			|	|w1y|
|M1z| =	zeta *	|			b2yM1							2*b3zI1			-b2yM1							b3zI1	| *	|w1z|
|F2x|			|	-a1M2											a1M2											|	|V2x|
|F2y|			|			-b1yM2							-b2zI2			b1yM2							-b2zI2	|	|V2y|
|F2z|			|					-b1zM2			b2yM2							b1zM2			b2yM2			|	|V2z|
|M2x|			|							-a2I2											a2I2					|	|w2x|
|M2y|			|					-b2zM2			b3yI2							b2zM2			2*b3yI2			|	|w2y|
|M2z|			|			b2yM2							b3zI2			-b2yM2							2*b3zI2	|	|w2z|

a1M1 = 2*sqrt(a1*m1)
a1M2 = 2*sqrt(a1*m2)
b1yM1 = 2*sqrt(b1y*m2)
etc...

(extra negation added to make signs work out)
F1x = -zeta*a1M1*(V2x-V1x)
F1y = -zeta*b1yM1(V2y-V1y) + zeta*b2zI1(w1z+w2z)
F1z = -zeta*b1zM1(V2z-V1z) - zeta*b2yI1(w1y+w2y)
M1x = -zeta*a2I1(w2x-w1x)
M1y = zeta*b2zM1(V2z-V1z) + zeta*b3yI1(2*w1y+w2y)
M1z = -zeta*b2yM1(V2y-V1y) + zeta*b3zI1(2*w1z+w2z)
F2x = zeta*a1M2*(V2x-V1x)
F2y = zeta*b1yM2(V2y-V1y) - zeta*b2zI2(w1z+w2z)
F2z = zeta*b1zM2(V2z-V1z) + zeta*b2yI2(w1y+w2y)
M2x = zeta*a2I2(w2x-w1x)
M2y = zeta*b2zM2(V2z-V1z) + zeta*b3yI2(w1y+2*w2y)
M2z = -zeta*b2yM2(V2y-V1y) + zeta*b3zI2(w1z+2*w2z)


STRAIN ENERGY of the bond is the area under the force-displacement curve that can be recovered.

PURE TENSION AND COMPRESSION:
For our purposes, even with non-linear materials, it is assumed materials will rebound with their base elastic modulus.

Strain energy is the shaded area, slope is EA/L, therefore bottom edge length is FL/AE.
Area is then Fcurrent*(FL/AE)/2. (or, given EA/L is stiffness k, strain energy = 0.5*F^2/k (or F^2/(2k))
http://www.roymech.co.uk/Useful_Tables/Beams/Strain_Energy.html
http://www.freestudy.co.uk/statics/complex/t5.pdf

Fcurrent
F (n)
|
|       ___
|     /   /|
|   /    /#|
|  /    /##|
| /    /###|
|/    /####|
------------------------
	Disp (m)

PURE TORSION
For rotational, the process is similar: T^2/(2k) where K is units of N-m

BENDING
The strain energy from bending = integral [M(x)^2 dx / (2EI)].
Because we only load the ends of the beam, M(x) = (M2-M1)*x/L+M1 (Linear interoplation, M1 and M2 are the moments at each end).
Plugging and chugging though the integral, we get:

Strain energy = L/(6EI) * (M1^2 + M1*M2 + M2^2)

Note that the sign of M2 must be reversed from that calculated by the stiffness matrix.
This is because a positive "Moment" on a beam (in the moment diagram) is actually two opposite-direction torques on the ends of the beam. ALA http://www.efunda.com/formulae/solid_mechanics/beams/sign_convention.cfm

This means Strain energy = L/(6EI) * (M1^2 - M1*M2' + M2'^2)

SHEAR
Not currently modeled in voxcad.

SUPERPOSITION:
We assume each of these modes is independent, thus total strain energy is simply the sum of them all.

*/




#endif //VXS_BONDINTERNAL_H
