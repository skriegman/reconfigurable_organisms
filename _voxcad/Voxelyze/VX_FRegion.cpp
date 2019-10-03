/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "VX_FRegion.h"
#include <math.h>


#ifdef USE_OPEN_GL
#ifdef QT_GUI_LIB
#include <qgl.h>
#else
#include "OpenGLInclude.h" //If not using QT's openGL system, make a header file "OpenGLInclude.h" that includes openGL library functions 
#endif
#include "Utils/GL_Utils.h"
#endif

#ifdef USE_OPEN_GL
void CVX_FRegion::DrawScaled(Vec3D<>* Scale)
{
	pRegion->Draw(Scale);
}
#endif


CVX_FRegion::CVX_FRegion(void)
{
	Box = NULL;
	Cylinder = NULL;
	Sphere = NULL;
	Mesh = NULL;

	ClearRegion();
}

//CVX_FRegion::CVX_FRegion(Vec3D& BoxMin, Vec3D& BoxSize, bool FixedIn, Vec3D& ForceOrDisp)
//{
//	Box = NULL;
//	Cylinder = NULL;
//	Sphere = NULL;
//	Mesh = NULL;
//
//	ClearRegion();
//
//	CreateBoxRegion(BoxMin, BoxSize);
//	Fixed = FixedIn;
//	if (Fixed) Displace = ForceOrDisp;
//	else Force = ForceOrDisp;
//
//}


CVX_FRegion::CVX_FRegion(const CVX_FRegion& iFRegion) //copy constructor
{
	Box = NULL;
	Cylinder = NULL;
	Sphere = NULL;
	Mesh = NULL;

	ClearRegion(); //clear everything out...
	DofFixed = iFRegion.DofFixed;
//	Fixed = iFRegion.Fixed;
	Force = iFRegion.Force;
	Torque = iFRegion.Torque;
	Displace = iFRegion.Displace;
	AngDisplace = iFRegion.AngDisplace;

	if (iFRegion.IsBox()) //if a box...
		CreateBoxRegion(Vec3D<>(iFRegion.Box->X, iFRegion.Box->Y, iFRegion.Box->Z), Vec3D<>(iFRegion.Box->dX, iFRegion.Box->dY, iFRegion.Box->dZ));
	else if (iFRegion.IsCylinder()) //if a box...
		CreateCylRegion(Vec3D<>(iFRegion.Cylinder->X, iFRegion.Cylinder->Y, iFRegion.Cylinder->Z), Vec3D<>(iFRegion.Cylinder->dX, iFRegion.Cylinder->dY, iFRegion.Cylinder->dZ), iFRegion.Cylinder->Radius);
	else if (iFRegion.IsSphere()) //if a box...
		CreateSphRegion(Vec3D<>(iFRegion.Sphere->X, iFRegion.Sphere->Y, iFRegion.Sphere->Z), iFRegion.Sphere->Radius);
	else if (iFRegion.IsMesh()) //if a mesh...
		CreateMeshRegion(&iFRegion.Mesh->ThisMesh, Vec3D<>(iFRegion.Mesh->X, iFRegion.Mesh->Y, iFRegion.Mesh->Z), Vec3D<>(iFRegion.Mesh->dX, iFRegion.Mesh->dY, iFRegion.Mesh->dZ));

	SetColor(iFRegion.pRegion->R, iFRegion.pRegion->G, iFRegion.pRegion->B, iFRegion.pRegion->alpha);
}

CVX_FRegion::~CVX_FRegion(void)
{
	ResetRegion();
}


CVX_FRegion& CVX_FRegion::operator=(const CVX_FRegion& iFRegion)
{
	return SetEqual(iFRegion);
}

CVX_FRegion& CVX_FRegion::SetEqual(const CVX_FRegion& iFRegion) //function do do the equal-izing
{
	ClearRegion(); //clear everything out...
	DofFixed = iFRegion.DofFixed;
//	Fixed = iFRegion.Fixed;
	Force = iFRegion.Force;
	Torque = iFRegion.Torque;
	Displace = iFRegion.Displace;
	AngDisplace = iFRegion.AngDisplace;

	if (iFRegion.Box != NULL) //if a box...
		CreateBoxRegion(Vec3D<>(iFRegion.Box->X, iFRegion.Box->Y, iFRegion.Box->Z), Vec3D<>(iFRegion.Box->dX, iFRegion.Box->dY, iFRegion.Box->dZ));
	else if (iFRegion.Cylinder != NULL) //if a box...
		CreateCylRegion(Vec3D<>(iFRegion.Cylinder->X, iFRegion.Cylinder->Y, iFRegion.Cylinder->Z), Vec3D<>(iFRegion.Cylinder->dX, iFRegion.Cylinder->dY, iFRegion.Cylinder->dZ), iFRegion.Cylinder->Radius);
	else if (iFRegion.Sphere != NULL) //if a box...
		CreateSphRegion(Vec3D<>(iFRegion.Sphere->X, iFRegion.Sphere->Y, iFRegion.Sphere->Z), iFRegion.Sphere->Radius);
	else if (iFRegion.IsMesh()) //if a mesh...
		CreateMeshRegion(&iFRegion.Mesh->ThisMesh, Vec3D<>(iFRegion.Mesh->X, iFRegion.Mesh->Y, iFRegion.Mesh->Z), Vec3D<>(iFRegion.Mesh->dX, iFRegion.Mesh->dY, iFRegion.Mesh->dZ));

	SetColor(iFRegion.pRegion->R, iFRegion.pRegion->G, iFRegion.pRegion->B, iFRegion.pRegion->alpha);

	return *this;

}

void CVX_FRegion::WriteXML(CXML_Rip* pXML)
{
	pXML->DownLevel("FRegion");

	//the type of primitive:
	int PrimType = -1;
	if (Box != NULL) PrimType = PRIM_BOX;
	else if (Cylinder != NULL) PrimType = PRIM_CYLINDER;
	else if (Sphere != NULL) PrimType = PRIM_SPHERE;
	else if (Mesh != NULL) PrimType = PRIM_MESH;


	pXML->Element("PrimType", PrimType);
	pXML->Element("X", pRegion->X);
	pXML->Element("Y", pRegion->Y);
	pXML->Element("Z", pRegion->Z);
	pXML->Element("dX", pRegion->dX);
	pXML->Element("dY", pRegion->dY);
	pXML->Element("dZ", pRegion->dZ);
	pXML->Element("Radius", pRegion->Radius);
	pXML->Element("R", pRegion->R);
	pXML->Element("G", pRegion->G);
	pXML->Element("B", pRegion->B);
	pXML->Element("alpha", pRegion->alpha);
//	pXML->Element("Fixed", Fixed);
	pXML->Element("DofFixed", (int)DofFixed); //cast to int to make sure it's representable in ascii format...
	pXML->Element("ForceX", Force.x);
	pXML->Element("ForceY", Force.y);
	pXML->Element("ForceZ", Force.z);
	pXML->Element("TorqueX", Torque.x);
	pXML->Element("TorqueY", Torque.y);
	pXML->Element("TorqueZ", Torque.z);
	pXML->Element("DisplaceX", Displace.x);
	pXML->Element("DisplaceY", Displace.y);
	pXML->Element("DisplaceZ", Displace.z);
	pXML->Element("AngDisplaceX", AngDisplace.x);
	pXML->Element("AngDisplaceY", AngDisplace.y);
	pXML->Element("AngDisplaceZ", AngDisplace.z);
	if (PrimType == PRIM_MESH) Mesh->ThisMesh.WriteXML(pXML);

	pXML->UpLevel();
}

bool CVX_FRegion::ReadXML(CXML_Rip* pXML)
{
	int PrimType = -1;
	if (!pXML->FindLoadElement("PrimType", &PrimType)) return false;

	if (PrimType == PRIM_BOX)
		CreateBoxRegion(Vec3D<>(0,0,0), Vec3D<>(0,0,0)); //creates box and sets pointer
	else if (PrimType == PRIM_CYLINDER)
		CreateCylRegion(Vec3D<>(0,0,0), Vec3D<>(0,0,0), 0);  //creates cylinder and sets pointer
	else if (PrimType == PRIM_SPHERE)
		CreateSphRegion(Vec3D<>(0,0,0), 0);  //creates Sphere and sets pointer
	else if (PrimType == PRIM_MESH)
		CreateMeshRegion(NULL, Vec3D<>(0,0,0), Vec3D<>(0,0,0));  //creates Sphere and sets pointer
	

	if (!pXML->FindLoadElement("X", &pRegion->X)) pRegion->X = 0;
	if (!pXML->FindLoadElement("Y", &pRegion->Y)) pRegion->Y = 0;
	if (!pXML->FindLoadElement("Z", &pRegion->Z)) pRegion->Z = 0;
	if (!pXML->FindLoadElement("dX", &pRegion->dX)) pRegion->dX = 0;
	if (!pXML->FindLoadElement("dY", &pRegion->dY)) pRegion->dY = 0;
	if (!pXML->FindLoadElement("dZ", &pRegion->dZ)) pRegion->dZ = 0;
	if (!pXML->FindLoadElement("Radius", &pRegion->Radius)) pRegion->Radius = 0;
	if (!pXML->FindLoadElement("R", &pRegion->R)) pRegion->R = 0;
	if (!pXML->FindLoadElement("G", &pRegion->G)) pRegion->G = 0;
	if (!pXML->FindLoadElement("B", &pRegion->B)) pRegion->B = 0;
	if (!pXML->FindLoadElement("alpha", &pRegion->alpha)) pRegion->alpha = 0;

	int TmpDofFixed;
	if (pXML->FindLoadElement("DofFixed", &TmpDofFixed)) {DofFixed = (char) TmpDofFixed;}
	else { //if no DofFixed tag, could be an older version with binary "Fixed" tag
		bool IsOldFixed;
		if (pXML->FindLoadElement("Fixed", &IsOldFixed)) DofFixed = IsOldFixed?DOF_ALL:DOF_NONE;
		else DofFixed = DOF_NONE;
	}

	if (!pXML->FindLoadElement("ForceX", &Force.x)) Force.x = 0;
	if (!pXML->FindLoadElement("ForceY", &Force.y)) Force.y = 0;
	if (!pXML->FindLoadElement("ForceZ", &Force.z)) Force.z = 0;
	if (!pXML->FindLoadElement("TorqueX", &Torque.x)) Torque.x = 0;
	if (!pXML->FindLoadElement("TorqueY", &Torque.y)) Torque.y = 0;
	if (!pXML->FindLoadElement("TorqueZ", &Torque.z)) Torque.z = 0;
	if (!pXML->FindLoadElement("DisplaceX", &Displace.x)) Displace.x = 0;
	if (!pXML->FindLoadElement("DisplaceY", &Displace.y)) Displace.y = 0;
	if (!pXML->FindLoadElement("DisplaceZ", &Displace.z)) Displace.z = 0;
	if (!pXML->FindLoadElement("AngDisplaceX", &AngDisplace.x)) AngDisplace.x = 0;
	if (!pXML->FindLoadElement("AngDisplaceY", &AngDisplace.y)) AngDisplace.y = 0;
	if (!pXML->FindLoadElement("AngDisplaceZ", &AngDisplace.z)) AngDisplace.z = 0;
	if (PrimType == PRIM_MESH && pXML->FindElement("CMesh")){
		Mesh->ThisMesh.ReadXML(pXML);
		pXML->UpLevel();
	}

	pRegion->UpdateAspect();
	return true;
}


void CVX_FRegion::ClearRegion() //clears everything here...
{
	ResetRegion();
	
//	Fixed = false;
	DofFixed = DOF_NONE;
	Force = Vec3D<>(0,0,0);
	Torque = Vec3D<>(0,0,0);
	Displace = Vec3D<>(0,0,0);
	AngDisplace = Vec3D<>(0,0,0);
}

void CVX_FRegion::ResetRegion(void) //clears primitive info
{
	pRegion = NULL;
	
	if (Box != NULL) delete Box;
	Box = NULL;
	if (Cylinder != NULL) delete Cylinder;
	Cylinder = NULL;
	if (Sphere != NULL) delete Sphere;
	Sphere = NULL;
	if (Mesh != NULL) delete Mesh;
	Mesh = NULL;
}

void CVX_FRegion::CreateBoxRegion(Vec3D<> Pos, Vec3D<> Dim)
{
	ResetRegion();

	Box = new CP_Box;
	Box->X = Pos.x;
	Box->Y = Pos.y;
	Box->Z = Pos.z;
	Box->dX = Dim.x;
	Box->dY = Dim.y;
	Box->dZ = Dim.z;
	pRegion = Box;
	Box->UpdateAspect();
}

void CVX_FRegion::CreateBoxRegion(void) //use defaults
{
	ResetRegion();
	Box = new CP_Box;
	pRegion = Box;
}

void CVX_FRegion::CreateCylRegion(Vec3D<> Pos, Vec3D<> Axis, vfloat Rad)
{
	ResetRegion();

	Cylinder = new CP_Cylinder;
	Cylinder->X = Pos.x;
	Cylinder->Y = Pos.y;
	Cylinder->Z = Pos.z;
	Cylinder->dX = Axis.x;
	Cylinder->dY = Axis.y;
	Cylinder->dZ = Axis.z;
	Cylinder->Radius = Rad;
	pRegion = Cylinder;
	Cylinder->UpdateAspect();
}

void CVX_FRegion::CreateCylRegion() //use defaults
{
	ResetRegion();
	Cylinder = new CP_Cylinder;
	pRegion = Cylinder;
}

void CVX_FRegion::CreateSphRegion(Vec3D<> Pos, vfloat Rad)
{
	ResetRegion();

	Sphere = new CP_Sphere;
	Sphere->X = Pos.x;
	Sphere->Y = Pos.y;
	Sphere->Z = Pos.z;
	Sphere->Radius = Rad;
	pRegion = Sphere;
	Sphere->UpdateAspect();

}

void CVX_FRegion::CreateSphRegion(void)
{
	ResetRegion();
	Sphere = new CP_Sphere;
	pRegion = Sphere;
}

void CVX_FRegion::CreateMeshRegion(void)
{
	ResetRegion();
	Mesh = new CP_Mesh;
	pRegion = Mesh;
}

void CVX_FRegion::CreateMeshRegion(CMesh* pMeshToAdd, Vec3D<> Pos, Vec3D<> Dim)
{
	ResetRegion();

	Mesh = new CP_Mesh;
	Mesh->X = Pos.x;
	Mesh->Y = Pos.y;
	Mesh->Z = Pos.z;
	Mesh->dX = Dim.x;
	Mesh->dY = Dim.y;
	Mesh->dZ = Dim.z;
	if (pMeshToAdd) Mesh->ThisMesh = *pMeshToAdd;
	pRegion = Mesh;
	Mesh->UpdateAspect();

}


void CVX_FRegion::SetColor(vfloat r, vfloat g, vfloat b, vfloat alpha)
{
	pRegion->R = r;
	pRegion->G = g;
	pRegion->B = b;
	pRegion->alpha = alpha;
}

void CVX_FRegion::ScaleTo(Vec3D<> OldDims, Vec3D<> NewDims) //scales to fit new envelope provided
{
	pRegion->X *= (NewDims.x/OldDims.x);
	pRegion->Y *= (NewDims.y/OldDims.y);
	pRegion->Z *= (NewDims.z/OldDims.z);
	pRegion->dX *= (NewDims.x/OldDims.x);
	pRegion->dY *= (NewDims.y/OldDims.y);
	pRegion->dZ *= (NewDims.z/OldDims.z);
	pRegion->Radius *= (NewDims.x/OldDims.x);
}



//BOX
CP_Box::CP_Box(void)
{
	X = 0;
	Y = 0;
	Z = 0;
	dX = 0.001;
	dY = 0.001;
	dZ = 0.001;
	R = 0.5;
	G = 0.5;
	B = 0.5;
	Radius = 0.0;
	alpha = 1.0;

	UpdateAspect();
}

CP_Box::~CP_Box(void)
{
}

#ifdef USE_OPEN_GL
void CP_Box::Draw(Vec3D<>* Envelope)
{
	glPushMatrix();
	if (Envelope) glScaled(Envelope->x, Envelope->y, Envelope->z);

	Vec3D<> V1(X, Y, Z);
	Vec3D<> V2(X+dX, Y+dY, Z+dZ);
    CColor c(R, G, B, alpha);
	CGL_Utils::DrawCube(V1, V2, true, true, 2.0, c);

	glPopMatrix();

}
#endif

bool CP_Box::IsIn(Vec3D<>* P, Vec3D<>* Envelope)
{
	Vec3D<> Ps = P->ScaleInv(*Envelope);

	if (Ps.x > X && Ps.x < X+dX && Ps.y > Y && Ps.y < Y+dY && Ps.z > Z && Ps.z < Z+dZ) //if contained...
		return true;
	else
		return false;
}

bool CP_Box::IsTouching(Vec3D<>* P, vfloat Dist, Vec3D<>* Envelope)
{
	Vec3D<> Pos = Vec3D<>(X, Y, Z).Scale(*Envelope);
	Vec3D<> Size = Vec3D<>(dX, dY, dZ).Scale(*Envelope);

	if (P->x+Dist > Pos.x  &&  P->x-Dist < Pos.x+Size.x  &&  P->y+Dist > Pos.y  &&  P->y-Dist < Pos.y+Size.y  &&  P->z+Dist > Pos.z  &&  P->z-Dist < Pos.z+Size.z) //if contained including offsets...
		return true;
	else
		return false;
}


bool CP_Box::IsTouching(Vec3D<>* P, Vec3D<>* Dist, Vec3D<>* Envelope)
{
	Vec3D<> Ps = P->ScaleInv(*Envelope);
	Vec3D<> Ds = Dist->ScaleInv(*Envelope);

	if (Ps.x+Ds.x > X  &&  Ps.x-Ds.x < X+dX  &&  Ps.y+Ds.y > Y  &&  Ps.y-Ds.y < Y+dY  &&  Ps.z+Ds.z > Z  &&  Ps.z-Ds.z < Z+dZ) //if contained including offsets...
		return true;
	else
		return false;
}

void CP_Box::UpdateAspect(void)
{
	_NomAspect = Vec3D<>(dX, dY, dZ);
}




//Cylinder:

CP_Cylinder::CP_Cylinder(void)
{
	X = 0.0;
	Y = 0.0;
	Z = 0.0;
	Radius = 0.001;
	dX = 0.0;
	dY = 0.0;
	dZ = 0.001;
	R = 0.5;
	G = 0.5;
	B = 0.5;
	alpha = 1.0;

	UpdateAspect();

}

CP_Cylinder::~CP_Cylinder(void)
{
}

#ifdef USE_OPEN_GL
void CP_Cylinder::Draw(Vec3D<>* Envelope)
{
	Vec3D<> v1(X, Y, Z);
	Vec3D<> v2(X+dX, Y+dY, Z+dZ);
	vfloat Rad = Radius;

	if (Envelope){
		v1 = v1.Scale(*Envelope);
		v2 = v2.Scale(*Envelope);
		Rad *= Envelope->Max();
	}
//	Vec3D Scl;
//	if (Scale) Scl = *Scale;
//	else Scl = Vec3D(1,1,1);

//	vfloat MaxVal = Scl.x;
//	if (Scl.y>MaxVal) MaxVal = Scl.y;
//	if (Scl.z>MaxVal) MaxVal = Scl.z;

//	glPushMatrix();
//	glScalef(Scale->x, Scale->y, Scale->z);
	if (Radius !=0) {
        CColor c(R, G, B, alpha);
//		CGL_Utils::DrawCylinder(Vec3D(Scl.x*X,Scl.y*Y,Scl.z*Z), Vec3D(Scl.x*(X+dX),Scl.y*(Y+dY),Scl.z*(Z+dZ)), MaxVal*Radius, Vec3D(1.0, 1.0, 1.0), true, true, 2.0, c);
		CGL_Utils::DrawCylinder(v1, v2, (float)Rad, Vec3D<>(1.0, 1.0, 1.0), true, true, 2.0, c);
                                 
    }
//	glPopMatrix();
}
#endif

bool CP_Cylinder::IsIn(Vec3D<>* P, Vec3D<>* Envelope)
{
	//only 1 axis should be non-zero!
	//Vec3D ThruOr = Vec3D (P->x-X, P->y-Y, P->z-Z);
	//Vec3D dDim = Vec3D(dX, dY, dZ);
	//vfloat NormDist = ThruOr.Dot(dDim);
	//Vec3D ClosePoint = ThruOr/ThruOr.Length()*NormDist;
	//Vec3D vRadius = Vec3D(ThruOr.x - ClosePoint.x, ThruOr.y - ClosePoint.y, ThruOr.z - ClosePoint.z);
	//float thisRad = vRadius.Length();

	Vec3D<> Pos = Vec3D<>(X, Y, Z).Scale(*Envelope);
	Vec3D<> Size = Vec3D<>(dX, dY, dZ).Scale(*Envelope);
	vfloat Rad = Radius*Envelope->Max();


	if (dX != 0){
		if (P->x > Pos.x && P->x < Pos.x+Size.x && sqrt((P->y-Pos.y)*(P->y-Pos.y) + (P->z-Pos.z)*(P->z-Pos.z)) < Rad)
			return true;
	}
	else if (dY != 0){
		if (P->y > Pos.y && P->y < Pos.y+Size.y && sqrt((P->x-Pos.x)*(P->x-Pos.x) + (P->z-Pos.z)*(P->z-Pos.z)) < Rad)
			return true;
	}
	else if (dZ != 0){
		if (P->z > Pos.z && P->z < Pos.z+Size.z && sqrt((P->x-Pos.x)*(P->x-Pos.x) + (P->y-Pos.y)*(P->y-Pos.y)) < Rad)
			return true;
	}

	return false;
}

bool CP_Cylinder::IsTouching(Vec3D<>* P, vfloat Dist, Vec3D<>* Envelope)
{
	return (IsIn(P, Envelope)); //TODO: fix this when needed
}

bool CP_Cylinder::IsTouching(Vec3D<>* P, Vec3D<>* Dist, Vec3D<>* Envelope)
{
	return (IsIn(P, Envelope)); //TODO: fix this when needed
}

void CP_Cylinder::UpdateAspect(void)
{
	//here, spect ratio is: x = cylinder lenght, y = radius, z = nothing
	if (dX != 0) _NomAspect = Vec3D<>(dX, Radius*2, 0);
	else if (dY != 0) _NomAspect = Vec3D<>(dY, Radius*2, 0);
	else if (dZ != 0) _NomAspect = Vec3D<>(dZ, Radius*2, 0);
	else _NomAspect = Vec3D<>(dX, dY, dZ);
}

//Sphere:

CP_Sphere::CP_Sphere(void)
{
	X = 0.0;
	Y = 0.0;
	Z = 0.0;
	Radius = 0.001;
	R = 0.5;
	G = 0.5;
	B = 0.5;
	alpha = 1.0;

	UpdateAspect();
}

CP_Sphere::~CP_Sphere(void)
{
}

#ifdef USE_OPEN_GL
void CP_Sphere::Draw(Vec3D<>* Envelope)
{
	Vec3D<> v1(X, Y, Z);
	vfloat Rad = Radius;

	if (Envelope){
		v1 = v1.Scale(*Envelope);
		Rad *= Envelope->Max();
	}

	CColor c(R, G, B, alpha);
	CGL_Utils::DrawSphere(v1, Rad, Vec3D<>(1,1,1), c);
}
#endif

bool CP_Sphere::IsIn(Vec3D<>* P, Vec3D<>* Envelope)
{
	Vec3D<> Pos = Vec3D<>(X, Y, Z).Scale(*Envelope);
	Vec3D<> Size = Vec3D<>(dX, dY, dZ).Scale(*Envelope);
	vfloat Rad = Radius*Envelope->Max();

	if (sqrt((P->x-Pos.x)*(P->x-Pos.x)+(P->y-Pos.y)*(P->y-Pos.y)+(P->z-Pos.z)*(P->z-Pos.z)) < Rad) return true;
	else return false;
}

bool CP_Sphere::IsTouching(Vec3D<>* P, vfloat Dist, Vec3D<>* Envelope)
{
	Vec3D<> Pos = Vec3D<>(X, Y, Z).Scale(*Envelope);
	Vec3D<> Size = Vec3D<>(dX, dY, dZ).Scale(*Envelope);
	vfloat Rad = Radius*Envelope->Max();

	if (sqrt((P->x-Pos.x)*(P->x-Pos.x)+(P->y-Pos.y)*(P->y-Pos.y)+(P->z-Pos.z)*(P->z-Pos.z)) < Rad+Dist) return true; //test against spherical region around point
	else return false;
}

bool CP_Sphere::IsTouching(Vec3D<>* P, Vec3D<>* Dist, Vec3D<>* Envelope)
{

	return IsTouching(P, Dist->Length(), Envelope); //not quite right, but decent approximation for now...
}

void CP_Sphere::UpdateAspect(void)
{
	_NomAspect = Vec3D<>(Radius*2, Radius*2, Radius*2);
}



CP_Mesh::CP_Mesh(void)
{
	X = 0;
	Y = 0;
	Z = 0;
	dX = 0.001;
	dY = 0.001;
	dZ = 0.001;
	R = 0.5;
	G = 0.5;
	B = 0.5;
	Radius = 0;
	alpha = 1.0;
	ThisMesh.Clear();
	
	UpdateAspect();
	
}

CP_Mesh::~CP_Mesh(void)
{
}

#ifdef USE_OPEN_GL
void CP_Mesh::Draw(Vec3D<>* Envelope)
{
	glPushMatrix();
//	glScalef(MaxVal,MaxVal, MaxVal);

	glColor4d(R, G, B, alpha);
	Vec3D<> Min = ThisMesh.GetBBMin();
	Vec3D<> Size = ThisMesh.GetBBSize();
	Vec3D<> v1(X, Y, Z);
	
	if (Envelope){
		Vec3D<> ScaleFacV = Vec3D<>(dX, dY, dZ).Scale(Envelope->ScaleInv(Size)); //= WS/Size
		vfloat MinScaleFac = ScaleFacV.Min();
		v1 = v1.Scale(*Envelope);

		glTranslated(v1.x, v1.y, v1.z);
		glScaled(MinScaleFac, MinScaleFac, MinScaleFac);
	}
	else {
		glTranslated(v1.x, v1.y, v1.z);
		glScaled(dX/Size.x, dY/Size.y, dZ/Size.z);
	}
	glTranslated(-Min.x, -Min.y, -Min.z);

	ThisMesh.Draw(false, true, true, true);

	glPopMatrix();
}
#endif

bool CP_Mesh::IsIn(Vec3D<>* P, Vec3D<>* Envelope)
{
	Vec3D<> Point = *P;
	Vec3D<> Min = ThisMesh.GetBBMin();
	Vec3D<> Size = ThisMesh.GetBBSize();
	Vec3D<> ScaleFacV = Vec3D<>(dX, dY, dZ).Scale(Envelope->ScaleInv(Size)); //= WS/Size
	Vec3D<> v1(X, Y, Z);

	v1 = v1.Scale(*Envelope);
	
	vfloat ScaleFac = ScaleFacV.x;
	if (ScaleFacV.y < ScaleFac) ScaleFac = ScaleFacV.y;
	if (ScaleFacV.z < ScaleFac) ScaleFac = ScaleFacV.z;

	Point -= v1;
	Point = Point.ScaleInv(Vec3D<>(ScaleFac, ScaleFac, ScaleFac));
	Point += Min;

	return ThisMesh.IsInside(&Point);

}

bool CP_Mesh::IsTouching(Vec3D<>* P, vfloat Dist, Vec3D<>* Envelope)
{
	return IsIn(P, Envelope); //TODO: implement this when needed...
}

bool CP_Mesh::IsTouching(Vec3D<>* P, Vec3D<>* Dist, Vec3D<>* Envelope)
{
	return IsIn(P, Envelope); //TODO: implement this when needed...
}

void CP_Mesh::UpdateAspect(void)
{
//	Vec3D Size = ThisMesh.GetBBSize();

	_NomAspect = Vec3D<>(dX, dY, dZ);
}
