/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef VX_SIM_H
#define VX_SIM_H

#include "VXS_Voxel.h"
#include "VXS_BondInternal.h"
#include "VXS_BondCollision.h"
#include "VX_Environment.h"
#include "VX_MeshUtil.h"
#include <deque>
#include <vector>
#include <map>
#include <math.h>
//#ifdef USE_OPEN_GL
//#ifdef QT_GUI_LIB
//#include <qgl.h>
//#else
//#include "OpenGLInclude.h" //If not using QT's openGL system, make a header file "OpenGLInclude.h" that includes openGL library functions 
//#endif
//#endif

//#define MIN_TEMP_FACTOR 0.1 /*The smallest a voxel can go. (for stability reasons)*/
#define HISTORY_SIZE 500
//#define INPUT_VOX_INDEX -1

struct SimState { //Information about current simulation state:
	void Clear() {CurCM = TotalObjDisp = Vec3D<>(0,0,0); NormObjDisp = MaxVoxDisp = MaxVoxVel = MaxVoxKinE = MaxBondStrain = MaxBondStress = MaxBondStrainE = TotalObjKineticE = TotalObjStrainE = MaxPressure = MinPressure = 0.0;}
	Vec3D<> CurCM;
	std::vector< Vec3D<> > CMTrace;
	std::vector< vfloat > CMTraceTime;

	std::vector< Vec3D<vfloat> > VoxelIndexTrace;

    std::vector<vfloat> FloorTouchTrace;

	std::vector<vfloat> VoltageTrace;
	std::vector<vfloat> StrainTrace;
	std::vector<vfloat> StressTrace;
	std::vector<vfloat> PressureTrace;
	std::vector<vfloat> TouchTrace;
	std::vector<vfloat> RollTrace;
	std::vector<vfloat> PitchTrace;
	std::vector<vfloat> YawTrace;

	Vec3D<> TotalObjDisp; //a vector total of the magnitude of displacements
	vfloat NormObjDisp; //reduced to a scalar (magnitude) 
	vfloat MaxVoxDisp, MaxVoxVel, MaxVoxKinE, MaxBondStrain, MaxBondStress, MaxBondStrainE, MaxPressure, MinPressure;
	vfloat TotalObjKineticE, TotalObjStrainE;
};

//!Dynamic simulation class for time simulation of voxel objects.
/*!
To enable openGL rendering functions, define USE_OPEN_GL somewhere in the compiler's pre-processing routine.
*/
class CVX_Sim
{
public:
	CVX_Sim(void); //!< Constructor
	~CVX_Sim(void); //!< Destructor
	CVX_Sim& operator=(const CVX_Sim& rSim); //!< Overload "=" 

	//I/O function for save/loading
	void SaveVXAFile(std::string filename);
	bool LoadVXAFile(std::string filename, std::string* pRetMsg = NULL);

	void WriteVXA(CXML_Rip* pXML);
	bool ReadVXA(CXML_Rip* pXML, std::string* RetMessage = NULL);

	//I/O function for save/loading
	void WriteXML(CXML_Rip* pXML);
	bool ReadXML(CXML_Rip* pXML, std::string* RetMessage = NULL);
	virtual void WriteAdditionalSimXML(CXML_Rip* pXML) {};
	virtual bool ReadAdditionalSimXML(CXML_Rip* pXML, std::string* RetMessage = NULL) {return true;};


	//input information
	CVX_Environment* pEnv; //!< Pointer to the physical environment information. This variable is set on import() and should not be manually changed.
	CVX_Object LocalVXC; //!< Local copy of the voxel object. This copy is stored to ensure it never changes throughout the simulation. This variable is set on import() and should not be manually changed.

	//Simulation information
	std::vector<CVXS_Voxel> VoxArray; //!< The main array of voxels.
	bool UpdateAllVoxPointers(); //updates all pointers into the VoxArray (call if reallocated!)
	inline int NumVox(void) const {return (int)VoxArray.size();} //!< Returns the number of voxels in the simulation.


	std::vector<CVXS_BondInternal> BondArrayInternal; //!< The main array of bonds.
	std::vector<CVXS_BondCollision> BondArrayCollision; //!< collision bonds

	void UpdateAllBondPointers(); //updates all pointers into the VoxArray (call if reallocated!)
	inline int NumBond(void) const {return (int)BondArrayInternal.size();} //!< Returns the number of bonds in the simulation.
	inline int NumColBond(void) const {return (int)BondArrayCollision.size();} //!< Returns the number of bonds in the simulation.

	std::vector<int> XtoSIndexMap; //!< Maps the global CVX_Object index to the corresponding CVX_Sim voxel index.
	std::vector<int> StoXIndexMap; //!< Maps CVX_Sim voxel index to the original global CVX_Object index.
	int GetVoxIndex(int i, int j, int k) {return XtoSIndexMap[LocalVXC.GetIndex(i, j, k)];} //!< Returns the CVX_SIM voxel index at specified voxel location. If there is no instantiated voxel here -1 is returned. @param[in] i The X Voxel index of the desired voxel. @param[in] j The Y Voxel index of the desired voxel. @param[in] k The Z Voxel index of the desired voxel.

	//Simulation Management
	bool Import(CVX_Environment* pEnvIn = NULL, CMesh* pSurfMeshIn = NULL, std::string* RetMessage = NULL); //!< Imports a physical environment into the simulator.
	int CreatePermBond(int SIndexNegIn, int SIndexPosIn); //!< Creates a new permanent bond between two voxels. 
	int CreateColBond(int SIndex1In, int SIndex2In); //!< Creates a new collision bond between two voxels. 
//	bool UpdateBond(int BondIndex, int NewSIndex1In, int NewSIndex2In, bool LinkBond = true);

	void DeleteCollisionBonds(void); //!< Deletes all collision bonds.


	bool IsInitalized(void) const {return Initalized;} //!< Returns true if a valid environment has been imported.

	void ClearAll(void); //!< Clears all environment-specific information form the simulation.
	void ResetSimulation(void); //!< Resets the environment to its initial imported state.
	void SetVoxData(void);

	//Integration/simulation running
	bool TimeStep(std::string* pRetMessage = NULL); //!< Advances the simulation one time step. Calcstats moved to StatToCalc member bit flag
	vfloat CalcMaxDt(void); //!< Calculates the current maximum timestep based on the highest resonant frequency in the object.
	vfloat DtFrac; //percent of maximum dt to use
	vfloat OptimalDt; //calculated optimal dt
	vfloat dt; //actual seconds per timestep
	vfloat CurTime; //The current absolute time of the simulation in seconds.
	int CurStepCount;
	vfloat GetCurTime() {return CurTime;} //returns the current time
	bool DtFrozen; //it dt frozen?
	void DtFreeze(void) {OptimalDt = CalcMaxDt(); dt = DtFrac*OptimalDt; DtFrozen = true;}
	void DtThaw(void) {DtFrozen = false;}

	bool CmInitialized; //nac


	//Simulator features:

	void EnableFeature(const int VXSFEAT, bool Enabled=true);
	void DisableFeature(const int VXSFEAT, bool Enabled=false);
	bool IsFeatureEnabled(const int VXSFEAT) {return (CurSimFeatures & VXSFEAT);}

	//Self collision:

	std::vector<int> SurfVoxels; //A list of voxels that are on the surface (IE eligible for contact bonds...) (containts SIndex!)
	int NumSurfVoxels(void) {return (int)SurfVoxels.size();}; //how 
	void CalcL1Bonds(vfloat Dist); //creates contact bonds for all voxels within specified distance
	vfloat MaxDispSinceLastBondUpdate;


	//Damping:
	void SetBondDampZ(vfloat BondDampZIn) {BondDampingZ = BondDampZIn;} //!< Sets the damping ratio for connected voxels. When this is non-zero, each voxel is damped (based on its mass and stiffness) according to its relative velocity to the other voxel in each bond. Range is [0.0 to 1.0]. Values greater than 1.0 may cause numerical instability.
	void SetCollisionDampZ(vfloat ColDampZIn) {ColDampingZ = ColDampZIn;} //!< Sets the damping ratio for voxels in colliding state. When this is non-zero, each voxel is damped (based on its mass and stiffness) according to the penetration velocity. Range is [0.0 to 1.0]. Values greater than 1.0 may cause numerical instability.
	void SetSlowDampZ(vfloat SlowDampIn) {SlowDampingZ = SlowDampIn; } //!< Sets the damping ratio that slows downs voxels. When this is non-zero, each voxel is damped (based on its mass and stiffness) to ground. Range is [0.0 to 1.0]. Values greater than 1.0 may cause numerical instability.
	
	vfloat GetBondDampZ(void) {return BondDampingZ;} //!< Returns the current bond damping.
	vfloat GetCollisionDampZ(void) {return ColDampingZ;} //!< Returns the current collision damping.
	vfloat GetSlowDampZ(void) {return SlowDampingZ;} //!< Returns the current voxel slowing damping.


	int NumYielded(void);
	int NumBroken(void);

	//Material Blending
//	bool BlendingEnabled;
//	vfloat MixRadius;
	Vec3D<> MixRadius;
	MatBlendModel BlendModel;
	vfloat PolyExp; //polynomial exponent if using polynomial model

	vfloat GetMaxVoxVelLimit(void) {return MaxVoxVelLimit;}
	void SetMaxVoxVelLimit(vfloat NewMaxVoxVelLimit) {MaxVoxVelLimit = NewMaxVoxVelLimit;}

	//Equlibrium mode
	void ZeroAllMotion(void);
	bool MotionZeroed;

	//Stop conditions
//	void SetStopCondition(StopCondition StopConditionTypeIn = SC_NONE, vfloat StopConditionValueIn = 0.0) {SetStopConditionType(StopConditionTypeIn); SetStopConditionValue(StopConditionValueIn);}
	void SetStopConditionType(StopCondition StopConditionTypeIn = SC_NONE) {StopConditionType = StopConditionTypeIn;}
	void SetStopConditionValue(vfloat StopConditionValueIn = 0.0) {StopConditionValue = StopConditionValueIn;} 
	void SetInitCmTime(vfloat _InitCmTime= 0.0) {InitCmTime = _InitCmTime;}
	void SetActuationStartTime(vfloat ActuationStartTimeIn= 0.0) {ActuationStartTime = ActuationStartTimeIn;}
	StopCondition GetStopConditionType(void){return StopConditionType;}	
	vfloat GetStopConditionValue(void){return StopConditionValue;}
	vfloat GetInitCmTime(void){return InitCmTime;}
	vfloat GetActuationStartTime(void){return ActuationStartTime;}
	bool StopConditionMet(void); //have we met the stop condition yet?

	//Information about current state:
	SimState SS;
	int StatToCalc;

	Vec3D<> GetCM(void);
	Vec3D<> IniCM; //initial center of mass

	int GetNumTouchingFloor();

	Vec3D<> GetSumForce(CVX_FRegion* pRegion); //returns total force on a region
	vfloat GetSumForceDir(CVX_FRegion* pRegion); //returns total force on a region in the direction of its displacement
	Vec3D<> GetAvgDisplace(CVX_FRegion* pRegion); //returns the average displacement in x,y,z of a region

	// nac: for neural net
	// std::vector<double> inputNeuronValues;
	// std::vector<double> hiddenNeuronValues;
	// std::vector<double> hiddenNeuronValuesOld;
	// std::vector<double> outputNeuronValues;
	// std::vector<double> neuronValues;
	// std::vector<double> neuronValuesOld;
	// std::vector< std::vector<double> > synapseWeights;
	// int numTotalNeurons;

	vfloat MaxStressSoFar;
	vfloat MaxPressureSoFar;
	vfloat MinPressureSoFar;

	vfloat TimeOfLastRegenerationModelUpdate;
	vfloat TimeOfLastForwardModelUpdate;
	double GetAvgForwardModelError();

	float GetBlockPos();

	double GetAvgRoll();
	double GetAvgPitch();
	double GetAvgYaw();
	double GetAvgStress();
	double GetAvgPressure();

	double avgRoll;
	double avgPitch;
	double avgYaw;
	double avgStress;
	double avgPressure;

	bool GetHasFallenOver();
	bool FellOver;

	std::vector<vfloat> Rolls;
	std::vector<vfloat> Pitches;
	std::vector<vfloat> Yaws;
	vfloat TimeOfLastTiltVectorsUpdate;

	vfloat TimeOfLastControllerUpdate;
	bool UpdateControllerNow;

	vfloat TimeOfLastSignalingUpdate;
	bool UpdateSignalingNow;

	vfloat TimeOfLastOcclusionUpdate;
	std::vector<int> surfaceVoxels;

	// void InitializeSynpaseArray(void);

	// nac: for island fitness evals
	// std::vector<bool> ringsCompleted;
	// int stepsIn;
	// int stepsOut;
	// // int numTotalRings;
	// float ringRadius;
	// float interRingDistance;
	// float ringDifficulty;
	// // std::map<std::pair<float,float>,int> floorIsLava;
	// std::map< float, float > floorIsLava;
	float COMZ;
	int numSamples;

	double getMinTempFact(){ return MIN_TEMP_FACT; }

	// nac: for water (with mesh):
	// CVXS_SimGLView* pSimView;
	CVX_MeshUtil VoxMesh; // This mesh is never visualized, but is updated internally to compute forces (e.g. fluid-dynamics). There is another mesh, CVX_MeshUtil VoxMesh in VXS_SimGLView.h, that is the one actually displayed. The drawback is that we keep two meshes updated when using VoxCad (this does not happen when using voxelyze alone). On the other hand, this ensures that physics and view are decoupled.
	void setInternalMesh(CVX_MeshUtil* pMeshIn){ internalMesh = pMeshIn; }
	CVX_MeshUtil* getInternalMesh(){ return internalMesh; }
		
//	bool usingWater;
//	vfloat DragConstant;

	bool fluidEnvironment;
	float aggregateDragCoefficient;
	
	double getAverageStiffnessChange();

	// These are sometimes used in voxelyze main when we have a single evaluation broken down into two parts (e.g. altering gravity halfway), to get stats regarding the two parts
	double fitPhase1, fitPhase2, avgStiffChange1, avgStiffChange2;

	Vec3D<> getNormCOMdisplacement3D(){ return (SS.CurCM-IniCM)/LocalVXC.GetLatticeDim(); }
	float getNormCOMdisplacement(){ return getNormCOMdisplacement3D().Length(); }

protected:
	CVX_MeshUtil* internalMesh;

	double MIN_TEMP_FACT;

	bool Initalized; //!< Flag to denote if simulation is runnable. True if there is an environement successfully loaded, false otherwise.

	//Integration
	bool Integrate();
	bool UpdateStats(std::string* pRetMessage = NULL); //returns false if simulation diverged...

	StopCondition StopConditionType;

	vfloat StopConditionValue;
	vfloat InitCmTime;
	vfloat ActuationStartTime;

	void EnableEquilibriumMode(bool Enabled);
	
//	int numOutputs;
//	int numSensors;
//	int numHiddenPerLayer;
//	int numHiddenLayers;
//	int numHidden;


	//collision
	void UpdateCollisions(void); //!< Called every timestep to watch for collisions
	bool ColEnableChanged; //flag to erase all temporary (contact) bonds next simulation round or force collision update
	ColSystem CurColSystem;
	vfloat CollisionHorizon; //multiple of voxel dimension to add to potential collision bond list

	int CurSimFeatures;

	//temperature
	void UpdateMatTemps(void); //updates expansions for each material

	//Damping:
	vfloat BondDampingZ; //Damping factor zeta (1 = critical damping, 0 = no damping...
	vfloat ColDampingZ;	//Damping factor zeta (1 = critical damping, 0 = no damping...
	vfloat SlowDampingZ; //Damping factor zeta (1 = critical damping, 0 = no damping...

	//Velocity limit
	vfloat MaxVoxVelLimit; //in percentage of voxel base size per time step (1 = can move whole voxel distance in a single timestep)

	//Equilibrium mode
	bool KineticEDecreasing(void); //returns true if kinetic energy of the structure is decreasing
	vfloat MemBondDampZ, MemSlowDampingZ;
	bool MemMaxVelEnabled;

	void ClearHistories() {std::fill(KinEHistory.begin(), KinEHistory.end(), -1); std::fill(TotEHistory.begin(), TotEHistory.end(), -1); std::fill(MaxMoveHistory.begin(), MaxMoveHistory.end(), -1);}
	// std::deque<vfloat> KinEHistory; //value [0] is the newest...
	// std::deque<vfloat> TotEHistory;
	// std::deque<vfloat> MaxMoveHistory;

	public:
	std::deque<vfloat> KinEHistory; //value [0] is the newest...
	std::deque<vfloat> TotEHistory;
	std::deque<vfloat> MaxMoveHistory;


	CMesh* ImportSurfMesh; //local copy of any imported geometry surface mesh (optional)

//	bool MeshAutoGenerated; //flag to know whether to save the mesh or not...
//	CVX_MeshUtil SurfMesh; //local copy of any imported geometry surface mesh (optional)

//#ifdef USE_OPEN_GL
//	bool NeedStatsUpdate; //do we need to re-calculate statistics relevant to the opengl view? i.e. maximums...
//	bool ViewForce; //look at force vectors?
//	bool ViewAngles; //look at axes for each point?
//
//	CVX_MeshUtil VoxMesh; //voxel geometry mesh (generated internally)
//
//	void SetCurViewMode(ViewMode CurViewModeIn){CurViewMode=CurViewModeIn; NeedStatsUpdate=true;}
//	void SetCurViewCol(ViewColor CurViewColIn){CurViewCol=CurViewColIn; NeedStatsUpdate=true;}
//	void SetCurViewVox(ViewVoxel CurViewVoxIn){CurViewVox=CurViewVoxIn;}
//	ViewMode GetCurViewMode(void) {return CurViewMode;}
//	ViewColor GetCurViewCol(void) {return CurViewCol;}
//	ViewVoxel GetCurViewVox(void) {return CurViewVox;}
//	CColor GetJet(vfloat val){if (val > 0.75) return CColor(1, 4-val*4, 0, 1.0);	else if (val > 0.5) return CColor(val*4-2, 1, 0, 1.0); else if (val > 0.25) return CColor(0, 1, 2-val*4, 1.0); else return CColor(0, val*4, 1, 1.0);};
//	CColor GetCurVoxColor(int SIndex, int Selected);
//	CColor GetInternalBondColor(CVXS_BondInternal* pBond);
//	CColor GetCollisionBondColor(CVXS_BondCollision* pBond);
//
//
//	//Drawing
//	void Draw(int Selected = -1, bool ViewSection=false, int SectionLayer=0);
//	void DrawGeometry(int Selected = -1, bool ViewSection=false, int SectionLayer=0);
//	void DrawSurfMesh(int Selected = -1);
//	void DrawVoxMesh(int Selected = -1);
//	void DrawVoxHandles(int Selected = -1);
//	void DrawFloor(void);
//	void DrawBonds(void);
//	void DrawForce(void);
//	void DrawAngles(void);
//	void DrawStaticFric(void);
//
//	int StatRqdToDraw(); //returns the stats bitfield that we need to calculate to draw the current view.
//
//protected:
//	ViewMode CurViewMode;
//	ViewColor CurViewCol;
//	ViewVoxel CurViewVox;
//#endif
};

#endif //VX_SIM_H
