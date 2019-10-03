/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef MESH_H
#define MESH_H

#include <vector>
#include "Vec3D.h"
#include "XML_Rip.h"

//#include "VX_Object.h"

#ifdef USE_OPEN_GL
#include "GL_Utils.h" //really just need the CColor container from here
#else
//basic RGB color container
struct CColor { //copied from GL_Utils.h
	CColor(){r=-1; g=-1; b=-1;a=-1;};
	CColor (const CColor& C) {r = C.r; g = C.g; b = C.b; a = C.a;};
	CColor(vfloat rIn, vfloat gIn, vfloat bIn, vfloat aIn=1.0){r=rIn; g=gIn; b=bIn; a=aIn;};
	vfloat r, g, b, a;
	bool isValid(void){if(r>=0.0 && r <=1.0 && g>=0.0 && g <=1.0 && b>=0.0 && b <=1.0 && a>=0.0 && a <=1.0) return true; else return false;};

};
#endif

//structure to hold each vertex
struct CVertex {
	CVertex() {n = Vec3D<>(0,0,0); v = Vec3D<>(0,0,0); VColor = CColor(1,1,1,1); DrawOffset = Vec3D<>(0,0,0);}
	CVertex(const Vec3D<>& av) {n =  Vec3D<>(0,0,0); v = av; VColor = CColor(1,1,1,1); DrawOffset = Vec3D<>(0,0,0);}
	CVertex(const Vec3D<>& an, const Vec3D<>& av) {n = an; v = av; VColor = CColor(1,1,1,1); DrawOffset = Vec3D<>(0,0,0);}
	CVertex(const Vec3D<>& av, const CColor& ac) {n = Vec3D<>(0,0,0); v = av; VColor = ac; DrawOffset = Vec3D<>(0,0,0);}
	inline CVertex& operator=(const CVertex& V) {n=V.n; v=V.v; VColor = V.VColor; DrawOffset = V.DrawOffset; return *this;}; //overload equals
	CVertex(const CVertex& V) {*this = V;} // DrawAxis = V.DrawAxis; DrawAngle = V.DrawAngle;}
	Vec3D<> OffPos(void) {return v+DrawOffset;} //return the deformed position to draw this one...

	void WriteXML(CXML_Rip* pXML);
	bool ReadXML(CXML_Rip* pXML);

	Vec3D<> n; //normal
	Vec3D<> v; //Vertex location
	CColor VColor;
	Vec3D<> DrawOffset; //apply offset to each point
};

//structure to hold each facet
struct CFacet {
	CFacet() {n=Vec3D<>(0,0,0); FColor = CColor(1,1,1,1); vi[0] = 0; vi[1] = 0; vi[2] = 1; Name = -1;}
	CFacet(const int& av1, const int& av2, const int& av3) { n = Vec3D<>(0,0,0); vi[0] = av1; vi[1] = av2; vi[2] = av3; FColor = CColor(1,1,1,1); Name = -1;}
	CFacet(const Vec3D<>& an, const int& av1, const int& av2, const int& av3) { n = an; vi[0] = av1; vi[1] = av2; vi[2] = av3; FColor = CColor(1,1,1,1); Name = -1;}
	CFacet(const Vec3D<>& an, const int& av1, const int& av2, const int& av3, const int& NIn) { n = an; vi[0] = av1; vi[1] = av2; vi[2] = av3; FColor = CColor(1,1,1,1); Name = NIn;}
	CFacet(const Vec3D<>& an, const int& av1, const int& av2, const int& av3, const CColor& ac) { n = an; vi[0] = av1; vi[1] = av2; vi[2] = av3; FColor = ac; Name = -1;}
	inline CFacet& operator=(const CFacet& p) { vi[0]=p.vi[0]; vi[1]=p.vi[1]; vi[2]=p.vi[2]; n=p.n; FColor = p.FColor; Name = p.Name; return *this;}; //overload equals
	CFacet(const CFacet& p) {*this = p;}

	Vec3D<> n; //normal
	CColor FColor;
	int vi[3]; //vertex indices
	int Name; //my name (for GL picking)
	Vec3D<> drag; //nac: water
	Vec3D<> speed; 

};

struct CLine {
	CLine() {}
	CLine(const int& av1, const int& av2) {vi[0] = av1; vi[1] = av2;}
	inline CLine& operator=(const CLine& l) { vi[0]=l.vi[0]; vi[1]=l.vi[1]; return *this;}; //overload equals
	CLine(const CLine& l) {*this = l;}
	friend bool operator<(const CLine& L1, const CLine& L2) { if (L1.vi[0] == L2.vi[0]) return L1.vi[1] < L2.vi[1]; else return L1.vi[0] < L2.vi[0];} //for sorting
	friend bool operator==(const CLine& L1, const CLine& L2) {return ((L1.vi[0]==L2.vi[0] && L1.vi[1]==L2.vi[1]));} // || (vi[0]==O.vi[1] && vi[1]==O.vi[0]));} //Is equal

	int vi[2];
};


class CMesh
{
public:
	CMesh(void);
	~CMesh(void);

	CMesh(CMesh& s);
	CMesh& operator=(const CMesh& s);

	void WriteXML(CXML_Rip* pXML, bool MeshOnly = false); //meshonly only store stl-equiavalent info (no color, normals, etc...)
	bool ReadXML(CXML_Rip* pXML);

	bool Exists(void) {if (Facets.size() != 0) return true; else return false;}
	std::vector<CFacet> Facets;
	std::vector<CVertex> Vertices;
	std::vector<CLine> Lines;

	Vec3D<> _CurBBMin, _CurBBMax; //cached bounding box values... (UpdateBoundingBox to update())

	// file i/o
	bool LoadSTL(std::string filename);
	bool SaveSTL(std::string filename, bool Binary = true, bool SaveDeformed = false) const;

	bool DrawSmooth;
	void CalcFaceNormals(); //called to update the face normals...
	void CalcVertNormals(); //called once for each new geometry (or after deformed...) (depends on face normals!!!)

#ifdef USE_OPEN_GL
	void Draw(bool bModelhNormals = false, bool bShaded = true, bool bIngoreColors = false, bool bIgnoreNames = false, bool drawForces = false, int whichVectors = 0, float scalingFactor = 0.0); //requires OpenGL libs
#endif

	//add a facet
	void AddFacet(const Vec3D<>& v1, const Vec3D<>& v2, const Vec3D<>& v3, bool QuickAdd = false); //adds a facet, checks vertex list for existing vertices... (should be called with vertices in CCW for proper normal calculation!
	void AddFacet(const Vec3D<>& v1, const Vec3D<>& v2, const Vec3D<>& v3, const CColor& Col1, const CColor& Col2, const CColor& Col3, bool QuickAdd = false); //adds a facet... with color info
	void AddQuadFacet(const Vec3D<>& v1, const Vec3D<>& v2, const Vec3D<>& v3, const Vec3D<>& v4) {AddFacet(v1, v2, v3); AddFacet(v3, v4, v1);}; //Vertices should be CCW from outside

	// clear/reset the list of trianges
	void Clear() { Facets.clear(); Vertices.clear(); Lines.clear();}
	
	void ComputeBoundingBox(Vec3D<>& pmin, Vec3D<>& pmax);
	void UpdateBoundingBox(void);
	Vec3D<> GetBBMin(void) {return _CurBBMin;}
	Vec3D<> GetBBMax(void) {return _CurBBMax;}
	Vec3D<> GetBBSize(void) {return _CurBBMax - _CurBBMin;}


	//manipulation...
	void Scale(Vec3D<> d);
	void Translate(Vec3D<> d);
	void Rotate(Vec3D<> ax, vfloat a);
	void RotX(vfloat a);
	void RotY(vfloat a);
	void RotZ(vfloat a);

	//voxelizing stuff!
	void MeshChanged(void); //invalidates all cached voxelizing info!

	bool IsInside(Vec3D<>* Point);
	std::vector<int> TriLayer; //array of all triangle indices that cross the current z height
	std::vector<vfloat> TriLine; //array of all intersection points at the current z height at the curernt Y vale
	vfloat _TriLayerZ, _TriLineY; //height we calculated TriHeight, TriLine at
	void FillTriLayer(vfloat z); //fills in TriLayer with all triangles that bridge this plane
	void FillTriLine(vfloat y, vfloat z); //fills in TriLine with all triangle the bridge the line in the layer


	int GetXIntersections(vfloat z, vfloat y, vfloat* pIntersections, int NumtoCheck, int* pToCheck); //returns the number of intersections, stored in pIntersections
	


	static bool InsideTri(Vec3D<>& p, Vec3D<>& v0, Vec3D<>& v1, Vec3D<>& v2);
	static vfloat Det(Vec3D<>& v0, Vec3D<>& v1, Vec3D<>& v2);
	bool IntersectXRay(CFacet* pFacet, vfloat y, vfloat z, Vec3D<>& p, vfloat& pu, vfloat& pv);

	void WeldClose(float Distance); //welds vertices that are nearby (within Distance). Removes deleted triangles...
	void RemoveDupLines(void);

protected:
	bool LoadBinarySTL(std::string filename);
	bool LoadAsciiSTL(std::string filename);
};
#endif
