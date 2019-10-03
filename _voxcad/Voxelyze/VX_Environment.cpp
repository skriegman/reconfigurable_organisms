/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "VX_Environment.h"

#ifdef USE_OPEN_GL
#ifdef QT_GUI_LIB
#include <qgl.h>
#else
#include "OpenGLInclude.h" //If not using QT's openGL system, make a header file "OpenGLInclude.h" that includes openGL library functions 
#endif
#include "Utils/GL_Utils.h"
#endif

CVX_Environment::CVX_Environment(void)
{
	pObj = NULL;

	GravEnabled = true;
	GravAcc = -9.81; //m/s^2
	AlterGravityHalfway = 1.0;
	
	FloorEnabled = true;

	TempEnabled = true;
	VaryTempEnabled = false;
	TempAmplitude = 0;
	TempBase = 25; //degress celcius
	TempPeriod = 0.1; //in seconds
	CurTemp = 25;

	FallingProhibited = false;
	DampEvolvedStiffness = false;

	fluidEnvironment = 0;
	aggregateDragCoefficient = 0.0;
	FloorSlope = 0.0;
	FloorSlopeEnabled = true; // when false, it masks any floor slope that is eventually present
}

CVX_Environment::~CVX_Environment(void)
{
}

void CVX_Environment::SaveBCXFile(std::string filename)
{
	CXML_Rip XML;
	WriteXML(&XML);
	XML.SaveFile(filename);
}

bool CVX_Environment::LoadBCXFile(std::string filename)
{
	CXML_Rip XML;
	if (!XML.LoadFile(filename)) return false;
	ReadXML(&XML);
	return true;
}



void CVX_Environment::WriteXML(CXML_Rip* pXML)
{
	pXML->DownLevel("Environment");
		pXML->DownLevel("Boundary_Conditions");
			pXML->Element("NumBCs", (int)BCs.size());
			for (int i=0; i<(int)BCs.size(); i++) BCs[i].WriteXML(pXML);
		pXML->UpLevel();

		//pXML->DownLevel("Fixed_Regions");
		//pXML->Element("NumFixed", (int)Fixed.size());
		//for (int i=0; i<(int)Fixed.size(); i++){
		//	Fixed[i].WriteXML(pXML);
		//}
		//pXML->UpLevel();

		//pXML->DownLevel("Forced_Regions");
		//pXML->Element("NumForced", (int)Forced.size());
		//for (int i=0; i<(int)Forced.size(); i++){
		//	Forced[i].WriteXML(pXML);
		//}
		//pXML->UpLevel();

		pXML->DownLevel("Gravity");
		pXML->Element("GravEnabled", GravEnabled);
		pXML->Element("GravAcc", GravAcc);
		pXML->Element("AlterGravityHalfway", AlterGravityHalfway);
		
		pXML->Element("FloorEnabled", FloorEnabled);

		pXML->UpLevel();

		pXML->DownLevel("Thermal");
		pXML->Element("TempEnabled", TempEnabled);
		pXML->Element("TempAmplitude", TempAmplitude);
		pXML->Element("TempBase", TempBase);
		pXML->Element("VaryTempEnabled", VaryTempEnabled);
		pXML->Element("TempPeriod", TempPeriod);
		pXML->UpLevel();
	pXML->UpLevel();
	
//	if(!OnlyBCs) pObj->WriteXML(pXML);
}

bool CVX_Environment::ReadXML(CXML_Rip* pXML, std::string* RetMessage) //pXML pointer to an "Environment" element
{
	ClearBCs();

	if (pXML->FindElement("Boundary_Conditions")){ //if newest version
		int NumRegions;
		CVX_FRegion tmpRegion;

		if (!pXML->FindLoadElement("NumBCs", &NumRegions)) NumRegions = 0;
		for (int i=0; i<NumRegions; i++){
			pXML->FindElement("FRegion");
			tmpRegion.ReadXML(pXML);
			BCs.push_back(tmpRegion);
		}
		if (NumRegions != 0) pXML->UpLevel(); //FRegion
		pXML->UpLevel(); //Boundary_Conditions
	}
	else { //Load separate fixed/forced regions if an old version... (Legacy support code)
		//Load fixed Regions
		if (pXML->FindElement("Fixed_Regions")){
			int NumFixedRegions;
			CVX_FRegion tmpRegion;

			if (!pXML->FindLoadElement("NumFixed", &NumFixedRegions)) NumFixedRegions = 0;
			for (int i=0; i<NumFixedRegions; i++){
				pXML->FindElement("FRegion");
				tmpRegion.ReadXML(pXML);
				BCs.push_back(tmpRegion);
			}
			if (NumFixedRegions != 0) pXML->UpLevel(); //FRegion
			pXML->UpLevel(); //Fixed_Regions
		}
	
		//Load forced Regions
		if (pXML->FindElement("Forced_Regions")){
			int NumForcedRegions;
			CVX_FRegion tmpRegion;

			if (!pXML->FindLoadElement("NumForced", &NumForcedRegions)) NumForcedRegions = 0;
			for (int i=0; i<NumForcedRegions; i++){
				pXML->FindElement("FRegion");
				tmpRegion.ReadXML(pXML);
				BCs.push_back(tmpRegion);
			}
			if (NumForcedRegions != 0) pXML->UpLevel(); //FRegion
			pXML->UpLevel(); //Fixed_Regions
		}
	}

	if (pXML->FindElement("Gravity")){
		if (!pXML->FindLoadElement("GravEnabled", &GravEnabled)) GravEnabled = false;
		if (!pXML->FindLoadElement("GravAcc", &GravAcc)) GravAcc = -9.81;
		if (!pXML->FindLoadElement("AlterGravityHalfway", &AlterGravityHalfway)) AlterGravityHalfway = 1.0;

		if (!pXML->FindLoadElement("FloorEnabled", &FloorEnabled)) FloorEnabled = false;
		
		if (!pXML->FindLoadElement("FloorSlope", &FloorSlope)) FloorSlope = 0.0;

		if (FloorSlope >= 90.0)
		{
			FloorSlope = 89.0;
			std::cout << "VX_Environment.cpp : FloorSlope >= 90, truncated to 89.0" << std::endl;
		}
		else if (FloorSlope <= -90.0)
		{
			FloorSlope = -89.0;
			std::cout << "VX_Environment.cpp : FloorSlope <= -90, truncated to -89.0" << std::endl;
		}

		pXML->UpLevel();
	}

	if (pXML->FindElement("Thermal")){
		if (!pXML->FindLoadElement("TempEnabled", &TempEnabled)) TempEnabled = false;
		if (!pXML->FindLoadElement("TempBase", &TempBase)) TempBase = 25;
		if (!pXML->FindLoadElement("TempAmplitude", &TempAmplitude)){ //if we don't find TempAmplitude, possible older TempAmp setting
			vfloat TempAmp;
			if (pXML->FindLoadElement("TempAmp", &TempAmp)) TempAmplitude = TempAmp-TempBase;
			else TempAmplitude = 0;
		}
		

		if (!pXML->FindLoadElement("VaryTempEnabled", &VaryTempEnabled)) VaryTempEnabled = false;
		if (!pXML->FindLoadElement("TempPeriod", &TempPeriod)) TempPeriod = 0.1;
		CurTemp = TempBase;

		pXML->UpLevel();
	}

	if (pXML->FindElement("NeuralNet")){
		if (!pXML->FindLoadElement("NeuralNetUpdatesPerTempCycle", &NeuralNetUpdatesPerTempCycle)) NeuralNetUpdatesPerTempCycle = 0.0;
		if (!pXML->FindLoadElement("TouchSensorsEnabled", &TouchSensorsEnabled)) TouchSensorsEnabled = false;
		if (!pXML->FindLoadElement("ProprioceptionSensorsEnabled", &ProprioceptionSensorsEnabled)) ProprioceptionSensorsEnabled = false;
		if (!pXML->FindLoadElement("PacemakerSensorsEnabled", &PacemakerSensorsEnabled)) PacemakerSensorsEnabled = false;
		if (!pXML->FindLoadElement("NumHiddenNeuronsPerLayer", &NumHiddenNeuronsPerLayer)) NumHiddenNeuronsPerLayer = 0;
		if (!pXML->FindLoadElement("NumHiddenLayers", &NumHiddenLayers)) NumHiddenLayers = 0;
		if (!pXML->FindLoadElement("OutputSmoothing", &outputSmoothing)) outputSmoothing = 0;

		pXML->UpLevel();
	}

	if (pXML->FindElement("RegenerationModel")){
	    if (!pXML->FindLoadElement("TiltVectorsUpdatesPerTempCycle", &TiltVectorsUpdatesPerTempCycle)) TiltVectorsUpdatesPerTempCycle = 0.0;
		if (!pXML->FindLoadElement("RegenerationModelUpdatesPerTempCycle", &RegenerationModelUpdatesPerTempCycle)) RegenerationModelUpdatesPerTempCycle = 0.0;
		if (!pXML->FindLoadElement("NumHiddenRegenerationNeurons", &NumHiddenRegenerationNeurons)) NumHiddenRegenerationNeurons = 2;
		if (!pXML->FindLoadElement("RegenerationModelInputBias", &RegenerationModelInputBias)) RegenerationModelInputBias = false;
		pXML->UpLevel();
	}

	if (pXML->FindElement("ForwardModel")){
		if (!pXML->FindLoadElement("ForwardModelUpdatesPerTempCycle", &ForwardModelUpdatesPerTempCycle)) ForwardModelUpdatesPerTempCycle = 0.0;
		pXML->UpLevel();
	}

	if (pXML->FindElement("Controller")){
		if (!pXML->FindLoadElement("ControllerUpdatesPerTempCycle", &ControllerUpdatesPerTempCycle)) ControllerUpdatesPerTempCycle = 0.0;
		pXML->UpLevel();
	}

	if (pXML->FindElement("Signaling")){
	    if (!pXML->FindLoadElement("DepolarizationsPerTempCycle", &DepolarizationsPerTempCycle)) DepolarizationsPerTempCycle = 1.0;
	    if (!pXML->FindLoadElement("RepolarizationsPerTempCycle", &RepolarizationsPerTempCycle)) RepolarizationsPerTempCycle = 1.0;
		if (!pXML->FindLoadElement("SignalingUpdatesPerTempCycle", &SignalingUpdatesPerTempCycle)) SignalingUpdatesPerTempCycle = 0.0;
		pXML->UpLevel();
	}

	if (pXML->FindElement("LightSource")){
	    if (!pXML->FindLoadElement("X", &lightX)) lightX = 0.0;
	    if (!pXML->FindLoadElement("Y", &lightY)) lightY = 0.0;
		if (!pXML->FindLoadElement("Z", &lightZ)) lightZ = 0.0;
		pXML->UpLevel();
	}

	if (!pXML->FindLoadElement("GrowthAmplitude", &growthAmplitude)) growthAmplitude = 0;
	if (!pXML->FindLoadElement("GrowthSpeedLimit", &GrowthSpeedLimit)) GrowthSpeedLimit = 0;

	if (!pXML->FindLoadElement("GreedyGrowth", &GreedyGrowth)) GreedyGrowth = false;
	if (!pXML->FindLoadElement("GreedyThreshold", &GreedyThreshold)) GreedyThreshold = 0;

	if (!pXML->FindLoadElement("TimeBetweenTraces", &TimeBetweenTraces)) TimeBetweenTraces = 0.0;
	if (!pXML->FindLoadElement("SavePassiveData", &SavePassiveData)) SavePassiveData = false;

	if (!pXML->FindLoadElement("FluidEnvironment", &fluidEnvironment)) fluidEnvironment = false;
	if (!pXML->FindLoadElement("AggregateDragCoefficient", &aggregateDragCoefficient)) aggregateDragCoefficient = 0.0;

	if (!pXML->FindLoadElement("FallingProhibited", &FallingProhibited)) FallingProhibited = false;

	if (!pXML->FindLoadElement("DampEvolvedStiffness", &DampEvolvedStiffness)) DampEvolvedStiffness = false;

	if (!pXML->FindLoadElement("BlockPushing", &BlockPushing)) BlockPushing = false;
	if (!pXML->FindLoadElement("BlockMaterial", &BlockMaterial)) BlockMaterial = 0;

	if (!pXML->FindLoadElement("ContractOnly", &ContractOnly)) ContractOnly = false;
	if (!pXML->FindLoadElement("ExpandOnly", &ExpandOnly)) ExpandOnly = false;
	
	return true;
}

/*! All voxels touching this region will be immobile in the simulation.
@param[in] Location The corner of the region closest to the origin. Specified as a percentage (in X, Y, and Z respectively) of the overall workspace. (Location.x, Location.y, and Location.z each have a range of [0.0 to 1.0]).
@param[in] Size The size of the region. Specified as a percentage (in X, Y, and Z respectively) of the overall workspace. (Size.x, Size.y, and Size.z each have a range of [0.0 to 1.0]).
*/
void CVX_Environment::AddFixedBc(const Vec3D<>& Location, const Vec3D<>& Size, char DofToFix) //fixes a region of voxels
{
	CVX_FRegion MyRegion;
	MyRegion.CreateBoxRegion(Location, Size);
	MyRegion.SetColor(0.0f, 0.0f, 0.0f, 1.0f);
	SET_FIXED(DofToFix, MyRegion.DofFixed, true);
	BCs.push_back(MyRegion);
}

/*! All voxels touching this region will have an external force applied to them. The provided Force vector will be divided equally between all voxels.
@param[in] Location The corner of the region closest to the origin. Specified as a percentage (in X, Y, and Z respectively) of the overall workspace. (Location.x, Location.y, and Location.z each have a range of [0.0 to 1.0]).
@param[in] Size The size of the region. Specified as a percentage (in X, Y, and Z respectively) of the overall workspace. (Size.x, Size.y, and Size.z each have a range of [0.0 to 1.0]).
@param[in] Force The force to be distributed across this region in Newtons. The force is divided equally among all voxels in the forced region.
*/
void CVX_Environment::AddForcedBc(const Vec3D<>& Location, const Vec3D<>& Size, const Vec3D<>& Force, const Vec3D<>& Torque) //applies a force to a region of voxels
{
	CVX_FRegion MyRegion;
	MyRegion.CreateBoxRegion(Location, Size);
	MyRegion.SetColor(0.0f, 0.0f, 0.0f, 1.0f);
	SET_FIXED(DOF_ALL, MyRegion.DofFixed, false);
	MyRegion.Force = Force;
	MyRegion.Torque = Torque;
	BCs.push_back(MyRegion);
}

//void CVX_Environment::ClearBCs(void) //clears all fixed and forced regions
//{
//	Fixed.clear();
//	Forced.clear();
//}

int CVX_Environment::GetNumTouching(int BCIndex)
{
	CPrimitive* pThisPrim = BCs[BCIndex].GetRegion();
	
	Vec3D<> BCpoint;
	Vec3D<> BCsize = pObj->GetLatDimEnv()/2.0;
	Vec3D<> WSSize = pObj->GetWorkSpace();
	int NumTouching = 0;

	int StArraySize = pObj->GetStArraySize();
	for (int i=0; i<StArraySize; i++){
		if (pObj->GetMat(i) == 0) continue; //if there's no voxel here keep going...

		BCpoint = pObj->GetXYZ(i);
		if (pThisPrim->IsTouching(&BCpoint, &BCsize, &WSSize)) NumTouching++;
	}
	return NumTouching;
}




void CVX_Environment::RemoveDisconnected(void) //removes regions not connected to a fully grounded region... (and sets fixed list...) ALSO removes lone voxels (no bonds...)
{
	int NumFixed = 0;

	int NumVox = pObj->GetNumVox(); //number of voxels

	int* List = NULL; //list of voxels to expand
	List = new int[NumVox];
	int ListEnd = 0; //list iterators
	int ListBegin = 0;

	bool* Visited = NULL; //size of full voxel array, set to true if connected!
	Visited = new bool [pObj->GetStArraySize()];

	//Pupoluate fixed...
//	int DOFInd = 0;
	Vec3D<> point;
	Vec3D<> size = pObj->GetLatDimEnv()/2.0;
	Vec3D<> WSSize = pObj->GetWorkSpace();

	for (int i=0; i<pObj->GetStArraySize(); i++){ //for all posible voxels:
		Visited[i] = false;

		if (pObj->GetMat(i) != 0){ //if there's a voxel here...
			point = pObj->GetXYZ(i);

			//For each
			for (int j = 0; j<(int)BCs.size(); j++){ //go through each primitive defined as a constraint!
				if (!IS_ALL_FIXED(BCs[j].DofFixed)) continue;
				else if (Visited[i]) continue;
				else if (BCs[j].GetRegion()->IsTouching(&point, &size, &WSSize)){ //if this point is within
					NumFixed++;
					List[ListEnd++] = i; //add to our list to check later...
					Visited[i] = true;
					continue;
				}
			}
//			DOFInd++;
		}
		
	}

	//go through the list, adding neighbors to the list until we run out...
	int ThisIndex, ThatIndex;
	int x, y, z;
	bool FixedAlone;

	while (ListBegin != ListEnd){
		ThisIndex = List[ListBegin++];

		pObj->GetXYZNom(&x, &y, &z, ThisIndex);
		
		FixedAlone = true; //flag to see if this is a lone grounded one with no bonds... (true unless find a bond...)

		for (int i=0; i<6; i++){ //6 possible directions to find cube
			switch (i){ //go through each dimension
				case 0: ThatIndex = pObj->GetIndex(x+1, y, z); break; //+X direction
				case 1: ThatIndex = pObj->GetIndex(x-1, y, z); break; //-X direction
				case 2: ThatIndex = pObj->GetIndex(x, y+1, z); break; //+Y direction
				case 3: ThatIndex = pObj->GetIndex(x, y-1, z); break; //-Y direction
				case 4: ThatIndex = pObj->GetIndex(x, y, z+1); break; //+Z direction
				case 5: ThatIndex = pObj->GetIndex(x, y, z-1); break; //-Z direction
			}

			if (ThatIndex != -1 && pObj->GetMat(ThatIndex) != 0){ //if its there
				if (Visited[ThatIndex] == false){ //if we want to add it to the list
					Visited[ThatIndex] = true; //we've hit it now!
					List[ListEnd++] = ThatIndex;
				}
				FixedAlone = false; //we found a bond!
			}

			
		}

		if (FixedAlone) Visited[ThisIndex] = false; //flag for removal...
	}


	for (int i=0; i<pObj->GetStArraySize(); i++){ //for all posible voxels:
		if (Visited[i] == false) //if we never got there
			pObj->SetMat(i, 0); //erase non-connected material!
	}

	delete[] List;
	List = NULL;
	delete[] Visited;
	Visited = NULL;
}

void CVX_Environment::UpdateCurTemp(vfloat time, CVX_Object* pUpdateInObj)
{
	CVX_Object* pObjUpdate = pObj;
	if (pUpdateInObj) pObjUpdate = pUpdateInObj; //necessary b/c of how simulation is set up with a local un-modifiable CVX_Object

	if (VaryTempEnabled){
		if (TempPeriod == 0) return; //avoid NaNs.
		CurTemp = TempBase + TempAmplitude*sin(2*3.1415926/TempPeriod*time);	//update the global temperature
		for (int i = 0; i<(int)pObjUpdate->GetNumMaterials(); i++){ //now update the individual temperatures of each material (they can each have a different temperature)
			pObjUpdate->GetBaseMat(i)->SetCurMatTemp(TempBase + TempAmplitude*sin((2*3.1415926f/TempPeriod) * time + pObjUpdate->GetBaseMat(i)->GetMatTempPhase()));	//and update each one
		}
	}
	else {
		CurTemp = TempBase + TempAmplitude;
		for (int i = 0; i<pObj->GetNumMaterials(); i++){ //for each material...
			pObjUpdate->GetBaseMat(i)->SetCurMatTemp(CurTemp); //...update each one
		}
	}

}


#ifdef USE_OPEN_GL
void CVX_Environment::DrawBCs(int Selected) //draws the current boundary conditions
{
	Vec3D<> WS = pObj->GetWorkSpace();
	for (int j=0; j<(int)BCs.size(); j++){ //draw BCs
		int MyGLindex = BC_GLIND_OFF+j;
		if (IS_FIXED(DOF_ALL, BCs[j].DofFixed)){ //if anything is fixed, color it greens
			if (Selected == MyGLindex) BCs[j].SetColor(0.0, 1.0, 0.0, 1.0); //selected
			else BCs[j].SetColor(0.4, 0.6, 0.4, 1.0); //unselected
		}
		else { //no DOF fixed, color it purples.
			if (Selected == MyGLindex) BCs[j].SetColor(1.0, 0.0, 1.0, 1.0); //selected
			else BCs[j].SetColor(0.6, 0.4, 0.6, 1.0); //unselected
		}

		glLoadName (MyGLindex); //to enable picking
		CPrimitive& Reg = *BCs[j].GetRegion();
		BCs[j].DrawScaled(&WS);

		//todo:
		if (BCs[j].Displace.Length2() != 0){
			Vec3D<> NForce = BCs[j].Displace/BCs[j].Displace.Length()*pObj->GetLatticeDim()*1.5; //normalize to 1.5 vox length
			for (int x=0; x<2; x++){ for (int y=0; y<2; y++){ for (int z=0; z<2; z++){
				Vec3D<> CurCorner((Reg.X+x*Reg.dX)*WS.x, (Reg.Y+y*Reg.dY)*WS.y, (Reg.Z+z*Reg.dZ)*WS.z);
				CGL_Utils::DrawArrow(CurCorner, NForce, CColor(0.2, 0.8, 0.2, 1.0));
			}}}
		}

		if (BCs[j].Force.Length2() != 0){
			Vec3D<> NForce = BCs[j].Force/BCs[j].Force.Length()*pObj->GetLatticeDim()*1.5; //normalize to 1.5 vox length
			for (int x=0; x<2; x++){ for (int y=0; y<2; y++){ for (int z=0; z<2; z++){
				Vec3D<> CurCorner((Reg.X+x*Reg.dX)*WS.x, (Reg.Y+y*Reg.dY)*WS.y, (Reg.Z+z*Reg.dZ)*WS.z);
				CGL_Utils::DrawArrow(CurCorner, NForce, CColor(0.8, 0.2, 0.8, 1.0));
			}}}
		}
	}

	//for (int j=0; j<(int)Fixed.size(); j++){ //draw fixed constraints
	//	int MyGLindex = BCFIXED_GLIND_OFF+j;
	//	if (Selected == MyGLindex) Fixed[j].SetColor(0.0, 1.0, 0.0, 1.0); //selected
	//	else Fixed[j].SetColor(0.4, 0.6, 0.4, 1.0); //unselected

	//	glLoadName (MyGLindex); //to enable picking
	//	CPrimitive& Reg = *Fixed[j].pRegion;
	//	Fixed[j].DrawScaled(&WS);

	//	//todo:
	//	if (Fixed[j].Displace.Length2() != 0){
	//		Vec3D NForce = Fixed[j].Displace/Fixed[j].Displace.Length()*pObj->GetLatticeDim()*1.5; //normalize to 1.5 vox length
	//		for (int x=0; x<2; x++){ for (int y=0; y<2; y++){ for (int z=0; z<2; z++){
	//			Vec3D CurCorner = Vec3D((Reg.X+x*Reg.dX)*WS.x, (Reg.Y+y*Reg.dY)*WS.y, (Reg.Z+z*Reg.dZ)*WS.z);
	//			CGL_Utils::DrawArrow(CurCorner, NForce, CColor(0.5, 0.5, 0.5, 1.0));
	//		}}}
	//	}
	//}
	//for (int j=0; j<(int)Forced.size(); j++){ //draw force constraints
	//	int MyGLindex = BCFORCE_GLIND_OFF+j;
	//	if (Selected == MyGLindex) Forced[j].SetColor(1.0, 0.0, 1.0, 1.0); //selected
	//	else Forced[j].SetColor(0.6, 0.4, 0.6, 1.0); //unselected

	//	glLoadName (MyGLindex); //to enable picking
	//	CPrimitive& Reg = *Forced[j].pRegion;
	//	Forced[j].DrawScaled(&WS);

	//	//todo:
	//	Vec3D NForce = Forced[j].Force/Forced[j].Force.Length()*pObj->GetLatticeDim()*1.5; //normalize to 1.5 vox length
	//	for (int x=0; x<2; x++){ for (int y=0; y<2; y++){ for (int z=0; z<2; z++){
	//		Vec3D CurCorner = Vec3D((Reg.X+x*Reg.dX)*WS.x, (Reg.Y+y*Reg.dY)*WS.y, (Reg.Z+z*Reg.dZ)*WS.z);
	//		CGL_Utils::DrawArrow(CurCorner, NForce, CColor(0.5, 0.5, 0.5, 1.0));
	//	}}}
	//}
}

#endif

#ifdef USE_DEPRECATED

void CVX_Environment::GetNumVoxTouchingForced(std::vector<int>* pNumsOut)
{
	int NumForced = GetNumForced();
	int StArraySize = pObj->GetStArraySize();
	Vec3D BCpoint;
	Vec3D BCsize = pObj->GetLatDimEnv()/2.0;
	Vec3D WSSize = pObj->GetWorkSpace();

	pNumsOut->clear();
	*pNumsOut = std::vector<int>(NumForced, 0);
	for (int i=0; i<StArraySize; i++){
		if (pObj->GetMat(i) == 0) continue; //if there's no voxel here keep going...

		BCpoint = pObj->GetXYZ(i);
		for (int j=0; j<NumForced; j++){ //for each forcing region
			if (Forced[j].pRegion->IsTouching(&BCpoint, &BCsize, &WSSize)) (*pNumsOut)[j]++;
		}
	}
}

#endif
