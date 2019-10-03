/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef CVX_FRegion_H
#define CVX_FRegion_H

#include "Utils/Vec3D.h"
#include "Utils/XML_Rip.h"
#include "Utils/Mesh.h"

class CPrimitive;
class CP_Box;
class CP_Cylinder;
class CP_Sphere;
class CP_Mesh;


#define PRIM_BOX 0 //pasted.. just something to keep track of different primitives
#define PRIM_CYLINDER 1
#define PRIM_SPHERE 2
#define PRIM_MESH 3

#define DOF_X 0x01
#define DOF_Y 0x02
#define DOF_Z 0x04
#define DOF_TX 0x08
#define DOF_TY 0x10
#define DOF_TZ 0x20
#define DOF_ALL 0x3F
#define DOF_NONE 0x00
static inline bool IS_FIXED(char Dof, char C){return C&Dof;} //For DOF_ALL, returns true if ANY are fixed, false if none are fixed.
static inline bool IS_ALL_FIXED(char C){return (C&DOF_ALL) == DOF_ALL;} //returns true if EVERYTHING is fixed, false otherwise
static inline bool IS_NONE_FIXED(char C){return ~(C&DOF_ALL);} //returns true if NOTHING is fixed, false otherwise
static inline void SET_FIXED(char Dof, char& C, bool Fixed){Fixed ? C |= Dof : C &= ~Dof;} //Performs as expected for DOF_ALL

//
class CVX_FRegion //holds a region and directed force...
{
public:
	CVX_FRegion(void);
//	CVX_FRegion(Vec3D& BoxMin, Vec3D& BoxSize, bool FixedIn, Vec3D& ForceOrDisp); //if Fixed, ForceOrDisp is the displacement of the region. if !Fixed, ForceOrDisp is the force in newtons applied.
	~CVX_FRegion(void);
	CVX_FRegion(const CVX_FRegion& iFRegion); //copy constructor
	CVX_FRegion& operator=(const CVX_FRegion& iFRegion); //overload "=" 
	CVX_FRegion& SetEqual(const CVX_FRegion& iFRegion); //function do do the equal-izing

	void WriteXML(CXML_Rip* pXML);
	bool ReadXML(CXML_Rip* pXML);

	inline CPrimitive* GetRegion(void) const {return pRegion;}
	inline CP_Mesh* GetMesh(void) const {return Mesh;}

//	bool Fixed;
	char DofFixed; //Bits: 0 0 Tz, Ty, Tz, Z, Y, X. 0 if free, 1 if fixed
	Vec3D<> Force; //in N
	Vec3D<> Torque; // in Nm
	Vec3D<> Displace; //in m
	Vec3D<> AngDisplace; //in rad

	//functions
	void CreateBoxRegion(void);
	void CreateBoxRegion(Vec3D<> Pos, Vec3D<> Dim);
	void CreateCylRegion(void);
	void CreateCylRegion(Vec3D<> Pos, Vec3D<> Axis, vfloat Rad);
	void CreateSphRegion(void);
	void CreateSphRegion(Vec3D<> Pos, vfloat Rad);
	void CreateMeshRegion(void);
	void CreateMeshRegion(CMesh* pMeshToAdd, Vec3D<> Pos, Vec3D<> Dim);
	void SetColor(vfloat r, vfloat g, vfloat b, vfloat alpha);
	void ScaleTo(Vec3D<> OldDims, Vec3D<> NewDims);

	bool IsBox(void) const {if(Box != NULL) return true; else return false;}
	bool IsCylinder(void) const {if(Cylinder != NULL) return true; else return false;}
	bool IsSphere(void) const {if(Sphere != NULL) return true; else return false;}
	bool IsMesh(void) const {if(Mesh != NULL) return true; else return false;}

#ifdef USE_OPEN_GL
	void DrawScaled(Vec3D<>* Scale);
#endif

	//Vec3D GetScaledVec(void); //todo...

	void ClearRegion();
	void ResetRegion(); //doesn't reset fixed or force...

private:
	//variables
	CPrimitive* pRegion;

	//possible types...
	CP_Box* Box;
	CP_Cylinder* Cylinder;
	CP_Sphere* Sphere;
	CP_Mesh* Mesh;
};


class CPrimitive
{
public:
	CPrimitive(void) {};
	~CPrimitive(void) {};

#ifdef USE_OPEN_GL
	virtual void Draw(Vec3D<>* Envelope) = 0; //does openGL drawing functions (passes to sub class)
#endif
	virtual bool IsIn(Vec3D<>* P, Vec3D<>* Envelope) = 0; //P has values ranging from 0 to 1
	virtual bool IsTouching(Vec3D<>* P, vfloat Dist, Vec3D<>* Envelope) = 0;
	virtual bool IsTouching(Vec3D<>* P, Vec3D<>* Dist, Vec3D<>* Envelope) = 0; //(for non-1 aspect ratios
	virtual void UpdateAspect(void)=0;
//	void UpdateEnvelope(Vec3D& EnvIn){_Envelope = EnvIn;}

	vfloat X;
	vfloat Y;
	vfloat Z;
	vfloat Radius;
	vfloat dX;
	vfloat dY;
	vfloat dZ;

	vfloat R;
	vfloat G;
	vfloat B;
	vfloat alpha;

	//cached...
	Vec3D<> _NomAspect; //nominal aspect ratio...

};

class CP_Box : public CPrimitive
{
public:
	CP_Box(void);
	~CP_Box(void);

#ifdef USE_OPEN_GL
	void Draw(Vec3D<>* Envelope); //does openGL drawing functions
//	void DrawScaled(Vec3D* Scale);
#endif
	bool IsIn(Vec3D<>* P, Vec3D<>* Envelope);
	bool IsTouching(Vec3D<>* P, vfloat Dist, Vec3D<>* Envelope);
	bool IsTouching(Vec3D<>* P, Vec3D<>* Dist, Vec3D<>* Envelope); //(for non-1 aspect ratios
	void UpdateAspect(void);

};

class CP_Cylinder : public CPrimitive
{
public:
	CP_Cylinder(void);
	~CP_Cylinder(void);

#ifdef USE_OPEN_GL
	void Draw(Vec3D<>* Envelope = NULL); //does openGL drawing functions
//	void DrawScaled(Vec3D* Scale);
#endif
	bool IsIn(Vec3D<>* P, Vec3D<>* Envelope);
	bool IsTouching(Vec3D<>* P, vfloat Dist, Vec3D<>* Envelope);
	bool IsTouching(Vec3D<>* P, Vec3D<>* Dist, Vec3D<>* Envelope); //(for non-1 aspect ratios
	void UpdateAspect(void);

};

class CP_Sphere : public CPrimitive
{
public:
	CP_Sphere(void);
	~CP_Sphere(void);

#ifdef USE_OPEN_GL
	void Draw(Vec3D<>* Envelope = NULL); //does openGL drawing functions (passes to sub class)
//	void DrawScaled(Vec3D* Scale);
#endif
	bool IsIn(Vec3D<>* P, Vec3D<>* Envelope);
	bool IsTouching(Vec3D<>* P, vfloat Dist, Vec3D<>* Envelope);
	bool IsTouching(Vec3D<>* P, Vec3D<>* Dist, Vec3D<>* Envelope); //(for non-1 aspect ratios
	void UpdateAspect(void);


};

class CP_Mesh : public CPrimitive
{
public:
	CP_Mesh(void);
	~CP_Mesh(void);

	CMesh ThisMesh;

#ifdef USE_OPEN_GL
	void Draw(Vec3D<>* Envelope = NULL); //does openGL drawing functions (passes to sub class)
//	void DrawScaled(Vec3D* Scale);
#endif
	bool IsIn(Vec3D<>* P, Vec3D<>* Envelope);
	bool IsTouching(Vec3D<>* P, vfloat Dist, Vec3D<>* Envelope);
	bool IsTouching(Vec3D<>* P, Vec3D<>* Dist, Vec3D<>* Envelope); //(for non-1 aspect ratios
	void UpdateAspect(void);


};

#endif //CVX_FRegion_H