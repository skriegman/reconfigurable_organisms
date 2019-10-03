#include <iostream>
#include "VX_Object.h"
#include "VX_Environment.h"
#include "VX_Sim.h"
#include "VX_SimGA.h"


int main(int argc, char *argv[])
{
	char* InputFile;
	bool print_scrn = false;
	bool compoundTerrestrialEnvironment = false;

	std::string fitnessFileName = "";

	//bool twoGravityLevels = false;
	//float gravityMultiplier = 0.0;

	//first, parse inputs. Use as: -f followed by the filename of the .vxa file that describes the simulation. Can also follow this with -p to cause console output to occur
	if (argc < 3) 
	{ // Check the value of argc. If not enough parameters have been passed, inform user and exit.
		std::cout << "\nInput file required. Quitting.\n";
		return(0);	//return, indicating via code (0) that we did not complete the simulation
	} 
	else 
	{ // if we got enough parameters...
		for (int i = 1; i < argc; i++)
		{
			if (strcmp(argv[i],"-f") == 0) 
			{
				InputFile = argv[i + 1];	// We know the next argument *should* be the filename:
			}
			else if (strcmp(argv[i], "-of") == 0)
			{
			    fitnessFileName = argv[i + 1]; // We were asked to override the name of the output file.
			}
			/*else if (strcmp(argv[i],"--twoGravityLevels") == 0) 
			{
				twoGravityLevels = true;
				gravityMultiplier = atof(argv[i + 1]);	// We know the next argument *should* be the gravity multiplier

				//std::cout << "twoGravityLevels,  gravityMultiplier = " << gravityMultiplier << std::endl;

			}*/
			else if (strcmp(argv[i],"-p") == 0) 
			{
				print_scrn=true;	//decide if output to the console is desired
			}
			else if(strcmp(argv[i], "--compoundTerrestrialEnvironment") == 0)
			{
				compoundTerrestrialEnvironment = true; // this will entail running two different, isolated, instances of the simulator
				std::cout << "Compound Terrestrial environment "<< compoundTerrestrialEnvironment << std::endl;
			}
		}

	} 

	CVX_SimGA Simulator[2];


	Vec3D<> normDistPhase1, normDistPhase2;

	for(int count = 0; count < 2; count++)
	{
		
		//std::cout << "main, exec no. " << count << std::endl;

		//create the main objects
		CVXC_Structure structure;	
		CVX_Object Object;
		CVX_Environment Environment;
		//CVX_SimGA Simulator;
		CVX_MeshUtil DeformableMesh;		

		long int Step = 0;
		vfloat Time = 0.0; //in seconds

		//setup main object
		Simulator[count].pEnv = &Environment;	//connect Simulation to environment
		Environment.pObj = &Object;		//connect environment to object
		Simulator[count].setInternalMesh(&DeformableMesh);

		//import the configuration file
		if (!Simulator[count].LoadVXAFile(InputFile)){
			if (print_scrn) std::cout << "\nProblem importing VXA file. Quitting\n";
			return(0);	//return, indicating via code (0) that we did not complete the simulation
        }
        if (strcmp(fitnessFileName.c_str(), "") > 0)
        {
            Simulator[0].FitnessFileName = fitnessFileName;
        }
        else
        {
        std::cout << fitnessFileName.c_str() << std::endl;
        }

		std::string ReturnMessage;
		if (print_scrn) std::cout << "\nImporting Environment into simulator...\n";

		Simulator[count].Import(&Environment, 0, &ReturnMessage);
		if (print_scrn) std::cout << "Simulation import return message:\n" << ReturnMessage << "\n";
		
		Simulator[count].pEnv->UpdateCurTemp(Time);	//set the starting temperature (nac: pointer removed for debugging)


		if(compoundTerrestrialEnvironment && count == 0)
		{
			// First execution, forcing flat ground. The second one will be with inclined floor.
			//std::cout << "Count == 0, forcing floor slope to zero." << std::endl;
			Environment.SetFloorSlopeEnabled(false);
		}

		/*if(twoGravityLevels && count == 1)
		{
			// Altering gravity the second time, g = g*gravityMultiplier
			Environment.SetGravityAccel( Environment.GetGravityAccel()*gravityMultiplier );

			//std::cout << "Gravity accel is now " << Environment.GetGravityAccel() << std::endl;

		}*/


		bool alreadyAlteredGravity = false; 

		while (not Simulator[count].StopConditionMet())
		{
			/*if(twoGravityLevels && !alreadyAlteredGravity && Time >= Simulator[count].GetStopConditionValue()/2)
			{
				// Altering gravity the second time, g = g*gravityMultiplier
				Environment.SetGravityAccel( Environment.GetGravityAccel()*gravityMultiplier );
				//std::cout << "t = " << Time <<"/"<< Simulator[count].GetStopConditionValue() <<": altering gravity from now on: " << Environment.GetGravityAccel() << std::endl;
				alreadyAlteredGravity = true;
			}*/

			if(Environment.GetAlterGravityHalfway() != 1.0 && !alreadyAlteredGravity && Simulator[count].GetStopConditionType() == SC_MAX_SIM_TIME && Time >= Simulator[count].GetStopConditionValue()/2)
			{
				// Need to save some stats regarding the first phase
				normDistPhase1 = Simulator[count].getNormCOMdisplacement3D();
				Simulator[count].fitPhase1 = normDistPhase1.Length();				
				Simulator[count].avgStiffChange1 = Simulator[count].getAverageStiffnessChange();

				//std::cout << "About to alter gravity, norm dist phase 1 is: " << fitPhase1 << ", avgStiffChange1 = "<< avgStiffChange1 << std::endl;
				// Altering gravity from this timestep, g = g*gravityMultiplier
				Environment.SetGravityAccel( Environment.GetGravityAccel()*Environment.GetAlterGravityHalfway() );
				//std::cout << "t = " << Time <<"/"<< Simulator[count].GetStopConditionValue() <<": altering gravity from now on: " << Environment.GetGravityAccel() << std::endl;
				alreadyAlteredGravity = true;
			}


			// do some reporting via the stdoutput if required:
			if (Step%100 == 0.0 && print_scrn) //Only output every n time steps
			{
				std::cout << "Time: " << Time << std::endl;
				std::cout << "CM: " << Simulator[count].GetCM().Length() << std::endl << std::endl;
				
				// std::cout << " \tVox 0 X: " << Vox0Pos.x << "mm" << "\tVox 0 Y: " << Vox0Pos.y << "mm" << "\tVox 0 Z: " << Vox0Pos.z << "mm\n";	//just display the position of the first voxel in the voxelarray
				std::cout << "Vox[0]  Scale: " << Simulator[count].VoxArray[0].GetCurScale() << std::endl;
				std::cout << "Vox[0]  TempAmp: " << Simulator[count].VoxArray[0].TempAmplitude << std::endl;
				std::cout << "Vox[0]  TempPer: " << Simulator[count].VoxArray[0].TempPeriod << std::endl;
				std::cout << "Vox[0]  phaseOffset: " << Simulator[count].VoxArray[0].phaseOffset << std::endl;
				// std::cout << "Vox[5]  Scale: " << Simulator.VoxArray[5].GetCurScale() << std::endl;
				// std::cout << "Vox[10] Scale: " << Simulator.VoxArray[10].GetCurScale() << std::endl;
			}

			//do the actual simulation step
			Simulator[count].TimeStep(&ReturnMessage);
			Step += 1;	//increment the step counter
			Time += Simulator[count].dt;	//update the sim tim after the step
			Simulator[count].pEnv->UpdateCurTemp(Time);	//pass in the global time, and a pointer to the local object so its material temps can be modified (nac: pointer removed for debugging)	
		}


		if(Environment.GetAlterGravityHalfway() != 1.0 && alreadyAlteredGravity)
		{
			// Computing phase 2 stats
			normDistPhase2 = normDistPhase1 - Simulator[count].getNormCOMdisplacement3D();
			Simulator[count].fitPhase2 = normDistPhase2.Length();
			Simulator[count].avgStiffChange2 = Simulator[count].getAverageStiffnessChange() - Simulator[count].avgStiffChange1;

			//std::cout << "Norm dist phase 2 is: " << fitPhase2 << ", avgStiffChange2 = "<< avgStiffChange2 << std::endl;			
		}


		if (print_scrn) std::cout << "Ended at: " << Time << std::endl;
		
		if( (!compoundTerrestrialEnvironment) ) // && (!twoGravityLevels)) // Returning if we had a single Simulator invokation (no need to combine results two instances)
		{
			Simulator[count].SaveResultFile(Simulator[count].FitnessFileName);
			return 1; //code for successful completion  // could return fitness value if greater efficiency is desired
		}

	}

	// if we are here, need to combine results in "sim[]" --> Invoking SaveResultFile with an extra Simulator argument
	//std::cout << "Fitness exec 1 is: "<< Simulator[0].getNormCOMdisplacement() << std::endl;
	//std::cout << "Fitness exec 2 is: "<< Simulator[1].getNormCOMdisplacement() << std::endl;

	// Invoking SaveResultsFile passing Simulator[1] as well to combine results
	Simulator[0].SaveResultFile(Simulator[0].FitnessFileName, &(Simulator[1]));

	return 1;
}
