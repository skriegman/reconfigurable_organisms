/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "VX_FEA.h"
#include <math.h>
#include <iomanip>
#include <iostream>
#include <fstream>

//#include <stdio.h>

//#include <stdlib.h>


#ifdef USE_OPEN_GL
#ifdef QT_GUI_LIB
#include <qgl.h>
#else
#include "OpenGLInclude.h" //If not using QT's openGL system, make a header file "OpenGLInclude.h" that includes openGL library functions 
#endif
#include "Utils/GL_Utils.h"
#endif

//Jonathan Hiller (jdh74)
//VERSION 5

#ifdef USE_PARDISO
#pragma comment (lib, "libpardiso410_WIN_X86_P.lib") //link to the Pardiso library
extern "C" int PARDISOINIT (void* pt, int* mtype, int* solver, int* iparm, double* dparm, int* error);
extern "C" int PARDISO (void* pt, int* maxfct, int* mnum, int* mtype, int* phase, int* n, double* a, int* ia, int* ja, int* perm, int* nrhs, int* iparm, int* msglvl, double* b, double* x, int* error, double* dparm);
#endif

CVX_FEA::CVX_FEA(void)
{
	pEnv = NULL;

//	ResetFEA();
	Indi = NULL;
	Indj = NULL;
	BondDir = NULL;
	IndextoDOF = NULL;
	FixedList = NULL;
	F = NULL;
	e = NULL;
	MaxForces = NULL;
	MaxDisps = NULL;
	MaxReactions = NULL;
	MaxStrains = NULL;
	MaxSE = NULL;


	//Parameters ...that change
	a = NULL;
	ja = NULL;
	ia = NULL;
	b = NULL;
	x = NULL;
	DOF = -1;

	//Pardiso Params!
	//...that don't change
	mtype = 2; // Real symmetric matrix //was 2
	nrhs = 1; // Number of right hand sides.

	maxfct = 1; //Maximum number of numerical factorizations.
	mnum = 1; //Which factorization to use.
	msglvl = 1; //Print statistical information
	error = 0; //Initialize error flag

	int solver = 0; //use default (non-iterative) Pardiso solver

#ifdef USE_PARDISO
	PARDISOINIT(pt, &mtype, &solver, iparm, dparm, &error); //initialize pardiso
#endif

	Element_type = FRAME; //the type of element! (default)
	DOFperBlock = 0; //the dimension of each metablock
	ELperDBlock = 0; //the number of elements per metablock 
	ELperOBlock = 0; //the number of elements per metablock 

	ResetFEA();

	WantForces = true;
	PrescribedDisp = true;

	CurProgTick = 0;
	CurProgMaxTick = 1;
	CurProgMsg = "";
	CancelFlag = false;
}

CVX_FEA::~CVX_FEA(void)
{
	ResetFEA(); //clear all dynamic memory, among other things...
}

bool CVX_FEA::ImportObj(CVX_Environment* EnvIn , std::string* RetMessage) //links to object, or re-loads
{
	if (EnvIn != NULL) pEnv = EnvIn; //set the environemnt pointer

	if (pEnv == NULL){if (RetMessage) *RetMessage += "No Environemnt found. Import failed."; return false;}
	if (pEnv->pObj == NULL){if (RetMessage) *RetMessage += "Environment found but with invalid voxel object. Import failed."; return false;}

	switch (Element_type){ //set the parameters for each element type.
	case BARSHEAR:
		DOFperBlock = 3;
		ELperDBlock = 3;
		ELperOBlock = 3;
		break;
	case FRAME:
		DOFperBlock = 6;
		ELperDBlock = 12;
		ELperOBlock = 18; //some wasted space, but oh well
		break;
	}

	int numVox = pEnv->pObj->GetNumVox(); //number of voxels present
	if (numVox <=0){
		if (RetMessage) *RetMessage += "No voxels found to import into FEA. Aborting.";
		return false;
	}

	int tmpNumDOF = numVox*DOFperBlock; //3DOF for each instantiated voxel
	if (DOF != tmpNumDOF){ //if we need to reallocate mem...
		//if (ObjIn != NULL) ResetFEA(); //make sure dynamic arrays are deleted
		ResetFEA(); //make sure dynamic arrays are deleted

		DOF = tmpNumDOF;
		b = new double[DOF];
		x = new double[DOF];
		if (WantForces) F = new double[DOF];
		if (WantForces) e = new double[DOF];
		MaxDisps = new float[numVox];
		if (WantForces){
			MaxForces = new float[numVox];
			MaxReactions = new float[numVox];
			MaxStrains = new float[numVox];
			MaxSE = new float[numVox];
		}
		Indi = new int[numVox*6]; //6 is for maximum number of connections per block
		Indj = new int[numVox*6];
		BondDir = new int[numVox*6];

		IndextoDOF = new int[pEnv->pObj->GetStArraySize()];
		FixedList = new bool[numVox];

	}
	else { //just delete the pardiso matrices...
		if (a != NULL) delete [] a; a = NULL;
		if (ia != NULL) delete [] ia; ia = NULL;
		if (ja != NULL) delete [] ja; ja = NULL;
	}

	return true;
}

void CVX_FEA::ResetFEA(void) //clears everything and returns to initial state. (leaves VXC object linked)
{
	if (b != NULL) delete [] b; b = NULL;
	if (x != NULL) delete [] x; x = NULL;
	if (F != NULL) delete [] F; F = NULL;
	if (e != NULL) delete [] e; e = NULL;
	if (Indi != NULL) delete [] Indi; Indi = NULL;
	if (Indj != NULL) delete [] Indj; Indj = NULL;
	if (BondDir != NULL) delete [] BondDir; BondDir = NULL;
	if (IndextoDOF != NULL) delete [] IndextoDOF; IndextoDOF = NULL;
	if (FixedList != NULL) delete [] FixedList; FixedList = NULL;
	if (a != NULL) delete [] a; a = NULL;
	if (ia != NULL) delete [] ia; ia = NULL;
	if (ja != NULL) delete [] ja; ja = NULL;
	if (MaxForces != NULL) delete [] MaxForces; MaxForces = NULL;
	if (MaxDisps != NULL) delete [] MaxDisps; MaxDisps = NULL;
	if (MaxReactions != NULL) delete [] MaxReactions; MaxReactions = NULL;
	if (MaxStrains != NULL) delete [] MaxStrains; MaxStrains = NULL;
	if (MaxSE != NULL) delete [] MaxSE; MaxSE = NULL;

	DOF = -1;

	ViewMode = VIEW_DISP;
	ViewModeDir = MAXDIR;
#ifdef USE_OPEN_GL
	ViewThresh = 0.0f;
	ViewZChop = 1.0f;
	ViewDefPerc = 2.0f;
#endif



//	if(ResetVXC && pObj!= NULL){
//		pEnv->pObj->ClearMatter(); //clear the VXC if we want, but stay linked to it...
//		pEnv->ClearBCs();
//	}

	SolnExists=false;
}



bool CVX_FEA::Solve(std::string* RetMessage) //formulates and solves system!
{
//	int Before = (int)GetTickCount();
	CurProgTick = 0;
	CurProgMaxTick = 100;
	CurProgMsg = "Beginning solve process...";
	CancelFlag = false; //this may be set to tru, in which case we should interrupt the solve process...
	bool Success = true;

	if (pEnv->GetNumFixedBCs() == 0){ if (RetMessage) *RetMessage += "No fixed regions found. Aborting.\n"; return false;}

	pEnv->RemoveDisconnected(); 
	if (pEnv->pObj->GetNumVox() == 0){ if (RetMessage) *RetMessage += "No voxels connected to ground\n"; return false;}

	CurProgMsg = "Forming matrices...";

	CalcDOF();
	CalcBonds();
	CalcStiffness();
	ApplyForces();
	ApplyFixed(); //needs to be called after apply forces for prescribed displacement to work
	ConsolidateA(); //removes zeros, help, but only marginally. (maybe pardiso does this internally?)

//	int Before2 = (int)GetTickCount();

	if (DOF == 0){ if (RetMessage) *RetMessage +=  "No free degrees of freedom found. Aborting.\n"; return false;}

	iparm[0] = 0;
	iparm[2] = -1;

	double ddum = 0; //float dummy var
	int idum = 0; //Integer dummy var

#ifdef USE_PARDISO
	CurProgTick = 2;
	CurProgMsg = "Pardiso: Analyzing...";
	phase = 11;
	PARDISO(pt, &maxfct, &mnum, &mtype, &phase, &DOF, a, ia, ja, &idum, &nrhs, iparm, &msglvl, b, x, &error, dparm);

	CurProgTick = 5;
	CurProgMsg = "Pardiso: Numerical factorization...";
	phase = 22;
	PARDISO(pt, &maxfct, &mnum, &mtype, &phase, &DOF, a, ia, ja, &idum, &nrhs, iparm, &msglvl, b, x, &error, dparm);

	CurProgTick = 79;
	CurProgMsg = "Pardiso: Solving, iterative refinement...";
	phase = 33;
	PARDISO(pt, &maxfct, &mnum, &mtype, &phase, &DOF, a, ia, ja, &idum, &nrhs, iparm, &msglvl, b, x, &error, dparm);
	//We recommend using the in-core PARDISO for all cases where the memory required for storing PARDISO factors exceeds the RAM by less than 30%. The size of the factors in kbytes can be obtained with the help of  iparm(17) after phase 11 (see Intel MKL reference manual).

	if (error == -1){ if (RetMessage) *RetMessage += "Pardiso error: Input inconsistent\n"; Success = false;}
	else if (error == -2){ if (RetMessage) *RetMessage += "Pardiso error: Not enough memory\n"; Success = false;}
	else if (error == -3){ if (RetMessage) *RetMessage += "Pardiso error: Reodering Problem\n"; Success = false;}
	else if (error == -4){ if (RetMessage) *RetMessage += "Pardiso error: Zero pivot, numerical factorization or iterative refinement problem\n"; Success = false;}
	else if (error == -10){ if (RetMessage) *RetMessage += "No License file Pardiso.lic found.\nGo to www.pardiso-project.org and follow the instructions to obtain a license file for your computer.\nPlace the file in VoxCad's main directory (Usually Program Files/Cornell/Voxcad)\n"; Success = false;}
	else if (error == -11){ if (RetMessage) *RetMessage += "License is expired\n"; Success = false;}
	else if (error == -12){ if (RetMessage) *RetMessage += "Wrong username or hostname\n"; Success = false;}
	else if (error != 0){ if (RetMessage) *RetMessage += "Pardiso Error\n"; Success = false;}

	CurProgTick = 80;
	CurProgMsg = "Pardiso cleaning...";
	phase = -1; /* Release internal memory. */
	PARDISO(pt, &maxfct, &mnum, &mtype, &phase, &DOF, &ddum, ia, ja, &idum, &nrhs, iparm, &msglvl, &ddum, &ddum, &error, dparm);
#else
	Success = false;
#endif

//	int After = Before2 - Before;
//	int After2 = (int)GetTickCount() - Before2;

	if (!Success) return false;

	CurProgTick = 90;
	CurProgMsg = "Processing results...";
	FindMaxOverall(&Disp, x, MaxDisps);
	if (Disp.Max == 0){
		if (RetMessage) *RetMessage += "No forces or displacements acting on structure. (Zero displacement)\n";
		return false;
	}

	if (WantForces)
		CalcForces();

//	OutputMatrices();
	if (a != NULL) {delete [] a; a = NULL;}
	if (ia != NULL) {delete [] ia; ia = NULL;}
	if (ja != NULL) {delete [] ja; ja = NULL;}



	ViewMode = VIEW_DISP; //automatically view the result, with default settings
	ViewModeDir = MAXDIR;
#ifdef USE_OPEN_GL
	ViewThresh = 0.0f;
	ViewZChop = 1.0f;
	ViewDefPerc = 2.0f;
#endif

	SolnExists = true;
	return true;
}

void CVX_FEA::CalcDOF()
{
	DOF = pEnv->pObj->GetNumVox()*DOFperBlock; //set this to our new DOF value

	//pre compute DOFtoIndex
	int DOFInd = 0;
	for (int i=0; i<pEnv->pObj->GetStArraySize(); i++){ //for all posible voxels:
		if (pEnv->pObj->GetMat(i) != 0){ //if there's a voxel here...
			IndextoDOF[i] = DOFInd++;
		}
		else IndextoDOF[i] = -1;
	}
}


void CVX_FEA::CalcBonds() //creates list of connecting voxel indicies!
{
	NumBonds = 0;
	int tmpIndex = 0;

	for (int i=0; i<pEnv->pObj->GetStArraySize(); i++){ //for every possible voxel
		if (pEnv->pObj->GetMat(i) != 0){ //if there's a voxel here...
			for (int Dim = 0; Dim < 3; Dim++){ //check each possible connection (x, y, z). Only in positive direction so we only get each bond once
				switch (Dim) {
					case XDIR: //X direction
						tmpIndex = i+1; //index of voxel to check
						if (tmpIndex % pEnv->pObj->GetVXDim() == 0) //account for edge condition
							tmpIndex = pEnv->pObj->GetStArraySize(); //set to high!
						
						break;
					case YDIR: //Y direction
						tmpIndex = i + pEnv->pObj->GetVXDim(); //index of voxel to check
						if ((int)(tmpIndex/pEnv->pObj->GetVXDim()) % pEnv->pObj->GetVYDim() == 0) //account for edge condition
							tmpIndex = pEnv->pObj->GetStArraySize(); //set to high!

						break;
					case ZDIR: //Z direction
						tmpIndex = i + pEnv->pObj->GetVXDim()*pEnv->pObj->GetVYDim(); //index of voxel to check
				}
				if (tmpIndex < pEnv->pObj->GetStArraySize() && pEnv->pObj->GetMat(tmpIndex) != 0){ //if the voxel to check is instantiated...
					BondDir[NumBonds] = Dim;
					Indi[NumBonds] = i; //this is voxel one of the bond
					Indj[NumBonds++] = tmpIndex; //this is voxel 2 of the bond
				}
			}
		}
	}
}

void CVX_FEA::CalcStiffness() //calculates the big stiffness matrix!
{
	int NumEl = DOF/DOFperBlock*ELperDBlock + NumBonds*ELperOBlock; // - NumFixed; //approximate number of metablocks in our K matrix (overestimates slightly)

	if (a != NULL) {delete [] a; a = NULL;}
	a = new double[NumEl]; //values of sparse matrix
	if (ja != NULL) {delete [] ja; ja = NULL;}
	ja = new int[NumEl]; //columns each value is in
	if (ia != NULL) {delete [] ia; ia = NULL;}
	ia = new int[DOF+1]; //row index

	for (int i=0; i<NumEl; i++){ a[i] = 0; } //initialize A;

	CalciA();
	CalcjA();
	CalcA();

}

void CVX_FEA::CalciA() //populate ia!
{
	int IndIndex = 0;
	int iaIndex = 0;
	ia[iaIndex] = 1; //first element is always 1!

	int NumLocBonds; //number of bonds in a given row 
	int NumToAdd; //number to add

	for (int i=0; i<DOF/DOFperBlock; i++){ //iterate through each meta-row
		NumLocBonds = 0; //number of bonds in this row 

		while (IndIndex<NumBonds && IndextoDOF[Indi[IndIndex]] <= i){ //calculate how many bonds involve this block
			NumLocBonds++;
			IndIndex++;
		}

		for (int j=0; j<DOFperBlock; j++){ //now go through each row of the metarow
			if (Element_type == BARSHEAR)
				ia[++iaIndex] = ia[iaIndex-1] + 1 + NumLocBonds; //add number of elements in this row!
			else { //if Element_type == FRAME
				NumToAdd = 1 + 3*(NumLocBonds);; //1 for diagonal elements, 3*(NumLocBonds) = 3 more elements for every bond metablock
				if (j<3) NumToAdd +=2; //the off-diag terms of diagonal metablocks
				ia[++iaIndex] = ia[iaIndex-1] + NumToAdd; //add number of elements in this row!
			}
		}
	}
}

void CVX_FEA::CalcjA() //calculate ja!
{
	int IndIndex = 0;
	int CurInd;
	int NumRowBonds; //number of bonds in this row

	int jaIndex; //tmp ja index
	int ElNum; //tmp number of elements

	for (int i=0; i<DOF/DOFperBlock; i++){ //iterate through each meta-row
		NumRowBonds = 0; //number of bonds in this row

		//diagonal elements
		for (int j=0; j<DOFperBlock; j++){ //now go through each row of the metarow
			//all diagonal elements are set no matter what...+
			CurInd = i*DOFperBlock+j;
			jaIndex = ia[CurInd]-1;
			ja[jaIndex] = CurInd +1; //this will get redone for more complicated elements...
			
			if (Element_type == FRAME){
				switch (j){
					case 0:
						ja[++jaIndex] = CurInd + 5; //+4 for offset to next term... + 1 for silly offset (1-based index)
						ja[++jaIndex] = CurInd + 6; //+5
						break;
					case 1:
						ja[++jaIndex] = CurInd + 3; //+2
						ja[++jaIndex] = CurInd + 5; //+4
						break;
					case 2:
						ja[++jaIndex] = CurInd + 2; //+1
						ja[++jaIndex] = CurInd + 3; //+2
						break;
				}
			}
		}

		//for each bond
		while (IndIndex<NumBonds && IndextoDOF[Indi[IndIndex]] <= i){ //calculate how many bonds involve this 
			for (int j=0; j<DOFperBlock; j++){ //now go through each row of the metarow
				if (Element_type == BARSHEAR){
					jaIndex = ia[i*DOFperBlock+j] + NumRowBonds;
					ja[jaIndex] = IndextoDOF[Indj[IndIndex]]*DOFperBlock+j+1; //this will get overwritten with more complicated types...
				}
				else if (Element_type == FRAME){
					if (j<3) ElNum = 3; //number of elements taken in the diagonal block...
					else ElNum = 1;
					ElNum += 3*NumRowBonds;

					jaIndex = ia[i*DOFperBlock+j]-1 + ElNum;
					CurInd = IndextoDOF[Indj[IndIndex]]*DOFperBlock+j;

					switch (j){
						case 0:
							ja[jaIndex] = CurInd+1; //+0 (after accounting for 1-based index)
							ja[++jaIndex] = CurInd+5; //+4
							ja[++jaIndex] = CurInd+6; //+5
							break;
						case 1:
							ja[jaIndex] = CurInd+1; //+0
							ja[++jaIndex] = CurInd+3; //+2
							ja[++jaIndex] = CurInd+5; //+4
							break;
						case 2:
							ja[jaIndex] = CurInd+1; //+0
							ja[++jaIndex] = CurInd+2; //+1
							ja[++jaIndex] = CurInd+3; //+2
							break;
						case 3:
							ja[jaIndex] = CurInd-1; //-2
							ja[++jaIndex] = CurInd; //-1
							ja[++jaIndex] = CurInd+1; //+0
							break;
						case 4:
							ja[jaIndex] = CurInd-3; //-4
							ja[++jaIndex] = CurInd-1; //-2
							ja[++jaIndex] = CurInd+1; //+0
							break;
						case 5:
							ja[jaIndex] = CurInd-4; //-5
							ja[++jaIndex] = CurInd-3; //-4
							ja[++jaIndex] = CurInd+1; //+0
							break;
					}
				}
			}

			IndIndex++;
			NumRowBonds++;
		}
	}
}

void CVX_FEA::CalcA() //populate "a" matrix!
{
	for(int b=0; b<NumBonds; b++){ //for each bond!
		MakeBond(a, b);
	}
}


void CVX_FEA::MakeBond(double* Ain, int BondIndex)
{
//	bool FullOnlyFlag;

	int El1 = IndextoDOF[Indi[BondIndex]];
	int El2 = IndextoDOF[Indj[BondIndex]];
	if (Ain != a) { //tmp
		El1 = 0;
		El2 = 1;
	}

	int BondDirec = BondDir[BondIndex]; //which direction is this?

	float E1 = (float)(pEnv->pObj->GetLeafMat(Indi[BondIndex]))->GetElasticMod(); //elastic modulus
	float v1 = (float)(pEnv->pObj->GetLeafMat(Indi[BondIndex]))->GetPoissonsRatio(); //poissons ratio
	float G1 = (float)(E1/(2*(1+v1))); //shear modulus [E = 2G(1+v)) -> G = E/(2*(1+v))]
	float E2 = (float)(pEnv->pObj->GetLeafMat(Indj[BondIndex]))->GetElasticMod();
	float v2 = (float)(pEnv->pObj->GetLeafMat(Indj[BondIndex]))->GetPoissonsRatio(); //poissons ratio
	float G2 = (float)(E2/(2*(1+v2)));

	float E = (E1*E2/(E1+E2))*2; //composite elastic modulus: series spring equation... from 2 materials
	float G = (G1*G2/(G1+G2))*2;	//composite shear modulus: series spring equation... from 2 materials
	Vec3D<> Bm = pEnv->pObj->GetLatDimEnv(); //length between points (lattice_Dim*X_ADj, Lattice_Dim*Y_ADj, etc)

	if (Element_type == BARSHEAR){
		//float NormalStiff = E*L; //k=EA/l
		//float ShearStiff = G*L; //series spring equation... from 2 materials, G=F*I/dX*A

		////diagonal of element 1:
		//for (int i=0; i<DOFperBlock; i++){ //for each individual direction of freedom
		//	float ToAdd = ShearStiff; //add the shear...
		//	if (i==BondDirec) ToAdd = NormalStiff; //unless this is normal direction
		//	ImposeValA(El1, El1, i, i, ToAdd, Ain);
		//	ImposeValA(El2, El2, i, i, ToAdd, Ain);

		//	FullOnlyFlag = true;
		//	if (!FixedList[El1] && !FixedList[El2]) //if neither element is fixed
		//		FullOnlyFlag = false;
		//	
		//	ImposeValA(El1, El2, i, i, -ToAdd, Ain, FullOnlyFlag);

		//}
	}
	else if (Element_type == FRAME){
		//http://www.eng.fsu.edu/~chandra/courses/eml4536/Chapter4.ppt

		//float a1 = E*L; //EA/L
		//float a2 = G*L*L*L/6; //GJ/L, J=BH/12*(BB+HH) = L^4/6
		//float b1 = E*L; //12EI/L^3, I=bh^3/12 = L^4/12
		//float b2 = E*L*L/2; //6EI/L^2
		//float b3 = E*L*L*L/6; //2EI/L

		if (BondDirec == XDIR){
			float A = (float)(Bm.y*Bm.z);
			float L = (float)Bm.x;
			float Jx = (float)(Bm.y*Bm.z/12.0*(Bm.y*Bm.y+Bm.z*Bm.z)); //Polar moment of inertia. Apparently for rectangles, the actuall bending stiffness is a little different, but this should be close enough
			float Iy = (float)(Bm.z*Bm.y*Bm.y*Bm.y/12.0);
			float Iz =(float)(Bm.y*Bm.z*Bm.z*Bm.z/12.0);

			float a1 = E*A/L; //EA/L
			float a2 = G*Jx/L; //GJ/L, J=BH/12*(BB+HH) = L^4/6
			float b1y = 12*E*Iy/(L*L*L); //12EI/L^3, I=bh^3/12 = L^4/12
			float b1z = 12*E*Iz/(L*L*L);
			float b2y = 6*E*Iy/(L*L); //6EI/L^2
			float b2z = 6*E*Iz/(L*L); //6EI/L^2
			float b3y = 2*E*Iy/L; //2EI/L
			float b3z = 2*E*Iz/L; //2EI/L

			//upper diagonal
			ImposeValA(El1, El1, 0, 0, a1, Ain);
			ImposeValA(El1, El1, 1, 1, b1y, Ain);
			ImposeValA(El1, El1, 5, 1, b2y, Ain);
			ImposeValA(El1, El1, 2, 2, b1z, Ain);
			ImposeValA(El1, El1, 4, 2, -b2z, Ain);
			ImposeValA(El1, El1, 3, 3, a2, Ain);
			ImposeValA(El1, El1, 4, 4, 2*b3z, Ain);
			ImposeValA(El1, El1, 5, 5, 2*b3y, Ain);

			//lower diagonal
			ImposeValA(El2, El2, 0, 0, a1, Ain);
			ImposeValA(El2, El2, 1, 1, b1y, Ain);
			ImposeValA(El2, El2, 5, 1, -b2y, Ain);
			ImposeValA(El2, El2, 2, 2, b1z, Ain);
			ImposeValA(El2, El2, 4, 2, b2z, Ain);
			ImposeValA(El2, El2, 3, 3, a2, Ain);
			ImposeValA(El2, El2, 4, 4, 2*b3z, Ain);
			ImposeValA(El2, El2, 5, 5, 2*b3y, Ain);

			//off diagonal
//			FullOnlyFlag = true;
//			if (!FixedList[El1] && !FixedList[El2])//{ //don't add if fixed! (if both are not fixed...)
//				FullOnlyFlag = false;

			ImposeValA(El1, El2, 0, 0, -a1, Ain); //, FullOnlyFlag);
			ImposeValA(El1, El2, 1, 1, -b1y, Ain); //, FullOnlyFlag);
			ImposeValA(El1, El2, 5, 1, b2y, Ain); //, FullOnlyFlag);
			ImposeValA(El1, El2, 2, 2, -b1z, Ain); //, FullOnlyFlag);
			ImposeValA(El1, El2, 4, 2, -b2z, Ain); //, FullOnlyFlag);
			ImposeValA(El1, El2, 3, 3, -a2, Ain); //, FullOnlyFlag);
			ImposeValA(El1, El2, 2, 4, b2z, Ain); //, FullOnlyFlag);
			ImposeValA(El1, El2, 4, 4, b3z, Ain); //, FullOnlyFlag);
			ImposeValA(El1, El2, 1, 5, -b2y, Ain); //, FullOnlyFlag);
			ImposeValA(El1, El2, 5, 5, b3y, Ain); //, FullOnlyFlag);
		}

		else if (BondDirec == YDIR){
			float A = (float)(Bm.x*Bm.z);
			float L = (float)Bm.y;
			float Jy = (float)(Bm.x*Bm.z/12.0*(Bm.x*Bm.x+Bm.z*Bm.z)); //Polar moment of inertia. Apparently for rectangles, the actuall bending stiffness is a little different, but this should be close enough
			float Ix = (float)(Bm.z*Bm.x*Bm.x*Bm.x/12.0);
			float Iz = (float)(Bm.x*Bm.z*Bm.z*Bm.z/12.0);

			float a1 = E*A/L; //EA/L
			float a2 = G*Jy/L; //GJ/L, J=BH/12*(BB+HH) = L^4/6
			float b1x = 12*E*Ix/(L*L*L); //12EI/L^3, I=bh^3/12 = L^4/12
			float b1z = 12*E*Iz/(L*L*L);
			float b2x = 6*E*Ix/(L*L); //6EI/L^2
			float b2z = 6*E*Iz/(L*L); //6EI/L^2
			float b3x = 2*E*Ix/L; //2EI/L
			float b3z = 2*E*Iz/L; //2EI/L

			//upper diagonal
			ImposeValA(El1, El1, 0, 0, b1x, Ain);
			ImposeValA(El1, El1, 5, 0, -b2x, Ain);
			ImposeValA(El1, El1, 1, 1, a1, Ain);
			ImposeValA(El1, El1, 2, 2, b1z, Ain);
			ImposeValA(El1, El1, 3, 2, b2z, Ain);
			ImposeValA(El1, El1, 3, 3, 2*b3z, Ain);
			ImposeValA(El1, El1, 4, 4, a2, Ain);
			ImposeValA(El1, El1, 5, 5, 2*b3x, Ain);

			//lower diagonal
			ImposeValA(El2, El2, 0, 0, b1x, Ain);
			ImposeValA(El2, El2, 5, 0, b2x, Ain);
			ImposeValA(El2, El2, 1, 1, a1, Ain);
			ImposeValA(El2, El2, 2, 2, b1z, Ain);
			ImposeValA(El2, El2, 3, 2, -b2z, Ain);
			ImposeValA(El2, El2, 3, 3, 2*b3z, Ain);
			ImposeValA(El2, El2, 4, 4, a2, Ain);
			ImposeValA(El2, El2, 5, 5, 2*b3x, Ain);

			//off diagonal
//			FullOnlyFlag = true;
//			if (!FixedList[El1] && !FixedList[El2])//{ //don't add if fixed! (if both are not fixed...)
//				FullOnlyFlag = false;

			ImposeValA(El1, El2, 0, 0, -b1x, Ain); //, FullOnlyFlag);
			ImposeValA(El1, El2, 5, 0, -b2x, Ain); //, FullOnlyFlag);
			ImposeValA(El1, El2, 1, 1, -a1, Ain); //, FullOnlyFlag);
			ImposeValA(El1, El2, 2, 2, -b1z, Ain); //, FullOnlyFlag);
			ImposeValA(El1, El2, 3, 2, b2z, Ain); //, FullOnlyFlag);
			ImposeValA(El1, El2, 2, 3, -b2z, Ain); //, FullOnlyFlag);
			ImposeValA(El1, El2, 3, 3, b3z, Ain); //, FullOnlyFlag);
			ImposeValA(El1, El2, 4, 4, -a2, Ain); //, FullOnlyFlag);
			ImposeValA(El1, El2, 0, 5, b2x, Ain); //, FullOnlyFlag);
			ImposeValA(El1, El2, 5, 5, b3x, Ain); //, FullOnlyFlag);
		}

		else if (BondDirec == ZDIR){
			float A = (float)(Bm.x*Bm.y);
			float L = (float)Bm.z;
			float Jz = (float)(Bm.x*Bm.y/12.0*(Bm.x*Bm.x+Bm.y*Bm.y)); //Polar moment of inertia. Apparently for rectangles, the actuall bending stiffness is a little different, but this should be close enough
			float Ix = (float)(Bm.y*Bm.x*Bm.x*Bm.x/12.0);
			float Iy = (float)(Bm.x*Bm.y*Bm.y*Bm.y/12.0);

			float a1 = E*A/L; //EA/L
			float a2 = G*Jz/L; //GJ/L, J=BH/12*(BB+HH) = L^4/6
			float b1x = 12*E*Ix/(L*L*L); //12EI/L^3, I=bh^3/12 = L^4/12
			float b1y = 12*E*Iy/(L*L*L);
			float b2x = 6*E*Ix/(L*L); //6EI/L^2
			float b2y = 6*E*Iy/(L*L); //6EI/L^2
			float b3x = 2*E*Ix/L; //2EI/L
			float b3y = 2*E*Iy/L; //2EI/L

			//upper diagonal
			ImposeValA(El1, El1, 0, 0, b1x, Ain);
			ImposeValA(El1, El1, 4, 0, b2x, Ain);
			ImposeValA(El1, El1, 1, 1, b1y, Ain);
			ImposeValA(El1, El1, 3, 1, -b2y, Ain);
			ImposeValA(El1, El1, 2, 2, a1, Ain);
			ImposeValA(El1, El1, 3, 3, 2*b3y, Ain);
			ImposeValA(El1, El1, 4, 4, 2*b3x, Ain);
			ImposeValA(El1, El1, 5, 5, a2, Ain);

			//lower diagonal
			ImposeValA(El2, El2, 0, 0, b1x, Ain);
			ImposeValA(El2, El2, 4, 0, -b2x, Ain);
			ImposeValA(El2, El2, 1, 1, b1y, Ain);
			ImposeValA(El2, El2, 3, 1, b2y, Ain);
			ImposeValA(El2, El2, 2, 2, a1, Ain);
			ImposeValA(El2, El2, 3, 3, 2*b3y, Ain);
			ImposeValA(El2, El2, 4, 4, 2*b3x, Ain);
			ImposeValA(El2, El2, 5, 5, a2, Ain);

			//off diagonal
//			FullOnlyFlag = true;
//			if (!FixedList[El1] && !FixedList[El2])//{ //don't add if fixed! (if both are not fixed...)
//				FullOnlyFlag = false;

			ImposeValA(El1, El2, 0, 0, -b1x, Ain); //, FullOnlyFlag); //
			ImposeValA(El1, El2, 4, 0, b2x, Ain); //, FullOnlyFlag);
			ImposeValA(El1, El2, 1, 1, -b1y, Ain); //, FullOnlyFlag);
			ImposeValA(El1, El2, 3, 1, -b2y, Ain); //, FullOnlyFlag);
			ImposeValA(El1, El2, 2, 2, -a1, Ain); //, FullOnlyFlag);
			ImposeValA(El1, El2, 1, 3, b2y, Ain); //, FullOnlyFlag);
			ImposeValA(El1, El2, 3, 3, b3y, Ain); //, FullOnlyFlag);
			ImposeValA(El1, El2, 0, 4, -b2x, Ain); //, FullOnlyFlag);
			ImposeValA(El1, El2, 4, 4, b3x, Ain); //, FullOnlyFlag);
			ImposeValA(El1, El2, 5, 5, -a2, Ain); //, FullOnlyFlag);
		}
		//float a1 = E*L; //EA/L
		//float a2 = G*L*L*L/6; //GJ/L, J=BH/12*(BB+HH) = L^4/6
		//float b1 = E*L; //12EI/L^3, I=bh^3/12 = L^4/12
		//float b2 = E*L*L/2; //6EI/L^2
		//float b3 = E*L*L*L/6; //2EI/L

		//if (BondDirec == XDIR){
		//	//upper diagonal
		//	ImposeValA(El1, El1, 0, 0, a1, Ain);
		//	ImposeValA(El1, El1, 1, 1, b1, Ain);
		//	ImposeValA(El1, El1, 5, 1, b2, Ain);
		//	ImposeValA(El1, El1, 2, 2, b1, Ain);
		//	ImposeValA(El1, El1, 4, 2, -b2, Ain);
		//	ImposeValA(El1, El1, 3, 3, a2, Ain);
		//	ImposeValA(El1, El1, 4, 4, 2*b3, Ain);
		//	ImposeValA(El1, El1, 5, 5, 2*b3, Ain);

		//	//lower diagonal
		//	ImposeValA(El2, El2, 0, 0, a1, Ain);
		//	ImposeValA(El2, El2, 1, 1, b1, Ain);
		//	ImposeValA(El2, El2, 5, 1, -b2, Ain);
		//	ImposeValA(El2, El2, 2, 2, b1, Ain);
		//	ImposeValA(El2, El2, 4, 2, b2, Ain);
		//	ImposeValA(El2, El2, 3, 3, a2, Ain);
		//	ImposeValA(El2, El2, 4, 4, 2*b3, Ain);
		//	ImposeValA(El2, El2, 5, 5, 2*b3, Ain);

		//	//off diagonal
		//	FullOnlyFlag = true;
		//	if (!FixedList[El1] && !FixedList[El2])//{ //don't add if fixed! (if both are not fixed...)
		//		FullOnlyFlag = false;

		//	ImposeValA(El1, El2, 0, 0, -a1, Ain, FullOnlyFlag);
		//	ImposeValA(El1, El2, 1, 1, -b1, Ain, FullOnlyFlag);
		//	ImposeValA(El1, El2, 5, 1, b2, Ain, FullOnlyFlag);
		//	ImposeValA(El1, El2, 2, 2, -b1, Ain, FullOnlyFlag);
		//	ImposeValA(El1, El2, 4, 2, -b2, Ain, FullOnlyFlag);
		//	ImposeValA(El1, El2, 3, 3, -a2, Ain, FullOnlyFlag);
		//	ImposeValA(El1, El2, 2, 4, b2, Ain, FullOnlyFlag);
		//	ImposeValA(El1, El2, 4, 4, b3, Ain, FullOnlyFlag);
		//	ImposeValA(El1, El2, 1, 5, -b2, Ain, FullOnlyFlag);
		//	ImposeValA(El1, El2, 5, 5, b3, Ain, FullOnlyFlag);
		//}

		//else if (BondDirec == YDIR){
		//	//upper diagonal
		//	ImposeValA(El1, El1, 0, 0, b1, Ain);
		//	ImposeValA(El1, El1, 5, 0, -b2, Ain);
		//	ImposeValA(El1, El1, 1, 1, a1, Ain);
		//	ImposeValA(El1, El1, 2, 2, b1, Ain);
		//	ImposeValA(El1, El1, 3, 2, b2, Ain);
		//	ImposeValA(El1, El1, 3, 3, 2*b3, Ain);
		//	ImposeValA(El1, El1, 4, 4, a2, Ain);
		//	ImposeValA(El1, El1, 5, 5, 2*b3, Ain);

		//	//lower diagonal
		//	ImposeValA(El2, El2, 0, 0, b1, Ain);
		//	ImposeValA(El2, El2, 5, 0, b2, Ain);
		//	ImposeValA(El2, El2, 1, 1, a1, Ain);
		//	ImposeValA(El2, El2, 2, 2, b1, Ain);
		//	ImposeValA(El2, El2, 3, 2, -b2, Ain);
		//	ImposeValA(El2, El2, 3, 3, 2*b3, Ain);
		//	ImposeValA(El2, El2, 4, 4, a2, Ain);
		//	ImposeValA(El2, El2, 5, 5, 2*b3, Ain);

		//	//off diagonal
		//	FullOnlyFlag = true;
		//	if (!FixedList[El1] && !FixedList[El2])//{ //don't add if fixed! (if both are not fixed...)
		//		FullOnlyFlag = false;

		//	ImposeValA(El1, El2, 0, 0, -b1, Ain, FullOnlyFlag);
		//	ImposeValA(El1, El2, 5, 0, -b2, Ain, FullOnlyFlag);
		//	ImposeValA(El1, El2, 1, 1, -a1, Ain, FullOnlyFlag);
		//	ImposeValA(El1, El2, 2, 2, -b1, Ain, FullOnlyFlag);
		//	ImposeValA(El1, El2, 3, 2, b2, Ain, FullOnlyFlag);
		//	ImposeValA(El1, El2, 2, 3, -b2, Ain, FullOnlyFlag);
		//	ImposeValA(El1, El2, 3, 3, b3, Ain, FullOnlyFlag);
		//	ImposeValA(El1, El2, 4, 4, -a2, Ain, FullOnlyFlag);
		//	ImposeValA(El1, El2, 0, 5, b2, Ain, FullOnlyFlag);
		//	ImposeValA(El1, El2, 5, 5, b3, Ain, FullOnlyFlag);
		//}

		//else if (BondDirec == ZDIR){
		//	//upper diagonal
		//	ImposeValA(El1, El1, 0, 0, b1, Ain);
		//	ImposeValA(El1, El1, 4, 0, b2, Ain);
		//	ImposeValA(El1, El1, 1, 1, b1, Ain);
		//	ImposeValA(El1, El1, 3, 1, -b2, Ain);
		//	ImposeValA(El1, El1, 2, 2, a1, Ain);
		//	ImposeValA(El1, El1, 3, 3, 2*b3, Ain);
		//	ImposeValA(El1, El1, 4, 4, 2*b3, Ain);
		//	ImposeValA(El1, El1, 5, 5, a2, Ain);

		//	//lower diagonal
		//	ImposeValA(El2, El2, 0, 0, b1, Ain);
		//	ImposeValA(El2, El2, 4, 0, -b2, Ain);
		//	ImposeValA(El2, El2, 1, 1, b1, Ain);
		//	ImposeValA(El2, El2, 3, 1, b2, Ain);
		//	ImposeValA(El2, El2, 2, 2, a1, Ain);
		//	ImposeValA(El2, El2, 3, 3, 2*b3, Ain);
		//	ImposeValA(El2, El2, 4, 4, 2*b3, Ain);
		//	ImposeValA(El2, El2, 5, 5, a2, Ain);

		//	//off diagonal
		//	FullOnlyFlag = true;
		//	if (!FixedList[El1] && !FixedList[El2])//{ //don't add if fixed! (if both are not fixed...)
		//		FullOnlyFlag = false;

		//	ImposeValA(El1, El2, 0, 0, -b1, Ain, FullOnlyFlag);
		//	ImposeValA(El1, El2, 4, 0, b2, Ain, FullOnlyFlag);
		//	ImposeValA(El1, El2, 1, 1, -b1, Ain, FullOnlyFlag);
		//	ImposeValA(El1, El2, 3, 1, -b2, Ain, FullOnlyFlag);
		//	ImposeValA(El1, El2, 2, 2, -a1, Ain, FullOnlyFlag);
		//	ImposeValA(El1, El2, 1, 3, b2, Ain, FullOnlyFlag);
		//	ImposeValA(El1, El2, 3, 3, b3, Ain, FullOnlyFlag);
		//	ImposeValA(El1, El2, 0, 4, -b2, Ain, FullOnlyFlag);
		//	ImposeValA(El1, El2, 4, 4, b3, Ain, FullOnlyFlag);
		//	ImposeValA(El1, El2, 5, 5, -a2, Ain, FullOnlyFlag);
		//}
	}
}

void CVX_FEA::ImposeValA(int El1, int El2, int i, int j, float val, double* Ain, bool FullOnly) //adds a bond to the A matrix
{
	if (Ain == a){ //if we're making the main matrix
		//assumes IA, JA, set up to include this position!!!!!
		//El1, El2 are element indices of meta-block, i, j are sub-indicies, val is value to impose, FullOnly if we don't want in a matrix to solve... 
		//get the index of a:
		int ThisIndex = ia[El1*DOFperBlock+j]-1; //the row we want!
		while (ja[ThisIndex] != El2*DOFperBlock+i + 1) //go through jA until we find the right index!
			ThisIndex++;

		if (!FullOnly)
			a[ThisIndex] += val;

	}
	else { //if making little post-processing matrices
		Ain[(El2*DOFperBlock+i) + 2*DOFperBlock*(El1*DOFperBlock+j)] += val;
	}

}

void CVX_FEA::ConsolidateA() //gets rid of all the zeros for quicker solving! (maybe...)
{
	int index = 0; //master index of longer arrays
	int Shift = 0;

	for (int i=0; i<DOF; i++){ //for each element of ia;
		ia[i+1] -= Shift;
		while (index < ia[i+1]-1){ //for everything on this row...
			while (a[index + Shift] == 0){ //if this is an element to remove
				Shift++;
				ia[i+1] --;
			}
			a[index] = a[index+Shift];
			ja[index] = ja[index+Shift];

			index++;
		}
	}
}

void CVX_FEA::ApplyFixed()
{
	int DOFInd = 0;
	Vec3D<> point;
	Vec3D<> size = pEnv->pObj->GetLatDimEnv()/2.0;
	Vec3D<> WSSize = pEnv->pObj->GetWorkSpace();

	if(PrescribedDisp) for (int i=0; i<DOF; i++) x[i] =0; //zero out displacement vector to fill in with prescribed displacements (will be overwritten by pardiso eventually)

	int NumPossVox = pEnv->pObj->GetStArraySize();
	int NumVox =  pEnv->pObj->GetNumVox();
	for (int i=0; i<NumPossVox; i++){ //for all posible voxels:
		if (pEnv->pObj->GetMat(i) != 0){ //if there's a voxel here...
			FixedList[DOFInd] = false; //assume not fixed
			point = pEnv->pObj->GetXYZ(i);

			//Do fixed constraints
			for (int j = 0; j<(int)pEnv->GetNumBCs(); j++){ //go through each primitive defined as a constraint!
				CVX_FRegion* pThisBC = pEnv->GetBC(i);
				if (IS_FIXED(DOF_ALL, pThisBC->DofFixed) && pThisBC->GetRegion()->IsTouching(&point, &size, &WSSize)){ //if this point is within
					FixedList[DOFInd] = true; //set this one as fixed
					if (PrescribedDisp){
						x[DOFInd*DOFperBlock] += pThisBC->Displace.x; //note if there's a prescribed displacement...
						x[DOFInd*DOFperBlock+1] += pThisBC->Displace.y;
						x[DOFInd*DOFperBlock+2] += pThisBC->Displace.z;
					}

				}
			}
			DOFInd++;
		}
	}


	//aply reverse forces to make prescribed displacements... (x vector should have non-zero parts in it...)
	if (PrescribedDisp){
		double ToSubtractForce = 0;
		for (int i=0; i<NumVox; i++){
//			if (!FixedList[i]){ //if this one is NOT fixed 
				for (int k=0; k<DOFperBlock; k++){
					ToSubtractForce = 0;

					int Index = i*DOFperBlock+k;

					//get everything in this row: (EXCEPT the digaonal element)
					int BeginInd_0B = ia[Index]; //-1 at end includes the diagonal
					int EndInd_0B = ia[Index+1]-1;

					for (int j=BeginInd_0B; j<EndInd_0B; j++) ToSubtractForce += x[ja[j]-1]*a[j];
						
					//get everything in this column (INCLUDING the diagonal element)
					int Col_1B = ja[ia[Index]-1]; //this column in 0-based index!
					int tmpRow_1B = 1;
					for (int j=0; j<ia[Index]-1; j++){ // (was ia[NumVox*DOFperBlock]-1)
						if (j+1 == ia[tmpRow_1B-1+1]) tmpRow_1B++;


						if (ja[j] == Col_1B) ToSubtractForce += x[tmpRow_1B-1]*a[j];
						

					}

					if (ToSubtractForce != 0)
						int stop = 1;

					b[Index] -= ToSubtractForce;
				}
//			}
		}
	}


	//zero out all fixed degrees of freedom (row and column!)
	for (int i=0; i<NumVox; i++){
		if (FixedList[i]){ //if this one is fixed 
			for (int k=0; k<DOFperBlock; k++){
				int Index = i*DOFperBlock+k;

				//get everything in this row:
				int BeginInd_0B = ia[Index]; //-1 to get the diagonal
				int EndInd_0B = ia[Index+1]-1;

				for (int j=BeginInd_0B; j<EndInd_0B; j++) a[j] = 0.0;
					
				//get everything in this column
				int Col_1B = ja[ia[Index]-1]; //this column in 0-based index!
				for (int j=0; j<ia[Index]-1; j++){ //ia[NumVox*DOFperBlock]-1; j++){
					if (ja[j] == Col_1B) a[j] = 0.0;
				}

				//add back the one on the diagonal...
				if (x[Index] != 0 && b[Index] != 0)
					a[ia[Index]-1] = b[Index]/x[Index]; //add in the exact opposite so that the displacement calculates correctly
				else 
					a[ia[Index]-1] = 1;
			}
		}
	}

}

void CVX_FEA::ApplyForces()
{
//	int NumFixed = pEnv->GetNumFixedBCs();
	int NumBCs = pEnv->GetNumBCs();
	
	Vec3D<> point;
	Vec3D<> size3D = pEnv->pObj->GetLatDimEnv()/2.0;
	Vec3D<> WSSize = pEnv->pObj->GetWorkSpace();

	for (int i=0; i<DOF; i++) b[i] = 0;

	int* Sizes = new int[NumBCs]; //holds how many voxels are within each forcing region
	for (int j=0; j<NumBCs; j++) Sizes[j] = pEnv->GetNumTouching(j);

	//first, we have to count the number of voxels in the area to decide how much force each gets.
	//for (int i=0; i<pEnv->pObj->GetStArraySize(); i++){
	//	if (pEnv->pObj->GetMat(i) != 0){ //if there's a voxel here...
	//		point = pEnv->pObj->GetXYZ(i);
	//		for (int j=0; j<(int)pEnv->Forced.size(); j++){ //for each forcing region
	//			if (pEnv->Forced[j].pRegion->IsTouching(&point, &size3D, &WSSize)){
	//				Sizes[j]++;
	//			}
	//		}
	//	}
	//}

	for (int i=0; i<pEnv->pObj->GetStArraySize(); i++){
		if (pEnv->pObj->GetMat(i) != 0){ //if there's a voxel here...
			point = pEnv->pObj->GetXYZ(i);

			for (int j=0; j<NumBCs; j++){
				if (IS_ALL_FIXED(pEnv->GetBC(j)->DofFixed)) continue;
//				int size = Sizes[j];
				if (pEnv->GetBC(j)->GetRegion()->IsTouching(&point, &size3D, &WSSize)){
					b[IndextoDOF[i]*DOFperBlock] += pEnv->GetBC(j)->Force.x/Sizes[j];
					b[IndextoDOF[i]*DOFperBlock+1] += pEnv->GetBC(j)->Force.y/Sizes[j];
					b[IndextoDOF[i]*DOFperBlock+2] += pEnv->GetBC(j)->Force.z/Sizes[j];
				}
			}
		}
	}

	delete[] Sizes;
	Sizes = NULL;

}

void CVX_FEA::CalcForces() //fills in forces, reaction forces, and strains!!!
{
	for (int i=0; i<DOF; i++){ //set all to zero force
		F[i] = 0;
		e[i] = 0; //internal strains
		b[i] = 0; //do reactions here.... (if we want)
	}
	for (int i=0; i<pEnv->pObj->GetNumVox(); i++){ 
		MaxSE[i] = 0; 
	}
	
	//set up a smaller F = Kx matrix involving just the two blocks involved in each bond (to calculate "F")
	double* TmpK = new double[4*DOFperBlock*DOFperBlock]; //technically only need the upper diagonal
	double* TmpX = new double[2*DOFperBlock]; 
	double* TmpF = new double[2*DOFperBlock];

	double* FBig = new double [DOF*3]; //one for each dimension, so we can keep track of multiple bonds getting added up & average
	for (int i=0; i<3*DOF; i++){FBig[i] = 0;} //initialize to zeros

	for(int bond=0; bond<NumBonds; bond++){ //for each bond!
		int BondDirec = BondDir[bond]; //which direction is this?
		int El1 = IndextoDOF[Indi[bond]];
		int El2 = IndextoDOF[Indj[bond]];

		for (int i=0; i<4*DOFperBlock*DOFperBlock; i++) //zero out them all
			TmpK[i] = 0; 
		for (int i=0; i<2*DOFperBlock; i++){
			TmpX[i] = 0; 
			TmpF[i] = 0; 
		}

		MakeBond(TmpK, bond);

		for (int i=0; i<DOFperBlock; i++){ //set up our little displacement vector
			TmpX[i] = x[El1*DOFperBlock+i];
			TmpX[DOFperBlock + i] = x[El2*DOFperBlock+i];
		}

		//multiply out!
		for (int i=0; i<2*DOFperBlock; i++){ //cycle through each row
			for (int j=i; j<2*DOFperBlock; j++){ //cycle through each column (only for upper diag)
				TmpF[i] += (TmpK[i*2*DOFperBlock+j]*TmpX[j]);
				if (i != j)
					TmpF[j] += (TmpK[i*2*DOFperBlock+j]*TmpX[i]);
			}
		}

		float StrainEnergy = 0;
		for (int i=0; i<2*DOFperBlock; i++){ //cycle through each row
			StrainEnergy += (float)(TmpX[i]*TmpF[i]); //abs value already
		}

		//StrainEnergy = abs(StrainEnergy); //only care about magnitude!
		//check against both bonds!
		if (StrainEnergy > MaxSE[El1])
			MaxSE[El1] = StrainEnergy;
		if (StrainEnergy > MaxSE[El2])
			MaxSE[El2] = StrainEnergy;

		for (int i=0; i<DOFperBlock; i++) { //local forces to universal coordinates
			int CurIndex = DOF*BondDirec + El1*DOFperBlock+i; //first element
			if (FBig[CurIndex] == 0)
				FBig[CurIndex] = (float)abs(TmpF[i]);
			else
				FBig[CurIndex] = (float)(FBig[CurIndex] + abs(TmpF[i]))/2;

			CurIndex = DOF*BondDirec + El2*DOFperBlock+i;

			if (FBig[CurIndex] == 0)
				FBig[CurIndex] = (float)abs(TmpF[DOFperBlock + i]);
			else
				FBig[CurIndex] = (float)(FBig[CurIndex] + abs(TmpF[DOFperBlock + i]))/2;


			b[El1*DOFperBlock+i] += TmpF[i];
			b[El2*DOFperBlock+i] += TmpF[DOFperBlock + i];
		}

		float LinearStrain = (float)(abs(x[El1*DOFperBlock + BondDirec] - x[El2*DOFperBlock + BondDirec])/pEnv->pObj->GetLatticeDim()); //simple volumetric strain (no shear...)
		if (LinearStrain > e[El1*DOFperBlock + BondDirec]) e[El1*DOFperBlock + BondDirec] = LinearStrain;
		if (LinearStrain > e[El2*DOFperBlock + BondDirec]) e[El2*DOFperBlock + BondDirec] = LinearStrain;

	}

	//render FBig to F!
	for (int i=0; i<DOF; i++){ //each element
		for (int j=0; j<3; j++){ //each direction of possible bond
			F[i] += FBig[DOF*j + i];
		}
	}

	delete[]TmpK;
	TmpK = NULL;
	delete[]TmpX;
	TmpX = NULL;
	delete[]TmpF;
	TmpF = NULL;
	delete[]FBig;
	FBig = NULL;

	FindMaxOverall(&Reaction, b, MaxReactions);
	FindMaxOverall(&Force, F, MaxForces);
	FindMaxOverall(&Strain, e, MaxStrains);


}

void CVX_FEA::FindMaxOverall(INFO3D* info3D, double* Ar, float* EachMax) //calculate maximums overall and for each voxel
{
	info3D->Max = 0;
	info3D->MaxX = 0;
	info3D->MaxY = 0;
	info3D->MaxZ = 0;

	for(int i=0; i<DOF; i++){
		switch (i%DOFperBlock){
		case 0: //X
			if (abs(Ar[i]) > info3D->MaxX) info3D->MaxX = (float)abs(Ar[i]); 

			//maximum (could do anywhere, but here for convenience)
			EachMax[i/DOFperBlock] = (float)(sqrt(Ar[i]*Ar[i] + Ar[i+1]*Ar[i+1] + Ar[i+2]*Ar[i+2]));
			if (EachMax[i/DOFperBlock] > info3D->Max) info3D->Max = EachMax[i/DOFperBlock];

			break;
		case 1: //Y
			if (abs(Ar[i]) > info3D->MaxY) info3D->MaxY = (float)abs(Ar[i]); break;
		case 2: //Z
			if (abs(Ar[i]) > info3D->MaxZ) info3D->MaxZ = (float)abs(Ar[i]); break;
		}
	}
}

#ifdef USE_OPEN_GL

void CVX_FEA::DrawFEA(int Selected)
{
	if (SolnExists){ //will draw VXC if no simulation data
		
		Vec3D<> Center;
		char curMat = 0;
		char thismat = 0;
		CVXC_Material* pMaterial;

		int ToLayer = (int)(ViewZChop*pEnv->pObj->GetVZDim());

		for (int i = 0; i<ToLayer*pEnv->pObj->GetVXDim()*pEnv->pObj->GetVYDim(); i++) //go through all the voxels...
		{
			thismat = (char)pEnv->pObj->GetMat(i);
			if (thismat != 0) // present AND visible
			{
				pMaterial = pEnv->pObj->GetLeafMat(i);
				int index = IndextoDOF[i];

				if (pMaterial != NULL && index >= 0 && index < pEnv->pObj->GetStArraySize()){
					float Col = 0; //Color to display (normalized value!)

					switch (ViewMode){
						case VIEW_DISP:
							if (ViewModeDir == MAXDIR && Disp.Max != 0) Col = MaxDisps[index]/Disp.Max; 
							else if (ViewModeDir == XDIR && Disp.MaxX != 0) Col = (float)abs(x[index*DOFperBlock])/Disp.MaxX;
							else if (ViewModeDir == YDIR && Disp.MaxY != 0) Col = (float)abs(x[index*DOFperBlock+1])/Disp.MaxY;
							else if (ViewModeDir == ZDIR && Disp.MaxZ != 0) Col = (float)abs(x[index*DOFperBlock+2])/Disp.MaxZ;
							break;
						case VIEW_FORCE:
							if (ViewModeDir == MAXDIR && WantForces && MaxForces != NULL && Force.Max != 0) Col = ((float)MaxForces[index])/Force.Max;
							if (ViewModeDir == XDIR && WantForces && F != NULL && Force.MaxX != 0) Col = (float)abs(F[index*DOFperBlock])/Force.MaxX;
							if (ViewModeDir == YDIR && WantForces && F != NULL && Force.MaxY != 0) Col = (float)abs(F[index*DOFperBlock+1])/Force.MaxY;
							if (ViewModeDir == ZDIR && WantForces && F != NULL && Force.MaxZ != 0) Col = (float)abs(F[index*DOFperBlock+2])/Force.MaxZ;
							break;
						case VIEW_STRAIN: 
							if (ViewModeDir == MAXDIR && WantForces && MaxStrains != NULL && Strain.Max != 0) Col = ((float)MaxStrains[index])/Strain.Max;
							if (ViewModeDir == XDIR && WantForces && Strain.MaxX != 0) Col = (float)abs(e[index*DOFperBlock])/Strain.MaxX;
							if (ViewModeDir == YDIR && WantForces && Strain.MaxY != 0) Col = (float)abs(e[index*DOFperBlock+1])/Strain.MaxY;
							if (ViewModeDir == ZDIR && WantForces && Strain.MaxZ != 0) Col = (float)abs(e[index*DOFperBlock+2])/Strain.MaxZ; 
							break;
						case VIEW_REACTION: 
							if (ViewModeDir == MAXDIR && WantForces && MaxReactions != NULL && Reaction.Max != 0) Col = ((float)MaxReactions[index])/Reaction.Max;
							if (ViewModeDir == XDIR && WantForces && Reaction.MaxX != 0) Col = (float)abs(b[index*DOFperBlock])/Reaction.MaxX;
							if (ViewModeDir == YDIR && WantForces && Reaction.MaxY != 0) Col = (float)abs(b[index*DOFperBlock+1])/Reaction.MaxY;
							if (ViewModeDir == ZDIR && WantForces && Reaction.MaxZ != 0) Col = (float)abs(b[index*DOFperBlock+2])/Reaction.MaxZ; 
							break;

					}

					float Alpha = 1.0f;
					if (Col < ViewThresh)
						Alpha = 0.0f;

					//Jet colormap!
					if (Col > 0.75) glColor4d(1, 4-Col*4, 0, Alpha);
					else if (Col > 0.5) glColor4d(Col*4-2, 1, 0, Alpha);
					else if (Col > 0.25) glColor4d(0, 1, 2-Col*4, Alpha);
					else glColor4d(0, Col*4, 1, Alpha);
					
					if (i==Selected) glColor4d(1.0, 0.0, 1.0, 1.0); //to show current selected

					glPushMatrix();

					glLoadName (i); //to enable picking
					pEnv->pObj->GetXYZ(&Center, i);
					if (ViewDefPerc != 0.0f){
						glTranslated(Center.x + x[index*DOFperBlock] / Disp.Max * pEnv->pObj->GetLatticeDim() * ViewDefPerc, Center.y + x[index*DOFperBlock+1]/Disp.Max*pEnv->pObj->GetLatticeDim()*ViewDefPerc, Center.z + x[index*DOFperBlock+2]/Disp.Max*pEnv->pObj->GetLatticeDim()*ViewDefPerc);
						if (DOFperBlock == 6){ //if we have rotation data
							glRotated(x[index*DOFperBlock+3]*360/(2*3.14159) / Disp.Max * pEnv->pObj->GetLatticeDim() * ViewDefPerc, 1, 0, 0);
							glRotated(x[index*DOFperBlock+4]*360/(2*3.14159) / Disp.Max * pEnv->pObj->GetLatticeDim() * ViewDefPerc, 0, 1, 0);
							glRotated(x[index*DOFperBlock+5]*360/(2*3.14159) / Disp.Max * pEnv->pObj->GetLatticeDim() * ViewDefPerc, 0, 0, 1);

						}
					}
					else 
						glTranslated(Center.x, Center.y, Center.z);

					Center = Vec3D<>(0,0,0);
					pEnv->pObj->DrawVoxel(&Center, pEnv->pObj->GetLatticeDim());
					glPopMatrix();
				}
			}
		}
	}
	else if (pEnv->pObj) //if viewing VXC object
		pEnv->pObj->Draw();  //TODO!!!

}
#endif

float CVX_FEA::GetMagProperty(int px, int py, int pz, int dir, double* Ar, float* Maxs)
{
//	if (Ar != x && (!WantForces || Ar == NULL)) //if looking for a force-derived output, but doesn't exist
//		QMessageBox::warning(NULL, "Warning", "Set \"WantForces\" flag to true & restart to calculate internal forces!");

	int index = pEnv->pObj->GetIndex(px, py, pz);
	if (index != -1 && pEnv->pObj->GetMat(index) != 0){ //if a valid index...
		if (dir == XDIR) //return X force
			return (float)Ar[(int)(IndextoDOF[index]*DOFperBlock+XDIR)];
		else if (dir == YDIR) //return Y force
			return (float)Ar[IndextoDOF[index]*DOFperBlock+YDIR];
		else if (dir == ZDIR) //return Z force
			return (float)Ar[IndextoDOF[index]*DOFperBlock+ZDIR];
		else //return magnitude
			return Maxs[IndextoDOF[index]];
	}
	else
		return -1;
}

float CVX_FEA::GetMaxProperty(INFO3D* info3D, int dir)
{
	switch (dir){
		case XDIR: return info3D->MaxX; break;
		case YDIR: return info3D->MaxY; break;
		case ZDIR: return info3D->MaxZ; break;
		case MAXDIR: return info3D->Max; break;
	}
	return -1; //if error...
}

Vec3D<> CVX_FEA::GetDisp(CVX_FRegion* pRegion) //average displacement of a region! 
{
	CVX_FRegion Region = *pRegion;

//	Vec3D WS = pObj->GetWorkSpace();
//	Region.ScaleTo(Vec3D(1,1,1), pObj->GetWorkSpace());
	Vec3D<> point;
	Vec3D<> size = pEnv->pObj->GetLatDimEnv()/2.0;
	Vec3D<> WSSize = pEnv->pObj->GetWorkSpace();
	Vec3D<> Dist(0,0,0);
	int NumAdded = 0;

	for (int i=0; i< pEnv->pObj->GetStArraySize(); i++){
		if (pEnv->pObj->GetMat(i) != 0){ //if there's a voxel here...
			point = pEnv->pObj->GetXYZ(i);
			
			if(Region.GetRegion()->IsTouching(&point, &size, &WSSize)){ //if touching
				Dist += Vec3D<>(x[IndextoDOF[i]*DOFperBlock+XDIR], x[IndextoDOF[i]*DOFperBlock+YDIR], x[IndextoDOF[i]*DOFperBlock+ZDIR]);
				NumAdded++;
			}
		}
	}
	if (NumAdded == 0)
		return Vec3D<>(0,0,0);
	else 
		return Dist/NumAdded;
}







void CVX_FEA::GetFEAInfoStr(std::string* pString)
{
	if (SolnExists && pString){
		std::stringstream strm;
		switch (ViewMode){
			case VIEW_DISP:
				strm << "Maximum Displacement: \n";
				strm << "Max: " << std::setprecision(3) << GetMaxDisp(MAXDIR)*1000 << "mm\n";
				strm << "  X Dir: " << GetMaxDisp(XDIR)*1000 << "mm\n";
				strm << "  Y Dir: " << GetMaxDisp(YDIR)*1000 << "mm\n";
				strm << "  Z Dir: " << GetMaxDisp(ZDIR)*1000 << "mm\n";
				strm << "\nAverage BC Displacements:\n";
				for (int i=0; i<pEnv->GetNumBCs(); i++){
					Vec3D<> tmpDisp = GetDisp(pEnv->GetBC(i));
					strm << "Forced" << i << ": (" << std::setprecision(3) << tmpDisp.x*1000 << ", " << tmpDisp.y*1000 << ", " << tmpDisp.z*1000 << ") mm\n";
				}
				break;
			case VIEW_FORCE:
				strm << "Maximum TODO Stress: \n";
				strm << "Max: " << std::setprecision(3) << GetMaxForce(MAXDIR) << "N\n";
				strm << "  X Dir: " << GetMaxForce(XDIR) << "N\n";
				strm << "  Y Dir: " << GetMaxForce(YDIR)<< "N\n";
				strm << "  Z Dir: " << GetMaxForce(ZDIR)<< "N\n";
				break;
			case VIEW_STRAIN:
				strm << "Maximum Strain: \n";
				strm << "Max: " << std::setprecision(3) << GetMaxStrain(MAXDIR) << "\n";
				strm << "  X Dir: " << GetMaxStrain(XDIR) << "\n";
				strm << "  Y Dir: " << GetMaxStrain(YDIR) << "\n";
				strm << "  Z Dir: " << GetMaxStrain(ZDIR) << "\n";
				break;
			case VIEW_REACTION:
				strm << "Maximum Reaction: \n";
				strm << "Max: " << std::setprecision(3) << GetMaxReaction(MAXDIR)<< "N\n";
				strm << "  X Dir: " << GetMaxReaction(XDIR)<< "N\n";
				strm << "  Y Dir: " << GetMaxReaction(YDIR)<< "N\n";
				strm << "  Z Dir: " << GetMaxReaction(ZDIR)<< "N\n";
				break;

		}
		*pString = strm.str();
	}
}

void CVX_FEA::GetFEAInfoStr(int VoxIndex, std::string* pString)
{
	if (SolnExists && pString){
		int x, y, z;
		pEnv->pObj->GetXYZNom(&x, &y, &z, VoxIndex);
		std::stringstream strm;
		switch (ViewMode){
			case VIEW_DISP:
				strm << " Voxel Displacement: (" << x << "," << y << "," << z <<")\n";
				strm << "Mag: " << std::setprecision(3) << GetMagDisp(x, y, z, MAXDIR)*1000 << "mm\n";
				strm << "  X Dir: " << GetMagDisp(x, y, z, XDIR)*1000 << "mm\n";
				strm << "  Y Dir: " << GetMagDisp(x, y, z, YDIR)*1000 << "mm\n";
				strm << "  Z Dir: " << GetMagDisp(x, y, z, ZDIR)*1000 << "mm\n";
				break;
			case VIEW_FORCE:
				strm << "Maximum TODO Stress: (" << x << "," << y << "," << z <<")\n";
				strm << "Max: " << std::setprecision(3) << GetMagForce(x, y, z, MAXDIR) << "N\n";
				strm << "  X Dir: " << GetMagForce(x, y, z, XDIR) << "N\n";
				strm << "  Y Dir: " << GetMagForce(x, y, z, YDIR)<< "N\n";
				strm << "  Z Dir: " << GetMagForce(x, y, z, ZDIR)<< "N\n";
				break;
			case VIEW_STRAIN:
				strm << "Maximum Strain: (" << x << "," << y << "," << z <<")\n";
				strm << "Max: " << std::setprecision(3) << GetMagStrain(x, y, z, MAXDIR) << "\n";
				strm << "  X Dir: " << GetMagStrain(x, y, z, XDIR) << "\n";
				strm << "  Y Dir: " << GetMagStrain(x, y, z, YDIR) << "\n";
				strm << "  Z Dir: " << GetMagStrain(x, y, z, ZDIR) << "\n";
				break;
			case VIEW_REACTION:
				strm << "Maximum Reaction: (" << x << "," << y << "," << z <<")\n";
				strm << "Max: " << std::setprecision(3) << GetMagReaction(x, y, z, MAXDIR)<< "N\n";
				strm << "  X Dir: " << GetMagReaction(x, y, z, XDIR)<< "N\n";
				strm << "  Y Dir: " << GetMagReaction(x, y, z, YDIR)<< "N\n";
				strm << "  Z Dir: " << GetMagReaction(x, y, z, ZDIR)<< "N\n";
				break;
		}
		*pString = strm.str();
	}

}


void CVX_FEA::OutputMatrices()
{ 
	std::ofstream file("A_Matrix.txt"); //filename);
	if (ia){
		for (int j=0; j<ia[DOF-1]; j++){
			if (a) file << a[j] << "\t";
//			if (WantReactions && aFull) file << aFull[j] << "\t";
			if (ja) file << ja[j] << "\n";
			}
		file << "\n";

		for (int j=0; j<DOF+1; j++){
			file << ia[j] << "\t";
		}
		file << "\n\n";
	}

	if (Indi && Indj && BondDir){
		for (int j=0; j<NumBonds; j++){
			file << Indi[j] << "\t" << Indj[j] << "\t" << BondDir[j] << "\n";
		}
		file << "\n \n";
	}
	if (x && b){
		for (int j=0; j<DOF; j++){
			file << x[j] << "\t";
			file << b[j] << "\t";
			file << F[j] << "\n";
		}
	}
	file.close();
	
}


