/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/


#ifndef MARCHCUBE_H
#define MARCHCUBE_H

#include "Mesh.h"
#include "Array3D.h"

//takes a vector of 3D arrays of floats and turns them into an iso-surface...
//class CArray3Df;

typedef struct {
   Vec3D<> p[3];
} TRIANGLE;

typedef struct {
   CVertex p[8];
   double val[8];
} GRIDCELL;

class CMarchCube
{
public:
	CMarchCube(void);
	~CMarchCube(void);

	static void SingleMaterial(CMesh* pMeshOut, CArray3Df* pArray, float Thresh = 0.0f, float Scale = 1.0f);
	
	static void SingleMaterialMultiColor(CMesh* pMeshOut, CArray3Df* pArray, CArray3Df* rColorArray, CArray3Df* gColorArray, CArray3Df* bColorArray, float Thresh = 0.0f, float Scale = 1.0f);
	
	static void MultiMaterial(CMesh* pMeshOut, void* pArrays, bool SumMat, CColor* pColors = NULL, float Thresh = 0.0f, float Scale = 1.0f);

	static int Polygonise(GRIDCELL grid, double iso, CMesh* pMeshOut);
	static int PolygoniseCube(GRIDCELL grid, double iso, CMesh* pMeshOut);
	static int PolygoniseTet(GRIDCELL g, double iso, CMesh* pMeshOut, int v0,int v1,int v2,int v3);
	static CVertex VertexInterp(double iso, CVertex p1, CVertex p2, double valp1, double valp2);
};

#endif
