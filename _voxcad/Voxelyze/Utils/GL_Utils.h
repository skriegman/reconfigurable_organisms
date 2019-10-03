/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef QGL_Utils_H
#define QGL_Utils_H

#include "Vec3D.h"

struct CColor {
	CColor(){r=-1; g=-1; b=-1;a=-1;};
	CColor (const CColor& C) {r = C.r; g = C.g; b = C.b; a = C.a;}
//	CColor(vfloat rIn, vfloat gIn, vfloat bIn){r=rIn; g=gIn; b=bIn; a=1.0;}
	CColor(vfloat rIn, vfloat gIn, vfloat bIn, vfloat aIn=1.0){r=rIn; g=gIn; b=bIn; a=aIn;}

	vfloat r, g, b, a;
	bool isValid(void) const {if(r>=0.0 && r <=1.0 && g>=0.0 && g <=1.0 && b>=0.0 && b <=1.0 && a>=0.0 && a <=1.0) return true; else return false;};
};

static CColor defaultQColor = CColor();

#ifdef USE_OPEN_GL

//OpenGL primitives drawing class
class CGL_Utils
{

public:
	//3D
	static void DrawCube(bool Faces = true, bool Edges = true, float LineWidth = 0.0, const CColor& Color = CColor()); //draws unit cube. changes glColor if Color != NULL
	static void DrawCube(const Vec3D<>& Center, vfloat Dim, const Vec3D<>& Squeeze = Vec3D<>(1.0,1.0,1.0), bool Faces = true, bool Edges = true, float LineWidth = 0.0, const CColor& Color = CColor()); //Draws scaled and translated cube
	static void DrawCube(const Vec3D<>& V1, const Vec3D<>& V2, bool Faces = true, bool Edges = true, float LineWidth = 0.0, const CColor& Color = CColor()); //Draws arbitrary rectangular prism
	static void DrawCubeFace();
	static void DrawCubeEdge();

	static void DrawSphere(const CColor& Color = defaultQColor); //draws unit sphere (no need for lines...)
	static void DrawSphere(const Vec3D<>& p, vfloat Rad, const Vec3D<>& Squeeze = Vec3D<>(1.0,1.0,1.0), const CColor& Color = CColor()); //arbitrary sphere
	static void DrawSphereFace();


	static void DrawCylinder(float taper = 1.0, bool Faces = true, bool Edges = true, float LineWidth = 0.0, const CColor& Color = CColor()); //draws unit cylinder in +X direction
	static void DrawCylinder(const Vec3D<>& v0, const Vec3D<>& v1, float Rad, const Vec3D<>& Squeeze = Vec3D<>(1.0,1.0,1.0), bool Faces = true, bool Edges = true, float LineWidth = 0.0, const CColor& Color = CColor());
	static void DrawCylinder(const Vec3D<>& v0, const Vec3D<>& v1, float Rad, float Rad2, const Vec3D<>& Squeeze = Vec3D<>(1.0,1.0,1.0), bool Faces = true, bool Edges = true, float LineWidth = 0.0, const CColor& Color = CColor());
	static void DrawCone(const Vec3D<>& v0, const Vec3D<>& v1, float Rad, bool Faces = true, bool Edges = true, float LineWidth = 0.0, const CColor& Color = CColor());
	static void DrawCylFace(float taper = 1.0);
	static void DrawCylEdge(float taper = 1.0);

	//2D
	static void DrawRectangle(bool Fill = false, float LineWidth = 0.0, const CColor& Color = defaultQColor); //draw unit circle in Z Plane
	static void DrawRectangle(const Vec3D<>& Center, vfloat Dim, const Vec3D<>& Normal, const Vec3D<>& Squeeze = Vec3D<>(1.0,1.0,1.0), bool Fill = false, float LineWidth = 0.0, const CColor& Color = CColor()); //draw square
	static void DrawRectangle(const Vec3D<>& V1, const Vec3D<>& V2, bool Fill = false, float LineWidth = 0.0, const CColor& Color = CColor()); //draw rectangle

	static void DrawCircle(bool Fill = false, float LineWidth = 0.0, const CColor& Color = defaultQColor); //draw unit circle in Z Plane
	static void DrawCircle(const Vec3D<>& p, vfloat Rad, const Vec3D<>& Normal, const Vec3D<>& Squeeze = Vec3D<>(1.0,1.0,1.0), bool Fill = false, float LineWidth = 0.0, const CColor& Color = CColor()); //draw circle

	//compound objetc:
	static void DrawArrow(const CColor& Color = CColor()); //draw unit arrow from origin to ZPlane
	static void DrawArrowD(const Vec3D<>& vO, const Vec3D<>& vP, const CColor& Color = CColor()); //draws 3D arrow Directly from vO to vP
	static void DrawArrow(const Vec3D<>& vO, const Vec3D<>& Dir, const CColor& Color = CColor()); //draws 3D arrow from vO with direction&magnitude Dir

	static void DrawLineArrowD(const Vec3D<>& vO, const Vec3D<>& vP, const vfloat LineWidth = 1.0, const CColor& Color = CColor()); //draws 3D arrow Directly from vO to vP

	static int CurContextID; //keep track of current context for buildlists (since we have to build list for each context...

protected: //vector functions
	static void AlignWith(const Vec3D<>& Base, const  Vec3D<>& target, Vec3D<>&rotax, vfloat &rotamt); // find rotation axis and angle to align base vector with target vector
};

#endif

#endif // QGL_Utils_H
