/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef DM_MESHUTIL_H
#define DM_MESHUTIL_H

#include "Utils/Mesh.h"
#include <fstream>

class CVX_Sim;
class CVX_FEA;
class CVX_Object;
class CVXS_SimGLView;

//corners
#define NNN 0
#define NNP 1
#define NPN 2
#define NPP 3
#define PNN 4
#define PNP 5
#define PPN 6
#define PPP 7


struct CVertexComp { //a component of a inter-cube vertex average
//	CVertexComp() {}
	CVertexComp(int XIndexIn, int CornerIn) {XIndex = XIndexIn; Corner=CornerIn; Off = Vec3D<>(0,0,0); Weight = 1.0;}
	CVertexComp(int XIndexIn, Vec3D<> OffIn, vfloat WeightIn) {XIndex = XIndexIn; Corner=-1; Off = OffIn;; Weight = WeightIn;}
	inline CVertexComp& operator=(const CVertexComp& V) {XIndex=V.XIndex; Corner=V.Corner; Off=V.Off; Weight=V.Weight; return *this;}; //overload equals
	CVertexComp(const CVertexComp& V) {*this = V;}

	int XIndex; //voxel index (original VXC index)

	int Corner; //which corner is this? (According to NNN, NNP, etc. if -1, use Off and Weight.
	//or
	Vec3D<> Off; //Absolute offset in initial voxel orientation
	vfloat Weight; //how much should this component affect the calculated vetex position
};

struct CVertexCalc {
	CVertexCalc() {}
	inline CVertexCalc& operator=(const CVertexCalc& V) {ConVoxels=V.ConVoxels; return *this;}; //overload equals
	CVertexCalc(const CVertexCalc& V) {*this = V;} 

//	void UpdateVertexLocation(CVertex* pVertToUpdate, CVX_Sim* pSim); //todo... probably don't make this a local function, just use this as container...
	std::vector<CVertexComp> ConVoxels;

};

class CVX_MeshUtil
{
public:
	CVX_MeshUtil(void);
	~CVX_MeshUtil(void);

	void Clear(void);

	CMesh DefMesh;
	std::vector<CVertexCalc> CalcVerts; //same size (and order) as the vertices in the mesh
	std::vector<CVertexCalc> CalcVertsAll; //Every possible vertex point! (iterating in x, y,z, size one greater than number of voxels in each dimension
	std::vector<int> FacetToSIndex; //same size as the number of facets, contains the Simulation index of this voxel (to grab current color info!)

	void LinkSimExisting(CVX_Sim* pSimIn, CVXS_SimGLView* pSimViewIn, CMesh* pMeshIn=NULL); //Links existing mesh and simulation
	void LinkSimSmooth(CVX_Sim* pSimIn, CVXS_SimGLView* pSimViewIn); //creates smooth (marching cubes) mesh abd links to simulation
	void LinkSimVoxels(CVX_Sim* pSimIn, CVXS_SimGLView* pSimViewIn); //creates a voxel mesh and links to simulation


	void GetCurVLoc(CVertexCalc& VertCalc, Vec3D<>* pLocOut);
	void GetCurVCol(CVertexCalc& VertCalc, CColor* pColOut, int CurSel);
	inline void GetCornerDir(int Corner, Vec3D<>* pOut); //returns vector with component +/- 1 dependeind on which corner

	void UpdateMesh(int CurSel = -1); //updates mesh based on linked FEA/Relaxation
	void UpdateMeshPhysicsOnlyNoColors(int CurSel = -1); //updates mesh based on linked FEA/Relaxation
	

	void Draw(bool plottingForces = false, int curVectPlot = 0, float vectorsScalingView = 0.0);

	//misc
	bool ToStl(std::string BasePath, CVX_Object* pObj, bool WantDefMes = false);
	bool FromStl(CMesh* pMeshIn, CVX_Object* pObj, int MatIndex);

//	// VOLUMETRIC FUNCTIONS
	void initializeDeformableMesh(CVX_Sim* pSimIn){ LinkSimVoxels(pSimIn, NULL); DefMesh.DrawSmooth=false; }
	void updateDeformableMesh(){ UpdateMesh(-1); }
	int getMeshVertNum(){ return DefMesh.Vertices.size(); }
	int getMeshFacetsNum(){ return DefMesh.Facets.size(); }
	void printMeshNormals();
	void printMeshVertices();
	void printMeshFacets();
	void printAllMeshInfo();

private:
	bool usingGUI;

//	void ImportSimWithMesh(CVX_Sim* pSimIn, CVXS_SimGLView* pSimViewIn, CMesh* pMeshIn); //links the simulation to an existing surface mesh so it can be deformed correctly
//	void ImportLinkSim(CVX_Sim* pSimIn, CVXS_SimGLView* pSimViewIn); //creates a surface mesh to look like the voxels... (for rectangualr voxels...)
	CVX_Sim* pSim;
	CVXS_SimGLView* pSimView; //todo: refine this (mildly redundant)

	void ImportLinkFEA(CVX_FEA* pFEAIn); //not yet implemented
	CVX_FEA* pFEA;

	inline int D3IndCorner(int XInd, int YInd, int ZInd, int XSize, int YSize, const int Corner); //returns the vertex index in the [x+1, y+1, z+1] CalcVertsAll array corresponding to the specified corner of this voxel.
	inline int D3Ind(const int X, const int Y, const int Z, const int sX, const int sY){return Z*sX*sY + Y*sX + X;};

};

#endif