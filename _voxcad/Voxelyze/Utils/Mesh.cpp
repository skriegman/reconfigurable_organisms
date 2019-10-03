/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "Mesh.h"
#ifdef WIN32
#include <Math.h>
#else
#include <math.h>
#endif


#include <algorithm>
#include <iostream>

#ifdef USE_OPEN_GL
#ifdef QT_GUI_LIB
#include <qgl.h>
#else
#include "OpenGLInclude.h" //If not using QT's openGL system, make a header file "OpenGLInclude.h" that includes openGL library functions 
#endif
//#include "GL_Utils.h"
#endif

#define STL_LABEL_SIZE 80

CMesh::CMesh(void)
{
	DrawSmooth = true;
	_CurBBMin = Vec3D<>(0,0,0);
	_CurBBMax = Vec3D<>(0,0,0);
	MeshChanged();

}

CMesh::~CMesh(void)
{
}

//copy constructure
CMesh::CMesh(CMesh& s) {
	*this = s;
}

//overload =
CMesh& CMesh::operator=(const CMesh& s) {

	Facets.resize(s.Facets.size());
	for (int i = 0; i<(int)Facets.size(); i++)
		Facets[i] = s.Facets[i];

	Vertices.resize(s.Vertices.size());
	for (int i = 0; i<(int)Vertices.size(); i++)
		Vertices[i] = s.Vertices[i];

	Lines.resize(s.Lines.size());
	for (int i = 0; i<(int)Lines.size(); i++)
		Lines[i] = s.Lines[i];
	
	DrawSmooth = s.DrawSmooth;
	_CurBBMin = s._CurBBMin;
	_CurBBMax = s._CurBBMax;
	
	MeshChanged();

	return *this;
}


void CMesh::WriteXML(CXML_Rip* pXML, bool MeshOnly)
{
	pXML->DownLevel("CMesh");
		pXML->Element("DrawSmooth", DrawSmooth);
		pXML->DownLevel("Vertices");
		std::vector<CVertex>::iterator VIt;
		for(VIt=Vertices.begin(); VIt != Vertices.end(); VIt++){
			pXML->DownLevel("Vertex");
				pXML->Element("Vx", VIt->v.x);
				pXML->Element("Vy", VIt->v.y);
				pXML->Element("Vz", VIt->v.z);
				if (!MeshOnly){
					if (VIt->n != Vec3D<>(0,0,0)){
						pXML->Element("Nx", VIt->n.x);
						pXML->Element("Ny", VIt->n.y);
						pXML->Element("Nz", VIt->n.z);
					}
					pXML->Element("R", VIt->VColor.r);
					pXML->Element("G", VIt->VColor.g);
					pXML->Element("B", VIt->VColor.b);
					pXML->Element("A", VIt->VColor.a);
					if (VIt->DrawOffset != Vec3D<>(0,0,0)){
						pXML->Element("DOx", VIt->DrawOffset.x);
						pXML->Element("DOy", VIt->DrawOffset.y);
						pXML->Element("DOz", VIt->DrawOffset.z);
					}
				}
			pXML->UpLevel();
		}
		pXML->UpLevel();

		pXML->DownLevel("Facets");
		std::vector<CFacet>::iterator FIt;
		for(FIt=Facets.begin(); FIt != Facets.end(); FIt++){
			pXML->DownLevel("Facet");
				pXML->Element("V0", FIt->vi[0]);
				pXML->Element("V1", FIt->vi[1]);
				pXML->Element("V2", FIt->vi[2]);
				if (!MeshOnly){
					if (FIt->n != Vec3D<>(0,0,0)){
						pXML->Element("Nx", FIt->n.x);
						pXML->Element("Ny", FIt->n.y);
						pXML->Element("Nz", FIt->n.z);
					}
					pXML->Element("R", FIt->FColor.r);
					pXML->Element("G", FIt->FColor.g);
					pXML->Element("B", FIt->FColor.b);
					pXML->Element("A", FIt->FColor.a);
					pXML->Element("Name", FIt->Name);
				}
			pXML->UpLevel();
		}
		pXML->UpLevel();

		pXML->DownLevel("Lines");
		std::vector<CLine>::iterator LIt;
		for(LIt=Lines.begin(); LIt != Lines.end(); LIt++){
			pXML->DownLevel("Line");
				pXML->Element("V0", LIt->vi[0]);
				pXML->Element("V1", LIt->vi[1]);
			pXML->UpLevel();
		}
		pXML->UpLevel();
	pXML->UpLevel();
}

bool CMesh::ReadXML(CXML_Rip* pXML)
{
	Clear();

	if (!pXML->FindLoadElement("DrawSmooth", &DrawSmooth)) DrawSmooth = false;

	CVertex tmp;
	if (pXML->FindElement("Vertices")){
		while (pXML->FindElement("Vertex")){
			if (!pXML->FindLoadElement("Vx", &tmp.v.x)) tmp.v.x = 0.0;
			if (!pXML->FindLoadElement("Vy", &tmp.v.y)) tmp.v.y = 0.0;
			if (!pXML->FindLoadElement("Vz", &tmp.v.z)) tmp.v.z = 0.0;
			if (!pXML->FindLoadElement("Nx", &tmp.n.x)) tmp.n.x = 0.0;
			if (!pXML->FindLoadElement("Ny", &tmp.n.y)) tmp.n.y = 0.0;
			if (!pXML->FindLoadElement("Nz", &tmp.n.z)) tmp.n.z = 0.0;
			if (!pXML->FindLoadElement("R", &tmp.VColor.r)) tmp.VColor.r = 1.0;
			if (!pXML->FindLoadElement("G", &tmp.VColor.g)) tmp.VColor.g = 1.0;
			if (!pXML->FindLoadElement("B", &tmp.VColor.b)) tmp.VColor.b = 1.0;
			if (!pXML->FindLoadElement("A", &tmp.VColor.a)) tmp.VColor.a = 1.0;
			if (!pXML->FindLoadElement("DOx", &tmp.DrawOffset.x)) tmp.DrawOffset.x = 0.0;
			if (!pXML->FindLoadElement("DOy", &tmp.DrawOffset.y)) tmp.DrawOffset.y = 0.0;
			if (!pXML->FindLoadElement("DOz", &tmp.DrawOffset.z)) tmp.DrawOffset.z = 0.0;
			Vertices.push_back(tmp);
		}
		pXML->UpLevel();
	}

	CFacet Ftmp;
	if (pXML->FindElement("Facets")){
		while (pXML->FindElement("Facet")){
			if (!pXML->FindLoadElement("V0", &Ftmp.vi[0])) Ftmp.vi[0] = 0;
			if (!pXML->FindLoadElement("V1", &Ftmp.vi[1])) Ftmp.vi[1] = 0;
			if (!pXML->FindLoadElement("V2", &Ftmp.vi[2])) Ftmp.vi[2] = 0;
			if (!pXML->FindLoadElement("Nx", &Ftmp.n.x)) Ftmp.n.x = 0.0;
			if (!pXML->FindLoadElement("Ny", &Ftmp.n.y)) Ftmp.n.y = 0.0;
			if (!pXML->FindLoadElement("Nz", &Ftmp.n.z)) Ftmp.n.z = 0.0;
			if (!pXML->FindLoadElement("R", &Ftmp.FColor.r)) Ftmp.FColor.r = 1.0;
			if (!pXML->FindLoadElement("G", &Ftmp.FColor.g)) Ftmp.FColor.g = 1.0;
			if (!pXML->FindLoadElement("B", &Ftmp.FColor.b)) Ftmp.FColor.b = 1.0;
			if (!pXML->FindLoadElement("A", &Ftmp.FColor.a)) Ftmp.FColor.a = 1.0;
			if (!pXML->FindLoadElement("Name", &Ftmp.Name)) Ftmp.Name = -1;

			Facets.push_back(Ftmp);
		}
		pXML->UpLevel();
	}

	CLine Ltmp;
	if (pXML->FindElement("Lines")){
		while (pXML->FindElement("Line")){
			if (!pXML->FindLoadElement("V0", &Ltmp.vi[0])) Ltmp.vi[0] = 0;
			if (!pXML->FindLoadElement("V1", &Ltmp.vi[1])) Ltmp.vi[1] = 0;
			Lines.push_back(Ltmp);
		}
		pXML->UpLevel();
	}

	UpdateBoundingBox();

	CalcFaceNormals();
	CalcVertNormals();
	/*
	int PrimType = -1;
	float fX, fY, fZ; //to retrieve to Vec3D
	if (!pXML->FindLoadElement("PrimType", &PrimType)) return false;

	if (PrimType == PRIM_BOX)
		CreateBoxRegion(Vec3D(0,0,0), Vec3D(0,0,0)); //creates box and sets pointer
	else if (PrimType == PRIM_CYLINDER)
		CreateCylRegion(Vec3D(0,0,0), Vec3D(0,0,0), 0);  //creates cylinder and sets pointer
	else if (PrimType == PRIM_SPHERE)
		CreateSphRegion(Vec3D(0,0,0), 0);  //creates Sphere and sets pointer
	else if (PrimType == PRIM_MESH)
		CreateMeshRegion(Vec3D(0,0,0), Vec3D(0,0,0));  //creates Sphere and sets pointer
	

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
	if (!pXML->FindLoadElement("Fixed", &Fixed)) Fixed = 0;
	if (!pXML->FindLoadElement("ForceX", &fX)) fX = 0;
	if (!pXML->FindLoadElement("ForceY", &fY)) fY = 0;
	if (!pXML->FindLoadElement("ForceZ", &fZ)) fZ = 0;

	Force = Vec3D(fX, fY, fZ);
	pRegion->UpdateAspect();
	*/
	return true;
}

bool CMesh::LoadSTL(std::string filename)
{
	FILE *fp;
	bool binary=false;
#ifdef WIN32
	fopen_s(&fp, filename.c_str(), "r"); //secure version. preferred on windows platforms...
#else
	fp = fopen(filename.c_str(), "r");
#endif

	if(fp == NULL) return false;

	/* Find size of file */
	fseek(fp, 0, SEEK_END);
	int file_size = ftell(fp);
	int facenum;
	/* Check for binary or ASCII file */
	fseek(fp, STL_LABEL_SIZE, SEEK_SET);
	fread(&facenum, sizeof(int), 1, fp);
	int expected_file_size=STL_LABEL_SIZE + 4 + (sizeof(short)+12*sizeof(float) )*facenum ;
	if(file_size ==  expected_file_size) binary = true;
	unsigned char tmpbuf[128];
	fread(tmpbuf,sizeof(tmpbuf),1,fp);
	for(unsigned int i = 0; i < sizeof(tmpbuf); i++){
		if(tmpbuf[i] > 127){
			binary=true;
			break;
		}
	}
	// Now we know if the stl file is ascii or binary.
	fclose(fp);
	bool RetVal;
	if(binary) RetVal =  LoadBinarySTL(filename);
	else RetVal = LoadAsciiSTL(filename);

	UpdateBoundingBox(); //get the bounding box here and now...
	MeshChanged();
	return RetVal;
}

bool CMesh::LoadBinarySTL(std::string filename)
{
	FILE *fp;
	
#ifdef WIN32
	fopen_s(&fp, filename.c_str(), "rb"); //secure version. preferred on windows platforms...
#else
	fp = fopen(filename.c_str(), "rb");
#endif


	if(fp == NULL) return false;

	int facenum;
	fseek(fp, STL_LABEL_SIZE, SEEK_SET);
	fread(&facenum, sizeof(int), 1, fp);

	Clear();

	// For each triangle read the normal, the three coords and a short set to zero
	float N[3];
	float P[9];
	short attr;

	for(int i=0;i<facenum;++i) {
		fread(&N,3*sizeof(float),1,fp); //We end up throwing this out and recalculating because... we don't trust it!!!
		fread(&P,3*sizeof(float),3,fp);
		fread(&attr,sizeof(short),1,fp);
		AddFacet(Vec3D<>(P[0], P[1], P[2]), Vec3D<>(P[3], P[4], P[5]), Vec3D<>(P[6], P[7], P[8]));
	}
	fclose(fp);

	CalcFaceNormals();

	return true;
}

bool CMesh::LoadAsciiSTL(std::string filename)
{
	FILE *fp;

#ifdef WIN32
	fopen_s(&fp, filename.c_str(), "r"); //secure version. preferred on windows platforms...
#else
	fp = fopen(filename.c_str(), "r");
#endif

	if(fp == NULL) return false;

	long currentPos = ftell(fp);
	fseek(fp,0L,SEEK_END);
//	long fileLen = ftell(fp);
	fseek(fp,currentPos,SEEK_SET);

	Clear();

	/* Skip the first line of the file */
	while(getc(fp) != '\n') { }

	float N[3];
	float P[9];
	int cnt=0;
	int lineCnt=0;
	int ret;
	/* Read a single facet from an ASCII .STL file */
	while(!feof(fp)){
		ret=fscanf(fp, "%*s %*s %f %f %f\n", &N[0], &N[1], &N[2]); // --> "facet normal 0 0 0" (We throw this out and recalculate based on vertices)
		if(ret!=3){
			// we could be in the case of a multiple solid object, where after a endfaced instead of another facet we have to skip two lines:
			//     endloop
			//	 endfacet
			//endsolid     <- continue on ret==0 will skip this line
			//solid ascii  <- and this one.
			//   facet normal 0.000000e+000 7.700727e-001 -6.379562e-001
			lineCnt++;
			continue; 
		}
		ret=fscanf(fp, "%*s %*s"); // --> "outer loop"
		ret=fscanf(fp, "%*s %f %f %f\n", &P[0],  &P[1],  &P[2]); // --> "vertex x y z"
		if(ret!=3) return false;
		ret=fscanf(fp, "%*s %f %f %f\n", &P[3],  &P[4],  &P[5]); // --> "vertex x y z"
		if(ret!=3) return false;
		ret=fscanf(fp, "%*s %f %f %f\n", &P[6],  &P[7],  &P[8]); // --> "vertex x y z"
		if(ret!=3) return false;
		ret=fscanf(fp, "%*s"); // --> "endloop"
		ret=fscanf(fp, "%*s"); // --> "endfacet"
		lineCnt+=7;
		if(feof(fp)) break;

		AddFacet(Vec3D<>(P[0], P[1], P[2]), Vec3D<>(P[3], P[4], P[5]), Vec3D<>(P[6], P[7], P[8]));

	}
	fclose(fp);

	CalcFaceNormals();

	return true;
}


bool CMesh::SaveSTL(std::string filename, bool Binary, bool SaveDeformed) const { //writes ascii stl file...

	FILE *fp;

#ifdef WIN32
	if (Binary) fopen_s (&fp, filename.c_str(),"wb"); //secure version. preferred on windows platforms...
	else fopen_s(&fp, filename.c_str(),"w");
#else
	if (Binary) fp = fopen(filename.c_str(),"wb");
	else fp = fopen(filename.c_str(),"w");
#endif

	if(fp==0) return false;
	int NumFaces = (int)Facets.size();

	if(Binary){
		// Write Header
		std::string tmp = "DefaultSTL                                                                                                    "; 
		//char header[128]=;
		fwrite(tmp.c_str(),80,1,fp);
		// write number of facets
		fwrite(&NumFaces,1,sizeof(int),fp); 
		unsigned short attributes=0;

		for(int i=0; i<NumFaces; i++){
			float N[3] = {Facets[i].n.x, Facets[i].n.y, Facets[i].n.z};
			float P[9] = {0};
			if (SaveDeformed){
				P[0] = Vertices[Facets[i].vi[0]].v.x + Vertices[Facets[i].vi[0]].DrawOffset.x;
				P[1] = Vertices[Facets[i].vi[0]].v.y + Vertices[Facets[i].vi[0]].DrawOffset.y;
				P[2] = Vertices[Facets[i].vi[0]].v.z + Vertices[Facets[i].vi[0]].DrawOffset.z;
				P[3] = Vertices[Facets[i].vi[1]].v.x + Vertices[Facets[i].vi[1]].DrawOffset.x;
				P[4] = Vertices[Facets[i].vi[1]].v.y + Vertices[Facets[i].vi[1]].DrawOffset.y;
				P[5] = Vertices[Facets[i].vi[1]].v.z + Vertices[Facets[i].vi[1]].DrawOffset.z;
				P[6] = Vertices[Facets[i].vi[2]].v.x + Vertices[Facets[i].vi[2]].DrawOffset.x;
				P[7] = Vertices[Facets[i].vi[2]].v.y + Vertices[Facets[i].vi[2]].DrawOffset.y;
				P[8] = Vertices[Facets[i].vi[2]].v.z + Vertices[Facets[i].vi[2]].DrawOffset.z;
			}
			else {
				P[0] = Vertices[Facets[i].vi[0]].v.x;
				P[1] = Vertices[Facets[i].vi[0]].v.y;
				P[2] = Vertices[Facets[i].vi[0]].v.z;
				P[3] = Vertices[Facets[i].vi[1]].v.x;
				P[4] = Vertices[Facets[i].vi[1]].v.y;
				P[5] = Vertices[Facets[i].vi[1]].v.z;
				P[6] = Vertices[Facets[i].vi[2]].v.x;
				P[7] = Vertices[Facets[i].vi[2]].v.y;
				P[8] = Vertices[Facets[i].vi[2]].v.z;
			}

			// For each triangle write the normal, the three coords and a short set to zero
			fwrite(&N,3,sizeof(float),fp);
 			for(int k=0;k<3;k++){fwrite(&P[3*k],3,sizeof(float),fp);}
			fwrite(&attributes,1,sizeof(short),fp);
		}
	}
	else
	{
		fprintf(fp,"solid jdh\n");
		for(int i=0; i<NumFaces; i++){
		  	// For each triangle write the normal, the three coords and a short set to zero
			fprintf(fp,"  facet normal %13e %13e %13e\n", Facets[i].n.x, Facets[i].n.y, Facets[i].n.z);
			fprintf(fp,"    outer loop\n");
			for(int k=0; k<3; k++){
				fprintf(fp,"      vertex  %13e %13e %13e\n", Vertices[Facets[i].vi[k]].v.x, Vertices[Facets[i].vi[k]].v.y, Vertices[Facets[i].vi[k]].v.z);			
			}
			fprintf(fp,"    endloop\n");
			fprintf(fp,"  endfacet\n");
		}
		fprintf(fp,"endsolid vcg\n");
	}
	fclose(fp);

	return 0;
}

#ifdef USE_OPEN_GL
//---------------------------------------------------------------------------
void CMesh::Draw(bool bModelhNormals, bool bShaded, bool bIngoreColors, bool bIgnoreNames,  bool drawForces, int whichVectors, float scalingFactor)
//---------------------------------------------------------------------------
{
	if (bShaded) {
		for (int i=0; i<(int)Facets.size(); i++) {
			if (!bIgnoreNames) glLoadName(Facets[i].Name);
			glBegin(GL_TRIANGLES);

			if (!DrawSmooth){ //if setting things per triangle, can do normal and color here...
				glNormal3d(Facets[i].n.x, Facets[i].n.y, Facets[i].n.z);
				if (!bIngoreColors) glColor3d(Facets[i].FColor.r, Facets[i].FColor.g, Facets[i].FColor.b);
			}
			for (int j=0; j<3; j++) {
				CVertex& CurVert = Vertices[Facets[i].vi[j]]; //just a local reference for readability

				if (DrawSmooth){ //if we want to draw smoothed normals/colors per vertex
					glNormal3d(CurVert.n.x, CurVert.n.y, CurVert.n.z);
					if (!bIngoreColors) glColor3d(CurVert.VColor.r, CurVert.VColor.g, CurVert.VColor.b);
				}

				glVertex3d(CurVert.v.x + CurVert.DrawOffset.x, CurVert.v.y + CurVert.DrawOffset.y, CurVert.v.z + CurVert.DrawOffset.z);

			}
			glEnd();

		}

//		 // ---------------------------------------------------------------------------------------
//		 // nac: draw shadow
//
//		 for (int i=0; i<(int)Facets.size(); i++) {
//		 	if (!bIgnoreNames) glLoadName(Facets[i].Name);
//		 	glBegin(GL_TRIANGLES);
//
//		 	if (!DrawSmooth){ //if setting things per triangle, can do normal and color here...
//		 		glNormal3d(Facets[i].n.x, Facets[i].n.y, Facets[i].n.z);
//		 		if (!bIngoreColors) glColor3d(0, 0, 0);
//		 	}
//		 	for (int j=0; j<3; j++) {
//		 		CVertex& CurVert = Vertices[Facets[i].vi[j]]; //just a local reference for readability
//
//		 		if (DrawSmooth){ //if we want to draw smoothed normals/colors per vertex
//		 			glNormal3d(CurVert.n.x, CurVert.n.y, CurVert.n.z);
//		 			if (!bIngoreColors) glColor3d(0, 0, 0);
//		 		}
//
//		 		glVertex3d(CurVert.v.x + CurVert.DrawOffset.x, CurVert.v.y + CurVert.DrawOffset.y, 0.00001);
//
//		 	}
//		 	glEnd();
//
//		 }
//
//
//		// ---------------------------------------------------------------------------------------

		// nac: Drawing drag forces

		if(drawForces)
		{

			for (int i=0; i<(int)Facets.size(); i++) {
				if (!bIgnoreNames) glLoadName(Facets[i].Name);
				// glBegin(GL_TRIANGLES);

				// if (!DrawSmooth){ //if setting things per triangle, can do normal and color here...
				// 	glNormal3d(Facets[i].n.x, Facets[i].n.y, Facets[i].n.z);
				// 	if (!bIngoreColors) glColor3d(Facets[i].FColor.r, Facets[i].FColor.g, Facets[i].FColor.b);
				// }
				// float VertSumX = 0;
				// float VertSumY = 0;
				// float VertSumZ = 0;

				// FC: here we are computing the centroid of the facet,
				// the origin from which we are going to plot the drag force vector
				Vec3D<> VertPos;
				for (int j=0; j<3; j++) 
				{
					CVertex& CurVert = Vertices[Facets[i].vi[j]]; //just a local reference for readability

				// 	if (DrawSmooth){ //if we want to draw smoothed normals/colors per vertex
				// 		glNormal3d(CurVert.n.x, CurVert.n.y, CurVert.n.z);
				// 		if (!bIngoreColors) glColor3d(CurVert.VColor.r, CurVert.VColor.g, CurVert.VColor.b);
				// 	}

					// glVertex3d(CurVert.v.x + CurVert.DrawOffset.x, CurVert.v.y + CurVert.DrawOffset.y, CurVert.v.z + CurVert.DrawOffset.z);
					// VertSumX = VertSumX + CurVert.v.x/3.0;
					// VertSumY = VertSumY + CurVert.v.y/3.0;
					// VertSumZ = VertSumZ + CurVert.v.z/3.0;
					// std::cout << CurVert.v.x << std::endl;
					VertPos.setX(VertPos.x + CurVert.OffPos().x/3.0);
					VertPos.setY(VertPos.y + CurVert.OffPos().y/3.0);
					VertPos.setZ(VertPos.z + CurVert.OffPos().z/3.0);
				}

				// CVertex A = Vertices[Facets[i].vi[0]].v;
				// CVertex B = Vertices[Facets[i].vi[1]].v;
				// CVertex C = Vertices[Facets[i].vi[2]].v;
				// Vec3D<> A = Vertices[Facets[i].vi[0]].v;
				// Vec3D<> B = Vertices[Facets[i].vi[1]].v;
				// Vec3D<> C = Vertices[Facets[i].vi[2]].v;

				// Let's now compute two vectors defining this facet: we're gonna use them to compute the area of the facet
				
				/*
				Vec3D<> A = Vertices[Facets[i].vi[0]].OffPos();
				Vec3D<> B = Vertices[Facets[i].vi[1]].OffPos();
				Vec3D<> C = Vertices[Facets[i].vi[2]].OffPos();
				Vec3D<> AB = B-A;
				Vec3D<> AC = C-A;

				// Facet's area (will be useful to compute the drag)
				double Area = fabs(AB.Cross(AC).Length()/2.0);
				*/

				//AB = Vec3D<>(B.x-A.x, B.y-A.y, B.z-A.z);
				//AC = Vec3D<>(C.x-A.x, C.y-A.y, C.z-A.z);

				// std::cout << "AB:" << AB.x << ", " << AB.y << ", " << AB.z << std::endl;
				// std::cout << "AC:" << AC.x << ", " << AC.y << ", " << AC.z << std::endl;

				// std::cout << "AB.z:" << AB.z << std::endl;

				
				// std::cout << "area:" << Area << std::endl << std::endl;
				// glEnd();

				switch(whichVectors)
				{

					case 0: // DRAG
						CGL_Utils::DrawLineArrowD(VertPos, VertPos + (Facets[i].drag*scalingFactor), 1.0, CColor(0, 0, 0));  
					break;

					case 1: // SPEEDS
						 CGL_Utils::DrawLineArrowD(VertPos, VertPos + (Facets[i].speed*scalingFactor), 1.0, CColor(0, 0, 0));  
					break;

					case 2: // NORMALS
						CGL_Utils::DrawLineArrowD(VertPos, VertPos + (Facets[i].n*scalingFactor), 1.0, CColor(0, 0, 0));  
					break;

					default:
					break;

				}
				// CGL_Utils::DrawLineArrowD(VertPos, VertPos + (Facets[i].n*Area*200.0), 1.0, CColor(0, 0, 1)); //Red 
				// CGL_Utils::DrawLineArrowD(VertPos, Vec3D<>(VertPos.x+Facets[i].n.x*Area*100.0, VertPos.y+Facets[i].n.y*Area*100.0 ,VertPos.z+Facets[i].n.z*Area*100.0 ), 1.0, CColor(0, 0, 1)); //Red
				// std::cout << "vert sum:" << VertPos.x << std::endl << std::endl;

			}
		}

		// -------------------------------------------------------------------------------




		glLineWidth(1.0);

		glBegin(GL_LINES);
		glColor3d(0, 0, 0); //black only for now...

		for (int i=0; i<(int)Lines.size(); i++) {
			for (int j=0; j<2; j++) {
				CVertex& CurVert = Vertices[Lines[i].vi[j]]; //just a local reference for readability
				glVertex3d(CurVert.v.x + CurVert.DrawOffset.x, CurVert.v.y + CurVert.DrawOffset.y, CurVert.v.z + CurVert.DrawOffset.z);
			}
		}
		glEnd();

	}
	else { // wireframe
		for (int i=0; i<(int)Facets.size(); i++) {
			glBegin(GL_LINE_LOOP);
			glNormal3d(Facets[i].n.x, Facets[i].n.y, Facets[i].n.z);
			for (int j=0; j<3; j++) {
				CVertex& CurVert = Vertices[Facets[i].vi[j]]; //just a local reference for readability
				glColor3d(CurVert.VColor.r, CurVert.VColor.g, CurVert.VColor.b);
				glVertex3d(CurVert.v.x + CurVert.DrawOffset.x , CurVert.v.y + CurVert.DrawOffset.y, CurVert.v.z + CurVert.DrawOffset.z);
			}
			glEnd();
		}
	}

	if (bModelhNormals) {
		glColor3d(1,1,0);
		glBegin(GL_LINES);
		for (int i=0; i<(int)Facets.size(); i++) {
			Vec3D<> c = (Vertices[Facets[i].vi[0]].v + Vertices[Facets[i].vi[1]].v + Vertices[Facets[i].vi[2]].v)/3;
			Vec3D<> c2 = c - Facets[i].n*3;
			glVertex3d(c.x, c.y, c.z);
			glVertex3d(c2.x, c2.y, c2.z);
		}
		glEnd();
	}

}
#endif

//---------------------------------------------------------------------------
void CMesh::CalcFaceNormals() //called to update the face normals...
//---------------------------------------------------------------------------
{
	for (int i=0; i<(int)Facets.size(); i++){
		Facets[i].n = ((Vertices[Facets[i].vi[1]].OffPos()-Vertices[Facets[i].vi[0]].OffPos()).Cross(Vertices[Facets[i].vi[2]].OffPos()-Vertices[Facets[i].vi[0]].OffPos())).Normalized();
	}
}


//---------------------------------------------------------------------------
void CMesh::CalcVertNormals()
//---------------------------------------------------------------------------
{ //called once for each new geometry
	//fills in Vertices.n
	for (int i=0; i<(int)Vertices.size(); i++){
		Vertices[i].n = Vec3D<>(0,0,0);
	}

	for (int i=0; i<(int)Facets.size(); i++){

		for (int j=0; j<3; j++){
			Vertices[Facets[i].vi[j]].n += Facets[i].n;
		}
	}

	for (int i=0; i<(int)Vertices.size(); i++){
		Vertices[i].n.Normalize();
	}
}

void CMesh::AddFacet(const Vec3D<>& v1, const Vec3D<>& v2, const Vec3D<>& v3, bool QuickAdd) //adds a facet, checks vertex list for existing vertices...
{
	AddFacet(v1, v2, v3, CColor(0.5, 0.5, 0.5, 1.0), CColor(0.5, 0.5, 0.5, 1.0), CColor(0.5, 0.5, 0.5, 1.0));
}

//---------------------------------------------------------------------------
void CMesh::AddFacet(const Vec3D<>& v1, const Vec3D<>& v2, const Vec3D<>& v3, const CColor& Col1, const CColor& Col2, const CColor& Col3, bool QuickAdd) //adds a facet... with color info
//---------------------------------------------------------------------------
{
	vfloat WeldThresh = 1e-10f; //This needs to be around the precision of a float.

	Vec3D<> Points[3]; //make a local array for easy referencing
	Points[0] = v1;
	Points[1] = v2;
	Points[2] = v3;
	CColor Colors[3];
	Colors[0] = Col1;
	Colors[1] = Col2;
	Colors[2] = Col3;


	int FoundIndex[3]; //each index for a triangle...

	for (int j=0; j<3; j++){ //each point in this facet
		FoundIndex[j] = -1;

		if (!QuickAdd){
			for (int k=Vertices.size()-1; k>=0; k--){ //DO THIS BACKWARDS!!!! (more likely to have just added one next to us...)
				if (abs(Points[j].x - Vertices[k].v.x) < WeldThresh  &&  abs(Points[j].y - Vertices[k].v.y) < WeldThresh  &&  abs(Points[j].z - Vertices[k].v.z) < WeldThresh){ //if points are identical...
					FoundIndex[j] = k;
					break; //kicks out of for loop, because we've found!
				}
			}
		}

		if (FoundIndex[j] == -1){ //if we didn't find one...
			CVertex ThisPoint;
			ThisPoint.v.x = Points[j].x;
			ThisPoint.v.y = Points[j].y;
			ThisPoint.v.z = Points[j].z;
			ThisPoint.VColor = Colors[j];

			Vertices.push_back(ThisPoint);
			FoundIndex[j] = (int)Vertices.size() - 1; //-1 because zero-index based.
		}

	}

//	CFacet ThisFacet;
//	for (int m=0; m<3; m++) ThisFacet.vi[m] = FoundIndex[m];

	Facets.push_back(CFacet(FoundIndex[0], FoundIndex[1], FoundIndex[2])); //TODO... select whether to create new object or add to existing...
	MeshChanged();

}

//---------------------------------------------------------------------------
void CMesh::ComputeBoundingBox(Vec3D<> &pmin, Vec3D<> &pmax)
//---------------------------------------------------------------------------
{
	UpdateBoundingBox();
	pmin = _CurBBMin;
	pmax = _CurBBMax;

}

//---------------------------------------------------------------------------
void CMesh::UpdateBoundingBox(void)
//---------------------------------------------------------------------------
{
	if (Vertices.size() == 0)
		return;

	_CurBBMin = _CurBBMax = Vertices[0].v;
	
	for (int i=0; i<(int)Vertices.size(); i++) {
		_CurBBMin.x = _CurBBMin.x < Vertices[i].v.x ? _CurBBMin.x : Vertices[i].v.x;
		_CurBBMin.y = _CurBBMin.y < Vertices[i].v.y ? _CurBBMin.y : Vertices[i].v.y;
		_CurBBMin.z = _CurBBMin.z < Vertices[i].v.z ? _CurBBMin.z : Vertices[i].v.z;
		_CurBBMax.x = _CurBBMax.x > Vertices[i].v.x ? _CurBBMax.x : Vertices[i].v.x;
		_CurBBMax.y = _CurBBMax.y > Vertices[i].v.y ? _CurBBMax.y : Vertices[i].v.y;
		_CurBBMax.z = _CurBBMax.z > Vertices[i].v.z ? _CurBBMax.z : Vertices[i].v.z;
	}
}



//---------------------------------------------------------------------------
void CMesh::Translate(Vec3D<> d)
//---------------------------------------------------------------------------
{// translate geometry
	for (int i=0; i<(int)Vertices.size(); i++) {
		Vertices[i].v += d;
	}
	UpdateBoundingBox();
	MeshChanged();
}

//---------------------------------------------------------------------------
void CMesh::Scale(Vec3D<> s)
//---------------------------------------------------------------------------
{// scale geometry

	//check for zero scale factor
	if(s.x==0 || s.y==0 || s.z==0) return;
	for (int i=0; i<(int)Vertices.size(); i++) {
		Vertices[i].v.x *= s.x;
		Vertices[i].v.y *= s.y;
		Vertices[i].v.z *= s.z;
//		Vertices[i].n.x *= s.x; //do we really want to scale these?
//		Vertices[i].n.y *= s.y;
//		Vertices[i].n.z *= s.z;
///		Facets[i].n.Normalize();
	}
	UpdateBoundingBox();
	MeshChanged();
	
}

//---------------------------------------------------------------------------
void CMesh::Rotate(Vec3D<> ax, vfloat a)
//---------------------------------------------------------------------------
{

	for (int i=0; i<(int)Vertices.size(); i++) {
		Vertices[i].v = Vertices[i].v.Rot(ax, a);
		Vertices[i].n = Vertices[i].n.Rot(ax, a);
		Vertices[i].DrawOffset = Vertices[i].DrawOffset.Rot(ax, a);

		
	}
	for (int i=0; i<(int)Facets.size(); i++) {
		Facets[i].n = Facets[i].n.Rot(ax, a);
	}

	UpdateBoundingBox();
	MeshChanged();
	
}

//---------------------------------------------------------------------------
void CMesh::RotX(vfloat a)
//---------------------------------------------------------------------------
{
	for (int i=0; i<(int)Vertices.size(); i++) {
		Vertices[i].v.RotX(a);
		Vertices[i].n.RotX(a);
	}
	for (int i=0; i<(int)Facets.size(); i++) {
		Facets[i].n.RotX(a);
	}
	UpdateBoundingBox();
	MeshChanged();
	
}


//---------------------------------------------------------------------------
void CMesh::RotY(vfloat a)
//---------------------------------------------------------------------------
{
	for (int i=0; i<(int)Vertices.size(); i++) {
		Vertices[i].v.RotY(a);
		Vertices[i].n.RotY(a);
	}
	for (int i=0; i<(int)Facets.size(); i++) {
		Facets[i].n.RotY(a);
	}
	UpdateBoundingBox();
	MeshChanged();

}


//---------------------------------------------------------------------------
void CMesh::RotZ(vfloat a)
//---------------------------------------------------------------------------
{
	for (int i=0; i<(int)Vertices.size(); i++) {
		Vertices[i].v.RotZ(a);
		Vertices[i].n.RotZ(a);
	}
	for (int i=0; i<(int)Facets.size(); i++) {
		Facets[i].n.RotZ(a);
	}
	UpdateBoundingBox();
	MeshChanged();

}

//---------------------------------------------------------------------------
bool CMesh::IsInside(Vec3D<>* Point)
//---------------------------------------------------------------------------
{
	FillTriLine(Point->y, Point->z); //returns very fast if previously used z or y layers...
	//so, assume TriLine is filled correctly

	std::vector<vfloat>::iterator LIter;
	int count = 0;
	for (LIter = TriLine.begin(); LIter != TriLine.end(); LIter++){
		if (Point->x < *LIter) break;
		count ++;

	}
	if (count%2 == 1) return true; //if we've passed an off number of facets...
	else return false;
}


//---------------------------------------------------------------------------
int CMesh::GetXIntersections(vfloat z, vfloat y, vfloat* pIntersections, int NumtoCheck, int* pToCheck)
//---------------------------------------------------------------------------
{ //returns the number of intersections, stored in pIntersections. pToCheck is a vector of facet indices that are in this Z plane...
	Vec3D<> p;
	vfloat pu, pv, V1y, V2y, V3y;
	int NumFound = 0;

	for (int i=0; i<NumtoCheck; i++){ //for each facet we wish to check...
		V1y = Vertices[Facets[pToCheck[i]].vi[0]].v.y;
		V2y = Vertices[Facets[pToCheck[i]].vi[1]].v.y;
		V3y = Vertices[Facets[pToCheck[i]].vi[2]].v.y;
		//trivial checks (should get most of them...)
		if (V1y < y && V2y < y && V3y < y)
			continue;
		if (V1y > y && V2y > y && V3y > y)
			continue;

		if(IntersectXRay(&Facets[pToCheck[i]], y, z, p, pu, pv)) { //if it intersects
			if (InsideTri(p, Vertices[Facets[pToCheck[i]].vi[0]].v, Vertices[Facets[pToCheck[i]].vi[1]].v, Vertices[Facets[pToCheck[i]].vi[2]].v)){
				pIntersections[NumFound++] = p.x; //(1.0 - pu - pv)*Vertices[Facets[pToCheck[i]].vi[0]].v.x + pu*Vertices[Facets[pToCheck[i]].vi[1]].v.x + pv*Vertices[Facets[pToCheck[i]].vi[2]].v.x;
			}
		}
	}
	
//	if (NumFound%2 != 0) std::cout << "Uh-oh! Found an odd number of intersections!";
	
	//sort intersections... (bubble sort = slow, but these should be super small...
	vfloat tmp;
	for (int i=0; i<NumFound; i++){
		for (int j=0; j<NumFound - i - 1; j++){ //each iteration gets the largest element to the end...
			if(pIntersections[j] > pIntersections[j+1]){
				tmp = pIntersections[j+1];
				pIntersections[j+1] = pIntersections[j];
				pIntersections[j] = tmp;
			}
		}
	}

	return NumFound;
}

//---------------------------------------------------------------------------
bool CMesh::InsideTri(Vec3D<>& p, Vec3D<>& v0, Vec3D<>& v1, Vec3D<>& v2)
//---------------------------------------------------------------------------
{// True if point p projects to within triangle (v0;v1;v2)

	Vec3D<> xax = (v1-v0).Normalized();
	Vec3D<> zax = ((v2-v0).Cross(xax)).Normalized();
	Vec3D<> yax = zax.Cross(xax).Normalized();

	Vec3D<> p0(0,0,1);
	Vec3D<> p1((v1-v0).Dot(xax),(v1-v0).Dot(yax),1);
	Vec3D<> p2((v2-v0).Dot(xax),(v2-v0).Dot(yax),1);
	Vec3D<> pt((p-v0).Dot(xax),(p-v0).Dot(yax),1);

	vfloat d0 = Det(p0,p1,pt);
	vfloat d1 = Det(p1,p2,pt);
	vfloat d2 = Det(p2,p0,pt);

	if (d0<=0 && d1<=0 && d2<=0)
		return true;
	if (d0>=0 && d1>=0 && d2>=0)
		return true;

	return false;

}

//---------------------------------------------------------------------------
vfloat CMesh::Det(Vec3D<>& v0, Vec3D<>& v1, Vec3D<>& v2)
//---------------------------------------------------------------------------
{ // Compute determinant of 3x3 matrix v0,v1,v2

	return 

		v0.x*(v1.y*v2.z-v1.z*v2.y) +
		v0.y*(v1.z*v2.x-v1.x*v2.z) +
		v0.z*(v1.x*v2.y-v1.y*v2.x);

}

//---------------------------------------------------------------------------
bool CMesh::IntersectXRay(CFacet* pFacet, vfloat y, vfloat z, Vec3D<>& p, vfloat& pu, vfloat& pv)
//---------------------------------------------------------------------------
{
	// compute intersection point P of triangle plane with ray from origin O in direction D
	// D assumed to be normalized
	// if no interstion, return false
	// u and v are barycentric coordinates of the intersection point P = (1 - u - v)A + uB + vC 
	// see http://www.devmaster.net/wiki/Ray-triangle_intersection


	Vec3D<> a = Vertices[pFacet->vi[0]].v;
	Vec3D<> b = Vertices[pFacet->vi[1]].v;
	Vec3D<> c = Vertices[pFacet->vi[2]].v;
	
	vfloat MinX = a.x < b.x ? (a.x < c.x ? a.x : c.x) : (b.x < c.x ? b.x : c.x) - 1.0;
	Vec3D<> d(1,0,0);
	Vec3D<> o(MinX, y, z);

	//Vec3D n = pFacet->n; //((b-a).Cross(c-a)).Normalized();
	Vec3D<> n = ((b-a).Cross(c-a)).Normalized();
	//if (n.x > 0){ //flip vertices...
	//	Vec3D tmp = a;
	//	a = b;
	//	b = tmp;
	//	n = ((b-a).Cross(c-a)).Normalized();
	//}

	vfloat dn = d.Dot(n);
	if (fabs(dn)<1E-5)
		return false; //parallel

	vfloat dist = -(o-a).Dot(n)/dn;
	Vec3D<> sD = d*dist;
	p = o+sD;

	vfloat V1, V2, V3;
	V1 = (b-a).Cross(p-a).Dot(n);
	V2 = (c-b).Cross(p-b).Dot(n);
	V3 = (a-c).Cross(p-c).Dot(n);
	
	if (V1 >=0 && V2 >=0 && V2 >=0) return true;
	//if (V1 <=0 && V2 <=0 && V2 <=0) return true;
	else return false;

}

void CMesh::WeldClose(float Distance)
{

	int* NumVertHere = new int[Vertices.size()]; //keeps track of how many vertices have been averaged to get here...
	int* ConsolidateMap = new int[Vertices.size()]; //maps the whole vertex list to the welded vertex list (IE has holes)
	int* OldNewMap = new int [Vertices.size()]; //maps the old, larger vertex list to the new, smaller one.
	for (int i=0; i<(int)Vertices.size(); i++){
		NumVertHere[i] = 1;
		ConsolidateMap[i] = i;
		OldNewMap[i] = -1;
	}

	for (int i=0; i<(int)Facets.size(); i++){ //look through facets so we don't have to do exhaustive On2 search of all vertex combos
		for (int j=0; j<3; j++){ //look at all three combinations of vertices...
			int Vi1 = Facets[i].vi[j];
			int np = -1; while (np != Vi1){ np = Vi1; Vi1 = ConsolidateMap[Vi1]; } //iterates NewMap to get the final value...

			int Vi2 = Facets[i].vi[(j+1)%3];
			np = -1; while (np != Vi2){ np = Vi2; Vi2 = ConsolidateMap[Vi2]; } //iterates NewMap to get the final value...

			if (Vi1 != Vi2 && (Vertices[Vi1].v-Vertices[Vi2].v).Length() < Distance){ //if they are close enough but not already the same...
				Vertices[Vi1].v = (Vertices[Vi1].v*NumVertHere[Vi1] + Vertices[Vi2].v*NumVertHere[Vi2]) / (NumVertHere[Vi1]+NumVertHere[Vi2]); //Vertex 1 is the weighted average
				NumVertHere[Vi1] = NumVertHere[Vi1] + NumVertHere[Vi2]; //count how many vertices make up this point now...
				
				ConsolidateMap[Vi2] = Vi1; //effectively deletes Vi2... (points to Vi1)
			}
		}
	}

	std::vector<CFacet> NewFacets;
	std::vector<CVertex> NewVertices;

	for (int i=0; i<(int)Vertices.size(); i++){
		if (ConsolidateMap[i] == i) { //if this vertex ended up being part of the welded part
			NewVertices.push_back(Vertices[i]); //add to the new vertex list
			OldNewMap[i] = NewVertices.size()-1;
		}
	}

	//update the vertex indices
	for (int i=0; i<(int)Facets.size(); i++){ //look through facets so we don't have to do exhaustive On2 search of all vertex combos
		for (int j=0; j<3; j++){ //look at all three combinations of vertices...
			int n = Facets[i].vi[j];
			int np = -1; while (np != n){ np = n; n = ConsolidateMap[n]; } //iterates NewMap to get the final value...

			Facets[i].vi[j] = OldNewMap[n];
		}
		if (!(Facets[i].vi[0] == Facets[i].vi[1] || Facets[i].vi[0] == Facets[i].vi[2] || Facets[i].vi[2] == Facets[i].vi[1])) //if there aren't any the same...
			NewFacets.push_back(Facets[i]);
	}

	Facets = NewFacets;
	Vertices = NewVertices;

	delete [] NumVertHere;
	NumVertHere = NULL;
	delete [] ConsolidateMap;
	ConsolidateMap = NULL;
	delete [] OldNewMap;
	OldNewMap = NULL;

	CalcVertNormals(); //re-calculate normals!
	MeshChanged();
	
}


void CMesh::RemoveDupLines(void)
{
	//first order lines so lower index is first:
	int tmpHold;
	for (int i=0; i<(int)Lines.size(); i++){
		if(Lines[i].vi[0] > Lines[i].vi[1]){
			tmpHold = Lines[i].vi[0];
			Lines[i].vi[0] = Lines[i].vi[1];
			Lines[i].vi[1] = tmpHold;
		}
	}

	//now sort them...
	std::sort(Lines.begin(), Lines.end());

	//iterate up, checking for duplicates and removing...
	for (int i=1; i<(int)Lines.size(); i++){ //size changes, but that's ok!
		if (Lines[i] == Lines[i-1]){
			Lines.erase(Lines.begin()+i);
			i--;
		}
	}

}


void CMesh::MeshChanged(void) //invalidates all cached voxelizing info!
{
	_TriLayerZ = -1;
	TriLayer.clear();
	_TriLineY = -1;
	TriLine.clear();
}

void CMesh::FillTriLayer(vfloat z) //fills in TriHeight with all triangles that bridge this plane
{
	if(z == _TriLayerZ) return; //if we've already done this...
	_TriLayerZ = z;

	TriLayer.clear(); //clear previous list

	//add any Facets whose Z coordinates are not all above or all below this Z plane
	bool IsAbove, IsBelow;
	std::vector<CFacet>::iterator FIter;
	int m=0;
	for (FIter = Facets.begin(); FIter != Facets.end(); FIter++){
		IsAbove = true; IsBelow = true;

		for (int n=0; n<3; n++){
			if(Vertices[FIter->vi[n]].v.z > z) IsBelow = false;
			if(Vertices[FIter->vi[n]].v.z < z) IsAbove = false;
		}
		if (!IsAbove && !IsBelow) TriLayer.push_back(m); //if this facet is not fully above or fully below our ZPlane
		
		m++;
	}
}

void CMesh::FillTriLine(vfloat y, vfloat z) //fills in TriHeight with all triangles that bridge this plane
{
	if(y == _TriLineY && z == _TriLayerZ) return; //if we've already done this...
	FillTriLayer(z); //exits immediately if z is the same as cached...
	_TriLineY = y;

	TriLine.clear(); //clear previous list

	Vec3D<> p;
	vfloat pu, pv, V1y, V2y, V3y;

	//add any Facets whose Y coordinates are not all above or all below this y plane (from within those in the Z plane)
//	bool IsAbove, IsBelow;
	std::vector<int>::iterator ZFIter;
	for (ZFIter = TriLayer.begin(); ZFIter != TriLayer.end(); ZFIter++){
		V1y = Vertices[Facets[*ZFIter].vi[0]].v.y;
		V2y = Vertices[Facets[*ZFIter].vi[1]].v.y;
		V3y = Vertices[Facets[*ZFIter].vi[2]].v.y;
		//trivial checks (should get most of them...)
		if (V1y < y && V2y < y && V3y < y)
			continue;
		if (V1y > y && V2y > y && V3y > y)
			continue;

		if (abs(z-.0055)<0.000001 && abs(y-.0225)<0.000001)
			int tmpbreak = 0;

		if(IntersectXRay(&Facets[*ZFIter], y, z, p, pu, pv)) { //if it intersects
			if (InsideTri(p, Vertices[Facets[*ZFIter].vi[0]].v, Vertices[Facets[*ZFIter].vi[1]].v, Vertices[Facets[*ZFIter].vi[2]].v)){
				TriLine.push_back(p.x); 
			}
		}


//		IsAbove = true; IsBelow = true;

//		for (int n=0; n<3; n++){
//			if(Vertices[Facets[*ZFIter].vi[n]].v.y > y) IsBelow = false;
//			if(Vertices[Facets[*ZFIter].vi[n]].v.y < y) IsAbove = false;
//		}
//		if (!IsAbove && !IsBelow) TriLine.push_back(*ZFIter); //if this facet is not fully above or fully below our ZPlane
		
	}

	std::sort(TriLine.begin(), TriLine.end());

}

