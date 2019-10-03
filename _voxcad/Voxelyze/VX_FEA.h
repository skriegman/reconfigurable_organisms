/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef CVX_FEA_H
#define CVX_FEA_H

#include "VX_Environment.h"

//Jonathan Hiller (jdh74)
//VERSION 6
enum FeaViewMode {VIEW_DISP, VIEW_FORCE, VIEW_STRAIN, VIEW_REACTION}; //FEA viewing modes (for viewing output!)
enum FeaElType {BARSHEAR, FRAME}; //different element types
enum FeaDirections{XDIR, YDIR, ZDIR, MAXDIR}; //filters for directions

struct INFO3D { float Max; float MaxX; float MaxY; float MaxZ;};  //info about maximum properties for the 3D stucture

class CVX_FEA
{

public: //variables and flags to be interacted with

	CVX_FEA(void);
	~CVX_FEA(void);

	bool SolnExists;

	bool ImportObj(CVX_Environment* EnvIn = NULL, std::string* RetMessage = NULL); //links to object, or re-loads
	void ResetFEA(void); //clears everything and returns to initial state. (keeps linked VXC object, but can clear it, too.)



//	CVX_Object* pObj; //VXC object we are analyzing
	CVX_Environment* pEnv; //The physical environment we wish to test the object in

	FeaElType Element_type; //the type of element!
	bool WantForces; //calculate forces in post-processing step? (recomended: minimal overhead)
	bool PrescribedDisp; //include analysis for prescribed displacements? (recommended: minimal overhead!)
	bool Solve(std::string* RetMessage = NULL); //formulates and solves system!

	FeaViewMode ViewMode; //what to view (force, displacement, etc.)
	FeaDirections ViewModeDir; //what direction to view (XDIR, YDIR, ZDIR, or MAXDIR for max
#ifdef USE_OPEN_GL
	//Options for viewing the output
	void DrawFEA(int Selected = -1); //draw the polygons/lines of the object within an initialized OpenGL window
//	void DrawBCs(int Selected = -1); //draws the current boundary conditions
	float ViewThresh; //range from 0-1 to view iso-surfaces!
	float ViewZChop; // from 0 to 1, percentage of way up z axis to chop things
	float ViewDefPerc; //deflection percent. 0 = original, 1 = max displacement of 1 voxel, on up to arbitrary high.
#endif

	//Methods to get data back from the solution
	//gets the maximum throughout the whole structure:
	//get properties of a specific voxel (index px, py, pz) in a given direction (or maximum for that voxel)
	float GetMagForce(int px, int py, int pz, int dir = MAXDIR) {return GetMagProperty(px, py, pz, dir, F, MaxForces); } //returns force from a voxel in VXC x, y, z coords (or -1 if no voxel present)
	float GetMagDisp(int px, int py, int pz, int dir = MAXDIR) {return GetMagProperty(px, py, pz, dir, x, MaxDisps); } //returns displacement from a voxel in VXC x, y, z coords (or -1 if no voxel present)
	float GetMagReaction(int px, int py, int pz, int dir = MAXDIR) {return GetMagProperty(px, py, pz, dir, b, MaxReactions); } //returns external reaction force from a voxel in VXC x, y, z coords (or -1 if no voxel present)
	float GetMagStrain(int px, int py, int pz, int dir = MAXDIR) {return GetMagProperty(px, py, pz, dir, e, MaxStrains); } //returns internal strain from a voxel in VXC x, y, z coords (or -1 if no voxel present)
	
	//get properties of a specific voxel (index px, py, pz) in ALL directions (or maximum for that voxel)
	Vec3D<> GetForce(int x, int y, int z) {return Vec3D<>(GetMagForce(x, y, z, XDIR), GetMagForce(x, y, z, YDIR), GetMagForce(x, y, z, ZDIR));}; //returns displacement from a voxel in VXC x, y, z coords
	Vec3D<> GetDisp(int x, int y, int z) {return Vec3D<>(GetMagDisp(x, y, z, XDIR), GetMagDisp(x, y, z, YDIR), GetMagDisp(x, y, z, ZDIR));}; //returns displacement from a voxel in VXC x, y, z coords
	Vec3D<> GetReaction(int x, int y, int z) {return Vec3D<>(GetMagReaction(x, y, z, XDIR), GetMagReaction(x, y, z, YDIR), GetMagReaction(x, y, z, ZDIR));}; //returns displacement from a voxel in VXC x, y, z coords
	Vec3D<> GetStrain(int x, int y, int z) {return Vec3D<>(GetMagStrain(x, y, z, XDIR), GetMagStrain(x, y, z, YDIR), GetMagStrain(x, y, z, ZDIR));}; //returns displacement from a voxel in VXC x, y, z coords

	//gets the maximums for the entire structure
	float GetMaxForce(int dir = MAXDIR) {return GetMaxProperty(&Force, dir);}
	float GetMaxDisp(int dir = MAXDIR) {return GetMaxProperty(&Disp, dir);};
	float GetMaxReaction(int dir = MAXDIR) {return GetMaxProperty(&Reaction, dir);};
	float GetMaxStrain(int dir = MAXDIR) {return GetMaxProperty(&Strain, dir);};

	Vec3D<> GetDisp(CVX_FRegion* pRegion); //average displacement of a region! 

	//strings for outputting info
	void GetFEAInfoStr(std::string* pString = NULL); //fills pString with overall info about the VXC
	void GetFEAInfoStr(int VoxIndex, std::string* pString = NULL); //fills pString with info about a specific voxel

	//parameters to get information during the solving process
	int CurProgTick, CurProgMaxTick;
	std::string CurProgMsg;
	bool CancelFlag;

private: //off limits variable and functions (internal)
	int DOFperBlock; //the dimension of each metablock
	int ELperDBlock; //the number of elements per metablock on diagonal
	int ELperOBlock; //the number of elements per metablock off diagonal

	int* Indi; //information about bonds: indicis
	int* Indj;
	int* BondDir; //the direction of each bond
	int* IndextoDOF; //maps each voxel to its degree of freedom...
	bool* FixedList; //keep track of which are fixed
//	int NumFixed; //number of fixed
	int NumBonds;

	double* F; //to store forces in! (dimension = Num DOF)
	double* e; //to store strains in (dimension = Num DOF)

	//arrays to hold the euclidian maximum value for each voxel (from x, y, z)
	float GetMagProperty(int x, int y, int z, int dir, double* Ar, float* Maxs);
	float* MaxForces; //(dimension = number of voxels)
	float* MaxDisps; //(dimension = number of voxels)
	float* MaxReactions; //(dimension = number of voxels)
	float* MaxStrains;
	float* MaxSE; //maximum strain energy of any bond in this voxel (dimension = number of voxels)

	//output stuff:
	INFO3D Disp; //max displacements
	INFO3D Force; //max forces
	INFO3D Reaction; //max reactions
	INFO3D Strain; //max internal strains

	float GetMaxProperty(INFO3D* info3D, int dir);
	void FindMaxOverall(INFO3D* info3D, double* Ar, float* EachMax);

	void CalcBonds(); //creates list of connecting voxel indicies!
	void CalcDOF(); //does some pre-processing to figure out DOF stuff

	void CalcStiffness(); //calculates the a (stiffness) matrix!
	void CalciA(); //calculates the ia matrix
	void CalcjA(); //calculates the ja matrix
	void CalcA(); //calculates the a matrix!
	void MakeBond(double* Ain, int BondIndex); //adds values for specified bond to the specified matrix
	void ImposeValA(int El1, int El2, int i, int j, float val, double* Ain, bool FullOnly = false); //adds a bond to the A matrix
	void ConsolidateA(); //gets rid of all the zeros for solving!
	void CalcForces();

	void ApplyFixed(); //builds FixedList
	void ApplyForces();

	//Pardiso variables:
	double* a; //values of sparse matrix for solving
	int* ja; //columns each value is in (1-based!)
	int* ia; //row index (1 based!)
	double* b;
	double* x;
	int DOF; //degrees of freedom

	int mtype; //defines matrix type
	int nrhs; //number of right-hand side vectors
	void *pt[64];
	int iparm[64];
	double dparm[64];
	int maxfct, mnum, phase, error, msglvl;

	void OutputMatrices(); //for debugging small system only!!

};

//types:
//BARSHEAR    FRAME X
//|# * *| x   |# * * * * *| x
//|* # *| y   |* # * * * #| y
//|* * #| z   |* * # * # *| z
//            |* * * # * *| Tx
//            |* * # * # *| Ty
//            |* # * * * #| Tz
//

#endif
