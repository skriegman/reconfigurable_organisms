/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef VXS_VOXEL_H
#define VXS_VOXEL_H

#include "VX_Voxel.h"
#include <iostream>
#include <math.h>

class CVXS_BondCollision;


//http://gafferongames.com/game-physics/physics-in-3d/

//info about a voxel that changes during a simulation (i.e. state)
class CVXS_Voxel : public CVX_Voxel
{
public:
	CVXS_Voxel(CVX_Sim* pSimIn, int SIndexIn, int XIndexIn, int MatIndexIn, Vec3D<>& NominalPositionIn, vfloat OriginalScaleIn);
	~CVXS_Voxel(void);
	CVXS_Voxel(const CVXS_Voxel& VIn) : CVX_Voxel(VIn) {*this = VIn;} //copy constructor
	CVXS_Voxel& operator=(const CVXS_Voxel& VIn);

	void ResetVoxel(); //resets this voxel to its default (imported) state.

	void EulerStep(); //updates the state of the voxel based on the current forces and moments.

	//Collisions
	bool LinkColBond(int CBondIndex); //collision bond index...
	void UpdateColBondPointers(); //updates all links (pointers) to bonds according top current p_Sim
	void UnlinkColBonds();

	//input information
	inline void SetInputForce(const Vec3D<>& InputForceIn) {InputForce = InputForceIn;} //adds a specified force to this voxel. Subsequent calls over-write this force. (Can be used in conjunction with picking in gui to drag voxels around)
	inline void ScaleExternalInputs(const vfloat ScaleFactor=1.0) {ExternalInputScale=ScaleFactor;} //scales force, torque, etc. to some percentage of its set value

	//Get info about the current state of this voxel
	const inline Vec3D<> GetCurPos() const {return Pos;}
	const inline Vec3D<double> GetCurPosHighAccuracy(void) const {return Pos;}
	const inline CQuat<> GetCurAngle() const {return Angle;}
	const inline CQuat<double> GetCurAngleHighAccuracy(void) const {return Angle;}
	const inline vfloat GetCurScale() const {return Scale;}
	const inline vfloat GetLastScale() const {return lastScale;}
	const inline Vec3D<> GetCurVel() const { return Vel;}
	const inline Vec3D<> GetCurAngVel() const {return AngVel;}
	const inline vfloat GetPressure() const {return Pressure;}
	const inline vfloat GetCurKineticE() const {return KineticEnergy;}
	const inline bool GetCurStaticFric() const {return StaticFricFlag;}
	const inline vfloat GetCurAbsDisp() const {return (Vec3D<>(Pos)-GetNominalPosition()).Length();}
	inline Vec3D<> GetSizeCurrent() const {return CornerPosCur-CornerNegCur;}
	inline Vec3D<> GetCornerPos() const {return CornerPosCur;}
	inline Vec3D<> GetCornerNeg() const {return CornerNegCur;}

	vfloat GetMaxBondStrain() const;
	vfloat GetMaxBondStrainE() const;
	vfloat GetMaxBondStress() const;
	vfloat GetCurGroundPenetration(); //how far into the ground penetrating (penetration is positive, no penetration is zero)
	Vec3D<> GetCurForce(bool forceRecalc = false) {if (forceRecalc) CalcTotalForce(); return ForceCurrent;} //just returns last calculated force

	//yeilded and broken flags
	inline void SetYielded(const bool Yielded) {VYielded = Yielded;}
	inline bool GetYielded() const {return VYielded;}
	inline void SetBroken(const bool Broken) {VBroken = Broken;}
	inline bool GetBroken() const {return VBroken;}

	//utilities
	inline void ZeroMotion() {LinMom = Vec3D<double>(0,0,0); AngMom = Vec3D<double>(0,0,0); Vel = Vec3D<>(0,0,0); AngVel = Vec3D<>(0,0,0); KineticEnergy = 0;}
	vfloat CalcVoxMatStress(const vfloat StrainIn, bool* const IsPastYielded, bool* const IsPastFail) const;

	//display color stuff
	void SetColor(float r, float g, float b, float a);

	//Poissons!
	void SetStrainDir(BondDir Bond, vfloat StrainIn);
	Vec3D<> StrainPosDirsCur, StrainNegDirsCur; //cache the strain in each bond direction
	vfloat GetVoxelStrain(Axis DesiredAxis);

	bool inRing;
	bool oldInRing;

	float TempAmplitude;
	float TempPeriod;
	float phaseOffset;
	float finalPhaseOffset;

    // scale
	float initialVoxelSize;
	float finalVoxelSize;
	float currSize;
	double getNewScale();

    // stiffness
    float evolvedStiffness;
	float minElasticMod;
	float maxElasticMod;
	float maxStiffnessVariation;
	float stressAdaptationRate;
	float pressureAdaptationRate;
	float AdjTempAmp;

    // controller
    double oldMotorOutput;
    void UpdateController();
    std::vector<double> ControllerNeuronValues;

    // regeneration model
    double currRegenModelOutput;
    float GrowthDirection;
    double lastSurprise;
    double currSurprise;
    float RegenTimeLeft;
    float SurpriseAccretion;
    float GrowthAccretion;
	void UpdateRegenerationModel();
	void UpdateGreedyGrowth();
	void UpdateRegenerationNetwork();
	std::vector<double> RegenerationModelNeuronValues;

	double VestibularContribution;
	double PreDamageRoll;
	double PreDamagePitch;
	double PreDamageYaw;

	double StressContribution;
	double PreDamageStress;

	double PressureContribution;
	double PreDamagePressure;

    // forward model
	int VoxNum;
	void UpdateForwardModel();
	void UpdateForwardModel_With_Signaling();
	void UpdateForwardModel_Without_Signaling();
	std::vector<double> ForwardModelNeuronValues;
	double oldForwardModelError;
	double currentForwardModelError;
	double ForwardModelErrorIntegral;

	vfloat GetActualSensorData();

	// Proprioception
	// https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
	const inline double GetRoll(void) const
	{
	    return asin(2.0*(Angle.w*Angle.y - Angle.z*Angle.x));
	}
    const inline double GetPitch(void) const
    {
        return atan2(2.0*(Angle.w*Angle.x + Angle.y*Angle.z), 1.0 - 2.0*(Angle.x*Angle.x + Angle.y*Angle.y));
    }
    const inline double GetYaw(void) const
    {
        return atan2(2.0*(Angle.w*Angle.z + Angle.x*Angle.y), 1.0 - 2.0*(Angle.y*Angle.y + Angle.z*Angle.z));
    }

    // Exteroception
    double LightIntensity;
    bool isThrowingShadeOn(Vec3D<>);

    // Interoception
    vfloat CalcVoxelPressure();
    // stain and stress functions defined above


	// Electrical Activity:
	bool ElectricallyActiveOld;
	bool ElectricallyActiveNew;
	float MembranePotentialOld;
	float MembranePotentialNew;
	float RepolarizationStartTime;
    double Voltage;
	void UpdateElectricalSignaling();

	Vec3D<> DragForce;

	void SetEMod(vfloat Vox_E_in);
	double getCurStiffnessChange(){ return ((Vox_E-evolvedStiffness)/evolvedStiffness)*100; }

private:
	//State variable of this voxel being simulated
	Vec3D<double> Pos; //translation
	Vec3D<double> LinMom;
	CQuat<double> Angle; //rotation
	Vec3D<double> AngMom;
	vfloat Scale; //nominal scale based on temperature, etc.
	vfloat lastScale;
	Vec3D<> CornerPosCur, CornerNegCur; //actual size based on volume effects or other deviations from Scale
	bool StaticFricFlag; //flag to set if this voxel shouldnot move in X/Y due to being in static friction regime
	bool VYielded, VBroken; //is this voxel yielded or brokem according to the current material model?
		
	//convenience derived secondary state quantities:
	Vec3D<> Vel;
	vfloat KineticEnergy;
	Vec3D<> AngVel;
	vfloat Pressure, Stress;

	vfloat StressIntegral;
	vfloat PressureIntegral;

	//current input parameters
	vfloat ExternalInputScale; //Scales the external Force, displacement, torque (range: 0-1)
	Vec3D<> InputForce; //current input force (general purpose force acting on this voxel)

	//collision system information
	std::vector<int> ColBondInds; //collision bond indices
	std::vector<CVXS_BondCollision*> ColBondPointers; //collision bond pointers
	inline bool IAmVox2Col(const int BondDirColIndex) const; //returns true if this voxel is Vox2 of the specified bond

	//current display color of this voxel.
	float m_Red, m_Green, m_Blue, m_Trans; //can update voxel color based on state, mode, etc.

	//force calculations of this voxel
	Vec3D<> ForceCurrent; //cached current force, as last calculated by CalcTotalForce()

	Vec3D<> CalcTotalForce(); //calculates the total force acting on this voxel (without fixed constraints...)
	Vec3D<> CalcTotalMoment(); //Calculates the total moment action on this voxel

	//secondary force calculations
	Vec3D<> CalcFloorEffect(Vec3D<> TotalVoxForce); //calculates the object's interaction with a floor under gravity
	Vec3D<> CalcGndDampEffect(); //damps everything to ground as quick as possible...

};

#endif //VXS_VOXEL_H