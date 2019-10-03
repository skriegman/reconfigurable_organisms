/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef CVX_ENVIRONMENT_H
#define CVX_ENVIRONMENT_H

#include "VX_FRegion.h"
#include "VX_Object.h"
#include <iostream>

#define BC_GLIND_OFF 100000000
#define BCFORCE_GLIND_OFF 100000000 /*Old*/
#define BCFIXED_GLIND_OFF 100001000 /*Old*/

//!Describes physical environment of voxel object	
/*!Contains information about the boundary conditions (fixed and forced voxels) as well as parameters fro gravity, temperature, etc.*/
class CVX_Environment
{
public:
	CVX_Environment(void);
	~CVX_Environment(void);

	void AddObject(CVX_Object* pObjIn) {pObj = pObjIn;} //!< Links a voxel object to this environment. Only one voxel object may be linked at a time. @param[in] pObjIn Pointer to an initialized voxel object to link to this simulation.
	CVX_Object* pObj; //link to the object in this environment

	void SaveBCXFile(std::string filename);
	bool LoadBCXFile(std::string filename);

	//writes just local information
	void WriteXML(CXML_Rip* pXML);
	bool ReadXML(CXML_Rip* pXML, std::string* RetMessage = NULL);

	//Boundary conditions
	int AddBC(CVX_FRegion* pRegion){BCs.push_back(*pRegion); return BCs.size()-1;} //<! Copies and existing region into the Environment, returns the index @param[in] pRegion Pointer to region to copy and add to the Environment.
	void DelBC(int BCIndex){if(BCIndex>=0 && BCIndex<(int)BCs.size()) BCs.erase(BCs.begin()+BCIndex);} //!< Deletes a region at the specified index. @param[in] BCIndex Index to delete.
	void ClearBCs(void) {BCs.clear();} //!< Clears all regions
	int GetNumBCs(void) {return (int)BCs.size();} //!< Returns the number of boundary condition regions currently in the environment.
	int GetNumFixedBCs(void) {int Num=0; for(int i=0; i<GetNumBCs(); i++){if (IS_ALL_FIXED(GetBC(i)->DofFixed)) Num++;} return Num;} //!< Returns the number of boundary conditions where every DOF is fixed.
	CVX_FRegion* GetBC(int BCIndex) {if(BCIndex>=0 && BCIndex<(int)BCs.size()) return &BCs[BCIndex]; else return NULL;} //!< Returns a temporary pointer to the fixed region at the specified index. @param[in] BCIndex Index of boundary condition region.
	int GetNumTouching(int BCIndex); //!< Returns the number of voxels touching this region. @param[in] BCIndex Index of boundary condition region.
	void RemoveDisconnected(void); //!< Removes all voxels in the voxel object not connected to a fully fixed voxel. This is necessary to create non-singular matrices for static FEA analysis.

	//Convenience functions
	void AddFixedBc(const Vec3D<>& Location, const Vec3D<>& Size, char DofToFix = DOF_ALL); //!< Adds a region of voxels to be fixed to ground. @param[in] Location Unitless location of the minimum extreme of a box region in the range of [0, 1], to be scaled in each dimension by the bounding box size. @param[in] Size Unitless size of the region in the range of [0, 1], to be scaled in each dimension by the bounding box size. @param[in] DofToFix Indicates which degrees of freedom to fix.
	void AddForcedBc(const Vec3D<>& Location, const Vec3D<>& Size, const Vec3D<>& Force, const Vec3D<>& Torque); //!< Adds a region of voxels with the specified external force applied. @param[in] Location Unitless location of the minimum extreme of a box region in the range of [0, 1], to be scaled in each dimension by the bounding box size. @param[in] Size Unitless size of the region in the range of [0, 1], to be scaled in each dimension by the bounding box size. @param[in] Force The force in Newtons to be applied to this region. @param[in] Torque The torque in Newton meters to be applied to this region.


	//Other environment variables:
	//Gravity
	void EnableGravity(bool Enabled) {GravEnabled = Enabled;} //!< Enables or disables gravity. @param[in] Enabled Enable gravity.
	bool IsGravityEnabled(void) {return GravEnabled;} //!< Returns true if gravity is currently enabled.
	void SetGravityAccel(vfloat GravAccelIn) {GravAcc = GravAccelIn;} //!< Set the acceleration of gravity. @param[in] GravAccelIn Desired gravitational acceleration in m/s^2.
	vfloat GetGravityAccel(void){return GravAcc;} //!< Return current acceleration of gravity.

	void SetAlterGravityHalfway(vfloat AlterGravityHalfwayIn){ AlterGravityHalfway = AlterGravityHalfwayIn; }
	vfloat GetAlterGravityHalfway(void){ return AlterGravityHalfway; }


	void EnableFloor(bool Enabled) {FloorEnabled = Enabled;} //!< Enables or disables a simple floor. @param[in] Enabled Enable the floor.
	bool IsFloorEnabled(void) {return FloorEnabled;} //!< Returns true if the floor is currently enabled.
	
	void EnableTemp(bool Enabled) {TempEnabled = Enabled;} //!< Enables or disables temperature factors. @param[in] Enabled Enables temperature factors.
	bool IsTempEnabled(void) {return TempEnabled;} //!< Returns true if temperature sensitivity is currently enabled.
	void EnableTempVary(bool Enabled) {VaryTempEnabled = Enabled;} //!< Enables or disables temperature variation according to TempAmplitude, TempPeriod, etc. @param[in] Enabled Enables periodic temperature fluctuation.
	bool IsTempVaryEnabled(void) {return VaryTempEnabled;} //!< Returns true if temperature variation is currently enabled.

	void SetTempBase(vfloat TempBaseIn) {TempBase = TempBaseIn;} //!< Set the base temperature at which no expansion or contraction is present. For instance, room temperature. @param[in] TempBaseIn Desired base temperature in degrees celcius.
	vfloat GetTempBase(void) {return TempBase;} //!< Return current base temperature.
	void SetTempAmplitude(vfloat TempAmplitudeIn) {TempAmplitude = TempAmplitudeIn;} //!< Set the amplitude of periodic temperature variations in.  @param[in] TempAmplitudeIn Desired temperature variation amplitude in degrees celcius.
	vfloat GetTempAmplitude(void) {return TempAmplitude;} //!< Return periodic temperature variation amplitude.
	void SetTempPeriod(vfloat TempPeriodIn) {TempPeriod = TempPeriodIn;} //!< Set the period at which temperature varies. @param[in] TempPeriodIn Desired temperature variation period in seconds.
	vfloat GetTempPeriod(void) {return TempPeriod;} //!< Return current base temperature.

	void UpdateCurTemp(vfloat time, CVX_Object* pUpdateInObj = NULL); //!< Updates the current temperature based on provided simulation time.
	vfloat GetCurTemp() {return CurTemp;} //!< Returns the current temperature of the environment. (degrees C)

	float GetNeuralNetUpdatesPerTempCycle(void) {return NeuralNetUpdatesPerTempCycle;} //!< Returns true if neural net is currently enabled (nac).
	bool IsTouchSensorsEnabled(void) {return TouchSensorsEnabled;} //!< Returns true if neural net is currently enabled (nac).
	bool IsProprioceptionSensorsEnabled(void) {return ProprioceptionSensorsEnabled;} //!< Returns true if neural net is currently enabled (nac).
	bool IsPacemakerSensorsEnabled(void) {return PacemakerSensorsEnabled;} //!< Returns true if neural net is currently enabled (nac).
	int GetNumHiddenNeuronsPerLayer(void) {return NumHiddenNeuronsPerLayer;} //!< Returns true if neural net is currently enabled (nac).
	int GetNumHiddenLayers() {return NumHiddenLayers;} //!< Returns true if neural net is currently enabled (nac).
	float GetOutputSmoothing(void) {return outputSmoothing;} // nac: limits slope out output neuron to that of the average rate of change for "x" cycles per time period

    float GetTiltVectorsUpdatesPerTempCycle(void) {return TiltVectorsUpdatesPerTempCycle;}
    float GetRegenerationModelUpdatesPerTempCycle(void) {return RegenerationModelUpdatesPerTempCycle;}
    float GetForwardModelUpdatesPerTempCycle(void) {return ForwardModelUpdatesPerTempCycle;}
    float GetControllerUpdatesPerTempCycle(void) {return ControllerUpdatesPerTempCycle;}

    float GetSignalingUpdatesPerTempCycle(void) {return SignalingUpdatesPerTempCycle;}
    float GetDepolarizationsPerTempCycle(void) {return DepolarizationsPerTempCycle;}
    float GetRepolarizationsPerTempCycle(void) {return RepolarizationsPerTempCycle;}

	double getGrowthAmplitude(){ return growthAmplitude; } // relative to nominal scale
	double getGrowthSpeedLimit(){ return GrowthSpeedLimit; }

	bool getUsingGreedyGrowth(){return GreedyGrowth;}
	double getGreedyThreshold(){return GreedyThreshold;}

	int getNumHiddenRegenerationNeurons(){return NumHiddenRegenerationNeurons;}
	bool getUsingRegenerationModelInputBias(){return RegenerationModelInputBias;}

    Vec3D<> getLightSource(){ return Vec3D<>(lightX, lightY, lightZ);}

	bool getUsingSavePassiveData() { return SavePassiveData;}
	vfloat getTimeBetweenTraces() { return TimeBetweenTraces; }

	bool GetFluidEnvironment(){ return fluidEnvironment; }
	float GetAggregateDragCoefficient(){ return aggregateDragCoefficient; }

	bool IsFallingProhibited(){return FallingProhibited;}

	bool getUsingDampEvolvedStiffness(){return DampEvolvedStiffness;}

	bool IsPushingBlock(){return BlockPushing;}
	int GetBlockMaterial(){return BlockMaterial;}

	bool IsContractOnly(){return ContractOnly;}
	bool IsExpandOnly(){return ExpandOnly;}

	float GetFloorSlope(){ return FloorSlopeEnabled ? FloorSlope : 0; }
	void SetFloorSlope(float v){ FloorSlope = v; }

	bool GetFloorSlopeEnabled() { return FloorSlopeEnabled; }
	void SetFloorSlopeEnabled(bool b) {FloorSlopeEnabled = b; }
	
	// bool getUsingPhaseOffset(void) {return getUsingPhaseOffset;}

#ifdef USE_OPEN_GL
	void DrawBCs(int Selected); //draws the current boundary conditions
#endif

private:
	std::vector<CVX_FRegion> BCs; //Boundary (actually, volume) conditions

	// bool usingPhaseOffset;

	bool fluidEnvironment;
	float aggregateDragCoefficient;

	bool FallingProhibited;

    bool ContractOnly;
    bool ExpandOnly;

	bool BlockPushing;
	int BlockMaterial;

	bool DampEvolvedStiffness;

	bool GravEnabled;
	vfloat GravAcc; //m/s^2
	float AlterGravityHalfway;
	bool FloorEnabled; //do we want a floor system?
	vfloat FloorSlope;
	bool FloorSlopeEnabled;
	bool TempEnabled; //overall flag for temperature calculations
	bool VaryTempEnabled; //is periodic variation of temperature on?
	vfloat TempBase, TempAmplitude, TempPeriod; //degress celcius

	vfloat CurTemp; //updated based on time... (for phase 0... individual materials now have their own current temp

	float outputSmoothing;
	float NeuralNetUpdatesPerTempCycle;
	bool TouchSensorsEnabled;
	bool ProprioceptionSensorsEnabled;
	bool PacemakerSensorsEnabled;
	int NumHiddenNeuronsPerLayer;
	int NumHiddenLayers;

    float TiltVectorsUpdatesPerTempCycle;
    float RegenerationModelUpdatesPerTempCycle;
	float ForwardModelUpdatesPerTempCycle;
	float ControllerUpdatesPerTempCycle;

	float SignalingUpdatesPerTempCycle;
	float DepolarizationsPerTempCycle;
	float RepolarizationsPerTempCycle;

	float lightX;
	float lightY;
	float lightZ;

	double growthAmplitude;
	double GrowthSpeedLimit;

	bool GreedyGrowth;
	double GreedyThreshold;

	int NumHiddenRegenerationNeurons;
	bool RegenerationModelInputBias;

    bool SavePassiveData;
	vfloat TimeBetweenTraces;


#ifdef USE_DEPRECATED
public:
	void AddFixedRegion(Vec3D Location, Vec3D Size) {AddFixedBc(Location, Size);} //!< Adds a region of voxels to be fixed to ground. @param[in] Location Unitless location of the minimum extreme of a box region in the range of [0, 1], to be scaled in each dimension by the bounding box size. @param[in] Size Unitless size of the region in the range of [0, 1], to be scaled in each dimension by the bounding box size.
	void AddForcedRegion(Vec3D Location, Vec3D Size, Vec3D Force) {AddForceBc(Location, Size, Force);} //!< Adds a region of voxels with the specified external force applied. @param[in] Location Unitless location of the minimum extreme of a box region in the range of [0, 1], to be scaled in each dimension by the bounding box size. @param[in] Size Unitless size of the region in the range of [0, 1], to be scaled in each dimension by the bounding box size. @param[in] Force The force in Newtons to be applies to this region.



//	std::vector <CVX_FRegion> Fixed;
//	std::vector <CVX_FRegion> Forced;

//	void AddFixedRegion(CVX_FRegion* pRegion){CVX_FRegion tmp = *pRegion; tmp.Fixed = true; Fixed.push_back(tmp);} //!< DEPRECATED. Adds a pre-exisiting region as fixed. @param[in] pRegion Pointer to an initialized regions to be added as a fixed region.
//	void DelFixedRegion(int Index){if(Index>=0 && Index<(int)Fixed.size()) Fixed.erase(Fixed.begin()+Index);} //!< DEPRECATED. Deletes a fixed region at the specified index in the Fixed region array. @param[in] Index Index of Fixed array to delete.
//	void AddForcedRegion(CVX_FRegion* pRegion){CVX_FRegion tmp = *pRegion; tmp.Fixed = false; Forced.push_back(tmp);} //!< DEPRECATED. Adds a pre-exisiting region as forced with zero force. @param[in] pRegion Pointer to an initialized regions to be added as a forced region.
//	void DelForcedRegion(int Index){if(Index>=0 && Index<(int)Forced.size()) Forced.erase(Forced.begin()+Index);} //!< DEPRECATED. Deletes a forced region at the specified index in the Forced region array. @param[in] Index Index of Forced array to delete.
//	int GetNumFixed(void) {return (int)Fixed.size();} //!< DEPRECATED. Returns the number of fixed regions currently in the environment.
//	CVX_FRegion* GetFixedRegion(int FixedIndex) {if(FixedIndex>=0 && FixedIndex<(int)Fixed.size()) return &Fixed[FixedIndex]; else return NULL;} //!< DEPRECATED. Returns a temporary pointer to the fixed region at the specified index. @param[in] index Index of Fixed array element to return a pointer to.
//	int GetNumForced(void) {return (int)Forced.size();} //!< DEPRECATED. Returns the number of forced regions currently in the environment.
//	CVX_FRegion* GetForcedRegion(int ForcedIndex) {if(ForcedIndex>=0 && ForcedIndex<(int)Forced.size()) return &Forced[ForcedIndex]; else return NULL;}//!< DEPRECATED. Returns a temporary pointer to the forced  region at the specified index. @param[in] ForcedIndex Index of Forced array element to return a pointer to.
//	void GetNumVoxTouchingForced(std::vector<int>* pNumsOut); //DEPRECATED. 
#endif

};

#endif //CVX_ENVIRONMENT_H
