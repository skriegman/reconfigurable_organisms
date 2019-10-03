/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "VXS_BondInternal.h"
#include "VXS_Voxel.h"
#include "VX_Sim.h"



CVXS_BondInternal::CVXS_BondInternal(CVX_Sim* p_SimIn) : CVXS_Bond(p_SimIn)
{
	ResetBond(); //Zeroes out all state variables
}

CVXS_BondInternal::~CVXS_BondInternal(void)
{
}

CVXS_BondInternal& CVXS_BondInternal::operator=(const CVXS_BondInternal& Bond)
{
	CVXS_Bond::operator=(Bond);

	SmallAngle = Bond.SmallAngle;

	return *this;
}

void CVXS_BondInternal::UpdateBond() //calculates force, positive for tension, negative for compression
{
	CalcLinForce();
}

void CVXS_BondInternal::ResetBond() //calculates force, positive for tension, negative for compression
{
	CVXS_Bond::ResetBond();
	SmallAngle = true;

	AxialForce1 = Vec3D<>(0,0,0);
	AxialForce2 = Vec3D<>(0,0,0);
	ShearForce1 = Vec3D<>(0,0,0);
	ShearForce2 = Vec3D<>(0,0,0);
	BendingForce1 = Vec3D<>(0,0,0);
	BendingForce2 = Vec3D<>(0,0,0);

	MidPoint = 0.5;
}

//sub force calculation types...
void CVXS_BondInternal::CalcLinForce() //get bond forces given positions, angles, and stiffnesses...
{
	Vec3D<double> CurXRelPos(pVox2->GetCurPosHighAccuracy() - pVox1->GetCurPosHighAccuracy()); //digit truncation happens here...
	CQuat<double> CurXAng1(pVox1->GetCurAngleHighAccuracy());
	CQuat<double> CurXAng2(pVox2->GetCurAngleHighAccuracy()); 
	ToXDirBond(&CurXRelPos);
	ToXDirBond(&CurXAng1);
	ToXDirBond(&CurXAng2);
	
	Vec3D<double> Ang1AlignedRelPos(CurXAng1.RotateVec3DInv(CurXRelPos)); //undo current voxel rotation to put in line with original bond according to Angle 1
	CQuat<double> NewAng2(CurXAng1.Conjugate()*CurXAng2); 

	//todo: lump into stress calculations
	double NomDistance;
	if (p_Sim->IsFeatureEnabled(VXSFEAT_VOLUME_EFFECTS)) NomDistance = L.x;
	else NomDistance = (pVox1->GetCurScale() + pVox2->GetCurScale())*0.5; //nominal distance between voxels


	bool ChangedSaState = false;
	vfloat SmallTurn = (abs(Ang1AlignedRelPos.z)+abs(Ang1AlignedRelPos.y))/Ang1AlignedRelPos.x;
	vfloat ExtendPerc = Ang1AlignedRelPos.x/NomDistance;
	if (!SmallAngle && NewAng2.IsSmallAngle() && SmallTurn < SA_BOND_BEND_RAD && ExtendPerc < SA_BOND_EXT_PERC){	SmallAngle = true; ChangedSaState=true;}
	else if (SmallAngle && (!NewAng2.IsSmallishAngle() || SmallTurn > VEC3D_HYSTERESIS_FACTOR*SA_BOND_BEND_RAD || ExtendPerc > VEC3D_HYSTERESIS_FACTOR*SA_BOND_EXT_PERC)){SmallAngle = false; ChangedSaState=true;}


	CQuat<> TotalRot;

	//Shear stuff!
	vfloat ShearStrainY=0;
	vfloat ShearStrainZ=0;

	if (SmallAngle)	{ //Align so Angle1 is all zeros
		_Angle1 = Vec3D<>(0,0,0);
		_Angle2 = NewAng2.ToRotationVector();
		Ang1AlignedRelPos.x -= NomDistance; //only valid for small angles
		_Pos2 = Ang1AlignedRelPos;
		TotalRot = CurXAng1.Conjugate();

		ShearStrainY =-_Pos2.y/L.x; //delta Y/l
		ShearStrainZ =-_Pos2.z/L.x; //delta Z/l

		//vfloat Phi = 12*E*I/(G*A*L*L);
		//ShearStrainY = -(_Pos2.y/L.x + _Angle2.z*Phi/2);

	}
	else { //Large angle. Align so that Pos2.y, Pos2.z are zero.
		CQuat<double> Pos2AlignedRotAng;
		Pos2AlignedRotAng.FromAngleToPosX(Ang1AlignedRelPos); //get the angle to align this with the X axis
		TotalRot = Pos2AlignedRotAng * CurXAng1.Conjugate();

		vfloat Length = CurXRelPos.Length(); //Ang1AlignedRelPos.x<0 ? -Ang1AlignedRelPos.Length() : Ang1AlignedRelPos.Length();
		_Pos2 = Vec3D<>(Length - NomDistance, 0, 0); //Small angle optimization target!!
//		Vec3D<> Pos2a = Pos2AlignedRotAng.RotateVec3D(Ang1AlignedRelPos); //high performance (but slow version) for special cases. should never crop up. (i.e. 1dof sim dragging voxel past each other)
//		_Pos2 = Vec3D<>(Pos2a.x - NomDistance, 0, 0); 
		
		_Angle1 = Pos2AlignedRotAng.ToRotationVector(); //these are currently a source of error of up to 1e-6
		_Angle2 = (TotalRot * CurXAng2).ToRotationVector();

//		ShearStrainY = tan(_Angle2.z-_Angle1.z); //tan theta
//		ShearStrainZ = -tan(_Angle2.y-_Angle1.y); //tan theta
		ShearStrainY = -tan(_Angle1.z); //tan theta
		ShearStrainZ = tan(_Angle1.y); //tan theta
	}

	UpdateBondStrain(_Pos2.x/L.x); //updates the bond parameters (yielded, broken, stress...) based on the current Strain

	//Beam equations! (all terms here, even though some are zero for small angle and large angle (negligible performance penalty)
//	if (p_Sim->IsFeatureEnabled(VXSFEAT_VOLUME_EFFECTS))
//		Force1 = Vec3D<> (	-CurStress*(CSArea1+CSArea2)/2,				-b1z*_Pos2.y + b2z*(_Angle1.z + _Angle2.z) - G*A*ShearStrainY,		-b1y*_Pos2.z - b2y*(_Angle1.y + _Angle2.y)/*+ G*A*ShearStrainZ*/); //Use Curstress instead of -a1*Pos2.x to account for non-linear deformation 
//	else	
///		Force1 = Vec3D<> (	-CurStress*(CSArea1+CSArea2)/2,				-b1z*_Pos2.y + b2z*(_Angle1.z + _Angle2.z),		-b1y*_Pos2.z - b2y*(_Angle1.y + _Angle2.y)); //Use Curstress instead of -a1*Pos2.x to account for non-linear deformation 
		Force1 = Vec3D<> (	CurStress*(CSArea1+CSArea2)/2,				b1z*_Pos2.y - b2z*(_Angle1.z + _Angle2.z),		b1y*_Pos2.z + b2y*(_Angle1.y + _Angle2.y)); //Use Curstress instead of -a1*Pos2.x to account for non-linear deformation 
	Force2 = -Force1;

#ifdef DEBUG
	AxialForce1 = Vec3D<>(Force1.x, 0, 0);
	AxialForce2 = Vec3D<>(Force2.x, 0, 0);
	BendingForce1 = Vec3D<>(0, Force1.y, Force1.z);
	BendingForce2 = Vec3D<>(0, Force2.y, Force2.z);
	ShearForce1 = ShearForce2 = Vec3D<>(0,0,0);

	if (p_Sim->IsFeatureEnabled(VXSFEAT_VOLUME_EFFECTS)){
		vfloat CA = G*(CSArea1+CSArea2)/2;
		//vfloat CA = G*A;
		Force1.y += CA*ShearStrainY;
		Force2.y -= CA*ShearStrainY;
		Force1.z += CA*ShearStrainZ;
		Force2.z -= CA*ShearStrainZ;


		ShearForce1 = Vec3D<>(0, CA*ShearStrainY, CA*ShearStrainZ);
		ShearForce2 = Vec3D<>(0, -CA*ShearStrainY, -CA*ShearStrainZ);

	}
#endif


	Moment1 = Vec3D<> (	a2*(_Angle1.x - _Angle2.x),		b2z*_Pos2.z + b3y*(2*_Angle1.y + _Angle2.y),	-b2y*_Pos2.y + b3z*(2*_Angle1.z + _Angle2.z));
	Moment2 = Vec3D<> (	a2*(_Angle2.x - _Angle1.x),		b2z*_Pos2.z + b3y*(_Angle1.y + 2*_Angle2.y),	-b2y*_Pos2.y + b3z*(_Angle1.z + 2*_Angle2.z));

	if (p_Sim->StatToCalc & CALCSTAT_STRAINE) StrainEnergy = CalcStrainEnergy(); //depends on Force1, Force2, Moment1, Moment2 being set!
	if (!ChangedSaState) AddDampForces();

	//Unrotate back to global coordinate system:
	//!!possible optimization: Do this after summing forces for a voxel!
	Force1 = TotalRot.RotateVec3DInv(Force1);
	if (!HomogenousBond) Force2 = TotalRot.RotateVec3DInv(Force2); 
	Moment1 = TotalRot.RotateVec3DInv(Moment1);
	Moment2 = TotalRot.RotateVec3DInv(Moment2);

	ToOrigDirBond(&Force1);
	if (HomogenousBond) Force2 = -Force1;
	else ToOrigDirBond(&Force2); //Added
	ToOrigDirBond(&Moment1);
	ToOrigDirBond(&Moment2);

#ifdef DEBUG
	AxialForce1 = TotalRot.RotateVec3DInv(AxialForce1);
	AxialForce2 = TotalRot.RotateVec3DInv(AxialForce2);
	ShearForce1 = TotalRot.RotateVec3DInv(ShearForce1);
	ShearForce2 = TotalRot.RotateVec3DInv(ShearForce2);
	BendingForce1 = TotalRot.RotateVec3DInv(BendingForce1);
	BendingForce2 = TotalRot.RotateVec3DInv(BendingForce2);
	ToOrigDirBond(&AxialForce1);
	ToOrigDirBond(&AxialForce2);
	ToOrigDirBond(&ShearForce1);
	ToOrigDirBond(&ShearForce2);
	ToOrigDirBond(&BendingForce1);
	ToOrigDirBond(&BendingForce2);
#endif

}

bool CVXS_BondInternal::UpdateBondStrain(vfloat CurStrainIn)
{
	CurStrainTot = CurStrainIn;
	bool IsPlasticityEnabled = p_Sim->IsFeatureEnabled(VXSFEAT_PLASTICITY);
	bool IsVolEffectsEnabled = p_Sim->IsFeatureEnabled(VXSFEAT_VOLUME_EFFECTS);

	//Single material optimize possibilities
	if (!IsPlasticityEnabled || CurStrainIn >= MaxStrain){ //if we're in new territory on the stress-strain curve or plasticity is not enabled...
		MaxStrain = CurStrainIn; //set the high-water mark on the strains
		bool CurYielded, CurBroken;

		if (HomogenousBond){
			if (IsVolEffectsEnabled){
		//		vfloat Eh = E/((1-2*u)*(1+u));
				CurStress = Eh*(1-u)*CurStrainIn + Eh*u*(TStrainSum1+TStrainSum2)/2; // - E*alpha*DTemp/(1-2*u);
			}
			else {
				CurStress = pVox1->CalcVoxMatStress(CurStrainIn, &CurYielded, &CurBroken);

			}
			CurStrainV1 = CurStrainV2 = CurStrainIn; //equal strains...
			MidPoint = 0.5;
			//if (!Yielded && CurYielded) SetYielded(); //if any part of the bond yielded, its all yielded
			//if (!Broken && CurBroken) SetBroken(); //if any part of the bond broke, its all broke

			//if (p_Sim->IsPlasticityEnabled()){RestDist = OrigDist*(1+(MaxStrain - CurStress/E));}	//Update the rest distance for the voxels...
		}
//		else if(LinearBond){ //TODO: linear, but different materials
//
//		}
		else { //TODO: very inefficient
			if (IsVolEffectsEnabled){
		//		vfloat Eh = E/((1-2*u)*(1+u));
				CurStress = Eh*(1-u)*CurStrainIn + Eh*u*(TStrainSum1+TStrainSum2)/2; // - E*alpha*DTemp/(1-2*u);
			}
			else {
				//All this to set CurStrainTot, bond yield/broken		
				vfloat Stress1, Stress2;
				bool Yielded1, Yielded2, Broken1, Broken2;
		

				CurStrainV1 = CurStrainIn; //initial guesses at strains (could be MUCH smarter based on current strains, but need to initialize correctly somewhere
				CurStrainV2 = CurStrainIn;

				//get stiffness of each voxel in question...
				Stress1 = pVox1->CalcVoxMatStress(CurStrainV1, &Yielded1, &Broken1);
				Stress2 = pVox2->CalcVoxMatStress(CurStrainV2, &Yielded2, &Broken2);

				int MaxCount = 3;
				int count = 0;
				vfloat StressDiff = (Stress1 >= Stress2) ? Stress1-Stress2 : Stress2-Stress1;
				vfloat StressSum = Stress1+Stress2;
				if (StressSum<0) StressSum = -StressSum;

				while (StressDiff > StressSum*.0005 && count<MaxCount){ //refine guesses at the strains until difference is less than .05% of average stress
					CurStrainV1 = 2*Stress2/(Stress1+Stress2)*CurStrainV1;
					CurStrainV2 = 2*Stress1/(Stress1+Stress2)*CurStrainV2;

					Stress1 = pVox1->CalcVoxMatStress(CurStrainV1, &Yielded1, &Broken1);
					Stress2 = pVox2->CalcVoxMatStress(CurStrainV2, &Yielded2, &Broken2);
					StressDiff = (Stress1 >= Stress2) ? Stress1-Stress2 : Stress2-Stress1; //recalc stressDiff
					StressSum = Stress1+Stress2;
					if (StressSum<0) StressSum = -StressSum;
					count++;
				}


				CurStress = (Stress1+Stress2)/2; //average just in case we didn't quite converge to the exact same stress
				CurYielded = Yielded1 || Yielded2;
				CurBroken = Broken1 || Broken2;

			//	if (!Yielded && (Yielded1 || Yielded2)) SetYielded(); //if any part of the bond yielded, its all yielded
			//	if (!Broken && (Broken1 || Broken2)) SetBroken(); //if any part of the bond broke, its all broke

			
	//			if (p_Sim->IsPlasticityEnabled()) RestDist = OrigDist*(1+(MaxStrain - CurStress/E)); //Update the rest distance for the voxels...

			}
		}

		if (!Yielded && CurYielded) SetYielded(); //if any part of the bond yielded, its all yielded
		if (!Broken && CurBroken) SetBroken(); //if any part of the bond broke, its all broke


		if (IsPlasticityEnabled) StrainOffset = MaxStrain - CurStress/E;

	}
	else { //if we've backed off a max strain and linearly drop back down at the elastic modulus
//		CurStress = E*(CurStrainIn-(sqrt(RestDist.Length2()/OrigDist.Length2())-1.0));
		CurStress = E*(CurStrainIn-StrainOffset);

	}

	
	//Todo: clean this up? see if its worth it by profiling
	if(IsVolEffectsEnabled && p_Sim->IsFeatureEnabled(VXSFEAT_TEMPERATURE)){ // pEnv->IsTempEnabled()){ 
		vfloat TempFact1 = 1.0, TempFact2 = 1.0;

//		vfloat ThisTemp1 = p_Sim->pEnv->pObj->GetBaseMat(pVox1->GetMaterial())->GetCurMatTemp();
//		vfloat ThisTemp2 = p_Sim->pEnv->pObj->GetBaseMat(pVox2->GetMaterial())->GetCurMatTemp();
		vfloat ThisTemp1 = pVox1->GetpMaterial()->GetCurMatTemp();
		vfloat ThisTemp2 = pVox2->GetpMaterial()->GetCurMatTemp();
		vfloat TempBase =  p_Sim->pEnv->GetTempBase();

		vfloat Stress1 = E1*CTE1*(ThisTemp1 - TempBase)/(1-2*u1);
		vfloat Stress2 = E2*CTE2*(ThisTemp2 - TempBase)/(1-2*u2);

		CurStress -= (Stress1 + Stress2)/2;
	
	}

	switch (ThisBondAxis){
	case AXIS_X: pVox1->SetStrainDir(BD_PX, CurStrainV1); pVox2->SetStrainDir(BD_NX, CurStrainV2); break;
	case AXIS_Y: pVox1->SetStrainDir(BD_PY, CurStrainV1); pVox2->SetStrainDir(BD_NY, CurStrainV2); break; 
	case AXIS_Z: pVox1->SetStrainDir(BD_PZ, CurStrainV1); pVox2->SetStrainDir(BD_NZ, CurStrainV2); break; 
	}

	return true;
}


void CVXS_BondInternal::AddDampForces() //Adds damping forces IN LOCAL BOND COORDINATES (with bond pointing in +x direction, pos1 = 0,0,0
{
	if (p_Sim->dt != 0){ //F = -cv, zeta = c/(2*sqrt(m*k)), c=zeta*2*sqrt(mk). Therefore, F = -zeta*2*sqrt(mk)*v. Or, in rotational, Moment = -zeta*2*sqrt(Inertia*angStiff)*w
		vfloat BondZ = 0.5*p_Sim->GetBondDampZ();
		vfloat _DtInv = 1.0/p_Sim->dt;
		Vec3D<> RelVel2((_Pos2-_LastPos2)*_DtInv);
		Vec3D<> RelAngVel1((_Angle1-_LastAngle1)*_DtInv);
		Vec3D<> RelAngVel2((_Angle2-_LastAngle2)*_DtInv);

		Force1 += BondZ*Vec3D<>(_2xSqA1xM1*RelVel2.x,
			_2xSqB1YxM1*RelVel2.y - _2xSqB2ZxFM1*(RelAngVel1.z+RelAngVel2.z),
			_2xSqB1ZxM1*RelVel2.z + _2xSqB2YxFM1*(RelAngVel1.y+RelAngVel2.y));
		if (!HomogenousBond){ //otherwise this is just negative of F1
			Force2 += BondZ*Vec3D<>(-_2xSqA1xM2*RelVel2.x,
				-_2xSqB1YxM2*RelVel2.y + _2xSqB2ZxFM2*(RelAngVel1.z+RelAngVel2.z),
				-_2xSqB1ZxM2*RelVel2.z - _2xSqB2YxFM2*(RelAngVel1.y+RelAngVel2.y)); 
		}
		//Force1 += BondZ*Vec3D<>(-_2xSqA1xM1*RelVel2.x,
		//	-_2xSqB1YxM1*RelVel2.y + _2xSqB2ZxFM1*(RelAngVel1.z+RelAngVel2.z),
		//	-_2xSqB1ZxM1*RelVel2.z - _2xSqB2YxFM1*(RelAngVel1.y+RelAngVel2.y));
		//if (!HomogenousBond){ //otherwise this is just negative of F1
		//	Force2 += BondZ*Vec3D<>(_2xSqA1xM2*RelVel2.x,
		//		_2xSqB1YxM2*RelVel2.y - _2xSqB2ZxFM2*(RelAngVel1.z+RelAngVel2.z),
		//		_2xSqB1ZxM2*RelVel2.z + _2xSqB2YxFM2*(RelAngVel1.y+RelAngVel2.y)); 
		//}
		Moment1 += 0.5*BondZ*Vec3D<>(	-_2xSqA2xI1*(RelAngVel2.x - RelAngVel1.x),
			_2xSqB2ZxFM1*RelVel2.z + _2xSqB3YxI1*(2*RelAngVel1.y + RelAngVel2.y),
			-_2xSqB2YxFM1*RelVel2.y + _2xSqB3ZxI1*(2*RelAngVel1.z + RelAngVel2.z));
		Moment2 += 0.5*BondZ*Vec3D<>(	_2xSqA2xI2*(RelAngVel2.x - RelAngVel1.x),
			_2xSqB2ZxFM2*RelVel2.z + _2xSqB3YxI2*(RelAngVel1.y + 2*RelAngVel2.y),
			-_2xSqB2YxFM2*RelVel2.y + _2xSqB3ZxI2*(RelAngVel1.z + 2*RelAngVel2.z));

	}
	_LastPos2 = _Pos2;
	_LastAngle1 = _Angle1;
	_LastAngle2 = _Angle2;
}