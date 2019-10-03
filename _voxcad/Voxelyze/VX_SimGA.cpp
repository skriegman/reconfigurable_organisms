/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "VX_SimGA.h"
#include <iostream>

CVX_SimGA::CVX_SimGA()
{
	Fitness = 0.0f;
	TrackVoxel = 0;
	FitnessFileName = "";
//	print_scrn = false;
	WriteFitnessFile = false;
	FitnessType = FT_NONE;	//no reporting is default

}

void CVX_SimGA::SaveResultFile(std::string filename, CVX_SimGA* simToCombine)
{
	CXML_Rip XML;
	WriteResultFile(&XML, simToCombine);
	XML.SaveFile(filename);
}


void CVX_SimGA::WriteResultFile(CXML_Rip* pXML, CVX_SimGA* simToCombine)
{
// 	float totalPoints = 0;
// 	float goodSteps = 0;
// 	float badSteps = 0;
// 	for (std::map< float, float >::iterator it = floorIsLava.begin(); it != floorIsLava.end(); it++ )
// 	{
// 		totalPoints += it->second;
// 		if (it->second > 0 ) {goodSteps++;}
// 		if (it->second < 0 ) {badSteps++;}
// 	}
// 	// std::cout << "totalPoints: " << totalPoints << std::endl;
	//float dist = pow(pow(SS.CurCM.x-IniCM.x,2)+pow(SS.CurCM.y-IniCM.y,2),0.5);	


	Vec3D<> normCOMdisplacement = getNormCOMdisplacement3D();

	float normTotalDisplacement = (pEnv->IsFallingProhibited() && FellOver) ? 0.0 : normCOMdisplacement.Length();

	float normDistX = normCOMdisplacement.x;
	float normDistY = normCOMdisplacement.y;
	float normDistZ = normCOMdisplacement.z;
	int numVoxels = NumVox();
	float avgStiffnessChange = getAverageStiffnessChange();
	float avgForwardModelError = GetAvgForwardModelError();

	float blockPos = GetBlockPos();

	double integratedTiltError = 0.0;
	if (Rolls.size() > 0){
        for (std::vector<vfloat>::size_type i = 0; i != Rolls.size(); ++i){
	        integratedTiltError += fabs(Rolls[i]) + fabs(Pitches[i]) + fabs(Yaws[i]);
	    }
	    integratedTiltError /= Rolls.size();
    }

	if (GetActuationStartTime() == 0)
	{
	    avgRoll = GetAvgRoll();
	    avgPitch = GetAvgPitch();
	    avgYaw = GetAvgYaw();
	    avgStress = GetAvgStress();
	    avgPressure = GetAvgPressure();
	}


	if(simToCombine) // This is used when two SimGA objects are passed to this function, and results should be combined (same individual evaluated in two different environments)
	{
		//std::cout << "COMBINING FITNESS VALUES IN WRITERESULTSFILE" << std::endl;
		Vec3D<> normCOMdisplacement2 = simToCombine->getNormCOMdisplacement3D();
		CVX_MeshUtil* internalMesh2 = simToCombine->getInternalMesh();

		normTotalDisplacement = (normTotalDisplacement + normCOMdisplacement2.Length())/2;
		normDistX  			  = (normDistX  +  normCOMdisplacement2.x)/2;
		normDistY  			  = (normDistY  +  normCOMdisplacement2.y)/2;
		normDistZ  			  = (normDistZ  +  normCOMdisplacement2.z)/2;
		numVoxels 			  = (numVoxels	+  simToCombine->NumVox())/2;
		avgStiffnessChange 	  = (avgStiffnessChange   + simToCombine->getAverageStiffnessChange())/2;
	}


	pXML->DownLevel("Voxelyze_Sim_Result");
	pXML->SetElAttribute("Version", "1.0");
	pXML->DownLevel("Fitness");
	pXML->Element("VoxelNumber", numVoxels);
	pXML->Element("normAbsoluteDisplacement", normTotalDisplacement);
	
	pXML->Element("avgForwardModelError", avgForwardModelError);

	pXML->Element("BlockPos", blockPos);

	pXML->Element("avgRoll", avgRoll);
	pXML->Element("avgPitch", avgPitch);
	pXML->Element("avgYaw", avgYaw);
	pXML->Element("avgStress", avgStress);
	pXML->Element("avgPressure", avgPressure);

    pXML->Element("integratedTiltError", integratedTiltError);

	pXML->Element("avgProprioceptiveError", avgRoll + avgPitch + avgYaw );

	pXML->Element("avgInteroceptiveError", avgStress + avgPressure);

	pXML->Element("AverageStiffnessChange", avgStiffnessChange);

	pXML->Element("normDistX", normDistX);
	pXML->Element("normDistY", normDistY);
	pXML->Element("normDistZ", normDistZ);

	//if(pEnv->GetAlterGravityHalfway() != 1.0) // Saving extra stats that allow to break down some of the stats of the compound evaluation
	//{
		pXML->Element("normAbsDistPhase1", fitPhase1);
		pXML->Element("normAbsDistPhase2", fitPhase2);
		pXML->Element("avgStiffChange1", avgStiffChange1);
		pXML->Element("avgStiffChange2", avgStiffChange2);
	//}
	
	pXML->UpLevel();

	if (SS.CMTraceTime.size() > 0)
    {
        pXML->DownLevel("CMTrace");
            for(std::vector<vfloat>::size_type i = 0; i != SS.CMTraceTime.size(); ++i)
            {
                pXML->DownLevel("TraceStep");
                    pXML->Element("Time",SS.CMTraceTime[i]);
                    pXML->Element("TraceX",SS.CMTrace[i].x);
                    pXML->Element("TraceY",SS.CMTrace[i].y);
                    pXML->Element("TraceZ",SS.CMTrace[i].z);
                    pXML->Element("NumTouchingGround",SS.FloorTouchTrace[i]);
                pXML->UpLevel();
            }
        pXML->UpLevel();

//        pXML->DownLevel("SensorMotorData");
//        for(std::vector<vfloat>::size_type i = 0; i != SS.VoxelIndexTrace.size(); ++i)
//        {
//            pXML->DownLevel("Voxel");
//                pXML->Element("X", SS.VoxelIndexTrace[i].x);
//                pXML->Element("Y", SS.VoxelIndexTrace[i].y);
//                pXML->Element("Z", SS.VoxelIndexTrace[i].z);
//                pXML->Element("Voltage", SS.VoltageTrace[i]);
//                pXML->Element("Strain", SS.StrainTrace[i]);
//                pXML->Element("Stress", SS.StressTrace[i]);
//                pXML->Element("Pressure", SS.PressureTrace[i]);
//                pXML->Element("Touch", SS.TouchTrace[i]);
//                pXML->Element("Roll", SS.RollTrace[i]);
//                pXML->Element("Pitch", SS.PitchTrace[i]);
//                pXML->Element("Yaw", SS.YawTrace[i]);
//            pXML->UpLevel();
//        }
//        pXML->UpLevel();
    }

	pXML->UpLevel();

	// std::cout << "dist: " << dist/LocalVXC.GetLatticeDim()  << std::endl;
	// // std::cout << "height: " << COMZ << std::endl;
	// std::cout << "fitness: " << dist/LocalVXC.GetLatticeDim() * COMZ << std::endl;

}

void CVX_SimGA::WriteAdditionalSimXML(CXML_Rip* pXML)
{
	pXML->DownLevel("GA");
		pXML->Element("Fitness", Fitness);
		pXML->Element("FitnessType", (int)FitnessType);
		pXML->Element("TrackVoxel", TrackVoxel);
		pXML->Element("FitnessFileName", FitnessFileName);
		pXML->Element("WriteFitnessFile", WriteFitnessFile);
	pXML->UpLevel();
}

bool CVX_SimGA::ReadAdditionalSimXML(CXML_Rip* pXML, std::string* RetMessage)
{
	if (pXML->FindElement("GA")){
		int TmpInt;
		if (!pXML->FindLoadElement("Fitness", &Fitness)) Fitness = 0;
		if (pXML->FindLoadElement("FitnessType", &TmpInt)) FitnessType=(FitnessTypes)TmpInt; else Fitness = 0;
		if (!pXML->FindLoadElement("TrackVoxel", &TrackVoxel)) TrackVoxel = 0;
		if (!pXML->FindLoadElement("FitnessFileName", &FitnessFileName)) FitnessFileName = "";
//		if (!pXML->FindLoadElement("WriteFitnessFile", &WriteFitnessFile)) WriteFitnessFile = true;
		pXML->UpLevel();
	}

	return true;
}