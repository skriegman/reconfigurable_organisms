/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef VX_SIMGA_H
#define VX_SIMGA_H

//wrapper class for VX_Sim with convenience functions and nomenclature for using Voxelyze within a genetic algorithm.

#include "VX_Sim.h"

enum FitnessTypes{FT_NONE, FT_CENTER_MASS_DIST, FT_VOXEL_DIST};
class CVX_SimGA : public CVX_Sim
{

public:
	CVX_SimGA();
	~CVX_SimGA(){};

	void SaveResultFile(std::string filename, CVX_SimGA* simToCombine = NULL);
	void WriteResultFile(CXML_Rip* pXML, CVX_SimGA* simToCombine = NULL); // second argument allows to pass an additional SimGA object and combine results between "this" and simToCombine (used when evaluating an individual in multiple environments and combining the fitness)

	void WriteAdditionalSimXML(CXML_Rip* pXML);
	bool ReadAdditionalSimXML(CXML_Rip* pXML, std::string* RetMessage = NULL);

	float Fitness;	//!<Keeps track of whatever fitness we choose to track
	FitnessTypes FitnessType; //!<Holds the fitness reporting type. For now =0 tracks the center of mass, =1 tracks a particular Voxel number
	int	TrackVoxel;		//!<Holds the particular voxel that will be tracked (if used).
	std::string FitnessFileName;	//!<Holds the filename of the fitness output file that might be used
	bool WriteFitnessFile;
//	bool print_scrn;	//!<flags whether status will be sent to the console

};

#endif //VX_SIMGA_H