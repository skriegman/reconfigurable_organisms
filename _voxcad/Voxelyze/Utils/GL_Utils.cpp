/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifdef USE_OPEN_GL

#include "GL_Utils.h"
#include <math.h>

#ifdef QT_GUI_LIB
#include <qgl.h>
#else
#include "OpenGLInclude.h" //If not using QT's openGL system, make a header file "OpenGLInclude.h" that includes openGL library functions 
#endif

#if defined(_WIN32) || defined(_WIN64) //to get fmax, fmin to work on Windows/Visual Studio
#define fmax max
#define fmin min
#endif

//TODO: Lists must be built for each gl context. Hmm.... (otherwise don't show up in other windows...)
int CGL_Utils::CurContextID = 0;
#define MAXNUMWINDOWS 10 //maximum number of windows we'll open (for keeping track of contexts...)

// --------------------------------------------------------------------------
void CGL_Utils::DrawCube(bool Faces, bool Edges, float LineWidth, const CColor& Color)
// --------------------------------------------------------------------------
{ // draw unit cube
	if (Color.isValid())
		glColor4d(Color.r, Color.g, Color.b, Color.a);

	static GLfloat CurColor[4]; //get the current color
	glGetFloatv(GL_CURRENT_COLOR, CurColor);
	
	if (LineWidth != 0.0)
		glLineWidth(LineWidth);

	if (Faces && CurColor[3] != 0.0){ //If we want to draw the faces (and not fully transparent...)
		DrawCubeFace();
	}

	if (Edges){
		//store the last color and always draw edges black!
		glColor4f(0.0, 0.0, 0.0, 1.0);
		static GLboolean IsLighting;
		glGetBooleanv(GL_LIGHTING, &IsLighting);
		if (IsLighting) glDisable(GL_LIGHTING);
		CGL_Utils::DrawCubeEdge();
		if (IsLighting) glEnable(GL_LIGHTING);
	}

//	glColor4fv(CurColor); //restore the (previously) current color...

}

// --------------------------------------------------------------------------
void CGL_Utils::DrawCubeFace()
// --------------------------------------------------------------------------
{
	static GLuint BoxFaceList[MAXNUMWINDOWS] = {0};

	//if (CurColor[3] != 1.0) glDisable(GL_DEPTH_TEST); //if transparent at all, disable depth test to mitigate poly drawing order problems
	if (BoxFaceList[CurContextID] == 0){ //if We haven't built the list yet...
		BoxFaceList[CurContextID] = glGenLists(1);
		glNewList(BoxFaceList[CurContextID], GL_COMPILE);

		glBegin(GL_QUADS);
		glNormal3f(1.0,0,0); //+X face
		glVertex3d(0.5,-0.5,-0.5);
		glVertex3d(0.5, 0.5,-0.5);
		glVertex3d(0.5, 0.5, 0.5);
		glVertex3d(0.5,-0.5, 0.5);

		glNormal3f(-1,0,0); //-X face
		glVertex3d(-0.5,-0.5,-0.5);
		glVertex3d(-0.5,-0.5, 0.5);
		glVertex3d(-0.5, 0.5, 0.5);
		glVertex3d(-0.5, 0.5,-0.5);

		glNormal3f(0,1,0); //+Y face
		glVertex3d(-0.5, 0.5,-0.5);
		glVertex3d(-0.5, 0.5, 0.5);
		glVertex3d( 0.5, 0.5, 0.5);
		glVertex3d( 0.5, 0.5,-0.5);

		glNormal3f(0,-1,0); //-Y face
		glVertex3d(-0.5,-0.5,-0.5);
		glVertex3d( 0.5,-0.5,-0.5);
		glVertex3d( 0.5,-0.5, 0.5);
		glVertex3d(-0.5,-0.5, 0.5);

		glNormal3f(0,0,1); //+Z face
		glVertex3d(-0.5,-0.5,0.5);
		glVertex3d(0.5, -0.5,0.5);
		glVertex3d(0.5, 0.5, 0.5);
		glVertex3d(-0.5,0.5, 0.5);

		glNormal3f(0,0,-1); //-Z face
		glVertex3d(-0.5,-0.5,-0.5);
		glVertex3d(-0.5, 0.5,-0.5);
		glVertex3d( 0.5, 0.5,-0.5);
		glVertex3d( 0.5,-0.5,-0.5);
		glEnd();
		
		glEndList();
	
	}
	glCallList(BoxFaceList[CurContextID]);
	//glEnable(GL_DEPTH_TEST); //re-enable depth testing
}

// --------------------------------------------------------------------------
void CGL_Utils::DrawCubeEdge()
// --------------------------------------------------------------------------
{
	static GLuint BoxEdgeList[MAXNUMWINDOWS] = {0};

	if (BoxEdgeList[CurContextID] == 0){ //if we haven't built the list yet...
		BoxEdgeList[CurContextID] = glGenLists(1);
		glNewList(BoxEdgeList[CurContextID], GL_COMPILE);

		glBegin(GL_LINE_LOOP);
		glVertex3d(-0.5,-0.5,-0.5);
		glVertex3d( 0.5,-0.5,-0.5);
		glVertex3d (0.5, 0.5,-0.5);
		glVertex3d(-0.5, 0.5,-0.5);
		glEnd();

		glBegin(GL_LINE_LOOP);
		glVertex3d(-0.5,-0.5,0.5);
		glVertex3d( 0.5,-0.5,0.5);
		glVertex3d( 0.5, 0.5,0.5);
		glVertex3d(-0.5, 0.5,0.5);
		glEnd();

		glBegin(GL_LINES);
		glVertex3d(-0.5,-0.5, 0.5);
		glVertex3d(-0.5,-0.5,-0.5);
		glVertex3d( 0.5,-0.5, 0.5);
		glVertex3d( 0.5,-0.5,-0.5);
		glVertex3d( 0.5, 0.5, 0.5);
		glVertex3d( 0.5, 0.5,-0.5);
		glVertex3d(-0.5, 0.5, 0.5);
		glVertex3d(-0.5, 0.5,-0.5);
		glEnd();

		glEndList();
	}

	glCallList(BoxEdgeList[CurContextID]);
}

// --------------------------------------------------------------------------
void CGL_Utils::DrawCube(const Vec3D<>& Center, vfloat Dim, const Vec3D<>& Squeeze, bool Faces, bool edges, float LineWidth, const CColor& Color)
// --------------------------------------------------------------------------
{ //draw arbitrary scaled and translated cube
	glPushMatrix();
	glTranslated(Center.x, Center.y, Center.z);
	glScaled(Dim*Squeeze.x, Dim*Squeeze.y, Dim*Squeeze.z);
	DrawCube(Faces, edges, LineWidth, Color);
	glPopMatrix();
}

// --------------------------------------------------------------------------
void CGL_Utils::DrawCube(const Vec3D<>& V1, const Vec3D<>& V2, bool Faces, bool edges, float LineWidth, const CColor& Color)
// --------------------------------------------------------------------------
{ //Draws a rectangular prism
	//Make sure Vertices are correct orientation
	Vec3D<> MinVec(fmin(V1.x, V2.x), fmin(V1.y, V2.y), fmin(V1.z, V2.z));
	Vec3D<> MaxVec(fmax(V1.x, V2.x), fmax(V1.y, V2.y), fmax(V1.z, V2.z));
	Vec3D<> CenVec((V1.x+V2.x)/2.0, (V1.y+V2.y)/2.0, (V1.z+V2.z)/2.0);

	glPushMatrix();
	glTranslated(CenVec.x, CenVec.y, CenVec.z);
	glScaled(V2.x - V1.x, V2.y - V1.y, V2.z - V1.z);
	DrawCube(Faces, edges, LineWidth, Color);
	glPopMatrix();
}

// --------------------------------------------------------------------------
void CGL_Utils::DrawSphere(const CColor& Color)
// --------------------------------------------------------------------------
{ // draw solid unit sphere centered at 0,0,0

	
	// if (Color.isValid())
	// 	glColor4d(Color.r, Color.g, Color.b, Color.a);

	// static GLfloat CurColor[4]; //get the current color
	// glGetFloatv(GL_CURRENT_COLOR, CurColor);

	// if (CurColor[3] != 0.0){ //If we want to draw the faces (and not fully transparent...)
	// 	DrawSphereFace();
	// }
	// glColor4fv(CurColor); //restore the (previously) current color...

}

// --------------------------------------------------------------------------
void CGL_Utils::DrawSphere(const Vec3D<>& p, vfloat Rad, const Vec3D<>& Squeeze, const CColor& Color)
// --------------------------------------------------------------------------
{ // draw solid sphere at p
	
	glPushMatrix();
	glTranslated(p.x, p.y, p.z);
	glScaled(2*Rad*Squeeze.x, 2*Rad*Squeeze.y, 2*Rad*Squeeze.z);
	DrawSphere(Color);
	glPopMatrix();

}

// --------------------------------------------------------------------------
void CGL_Utils::DrawSphereFace()
// --------------------------------------------------------------------------
{
	static GLuint SphereList[MAXNUMWINDOWS] = {0};
	vfloat y, z, a1, a2, x1, x2, Rad1, Rad2;
	vfloat pi = atan(1.0)*4.0;
	vfloat da = pi/12;

	if (SphereList[CurContextID] == 0){ //if We haven't built the list yet...
		SphereList[CurContextID] = glGenLists(1);
		glNewList(SphereList[CurContextID], GL_COMPILE);

		// draw 12 segments
		for (a2=0; a2<pi; a2+=da) {
			x1 = 0.5*cos(a2);
			Rad1 = 0.5*sin(a2);
			x2 = 0.5*cos(a2+da);
			Rad2 = 0.5*sin(a2+da);
			glBegin(GL_QUAD_STRIP);
			for (a1=0; a1<pi*2.01; a1+=da) {
				y = Rad1*cos(a1);
				z = Rad1*sin(a1);
				glNormal3d(cos(a2),sin(a2)*cos(a1),sin(a2)*sin(a1));
				glVertex3d(x1,y,z);
				y = Rad2*cos(a1);
				z = Rad2*sin(a1);
				glNormal3d(cos(a2+da),sin(a2+da)*cos(a1),sin(a2+da)*sin(a1));
				glVertex3d(x2,y,z);
			}
			glEnd();
		}
		glEndList();
	}
	glCallList(SphereList[CurContextID]);
}

// --------------------------------------------------------------------------
void CGL_Utils::DrawCylinder(float taper, bool Faces, bool Edges, float LineWidth, const CColor& Color)
// --------------------------------------------------------------------------
{ // draw solid cylinder pointing in +Z direction

	
	if (Color.isValid())
		glColor4d(Color.r, Color.g, Color.b, Color.a);

	static GLfloat CurColor[4]; //get the current color
	glGetFloatv(GL_CURRENT_COLOR, CurColor);

	if (LineWidth != 0.0)
		glLineWidth(LineWidth);

	if (Faces && CurColor[3] != 0.0){ //If we want to draw the faces
		DrawCylFace(taper);
	}
	if (Edges){
		glColor4f(0.0, 0.0, 0.0, 1.0);

		static GLboolean IsLighting;
		glGetBooleanv(GL_LIGHTING, &IsLighting);
		if (IsLighting) glDisable(GL_LIGHTING);

		DrawCylEdge(taper);

		if (IsLighting) glEnable(GL_LIGHTING);

	}
	glColor4fv(CurColor); //restore the (previously) current color...

}

// --------------------------------------------------------------------------
void CGL_Utils::DrawCylinder(const Vec3D<>& v0, const Vec3D<>& v1, float Rad, float Rad2, const Vec3D<>& Squeeze, bool Faces, bool Edges, float LineWidth, const CColor& Color)
// --------------------------------------------------------------------------
{ // draw solid cylinder between two points, with arbitrary taper
		static vfloat pi = atan(1.0)*4.0;
		Vec3D<> ax = (v1-v0).Normalized();
		vfloat len = (v1-v0).Length();
		Vec3D<> rotax;
		vfloat rotang;
		AlignWith(ax, Vec3D<>(0,0,1), rotax, rotang);
		glPushMatrix();

		glTranslated(v0.x, v0.y, v0.z); //move to the right spot

		glRotated(-rotang*180/pi, rotax.x, rotax.y, rotax.z); //rotate to correct angle
		glScaled(2.0*Rad, 2.0*Rad, len); //scale the radius and length
		glTranslated(0, 0, 0.5); //move to the right spot to rotate from
		glScaled(Squeeze.x, Squeeze.y, Squeeze.z); //scale the radius and length

		DrawCylinder(Rad2/Rad, Faces, Edges, LineWidth, Color); //draw it!
		glPopMatrix();
	
}

// --------------------------------------------------------------------------
void CGL_Utils::DrawCylinder(const Vec3D<>& v0, const Vec3D<>& v1, float Rad, const Vec3D<>& Squeeze, bool Faces, bool Edges, float LineWidth, const CColor& Color)
// --------------------------------------------------------------------------
{ //Draw even cylinder
	DrawCylinder(v0, v1, Rad, Rad, Squeeze, Faces, Edges, LineWidth, Color);
}

// --------------------------------------------------------------------------
void CGL_Utils::DrawCone(const Vec3D<>& v0, const Vec3D<>& v1, float Rad, bool Faces, bool Edges, float LineWidth, const CColor& Color)
// --------------------------------------------------------------------------
{ //Draw cone
	DrawCylinder(v0, v1, Rad, Rad/1000, Vec3D<>(1.0, 1.0, 1.0), Faces, Edges, LineWidth, Color);
}

// --------------------------------------------------------------------------
void CGL_Utils::DrawCylFace(float taper)
// --------------------------------------------------------------------------
{
	static vfloat pi = atan(1.0)*4.0;

//	static GLuint CylFaceList[MAXNUMWINDOWS] = {0}; //can't build with different tapers... bummer!

//	if (CylFaceList[CurContextID] == 0){ //if We haven't built the list yet...
//		CylFaceList[CurContextID] = glGenLists(1);
//		glNewList(CylFaceList[CurContextID], GL_COMPILE);

		// draw segments
		glBegin(GL_QUAD_STRIP);
		for (vfloat a=0; a<pi*2.01; a+=pi/18) {
			glNormal3d(cos(a),sin(a),1-taper);
			glVertex3d(taper*0.5*cos(a),taper*0.5*sin(a),0.5);
			glVertex3d(0.5*cos(a),0.5*sin(a),-0.5);
		}
		glEnd();

		//Endcaps
		glNormal3d(0,0,1); //normal is opposite so we can draw in same order...
		glBegin(GL_TRIANGLE_FAN);
		glVertex3d(0,0,-0.5);
		for (vfloat a=0; a<pi*2.01; a+=pi/18) {glVertex3d(0.5*cos(a),0.5*sin(a),-0.5);}
		glEnd();

		glNormal3d(0,0,1);
		glBegin(GL_TRIANGLE_FAN);
		glVertex3d(0,0,0.5);
		for (vfloat a=0; a<pi*2.01; a+=pi/18) {glVertex3d(taper*0.5*cos(a),taper*0.5*sin(a),0.5);}
		glEnd();

//		glEndList();
//	}
//	glCallList(CylFaceList[CurContextID]);
}

// --------------------------------------------------------------------------
void CGL_Utils::DrawCylEdge(float taper)
// --------------------------------------------------------------------------
{
	static vfloat pi = atan(1.0)*4.0;

//	static GLuint CylEdgeList[MAXNUMWINDOWS] = {0};

//	if (CylEdgeList[CurContextID] == 0){ //if We haven't built the list yet...
//		CylEdgeList[CurContextID] = glGenLists(1);
//		glNewList(CylEdgeList[CurContextID], GL_COMPILE);

		glBegin(GL_LINE_STRIP);
		for (vfloat a=0; a<pi*2.01; a+=pi/18) {glVertex3d(0.5*cos(a),0.5*sin(a),-0.5);}
		glEnd();
			
		glBegin(GL_LINE_STRIP);
		for (vfloat a=0; a<pi*2.01; a+=pi/18) {glVertex3d(taper*0.5*cos(a),taper*0.5*sin(a),0.5);}
		glEnd();

//		glEndList();
//	}
//	glCallList(CylEdgeList[CurContextID]);
}

/* ---------------------------------------------------------------- */
void CGL_Utils::DrawRectangle(bool Fill, float LineWidth, const CColor& Color)
/* ---------------------------------------------------------------- */
{ //draw unit square in Z Plane
	glDisable(GL_LIGHTING);

	if (Fill){
		if (Color.isValid()) glColor4d(Color.r, Color.g, Color.b, Color.a);

		//Endcaps
		glNormal3d(0,0,1); //normal is opposite so we can draw in same order...
		glBegin(GL_TRIANGLE_FAN);
		glVertex3d(-0.5,-0.5,0);
		glVertex3d(0.5,-0.5,0);
		glVertex3d(0.5,0.5,0);
		glVertex3d(-0.5,0.5,0);
		glEnd();
	}

	//draw the outline:
	GLfloat LastColor[4];
	glGetFloatv(GL_CURRENT_COLOR, LastColor);
	glColor4f(0.0, 0.0, 0.0, 1.0); //defualt is black...
	if (LineWidth != 0.0)
		glLineWidth(LineWidth);

	glBegin(GL_LINE_STRIP);
	glVertex3d(-0.5,-0.5,0);
	glVertex3d(0.5,-0.5,0);
	glVertex3d(0.5,0.5,0);
	glVertex3d(-0.5,0.5,0);
	glVertex3d(-0.5,-0.5,0);
	glEnd();

	glColor4fv(LastColor);

	glEnable(GL_LIGHTING);
}

/* ---------------------------------------------------------------- */
void CGL_Utils::DrawRectangle(const Vec3D<>& Center, vfloat Dim, const Vec3D<>& Normal, const Vec3D<>& Squeeze, bool Fill, float LineWidth, const CColor& Color)
/* ---------------------------------------------------------------- */
{ //draw square
	static vfloat pi = atan(1.0)*4.0;

	Vec3D<> rotax;
	vfloat rotang;
	AlignWith(Normal, Vec3D<>(0,0,1), rotax, rotang);

	glPushMatrix();
	glTranslated(Center.x, Center.y, Center.z); //move to the right spot
	glScaled(Dim*Squeeze.x, Dim*Squeeze.y, Dim*Squeeze.z); //scale the size
	glRotated(-rotang*180/pi, rotax.x, rotax.y, rotax.z); //rotate to correct angle
	DrawRectangle(Fill, LineWidth, Color); //draw it!
	glPopMatrix();
}

/* ---------------------------------------------------------------- */
void CGL_Utils::DrawRectangle(const Vec3D<>& V1, const Vec3D<>& V2, bool Fill, float LineWidth, const CColor& Color)
/* ---------------------------------------------------------------- */
{ //draw rectangle. Assumes we want to draw in one of the ordinate planes.
	
	Vec3D<> MinVec(fmin(V1.x, V2.x), fmin(V1.y, V2.y), fmin(V1.z, V2.z));
	Vec3D<> MaxVec(fmax(V1.x, V2.x), fmax(V1.y, V2.y), fmax(V1.z, V2.z));
	Vec3D<> CenVec((V1.x+V2.x)/2.0, (V1.y+V2.y)/2.0, (V1.z+V2.z)/2.0);



	glPushMatrix();
	glTranslated(CenVec.x, CenVec.y, CenVec.z); //move to the right spot
	glScaled(MaxVec.x-MinVec.x, MaxVec.y-MinVec.y, MaxVec.z-MinVec.z); //scale the radius and length
	if(MinVec.x==MaxVec.x) { 
		glRotated(90.0, 0.0, 1.0, 0.0);} //rotate to correct angle
	else if(MinVec.y==MaxVec.y) {
		glRotated(90.0, 1.0, 0.0, 0.0);} //rotate to correct angle
	DrawRectangle(Fill, LineWidth, Color); //draw it!
	glPopMatrix();
}

/* ---------------------------------------------------------------- */
void CGL_Utils::DrawCircle(bool Fill, float LineWidth, const CColor& Color)
/* ---------------------------------------------------------------- */
{ //draw unit sphere in Z Plane
	glDisable(GL_LIGHTING);
	static vfloat pi = atan(1.0)*4.0;

	int Segments = 36;
	if (Fill){
		if (Color.isValid()) glColor4d(Color.r, Color.g, Color.b, Color.a);

		//Endcaps
		glNormal3d(0,0,1); //normal is opposite so we can draw in same order...
		glBegin(GL_TRIANGLE_FAN);
		glVertex3d(0,0,0);
		for (vfloat a=0; a<pi*2.01; a+=2*pi/Segments) {glVertex3d(cos(a),sin(a),0);}
		glEnd();
	}

	//draw the outline:
	GLfloat LastColor[4];
	glGetFloatv(GL_CURRENT_COLOR, LastColor);
	glColor4f(0.0, 0.0, 0.0, 1.0); //defualt is black...
	if (LineWidth != 0.0)
		glLineWidth(LineWidth);

	glBegin(GL_LINE_STRIP);
	for (vfloat a=0; a<pi*2.01; a+=2*pi/Segments) {glVertex3d(cos(a),sin(a),0);}
	glEnd();

	glColor4fv(LastColor);

	glEnable(GL_LIGHTING);
}

/* ---------------------------------------------------------------- */
void CGL_Utils::DrawCircle(const Vec3D<>& p, vfloat Rad, const Vec3D<>& Normal, const Vec3D<>& Squeeze, bool Fill, float LineWidth, const CColor& Color) 
/* ---------------------------------------------------------------- */
{
	static vfloat pi = atan(1.0)*4.0;

	Vec3D<> rotax;
	vfloat rotang;
	AlignWith(Normal, Vec3D<>(0,0,1), rotax, rotang);

	glPushMatrix();
	glTranslated(p.x, p.y, p.z); //move to the right spot
	glScaled(Rad*Squeeze.x, Rad*Squeeze.y, Rad*Squeeze.z); //scale the radius and length
	glRotated(-rotang*180/pi, rotax.x, rotax.y, rotax.z); //rotate to correct angle
	DrawCircle(Fill, LineWidth, Color); //draw it!
	glPopMatrix();
}

/* ---------------------------------------------------------------- */
void CGL_Utils::DrawArrow(const CColor& Color) //draw unit arrow from origin to ZPlane
/* ---------------------------------------------------------------- */
{
	// glPushMatrix();
	// glTranslatef(0.0f, 0.0f, 0.8f);
	// glScalef(0.12f, 0.12f, 0.2f); //scale the radius and length
	// glTranslatef(0.0f, 0.0f, 0.5f);
	// DrawCylinder(0.001f, true, true, 1.0f, Color);
	// glPopMatrix();

	// glPushMatrix();
	// glScalef(0.055f, 0.055f, 0.799f); //scale the radius and length
	// glTranslatef(0.0f, 0.0f, 0.5f);
	// DrawCylinder(1.0f, true, true, 1.0f, Color);
	// glPopMatrix();



}

/* ---------------------------------------------------------------- */
void CGL_Utils::DrawArrowD(const Vec3D<>& vO, const Vec3D<>& vP, const CColor& Color) //draws 3D arrow from vO to vP
/* ---------------------------------------------------------------- */
{
	static vfloat pi = atan(1.0)*4.0;
	Vec3D<> ax = (vP-vO).Normalized();
	vfloat len = (vP-vO).Length();
	Vec3D<> rotax;
	vfloat rotang;
	AlignWith(ax, Vec3D<>(0,0,1), rotax, rotang);
	glPushMatrix();

	glTranslated(vO.x, vO.y, vO.z); //move to the right spot

	glRotated(-rotang*180/pi, rotax.x, rotax.y, rotax.z); //rotate to correct angle
	glScaled(len, len, len); //scale uniformly
	DrawArrow(Color); //draw it!
	glPopMatrix();
}

void CGL_Utils::DrawArrow(const Vec3D<>& vO, const Vec3D<>& Dir, const CColor& Color) //draws 3D arrow from vO to vP
{
	DrawArrowD(vO, vO+Dir, Color);
}

void CGL_Utils::DrawLineArrowD(const Vec3D<>& vO, const Vec3D<>& vP, const vfloat LineWidth, const CColor& Color) //draws 3D arrow Directly from vO to vP
{
	vfloat HeadPerc = 0.2;

	glDisable(GL_LIGHTING);
	glColor4d(Color.r, Color.g, Color.b, Color.a);
	glLineWidth(LineWidth);
	glBegin(GL_LINES);
	glLoadName(-1); //to disable picking

	Vec3D<> ThisVec = vP-vO;
	Vec3D<> HeadBackPoint = -HeadPerc*ThisVec;

	Vec3D<> Perp[6];
	Perp[0] = 0.6*HeadPerc * Vec3D<>(-ThisVec.y, ThisVec.x, 0);
	Perp[1] = 0.6*HeadPerc * Vec3D<>(-ThisVec.z, 0, ThisVec.x);
	Perp[2] = 0.6*HeadPerc * Vec3D<>(0, -ThisVec.z, ThisVec.y);
	Perp[3] = -Perp[0];
	Perp[4] = -Perp[1];
	Perp[5] = -Perp[2];

	glVertex3d(vO.x, vO.y, vO.z);
	glVertex3d(vP.x, vP.y, vP.z);

	for (int i=0; i<6; i++){
		glVertex3d(vP.x, vP.y, vP.z);
		glVertex3d(vP.x+HeadBackPoint.x+Perp[i].x, vP.y+HeadBackPoint.y+Perp[i].y, vP.z+HeadBackPoint.z+Perp[i].z);
		glVertex3d(vP.x+HeadBackPoint.x, vP.y+HeadBackPoint.y, vP.z+HeadBackPoint.z);
		glVertex3d(vP.x+HeadBackPoint.x+Perp[i].x, vP.y+HeadBackPoint.y+Perp[i].y, vP.z+HeadBackPoint.z+Perp[i].z);
	}


	//glVertex3d(vP.x, vP.y, vP.z);
	//glVertex3d(vP.x+Perp2.x, vP.y+Perp2.y, vP.z+Perp2.z);

	//glVertex3d(vP.x, vP.y, vP.z);
	//glVertex3d(vP.x+Perp3.x, vP.y+Perp3.y, vP.z+Perp3.z);

	//glVertex3d(vP.x, vP.y, vP.z);
	//glVertex3d(vP.x+Perp4.x, vP.y+Perp4.y, vP.z+Perp4.z);

	//glVertex3d(vP.x, vP.y, vP.z);
	//glVertex3d(vP.x+Perp5.x, vP.y+Perp5.y, vP.z+Perp5.z);

	//glVertex3d(vP.x, vP.y, vP.z);
	//glVertex3d(vP.x+Perp6.x, vP.y+Perp6.y, vP.z+Perp6.z);

	glEnd();
	glEnable(GL_LIGHTING);

}

/* ---------------------------------------------------------------- */
void CGL_Utils::AlignWith(const Vec3D<>& base, const Vec3D<>& target, Vec3D<> &rotax, vfloat &rotamt)
/* ---------------------------------------------------------------- */
{ // find rotation axis and angle to align base vector with target vector

	Vec3D<> basevec = base;
	Vec3D<> targvec = target;
	basevec.Normalized();
	targvec.Normalized();
//	Vec3D rotaxis = Vec3D::crossProduct(basevec, targvec); //QT call
	Vec3D<> rotaxis = basevec.Cross(targvec); 
	if (rotaxis.Length2() == 0) 
		rotaxis = Vec3D<>(1, 0, 0);
	
	//return info
	rotax = rotaxis.Normalized();
//	rotamt = acos(Vec3D::dotProduct(basevec, targvec)); //QT call
	rotamt = acos(basevec.Dot(targvec));

}

#endif