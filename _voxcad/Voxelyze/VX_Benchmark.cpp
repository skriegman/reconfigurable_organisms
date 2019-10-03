/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "VX_Benchmark.h"
#include "VX_Sim.h"

CVX_Benchmark::CVX_Benchmark(void)
{

}

CVX_Benchmark::~CVX_Benchmark(void)
{

}

CVX_Benchmark& CVX_Benchmark::operator=(const CVX_Benchmark& rBenchmark) //overload "=" 
{

	return *this;
}

//TODO BiStrip curl-up (look for phantom torques)
//non-linear materials
//pure twisting action

bool CVX_Benchmark::AxialSimpleTest()
{
	int BeamLength = 20;
	vfloat VoxSize = 0.001; //1mm
	vfloat Force = 0.00003; //1mN
	vfloat E = 1000000;

	//Materials
	CVXC_Material ThisMat = CVXC_Material("1Mpa", 0.5, 0.,5, 0.5, E, 0.35);
	ThisMat.SetDensity(1.0);

	//Object
	CVX_Object ThisObj;
	ThisObj.InitializeMatter(VoxSize, BeamLength, 1, 1);
	ThisObj.ClearPalette();
	int Mat1Ind = ThisObj.AddMat(ThisMat);
	ThisObj.SetMatFill(Mat1Ind);

	//Environment
	CVX_Environment ThisEnv;
	ThisEnv.AddObject(&ThisObj);
	ThisEnv.AddFixedBc(Vec3D<>(0,0,0), Vec3D<>(VoxSize, VoxSize, VoxSize));
	ThisEnv.AddForcedBc(Vec3D<>((BeamLength-1)*VoxSize, 0, 0), Vec3D<>(VoxSize, VoxSize, VoxSize), Vec3D<>(-Force,0,0), Vec3D<>(0,0,0));
	ThisEnv.EnableGravity(false);
	ThisEnv.EnableFloor(false);
	ThisEnv.EnableTemp(false);
	ThisEnv.EnableTempVary(false);

	//Simulation
	CVX_Sim ThisSim;
	ThisSim.Import(&ThisEnv);
	ThisSim.EnableFeature(VXSFEAT_COLLISIONS, false);
	ThisSim.EnableFeature(VXSFEAT_FAILURE, false);
	ThisSim.EnableFeature(VXSFEAT_PLASTICITY, false);
	ThisSim.EnableFeature(VXSFEAT_MAX_VELOCITY, false);

//	ThisSim.EnableSelfCollision(false);
//	ThisSim.EnableFailure(false);
//	ThisSim.EnablePlasticity(false);
//	ThisSim.EnableMaxVelLimit(false);

	ThisSim.SetBondDampZ(1.0);
	ThisSim.SetSlowDampZ(0.001);

	//Analytical solution: PL^3/3EI, = 4*P*(Num)^3/E*VS
	vfloat NomDisp = 4*Force*(BeamLength-1)*(BeamLength-1)*(BeamLength-1)/(E*VoxSize); //.00027436

	return true;
}
