/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifdef USE_ZLIB_COMPRESSION
//#pragma comment(lib, "Utils/zlib.lib")
#pragma comment(lib, "zlib.lib")
#include "Utils/zlib.h"
#endif

#include "VX_Object.h"
#include <sstream>
#include <iomanip>
#include <cstdlib> //for rand(), srand()
#include <climits>
#include <stdlib.h>  // for atof

#ifdef USE_OPEN_GL
#ifdef QT_GUI_LIB
#include <qgl.h>
#else
#include "OpenGLInclude.h" //If not using QT's openGL system, make a header file "OpenGLInclude.h" that includes openGL library functions 
#endif
#include "Utils/GL_Utils.h"
#endif


CVX_Object::CVX_Object(void)
{
	ClearPalette();
}

CVX_Object::~CVX_Object(void)
{
	ClearMatter();
}

CVX_Object& CVX_Object::operator=(const CVX_Object& RefObj)
{
	Lattice = RefObj.Lattice;
	Voxel = RefObj.Voxel; 
	Structure = RefObj.Structure;
	Palette = RefObj.Palette;

	return *this;
}

void CVX_Object::ClearMatter(void) //clears out everything...
{
	Lattice.ClearLattice();
	Voxel.ClearVoxel();
	Structure.ClearStructure();
	ClearPalette();
}

/*!
@param[in] pLattice Pointer to a CVXC_Lattice object with the desired lattice parameters for the initialized voxel object
@param[in] xV The number of voxels in the X dimension of the workspace.
@param[in] yV The number of voxels in the Y dimension of the workspace.
@param[in] zV The number of voxels in the Z dimension of the workspace.
*/
void CVX_Object::InitializeMatter(CVXC_Lattice* pLattice, int xV, int yV, int zV)
{
	ClearMatter();
	
	LoadDefaultPalette();
	Lattice = *pLattice;
	Voxel.SetVoxName(VS_BOX);
	Structure.CreateStructure(xV, yV, zV);


}

/*! If "default.vxc" exists in the current directory this voxel object (including material palette) is loaded. 
If the file is not found, arbitrary defaults are used.
*/
void CVX_Object::InitializeMatter(void) //loads default 
{
	if(!LoadVXCFile("Default.vxc")){ //try loading default file (must be in working directory)... if not default to 10x 1mm cubes

		ClearMatter();
		AddMat("Flexible");
		AddMat("Stiff");
		AddMat("Light");
		Palette[1].SetColor(1, 0, 0);
		Palette[1].SetElasticMod(1000000.0);
		Palette[1].SetDensity(1000000.0);

		Palette[2].SetColor(0, 0, 1);
		Palette[2].SetElasticMod(100000000.0);
		Palette[2].SetDensity(1000000.0);

		Palette[3].SetColor(1, 1, 0);
		Palette[3].SetElasticMod(10000000.0);
		Palette[3].SetDensity(100000.0);

		Lattice.SetLattice(0.001);

		Voxel.SetVoxName(VS_BOX); 
		Structure.CreateStructure(10, 10, 10);
	}
}

/*! A cubic lattice is assumed at the provided inter-voxel lattice dimension.
@param[in] iLattice_Dim The base lattice dimension between adjacent voxels in meters.
@param[in] xV The number of voxels in the X dimension of the workspace.
@param[in] yV The number of voxels in the Y dimension of the workspace.
@param[in] zV The number of voxels in the Z dimension of the workspace.
*/
void CVX_Object::InitializeMatter(vfloat iLattice_Dim, int xV, int yV, int zV) //intializes object with some default params, including dynamic arrays
{
	ClearMatter();
	
	LoadDefaultPalette();
	Lattice.SetLattice(iLattice_Dim);
	Voxel.SetVoxName(VS_BOX);
	Structure.CreateStructure(xV, yV, zV);
}

/*! Attempts to load "Default.vxp" in the current directory to extract the palette. If the file is not found, a singel default material is added.
*/
void CVX_Object::LoadDefaultPalette(void)
{
	if (!LoadVXPFile("Default.vxp")){ //try to load the default palette from default VXC object
		AddMat("Default"); //otherwise, add a single, default material
	}
}


/*! If the associated structure of this voxel object has not been cleared, there will be material indices left in the structure that do not correspond to existing materials. This situation is to be avoided, as other function calls may produce errors in this situation.
*/
void CVX_Object::ClearPalette()
{
	int VecSize = (int)Palette.size();
	if (VecSize != 0)
		Palette.erase(Palette.begin(), Palette.begin()+VecSize); //erase the materials array

	//add back the default (empty) first material
	CVXC_Material Empty = CVXC_Material("Erase", NULL, NULL, NULL, NULL);

	Palette.push_back(Empty);
}

/*!
*/
bool CVX_Object::ReplaceMaterial(int IndexToReplace, int NewIndex, bool DeleteReplaced, std::string* RetMessage)
{
	if (IndexToReplace == NewIndex) return true; //if replacing with the same one...
	if (IndexToReplace == 0 && DeleteReplaced) DeleteReplaced = false; //never delete the null material
	if (IndexToReplace < 0 || IndexToReplace >= (int)Palette.size()){ if (RetMessage) *RetMessage += "Invalid material to replace\n"; return false;}
	if (NewIndex<0 || NewIndex >= (int)Palette.size()){ if (RetMessage) *RetMessage += "Invalid material to replace to\n";return false;}

	Structure.ReplaceMaterial(IndexToReplace, NewIndex, DeleteReplaced); //erase and shift from our main strcuture
	for (int i=0; i<GetNumMaterials(); i++){ //handle the internal materials, too.
		if (Palette[i].GetMatType() == INTERNAL)
			Palette[i].GetPStructure()->ReplaceMaterial(IndexToReplace, NewIndex, DeleteReplaced);
		else if (Palette[i].GetMatType() == DITHER){
			if (Palette[i].GetRandInd1() == IndexToReplace) Palette[i].SetRandInd1(0);
			if (DeleteReplaced && Palette[i].GetRandInd1() > IndexToReplace) Palette[i].SetRandInd1(Palette[i].GetRandInd1()-1); //decrement any indicae above
			if (Palette[i].GetRandInd2() == IndexToReplace) Palette[i].SetRandInd2(0);
			if (DeleteReplaced && Palette[i].GetRandInd2() > IndexToReplace) Palette[i].SetRandInd2(Palette[i].GetRandInd2()-1); //decrement any indicae above
		}
	}
	if (DeleteReplaced) Palette.erase(Palette.begin()+IndexToReplace);
	return true;
}

bool CVX_Object::FlattenMaterial(int IndexToFlatten, std::string* RetMessage) //converts a compound material into its subsidiary materials
{
	if (IndexToFlatten < 0 || IndexToFlatten >= (int)Palette.size() || Palette[IndexToFlatten].GetMatType() == SINGLE){ if (RetMessage) *RetMessage += "Cannot Flatten a non-compound material\n"; return false;}
	
	int LastMatIndex, NextMatIndex;
	for (int i=0; i<Structure.GetArraySize(); i++){
		LastMatIndex = Structure.GetData(i);
		for (int m=0; m<100; m++){ //anything more than 100 iterations is ridiculous
			if (LastMatIndex == IndexToFlatten) continue; 
			NextMatIndex = GetSubMatIndex(i, LastMatIndex);
			if (LastMatIndex == NextMatIndex) continue; //if reached the end of the iterations
			LastMatIndex = NextMatIndex;
		}

		if (LastMatIndex == IndexToFlatten) Structure.SetData(i, GetSubMatIndex(i, LastMatIndex));
	}
	return true;
}

bool CVX_Object::DeleteMat(int Index, bool FlattenFirst, std::string* RetMessage) //deletes a material (in palette and object) and moves the rest of the material indexes down
{
	if (Index == 0){ //check if we're trying to delete the null material
		if (RetMessage) *RetMessage += "Cannot delete Null material\n";
		return false;
	}
	if (FlattenFirst) FlattenMaterial(Index, RetMessage);
	return ReplaceMaterial(Index, 0, true, RetMessage);
}

/*! Returns the index within the palette that this material was created at. This index may change if materials are deleted from the palette. All colors and physical properties are copied from MatToAdd.
@param[in] MatToAdd Pre-existing material to add to the palette.
@param[in] ForceBasic If true, the material being added will be forced into a basic (non composite) material.
@param[out] RetMessage Pointer to an initialized string. Messages generated in this function will be appended to the string.
*/
int CVX_Object::AddMat(CVXC_Material& MatToAdd, bool ForceBasic, std::string* RetMessage)
{
	CVXC_Material MatCopy = MatToAdd;
	//checks
	if (MatCopy.GetName() == "" || MatCopy.GetName() == " "){
		if (RetMessage) *RetMessage += "Invalid material Name\n";
		return -1;
	}

	std::string NewName = MatCopy.GetName(); //append an index if the name already exists
	int index = 1;
	while (MatNameExists(NewName)){ //if the name already is in use
		std::stringstream Out;
		Out << MatCopy.GetName() << index++;
		NewName = Out.str();
	}
	MatCopy.SetName(NewName);

	if (ForceBasic) MatCopy.SetMatType(SINGLE); //if we want to avoid any recursion headaches, force to a local single material

	int NextIndex = (int)Palette.size(); //not plus one!
	if (NextIndex<=256){ //if we have a material slot left...
		Palette.push_back(MatCopy);
		return NextIndex;
	}
	else {
		if (RetMessage) *RetMessage += "Maximum number of materials reached!";
		return -1;
	}
}

/*! Returns the index within the palette that this material was created at. This index may change if materials are deleted from the palette. All colors and physical properties besides Elastic modulus and poisonn's ratio are set to defaults.
@param[in] Name The name of the material to add. If the name is already in use a variant will be generated.
@param[in] EMod The elatic modulus of the new material in Pascals.
@param[in] PRatio The poissons ratio of the new material.
@param[out] RetMessage Pointer to an initialized string. Messages generated in this function will be appended to the string.
*/
int CVX_Object::AddMat(std::string Name, vfloat EMod, vfloat PRatio, std::string* RetMessage) //Adds a new material to end of the list:
{
	CVXC_Material tmp;
	tmp.SetName(Name);
	tmp.SetColor(0.5f, 0.5f, 0.5f);
	tmp.SetElasticMod(EMod);
	tmp.SetPoissonsRatio(PRatio);
	tmp.SetDensity(1.0);
	tmp.SetuStatic(1.0);
	tmp.SetuDynamic(0.5);
	tmp.SetMatTempPhase(0.0);
	return AddMat(tmp, true, RetMessage);
}


bool CVX_Object::MatNameExists(std::string Name)
{
	for (int i=0; i<(int)Palette.size(); i++){
		if (Name == Palette[i].GetName()) return true;
	}
	return false;
}

/*! Creates or overwrites the provided file/path with all voxel object data in XML format. The file extension should be *.vxc unless OnlyPal is true, in which case the extension should be *.vxp.
@param[in] filename An absolute or relative file name. Overwrites any existing file.
@param[in] Compression A direct integer cast of the CompType enumeration to define if or how the raw voxel data should be compressed.
*/
void CVX_Object::SaveVXCFile(std::string filename, int Compression)
{
	CXML_Rip XML;
	WriteXML(&XML, Compression);
	XML.SaveFile(filename);
}

/*! Opens the provided file/path and attempts to parse it as XML. Returns true if any usable information was found. The provided file extension should be *.vxc.
@param[in] filename An absolute or relative file name.
@param[in] OnlyPal A boolean flag indication whether to expect an entire voxel object [false] or just the material palette [true].
*/
bool CVX_Object::LoadVXCFile(std::string filename)
{
	CXML_Rip XML;
	if (!XML.LoadFile(filename)) return false;
//	if (filename.find(".vxp") != std::string::npos || filename.find(".pal") != std::string::npos)	ReadPaletteXML(&XML); //if this is a VXC file
//	else 
	ReadXML(&XML); //if thie is a VXP file.
	return true;

}

/*! Creates or overwrites the provided file/path with all material palette data in XML format. The file extension should be *.vxp.
@param[in] filename An absolute or relative file name. Overwrites any existing file.
*/
void CVX_Object::SaveVXPFile(std::string filename)
{
	CXML_Rip XML;
	WritePaletteXML(&XML);
	XML.SaveFile(filename);
}

/*! Opens the provided file/path and attempts to parse it as XML. Returns true if any usable information was found. The provided file extension should be *.vxp.
@param[in] filename An absolute or relative file name.
*/
bool CVX_Object::LoadVXPFile(std::string filename)
{
	CXML_Rip XML;
	if (!XML.LoadFile(filename)) return false;
	ReadPaletteXML(&XML);
	return true;
}

/*! Creates or overwrites the provided file/path with an export into KV6 format (for use in Slab6 or Ace of Spades. The file extension should be *.kv6.
@param[in] filename An absolute or relative file name. Overwrites any existing file.
*/
bool CVX_Object::ExportKV6File(std::string filename)
{
	int i;
	unsigned short u;

	int Xs = GetVXDim(); int Ys = GetVYDim(); int Zs = GetVZDim();
	if (Xs > 255 || Ys > 255 || Zs > 255) return false; //MAX SIZE!

	FILE *fil;
	if (!(fil = fopen(filename.c_str(),"wb"))) return false;

	//header
	i = 0x6c78764b; fwrite(&i,4,1,fil); //Kvxl


	fwrite(&Xs,4,1,fil); fwrite(&Ys,4,1,fil); fwrite(&Zs,4,1,fil); //total sizes of the worksapce (ints)
	float Num=0.0f;
	fwrite(&Num,4,1,fil); fwrite(&Num,4,1,fil); fwrite(&Num,4,1,fil); //pivot point (floats)
	int NumVox = GetNumVox();
	fwrite(&NumVox,4,1,fil); //total number of voxels

	//voxel info
	std::vector<int> XLen; //size XSize 
	std::vector<unsigned short> XYLen; //size //XSize*YSize
	XLen.resize(Xs);
	XYLen.resize(Xs*Ys);

	for (int iX=0; iX<Xs; iX++){
		int NumInThisX = 0;
		for (int iY=0; iY<Ys; iY++){
			int NumInThisXY = 0;
			for (int iZ=Zs-1; iZ>=0; iZ--){ //Z goes top to bottom
//				int ThisMat = GetMat(iX, iY, iZ);
				CVXC_Material* pMat = GetLeafMat(GetIndex(iX, iY, iZ)); //this material (gotta look deep in case there's a composite that references erase 
				if (pMat->GetName() != "Erase"){ //if there's material here
					//color!
					int R, G, B;
					pMat->GetColori(&R, &G, &B);
					unsigned char uR = (unsigned char)R;
					unsigned char uG = (unsigned char)G;
					unsigned char uB = (unsigned char)B;

					fputc(uB,fil); //blue
					fputc(uG,fil); //green
					fputc(uR,fil); //red
					fputc(128,fil); //Gamma

					//other info
					unsigned char ThisZ = (unsigned char)Zs-1-iZ; //0 is top
					fputc(ThisZ,fil); fputc(0,fil); //the z level of this voxel  , zero filler
					
					unsigned char Vis = 0;
					int nxInd = GetIndex(iX-1, iY, iZ); if (nxInd == -1 || GetMat(nxInd) == 0) Vis |= 1;
					int pxInd = GetIndex(iX+1, iY, iZ); if (pxInd == -1 || GetMat(pxInd) == 0) Vis |= 2;
					int nyInd = GetIndex(iX, iY-1, iZ); if (nyInd == -1 || GetMat(nyInd) == 0) Vis |= 4;
					int pyInd = GetIndex(iX, iY+1, iZ); if (pyInd == -1 || GetMat(pyInd) == 0) Vis |= 8;
					int pzInd = GetIndex(iX, iY, iZ+1); if (pzInd == -1 || GetMat(pzInd) == 0) Vis |= 16;
					int nzInd = GetIndex(iX, iY, iZ-1); if (nzInd == -1 || GetMat(nzInd) == 0) Vis |= 32;

				
					fputc(Vis,fil); fputc(255,fil); //vis and dir fields. Not sure exactly...

					NumInThisX++;
					NumInThisXY++;
				}
			}
			XYLen[iX*Ys + iY] = (short)NumInThisXY;
		}
		XLen[iX] = NumInThisX;
	}

	for (int it=0; it<Xs; it++){i = XLen[it]; fwrite(&i,4,1,fil);}
	for (int it=0; it<Xs*Ys; it++){u = XYLen[it]; fwrite(&u,2,1,fil);}

	fclose(fil);

	return true;
}

/*! Creates a VXC tag at the current level of the XML rip tree and populates everything within it.
@param[in] pXML Pointer to an initialized XML ripping tree.
@param[in] Compression A direct integer cast of the CompType enumeration to define if or how the raw voxel data should be compressed.
@param[out] RetMessage Optional pointer to an initialized string. Information generated in this function will be appended to this string.
*/
void CVX_Object::WriteXML(CXML_Rip* pXML, int Compression, std::string* RetMessage)
{
	pXML->DownLevel("VXC");
	pXML->SetElAttribute("Version", "0.94");
		Lattice.WriteXML(pXML);
		Voxel.WriteXML(pXML);
		WritePaletteXML(pXML, Compression);
		Structure.WriteXML(pXML, Compression, RetMessage);

	pXML->UpLevel();
}

/*! Creates a Palette tag at the current level of the XML rip tree and populates everything within it.
@param[in] pXML Pointer to an initialized XML ripping stream.
@param[in] Compression A direct integer cast of the CompType enumeration to define if or how the raw voxel data should be compressed.
*/
void CVX_Object::WritePaletteXML(CXML_Rip* pXML, int Compression)
{
	pXML->DownLevel("Palette");
	for (int i=1; i<(int)Palette.size(); i++){ //Can skip the "empty" material (0)
		pXML->DownLevel("Material");
		pXML->SetElAttribute("ID", i);
		Palette[i].WriteXML(pXML, Compression);
		pXML->UpLevel();
	}
	pXML->UpLevel();
}

/*! The current level of the XML rip tree should be pointing to a "VXC" (or deprecated "DMF") tag when this function is called (Or if OnlyPal is true, the current tag in the XML tree should be a Palette tag.
@param[in] pXML Pointer to an initialized and populated XML ripping tree.
@param[in] OnlyPal A boolean flag indication whether to expect an entire voxel object [false] or just the material palette [true].
@param[out] RetMessage Optional pointer to an initialized string. Information generated in this function will be appended to this string.
*/
void CVX_Object::ReadXML(CXML_Rip* pXML, bool OnlyPal, std::string* RetMessage)
{
	ClearMatter();

	std::string ThisVersion = "0.94"; //version of the reading code...
	std::string Version;
	pXML->GetElAttribute("Version", &Version);
	if (atof(Version.c_str()) > atof(ThisVersion.c_str())) if (RetMessage) *RetMessage += "Attempting to open newer version of VXC file. Results may be unpredictable.\nUpgrade to newest version of VoxCAD.";

	if (!OnlyPal && pXML->FindElement("Lattice")){
		Lattice.ReadXML(pXML);
		pXML->UpLevel();
	}
	if (!OnlyPal && pXML->FindElement("Voxel")){
		Voxel.ReadXML(pXML);
		pXML->UpLevel();
	}
	if (pXML->FindElement("Palette")){
		ReadPaletteXML(pXML, Version);
		pXML->UpLevel(); //Material
	}
	if (!OnlyPal && pXML->FindElement("Structure")){
		Structure.ReadXML(pXML, Version.c_str(), RetMessage);
		pXML->UpLevel();
	}
}

/*! The current level of the XML rip tree should be pointing to a "Palette" tag when this function is called.
@param[in] pXML Pointer to an initialized and populated XML ripping tree.
@param[in] Version The internal file version number of the file being loaded.
*/
void CVX_Object::ReadPaletteXML(CXML_Rip* pXML, std::string Version)
{ 
	int PrevPalSize = GetNumMaterials(); //store how many materials were in there

	ClearPalette();
	CVXC_Material tmpMat;

	while (pXML->FindElement("Material")){
		tmpMat.ReadXML(pXML, Version);
		Palette.push_back(tmpMat);
	}

	if (GetNumMaterials() < PrevPalSize){ //add extra materials if the palette is too short! (only if needed!)
		int MaxMat = 0;
		for (int i=0; i<GetStArraySize(); i++){
			if (GetMat(i)>MaxMat) MaxMat = GetMat(i);
		}
		while (GetNumMaterials() <= MaxMat){
			AddMat("AutoGenerated");
		}
	}
}

/*!Accounts for dimension adjustmenst and line and layer offsets.
@param[out] Dim Vector to store the resulting dimensions in.
*/
void CVX_Object::GetWorkSpace(Vec3D<>* Dim) const //calculate actual workspace dimensions
{
	Dim->setX(GetLatticeDim()*Lattice.GetXDimAdj()*(Structure.GetVXDim() + Lattice.GetMaxOffsetX(GetVYDim(), GetVZDim()))); //distance between two furtherest points:
	Dim->setY(GetLatticeDim()*Lattice.GetYDimAdj()*(Structure.GetVYDim() + Lattice.GetMaxOffsetY(GetVXDim(), GetVZDim()))); //distance between two furtherest points:
	Dim->setZ(GetLatticeDim()*Lattice.GetZDimAdj()*Structure.GetVZDim()); //distance between two furtherest points:
}

/*!Accounts for dimension adjustmenst and line and layer offsets.
*/
Vec3D<> CVX_Object::GetWorkSpace() const //calculate actual workspace dimensions (acounting for dif offsets!)
{
	Vec3D<> ToReturn;
	GetWorkSpace(&ToReturn);
	return ToReturn;
}

/*! Returns true if a valid location was specified. Otherwise false. The resulting position is given in meters from the origin.
@param[out] Point The X, Y, and Z components of the voxel location is stored in this structure.
@param[in] index The global structural index to query.
@param[in] WithOff True if we want to compute the location with lattice offsets. False to jsut return the nominal position. This is faster and still correct for rectangular lattices)
*/
bool CVX_Object::GetXYZ(Vec3D<>* Point, int index, bool WithOff) const //gets the XYZ coordinates of an index in the master array, returns true if a valid location
{
	int iz = 0;
	int iy = 0;
	int ix = 0;
	GetXYZNom(&ix, &iy, &iz, index); //get nominal x, y, z indicies

	if (WithOff){
		vfloat Eps = (vfloat)0.000001; //hack, offset to make sure its all on the right side of zero
		vfloat TotalXOffP = (vfloat)0.0; //running variables to add the offsets to. (in percentage of width
		vfloat TotalYOffP = (vfloat)0.0;
		
		TotalXOffP = Lattice.GetXLiO()*iy + Lattice.GetXLaO()*iz + Eps; //total offset in x and y
		TotalYOffP = Lattice.GetYLiO()*ix + Lattice.GetYLaO()*iz + Eps;
		int NumBackX = ((int)(TotalXOffP/Lattice.GetXDimAdj()));
		int NumBackY = ((int)(TotalYOffP/Lattice.GetYDimAdj()));

		TotalXOffP = Lattice.GetXLiO()*(iy-NumBackY) + Lattice.GetXLaO()*iz + Eps; //Iterate twice to avoid offset jumps in Y that yield incorrect X offsets...
		TotalYOffP = Lattice.GetYLiO()*(ix-NumBackX) + Lattice.GetYLaO()*iz + Eps;
		NumBackX = ((int)(TotalXOffP/Lattice.GetXDimAdj()));
		NumBackY = ((int)(TotalYOffP/Lattice.GetYDimAdj()));

		TotalXOffP = TotalXOffP - NumBackX*Lattice.GetXDimAdj();
		TotalYOffP = TotalYOffP - NumBackY*Lattice.GetYDimAdj();

		Point->setX(Lattice.GetLatticeDim()*(Lattice.GetXDimAdj()*(0.5+ix)+TotalXOffP));
		Point->setY(Lattice.GetLatticeDim()*(Lattice.GetYDimAdj()*(0.5+iy)+TotalYOffP));
		Point->setZ(Lattice.GetLatticeDim()*Lattice.GetZDimAdj()*(0.5+iz));
	}
	else {
		*Point = GetLatDimEnv().Scale(Vec3D<>(ix+0.5, iy+0.5, iz+0.5));
	}
	return true;
	
}

/*! Only touching face-to-face is considered, not edge to edge or corner to corner. 
For physical simulation, a true result implies that there should be a structural connection between these two voxels.
Optionally provides the relative location (int meters) from voxel 1 to voxel 2.
@param[in] Index1 The global structural index of the first voxel to consider.
@param[in] Index2 The global structural index of the second voxel to consider.
@param[in] IncludeEmpty If set to false, the function returns false if either location has no material.
@param[out] RelativeLoc The relative location in meters between the first and second voxels.
*/
bool CVX_Object::IsAdjacent(int Index1, int Index2, bool IncludeEmpty, Vec3D<>* RelativeLoc) 
{
	vfloat LD = GetLatticeDim();
	Vec3D<> LD3 = GetLatDimEnv();
	if (Index1 < 0 || Index2 < 0 || Index1 >= Structure.GetArraySize() || Index2 >= Structure.GetArraySize()) return false;
	if (!IncludeEmpty && (Structure[Index1] == 0 || Structure[Index2] == 0)) return false;

	Vec3D<> P1, P2, Rel;

	GetXYZ(&P1, Index1);
	GetXYZ(&P2, Index2);

	Rel = P2-P1;
	if (RelativeLoc != NULL) //set the relativeness if wanted
		*RelativeLoc = Rel;

	if (Lattice.GetXLaO() == 0 && Lattice.GetXLiO() == 0 && Lattice.GetYLaO() == 0 && Lattice.GetYLiO() == 0){ //rectangular lattice...
		Vec3D<> aRel = Rel.Abs();
		if (aRel.y < LD3.y/2.0 && aRel.z < LD3.z/2.0 && aRel.x>LD3.x/2.0 && aRel.x<3.0*LD3.x/2.0) return true; //X Bond
		if (aRel.x < LD3.x/2.0 && aRel.z < LD3.z/2.0 && aRel.y>LD3.y/2.0 && aRel.y<3.0*LD3.y/2.0) return true; //Y Bond
		if (aRel.x < LD3.x/2.0 && aRel.y < LD3.y/2.0 && aRel.z>LD3.z/2.0 && aRel.z<3.0*LD3.z/2.0) return true; //Y Bond
		return false;
	}
	else if (Rel.Length()<1.4*LD) return true;//EEk. Still pretty hacky!
	else return false;

}

/*! All voxels that are translated outside of the workspace are discarded.
@param[in] Trans The desired translation in voxel integers. Each direction cast to integer, and the strucure moved by exactly that many voxels.
 */
void CVX_Object::Transform(Vec3D<> Trans) //shift the structure within the workspace (Caution! Trucates!)
{
	//create a copy of current object...
	char* pTmpVoxArray = NULL;
	pTmpVoxArray = new char[Structure.GetArraySize()];
	for (int i=0; i<Structure.GetArraySize(); i++){
		*(pTmpVoxArray+i) = Structure[i];
	}

	int tX = 0, tY = 0, tZ = 0;
	//overwrite with transformed values:
	for (int i=0; i<Structure.GetArraySize(); i++){
		GetXYZNom(&tX, &tY, &tZ, i);
		int NewIndex = GetIndex(tX-(int)Trans.x, tY-(int)Trans.y, tZ-(int)Trans.z);
		if(NewIndex != -1){ //if it was within bounds
			Structure[i] = *(pTmpVoxArray+NewIndex);
		}
		else {
			Structure[i] = 0;
		}
	}

	delete [] pTmpVoxArray;
	pTmpVoxArray = NULL;
}

int CVX_Object::GetNumVox(void)
{
	int NumVox = 0;
	for (int i=0; i<Structure.GetArraySize(); i++){ //for each voxel in the array
		if(Structure[i]!=0){ //if this is stuff we want to replace
			NumVox++;
		}
	}
	return NumVox;
}

int CVX_Object::GetNumVox(int MatIndex, bool OnlyLeafMat)
{
	int NumVox = 0;
	for (int i=0; i<Structure.GetArraySize(); i++){ //for each voxel in the array
		if(OnlyLeafMat && GetLeafMatIndex(i)== MatIndex) NumVox++;
		else if (!OnlyLeafMat && Structure[i] == MatIndex) NumVox++;
	}
	return NumVox;
}


vfloat CVX_Object::GetSurfaceArea()
{
	int XDim, YDim, ZDim;
	GetVDim(&XDim, &YDim, &ZDim);
	int NumXExposedFaces=0, NumYExposedFaces=0, NumZExposedFaces=0;

	for (int i=0; i<ZDim; i++){
		for (int j=0; j<YDim; j++){
			for (int k=0; k<XDim; k++){
				if (GetMat(k, j, i) > 0){ //if a valid location and non-void material
					if (GetMat(k+1, j, i) <= 0) NumXExposedFaces++;
					if (GetMat(k-1, j, i) <= 0) NumXExposedFaces++;
					if (GetMat(k, j+1, i) <= 0) NumYExposedFaces++;
					if (GetMat(k, j-1, i) <= 0) NumYExposedFaces++;
					if (GetMat(k, j, i+1) <= 0) NumZExposedFaces++;
					if (GetMat(k, j, i-1) <= 0) NumZExposedFaces++;
				}
			}
		}
	}
	
	Vec3D<> NominalSize = GetLatDimEnv();

	return NominalSize.y*NominalSize.z*NumXExposedFaces + NominalSize.x*NominalSize.z*NumYExposedFaces + NominalSize.x*NominalSize.y*NumZExposedFaces;
}

vfloat CVX_Object::GetVolume()
{
	Vec3D<> NominalSize = GetLatDimEnv();
	return GetNumVox()*NominalSize.x*NominalSize.y*NominalSize.z;
}

vfloat CVX_Object::GetWeight(void) //returns weight based on densities stored in material.
{
	vfloat SumWeight = 0;

	Vec3D<> NominalSize = GetLatDimEnv();
	vfloat VoxVol = NominalSize.x*NominalSize.y*NominalSize.z;

	for (int i=0; i<Structure.GetArraySize(); i++){ //for each voxel in the array
		if (Structure[i]!= 0)
			SumWeight += Palette[(int)Structure[i]].GetDensity()*VoxVol;
	}
	return SumWeight;
}

int CVX_Object::GetNumLeafMatInUse(void)
{
	int NumInUse = 0;
	for (int i=1; i<GetNumMaterials(); i++){
		if (GetNumVox(i, true) != 0) NumInUse++;
	}
	return NumInUse;
}


bool CVX_Object::IsInRecursivePath(int MatIndexStart, int MatIndexRef) //returns true if MatIndexRef is part of the recusive path of MatIndexStart
{
	if (MatIndexStart == MatIndexRef) return true; //trivial case (to catch stupid calls)
	if (MatIndexStart < 0 || MatIndexRef >= GetNumMaterials()) return false; //bad index called... but not recursive!

	switch (Palette[MatIndexStart].GetMatType()){
		case SINGLE:
			if (MatIndexStart == MatIndexRef) return true; //trivial case
			break;
		case INTERNAL: 
			for (int i=0; i<GetNumMaterials(); i++){ //check all materials in the palette to see if they're in here
				if (Palette[MatIndexStart].GetPStructure()->ContainsMatIndex(i)){ //if it contains this current (iterated) material
					bool FoundRec = IsInRecursivePath(i, MatIndexRef);
					if (FoundRec) return true; //if didn't find one, we have to keep checking...
				}
			}
			return false;
			break;
		case DITHER:
			if (IsInRecursivePath(Palette[MatIndexStart].GetRandInd1(), MatIndexRef) || IsInRecursivePath(Palette[MatIndexStart].GetRandInd2(), MatIndexRef)) return true;
			else return false;
			break;
	}
	return false;
}

/*!
@param[in] StructIndex The global voxel structure index to query. 
@param[out] pVisible Set to false if this material is currently to be hidden for visualization. Otherwise true.
*/
int CVX_Object::GetLeafMatIndex(int StructIndex, bool*pVisible) //get the final material to display
{
	if (pVisible) *pVisible = true; //assume its visible. Any sub-step along the way can set this to false and it will not be shown.
	int LastMatIndex = Structure.GetData(StructIndex);
	int NextMatIndex, CurXInd, CurYInd, CurZInd;
	GetXYZNom(&CurXInd, &CurYInd, &CurZInd, StructIndex);

	for (int i=0; i<100; i++){ //anything more than 100 iterations we'll decide is infinite recursion
		NextMatIndex = GetSubMatIndex(&CurXInd, &CurYInd, &CurZInd, LastMatIndex, pVisible);
		if (LastMatIndex == NextMatIndex) return LastMatIndex; //if they are the same, its a single material.
		LastMatIndex = NextMatIndex;
	}
	return 0; //didn't find a material
}

/*! Used for recursivly determining the leaf material.
@param[in] StructIndex The global voxel structure index to query. 
@param[in] MatIndex The current sub-material we are evaluating at this location.
@param[out] pVisible Set to false if this material is currently to be hidden for visualization. Otherwise true.
*/
int CVX_Object::GetSubMatIndex(int StructIndex, int MatIndex, bool* pVisible)
{
//	if (MatIndex <= 0 || MatIndex > (int)Palette.size()) return 0;
//	CVXC_Material* pMat = &Palette[MatIndex]; //for convenience, make a pointer to this material
//	if (pVisible && !pMat->IsVisible()) *pVisible = false; //whatever the sub-GetLeaf returns, if this material is hidden, we hide.

	int x, y, z;
	GetXYZNom(&x, &y, &z, StructIndex);
	return GetSubMatIndex(&x, &y, &z, MatIndex, pVisible);

//	switch (pMat->GetMatType()){
//		case SINGLE: return MatIndex; break;
//		case INTERNAL: 
//			if (pMat->HasLocalStructure()){
//				x = x - pMat->GetSubXOffset();
//				y = y - pMat->GetSubYOffset();
//				z = z - pMat->GetSubZOffset();
//
//				//rotation
//				int tmpInt;
//				if (pMat->GetSubRotateAmount() != RAM_0){
//					switch (pMat->GetSubRotateAxis()){
//					case RAX_X: switch (pMat->GetSubRotateAmount()){
//						case RAM_90: tmpInt=y; y=z; z=-tmpInt-1; break;
//						case RAM_180: y=-y-1; z=-z-1; break;
//						case RAM_270: tmpInt=y; y=-z-1; z=tmpInt; break;
//					} break;
//					case RAX_Y: switch (pMat->GetSubRotateAmount()){
//						case RAM_90: tmpInt=x; x=-z-1; z=tmpInt; break;
//						case RAM_180: x=-x-1; z=-z-1; break;
//						case RAM_270: tmpInt=x; x=z; z=-tmpInt-1; break;
//					} break;
//					case RAX_Z: switch (pMat->GetSubRotateAmount()){
//						case RAM_90: tmpInt=x; x=y; y=-tmpInt-1; break;
//						case RAM_180: x=-x-1; y=-y-1; break;
//						case RAM_270: tmpInt=x; x=-y-1; y=tmpInt; break;
//					} break;
//					}
//				}
//
//				//TODO: offset is post-rotation coordinate system?
//
//				//offset
//				//x = (x - pMat->GetSubXOffset()) % pMat->GetSubXSize();
//				//y = (y - pMat->GetSubYOffset()) % pMat->GetSubYSize();
//				//z = (z - pMat->GetSubZOffset()) % pMat->GetSubZSize();
//				x = x % pMat->GetSubXSize();
//				y = y % pMat->GetSubYSize();
//				z = z % pMat->GetSubZSize();
//
//				while (x<0) x += pMat->GetSubXSize();
//				while (y<0) y += pMat->GetSubYSize();
//				while (z<0) z += pMat->GetSubZSize();
//				
////				int LocalStructIndex = pMat->GetPStructure()->GetIndex(x, y, z);
//				int LocalStructIndex = pMat->GetPStructure()->GetIndex(x, y, z);
//				return pMat->GetPStructure()->GetData(LocalStructIndex);
//			}
//			else return MatIndex; //default to default material values... (this only happens if something is messed up!)
//			break;
//		case DITHER:
//			//get a unique index for this material to seed the random generator:
//			vfloat Rand = prsm((vfloat)x, (vfloat)y, (vfloat)z, pMat->GetRandInd1()+pMat->GetRandInd2());
//			if (Rand < pMat->GetRandPerc1()) return pMat->GetRandInd1();
//			else return pMat->GetRandInd2();
//			break;
//	}
//	return 0;
}

int CVX_Object::GetSubMatIndex(int* pXIndex, int* pYIndex, int* pZIndex, int MatIndex, bool* pVisible)
{
	if (MatIndex <= 0 || MatIndex > (int)Palette.size()) return 0;
	CVXC_Material* pMat = &Palette[MatIndex]; //for convenience, make a pointer to this material
	if (pVisible && !pMat->IsVisible()) *pVisible = false; //whatever the sub-GetLeaf returns, if this material is hidden, we hide.

	int x=*pXIndex, y=*pYIndex, z=*pZIndex;

	switch (pMat->GetMatType()){
		case SINGLE: return MatIndex; break;
		case INTERNAL: 
			if (pMat->HasLocalStructure()){

				//rotation
				int tmpInt;
				if (pMat->GetSubRotateAmount() != RAM_0){
					switch (pMat->GetSubRotateAxis()){
					case RAX_X: switch (pMat->GetSubRotateAmount()){
						case RAM_90: tmpInt=y; y=z; z=-tmpInt-1; break;
						case RAM_180: y=-y-1; z=-z-1; break;
						case RAM_270: tmpInt=y; y=-z-1; z=tmpInt; break;
					} break;
					case RAX_Y: switch (pMat->GetSubRotateAmount()){
						case RAM_90: tmpInt=x; x=-z-1; z=tmpInt; break;
						case RAM_180: x=-x-1; z=-z-1; break;
						case RAM_270: tmpInt=x; x=z; z=-tmpInt-1; break;
					} break;
					case RAX_Z: switch (pMat->GetSubRotateAmount()){
						case RAM_90: tmpInt=x; x=y; y=-tmpInt-1; break;
						case RAM_180: x=-x-1; y=-y-1; break;
						case RAM_270: tmpInt=x; x=-y-1; y=tmpInt; break;
					} break;
					}
				}
	
				//offset
				int fXs = pMat->GetSubXSize(), fYs = pMat->GetSubYSize(), fZs = pMat->GetSubZSize();
				x = (x - pMat->GetSubXOffset()) % fXs;
				y = (y - pMat->GetSubYOffset()) % fYs;
				z = (z - pMat->GetSubZOffset()) % fZs;

				while (x<0) x += fXs; //keep values positive
				while (y<0) y += fYs;
				while (z<0) z += fZs;

				
				*pXIndex = x; *pYIndex = y; *pZIndex = z;
				int LocalStructIndex = pMat->GetPStructure()->GetIndex(x, y, z);
				return pMat->GetPStructure()->GetData(LocalStructIndex);
			}
			else return MatIndex; //default to default material values... (this only happens if something is messed up!)
			break;
		case DITHER:
			//get a unique index for this material to seed the random generator:
			vfloat Rand = prsm((vfloat)x, (vfloat)y, (vfloat)z, pMat->GetRandInd1()+pMat->GetRandInd2());
			if (Rand < pMat->GetRandPerc1()) return pMat->GetRandInd1();
			else return pMat->GetRandInd2();
			break;
	}
	return 0;
}


/*! Returns true if succesful. Returns false if indices are outside of workspace or material index is not contained within current palette. 
@param[in] StructIndex Global voxel index of voxel location to set.  
@param[in] MatIndex Specifies the index within the material palette to set this voxel to.
*/
bool CVX_Object::SetMat(int StructIndex, int MatIndex) //sets the material index at this location based on the index (advanced users)
{
	if (MatIndex >=0 && MatIndex < GetNumMaterials() && StructIndex >=0 && StructIndex < Structure.GetArraySize()){
		Structure.SetData(StructIndex, MatIndex);
		return true; //successful
	}
	else return false; //error out
}

/*! Returns true if succesful. Returns false if material index is not contained within current palette. 
@param[in] MatIndex Specifies the index within the material palette to set this voxel to.
*/
bool CVX_Object::SetMatFill(int MatIndex)
{
	if (MatIndex >=0 && MatIndex < GetNumMaterials()){
		int NumInStructure = Structure.GetArraySize();
		for (int i=0; i<NumInStructure; i++){
			SetMat(i, MatIndex);
		}
		return true; //successful
	}
	else return false; //error out



}

/*! Returns -1 if an invalid structure index was provided.
@param[in] StructIndex Global voxel index of voxel location to get.  
*/
int CVX_Object::GetMat(int StructIndex) const //returns the material index (at a lattice location)
{
	if (StructIndex >= 0 && StructIndex < Structure.GetArraySize()) return Structure[StructIndex];
	else return -1;
}

#ifdef USE_OPEN_GL

void CVX_Object::DrawVoxel(Vec3D<>* Center, vfloat Lat_Dim) {
	Voxel.DrawVoxel(Center, Lat_Dim);
}

void CVX_Object::DrawSingleVoxel(int StructIndex) //draws voxel with the correct material color, etc.
{
	float r, g, b, a;
	Vec3D<> Center;
	CVXC_Material* pMaterial;

	//quickly exit if there's no voxel to draw!
	if (Structure[StructIndex] <= 0) return; //don't draw if nothing there
	bool IsVis;
	pMaterial = GetLeafMat(StructIndex, &IsVis);
	if (pMaterial == NULL || !IsVis) return;
	if (pMaterial->GetAlphai() == 0) return;

	//set current GL color
	glColor4f(pMaterial->GetRedf(), pMaterial->GetGreenf(), pMaterial->GetBluef(), pMaterial->GetAlphaf());


	glLoadName (StructIndex); //to enable picking
	GetXYZ(&Center, StructIndex); 
	Voxel.DrawVoxel(&Center, Lattice.GetLatticeDim()); //draw this voxel if we got this far!
}

void CVX_Object::Draw(int Selected, int XMin, int XMax, int YMin, int YMax, int ZMin, int ZMax) //a -1 in SectionAt denotes don't do hide any voxels in that plane
//Show Selected highlights current sleceted voxel, Section view uses the currently selected axis and direction to show section, FadeBehindSelection optionally fades out the layers (for 2D view)
{
	int x, y, z;

	for (int i = 0; i<Structure.GetArraySize(); i++) //go through all the voxels...
	{
		GetXYZNom(&x, &y, &z, i);
		if (x<XMin || x>XMax || y<YMin || y>YMax || z<ZMin || z>ZMax) continue; //exit if obscured in a section view!

		DrawSingleVoxel(i); //, i==Selected);
	}
}




#endif

void CVX_Object::GetVXCInfoStr(std::string* pString)
{
	if (pString){
		std::stringstream strm;
		Vec3D<> WS = GetWorkSpace();
		strm << "Total Voxels: " << GetNumVox() << "\n";
		for (int i=1; i<GetNumMaterials(); i++)
			strm << "  " << Palette[i].GetName().c_str() << ": " << GetNumVox(i) << "\n";

		strm << "Size: (" << std::setprecision(3) << WS.x*1000 << "mm," << WS.y*1000 << "mm," << WS.z*1000 << "mm)\n";
		strm << "Volume: " << GetVolume()*1e9 << "mm^3, Weight: " << GetWeight()*1000 << "g\n";
		strm << "Surface Area: " << GetSurfaceArea()*1e6 << "mm^2\n";
		*pString = strm.str();
	}


}

void CVX_Object::GetVoxInfoStr(int VoxIndex, std::string* pString)
{
	if (pString && VoxIndex>=0 && VoxIndex<Structure.GetArraySize()){
		std::stringstream strm;
		int x, y, z;
		Vec3D<> pt = GetXYZ(VoxIndex);
		GetXYZNom(&x, &y, &z, VoxIndex);

		strm << "Voxel: " << VoxIndex << "\n";
		strm << "Index: (" << x << "," << y << "," << z << ")\n";
		strm << "Location: (" << std::setprecision(3) << pt.x*1000 << "," << pt.y*1000 << "," << pt.z*1000 << ")mm\n";
		strm << "Top Material: " << GetBaseMatAt(VoxIndex)->GetName() << "\n";
		strm << "Leaf Material: " << GetLeafMat(VoxIndex)->GetName() << "\n";

		*pString = strm.str();
	}
}

////////////////////////////LATTICE///////////////////////////////////////
CVXC_Lattice::CVXC_Lattice(void)
{
	ClearLattice();
}

CVXC_Lattice::~CVXC_Lattice(void)
{
}

/*!
@param[in] RefLat Reference lattice to set members equal to.
*/
CVXC_Lattice& CVXC_Lattice::operator=(const CVXC_Lattice& RefLat)
{
	Lattice_Dim = RefLat.Lattice_Dim;
	X_Dim_Adj = RefLat.X_Dim_Adj;
	Y_Dim_Adj = RefLat.Y_Dim_Adj;
	Z_Dim_Adj = RefLat.Z_Dim_Adj;
	X_Line_Offset = RefLat.X_Line_Offset;
	Y_Line_Offset = RefLat.Y_Line_Offset;
	X_Layer_Offset = RefLat.X_Layer_Offset;
	Y_Layer_Offset = RefLat.Y_Layer_Offset;
	
	return *this;
}

/*!
	@param[in] LatDimIn The base lattice dimension in meters. 
	@param[in] XDAdjIn The lattice dimension X adjustment percentage. Range [0.0, 1.0].
	@param[in] YDAdjIn The lattice dimension Y adjustment percentage. Range [0.0, 1.0].
	@param[in] ZDAdjIn The lattice dimension Z adjustment percentage. Range [0.0, 1.0].
	@param[in] XLiOIn The lattice line X adjustment percentage. Range [0.0, 1.0].
	@param[in] YLiOIn The lattice line Y adjustment percentage. Range [0.0, 1.0].
	@param[in] XLaOIn The lattice layer X adjustment percentage. Range [0.0, 1.0].
	@param[in] YLaOIn The lattice layer Y adjustment percentage. Range [0.0, 1.0].
*/
void CVXC_Lattice::SetLattice(vfloat LatDimIn, vfloat XDAdjIn, vfloat YDAdjIn, vfloat ZDAdjIn, vfloat XLiOIn, vfloat YLiOIn, vfloat XLaOIn, vfloat YLaOIn)
{
	Lattice_Dim = LatDimIn; 
	X_Dim_Adj = XDAdjIn; 
	Y_Dim_Adj = YDAdjIn; 
	Z_Dim_Adj = ZDAdjIn; 
	X_Line_Offset = XLiOIn; 
	Y_Line_Offset = YLiOIn; 
	X_Layer_Offset = XLaOIn; 
	Y_Layer_Offset = YLaOIn; 
}; 

/*!	Lattice must be re-initialized to be used.
*/
void CVXC_Lattice::ClearLattice()
{
	Lattice_Dim = 0.0;
	X_Dim_Adj = 1.0;
	Y_Dim_Adj = 1.0;
	Z_Dim_Adj = 1.0;
	X_Line_Offset = 0.0;
	Y_Line_Offset = 0.0;
	X_Layer_Offset = 0.0;
	Y_Layer_Offset = 0.0;
}

/*! Read all information from a pre-initialized and populated XML tree. The current level of the stack should point to a "Lattice" element.
@param[in,out] pXML Initialized pointer to an XML ripping tree for information to be extracted from.
*/
void CVXC_Lattice::ReadXML(CXML_Rip* pXML)
{
	if (!pXML->FindLoadElement("Lattice_Dim", &Lattice_Dim)) Lattice_Dim = 0.001;
	if (!pXML->FindLoadElement("X_Dim_Adj", &X_Dim_Adj)) X_Dim_Adj = 1.0;
	if (!pXML->FindLoadElement("Y_Dim_Adj", &Y_Dim_Adj)) Y_Dim_Adj = 1.0;
	if (!pXML->FindLoadElement("Z_Dim_Adj", &Z_Dim_Adj)) Z_Dim_Adj = 1.0;
	if (!pXML->FindLoadElement("X_Line_Offset", &X_Line_Offset)) X_Line_Offset = 0.0;
	if (!pXML->FindLoadElement("Y_Line_Offset", &Y_Line_Offset)) Y_Line_Offset = 0.0;
	if (!pXML->FindLoadElement("X_Layer_Offset", &X_Layer_Offset)) X_Layer_Offset = 0.0;
	if (!pXML->FindLoadElement("Y_Layer_Offset", &Y_Layer_Offset)) Y_Layer_Offset = 0.0;
}

/*! Writes all information to a pre-initialized XML tree.
@param[in,out] pXML Initialized pointer to an XML ripping tree for information to be stored to.
*/
void CVXC_Lattice::WriteXML(CXML_Rip* pXML)
{
	pXML->DownLevel("Lattice");
		pXML->Element("Lattice_Dim", Lattice_Dim);
		pXML->Element("X_Dim_Adj", X_Dim_Adj);
		pXML->Element("Y_Dim_Adj", Y_Dim_Adj);
		pXML->Element("Z_Dim_Adj", Z_Dim_Adj);
		pXML->Element("X_Line_Offset", X_Line_Offset);
		pXML->Element("Y_Line_Offset", Y_Line_Offset);
		pXML->Element("X_Layer_Offset", X_Layer_Offset);
		pXML->Element("Y_Layer_Offset", Y_Layer_Offset);
	pXML->UpLevel();
}

/*!Returns as percentage of the base lattice dimension in the range [0.0, 1.0]. 
Used to deterime how much to pad bounding box. 
@param[in] yV Number of Y rows to consider. 
@param[in] zV Number of Z layers to consider. 
*/
vfloat CVXC_Lattice::GetMaxOffsetX(int yV, int zV) const
{
	vfloat XMaxOffset = 0;
	vfloat tmp;

	for (int i=0; i<yV; i++){
		for (int j=0; j<zV; j++){
			tmp = i*X_Line_Offset + j*X_Layer_Offset;
			if (tmp > XMaxOffset && tmp < X_Dim_Adj) XMaxOffset = tmp;
		}
	}
	return XMaxOffset;
}

/*!Returns as percentage of the base lattice dimension in the range [0.0, 1.0]. 
Used to deterime how much to pad bounding box. 
@param[in] xV Number of X rows to consider. 
@param[in] zV Number of Z layers to consider. 
*/
vfloat CVXC_Lattice::GetMaxOffsetY(int xV, int zV) const
{
	vfloat YMaxOffset = 0;
	vfloat tmp;

	for (int i=0; i<xV; i++){
		for (int j=0; j<zV; j++){
			tmp = i*Y_Line_Offset + j*Y_Layer_Offset;
			if (tmp > YMaxOffset && tmp < 1) YMaxOffset = tmp;
		}
	}
	return YMaxOffset;
}





//////////////////////////MATERIAL/////////////////////////////

CVXC_Material::CVXC_Material(void)
{
	pStructure = NULL;
	ClearMaterial();
}

CVXC_Material::CVXC_Material(std::string newName, float r, float g, float b, float a, vfloat EMod, vfloat Poiss) //allow us to create easily
{
	pStructure = NULL;
	ClearMaterial();

	MatType = SINGLE; //inferred from list of params!
	MatModel = MDL_LINEAR;

	Name = newName;
	SetColor(r, g, b, a);
	SetElasticMod(EMod);
	SetPoissonsRatio(Poiss);
}

CVXC_Material& CVXC_Material::operator=(const CVXC_Material& RefMat)
{
	Name = RefMat.Name;
	MatType = RefMat.MatType;
	MatModel = RefMat.MatModel;
	FailModel = RefMat.FailModel;

	Red = RefMat.Red;
	Green = RefMat.Green;
	Blue = RefMat.Blue;
	Alpha = RefMat.Alpha;

	Elastic_Mod = RefMat.Elastic_Mod;
	Plastic_Mod = RefMat.Plastic_Mod;
	Yield_Stress = RefMat.Yield_Stress;
	Fail_Stress = RefMat.Fail_Stress;
	Fail_Strain = RefMat.Fail_Strain;

	DStress = RefMat.DStress;
	DStrain = RefMat.DStrain;

	Density = RefMat.Density;
	Poissons_Ratio = RefMat.Poissons_Ratio;
	CTE = RefMat.CTE;
	CurMaterialTemp = RefMat.CurMaterialTemp;
	MaterialTempPhase = RefMat.MaterialTempPhase;
	uStatic = RefMat.uStatic;
	uDynamic = RefMat.uDynamic;

	X_Offset = RefMat.X_Offset;
	Y_Offset = RefMat.Y_Offset;
	Z_Offset = RefMat.Z_Offset;
	StructRotateAxis = RefMat.StructRotateAxis;
	StructRotateAmount = RefMat.StructRotateAmount;

	pStructure = NULL;
	if (RefMat.pStructure){
		AllocateIntStruct();
		*pStructure = *RefMat.pStructure;
	}

	RandIndex1 = RefMat.RandIndex1;
	RandIndex2 = RefMat.RandIndex2;
	PercIndex1 = RefMat.PercIndex1;

	Visible = RefMat.Visible;

	return *this;
}

void CVXC_Material::ClearMaterial(void)
{
	Name = "";
	MatType = SINGLE;
	MatModel = MDL_LINEAR;
	FailModel = FM_MAXSTRESS;
	Visible = true;

	Red = 0.5f;
	Green = 0.5f;
	Blue = 0.5f;
	Alpha = 1.0f;

	Elastic_Mod = 0.0;
	Plastic_Mod = 0.0;
	Yield_Stress = 0.0;
	Fail_Stress = 0.0;
	Fail_Strain = 0.0;
	DStress.clear();
	DStrain.clear();

	Density = 0.0;
	Poissons_Ratio = 0.0;
	CTE = 0.0;
	CurMaterialTemp = 25.0;
	MaterialTempPhase = 0;
	uStatic = 0.0;
	uDynamic = 0.0;

	DeleteIntStruct();
	X_Offset = 0;
	Y_Offset = 0;
	Z_Offset = 0;
	StructRotateAxis = RAX_X;
	StructRotateAmount = RAM_0;

	RandIndex1 = 0;
	RandIndex2 = 0;
	PercIndex1 = 0.5; //ranges from 0 to 1;
}

void CVXC_Material::SetMatType(int NewMatType)
{
	switch (NewMatType){
		case SINGLE: MatType = SINGLE; break; //don't reset other stuff, in case we switch back... (other stuff won't be saved)
		case INTERNAL: MatType = INTERNAL; if (!HasLocalStructure()) AllocateIntStruct(2, 2, 2); break;
		case DITHER: MatType = DITHER; break;
	}
}

void CVXC_Material::SetMatModel(int NewMatModel)
{
	switch (NewMatModel){
		case MDL_LINEAR: MatModel = MDL_LINEAR; break;
		case MDL_LINEAR_FAIL: MatModel = MDL_LINEAR_FAIL; break;
		case MDL_BILINEAR: MatModel = MDL_BILINEAR; break;
		case MDL_DATA: MatModel = MDL_DATA; break;
	}
	//do checks here for sensible values?
}

void CVXC_Material::SetFailModel(int NewFailModel)
{
	switch (NewFailModel){
		case FM_MAXSTRESS: FailModel = FM_MAXSTRESS; break;
		case FM_MAXSTRAIN: FailModel = FM_MAXSTRAIN; break;
	}
	//do checks here for sensible values?
}

#ifdef USE_OPEN_GL
void CVXC_Material::SetGLColor(void)
{
	glColor4d(Red, Green, Blue, Alpha);
}
#endif

void CVXC_Material::WriteXML(CXML_Rip* pXML, int Compression)
{
	pXML->Element("MatType", MatType);
	pXML->Element("Name", Name);
	pXML->DownLevel("Display");
		pXML->Element("Red", Red);
		pXML->Element("Green", Green);
		pXML->Element("Blue", Blue);
		pXML->Element("Alpha", Alpha);
	pXML->UpLevel();

	switch (MatType){
		case SINGLE:
			pXML->DownLevel("Mechanical");
				pXML->Element("MatModel", MatModel);
				if (MatModel == MDL_DATA){
					pXML->DownLevel("SSData");
						pXML->Element("NumDataPts", (int)DStrain.size());
						pXML->DownLevel("StrainData");
						for (int i=0; i<(int)DStrain.size(); i++) pXML->Element("Strain", DStrain[i]);
						pXML->UpLevel();

						pXML->DownLevel("StressData");
						for (int i=0; i<(int)DStress.size(); i++) pXML->Element("Stress", DStress[i]);
						pXML->UpLevel();
					pXML->UpLevel();
				}

				pXML->Element("Elastic_Mod", Elastic_Mod);
				pXML->Element("Plastic_Mod", Plastic_Mod);
				pXML->Element("Yield_Stress", Yield_Stress);
				pXML->Element("FailModel", FailModel);
				pXML->Element("Fail_Stress", Fail_Stress);
				pXML->Element("Fail_Strain", Fail_Strain);
				
				pXML->Element("Density", Density);
				pXML->Element("Poissons_Ratio", Poissons_Ratio);
				pXML->Element("CTE", CTE);
				pXML->Element("MaterialTempPhase", MaterialTempPhase);
				pXML->Element("uStatic", uStatic);
				pXML->Element("uDynamic", uDynamic);
			pXML->UpLevel();
			break;
		case DITHER:
			pXML->Element("RandIndex1", RandIndex1);
			pXML->Element("RandIndex2", RandIndex2);
			pXML->Element("PercIndex1", PercIndex1);
			break;
		case INTERNAL:
			pStructure->WriteXML(pXML, Compression);
			pXML->Element("X_Offset", X_Offset);
			pXML->Element("Y_Offset", Y_Offset);
			pXML->Element("Z_Offset", Z_Offset);
			pXML->Element("StructRotateAxis", StructRotateAxis);
			pXML->Element("StructRotateAmount", StructRotateAmount);

			break;
	}
}

void CVXC_Material::ReadXML(CXML_Rip* pXML, std::string Version, std::string* RetMessage)
{
	DStrain.clear();
	DStress.clear();

	if (!pXML->FindLoadElement("MatType", &MatType)){ //if there's not a MatType tag, we need some detective work...
		if (pXML->FindElement("Structure")) {pXML->UpLevel(); MatType = INTERNAL;}
		if (pXML->FindElement("RandIndex1")) {pXML->UpLevel(); MatType = DITHER;}
		else if (pXML->FindElement("VXC") || pXML->FindElement("DMF")) {pXML->UpLevel(); MatType = EXTERNAL;}
		else MatType = SINGLE;
	}
	if (!pXML->FindLoadElement("Name", &Name)) Name = "Default";
	if (pXML->FindElement("Display")){
		if (!pXML->FindLoadElement("Red", &Red)) Red = 0.5;
		if (!pXML->FindLoadElement("Green", &Green)) Green = 0.5;
		if (!pXML->FindLoadElement("Blue", &Blue)) Blue = 0.5;
		if (!pXML->FindLoadElement("Alpha", &Alpha)) Alpha = 1.0;
		pXML->UpLevel();
	}

	switch (MatType){
		case SINGLE:
			if (pXML->FindElement("Mechanical")){
				if (!pXML->FindLoadElement("MatModel", &MatModel)) MatModel = MDL_LINEAR;
				if (pXML->FindElement("SSData")){
					int TmpNumDataPts;
					vfloat TmpDouble;
					if (!pXML->FindLoadElement("NumDataPts", &TmpNumDataPts)) TmpNumDataPts = 0;
					if (pXML->FindElement("StrainData")){
						for (int i=0; i<TmpNumDataPts; i++){
							pXML->FindLoadElement("Strain", &TmpDouble, true);
							DStrain.push_back(TmpDouble);
						}
						pXML->UpLevel();
						pXML->UpLevel();
					}
					if (pXML->FindElement("StressData")){
						for (int i=0; i<TmpNumDataPts; i++){
							pXML->FindLoadElement("Stress", &TmpDouble, true);
							DStress.push_back(TmpDouble);
						}
						pXML->UpLevel();
						pXML->UpLevel();
					}
					pXML->UpLevel();

				}
				if (!pXML->FindLoadElement("Elastic_Mod", &Elastic_Mod)) Elastic_Mod = 0;
				if (!pXML->FindLoadElement("Plastic_Mod", &Plastic_Mod)) Plastic_Mod = 0;
				if (!pXML->FindLoadElement("Yield_Stress", &Yield_Stress)) Yield_Stress = 0;
				if (!pXML->FindLoadElement("Fail_Stress", &Fail_Stress)) Fail_Stress = 0;
				if (!pXML->FindLoadElement("Fail_Strain", &Fail_Strain)) Fail_Strain = 0;
				if (!pXML->FindLoadElement("Density", &Density)) Density = 0;
				if (!pXML->FindLoadElement("Poissons_Ratio", &Poissons_Ratio)) Poissons_Ratio = 0;
				if (!pXML->FindLoadElement("CTE", &CTE)) CTE = 0;
				if (!pXML->FindLoadElement("MaterialTempPhase", &MaterialTempPhase)) MaterialTempPhase = 0;
				if (!pXML->FindLoadElement("uStatic", &uStatic)) uStatic = 0;
				if (!pXML->FindLoadElement("uDynamic", &uDynamic)) uDynamic = 0;
				
				if (!pXML->FindLoadElement("FailModel", &FailModel)){ //smartly set failure mode if not specified specifically...
					if (Fail_Stress != 0) FailModel = FM_MAXSTRESS;
					else if (Fail_Strain != 0) FailModel = FM_MAXSTRAIN;
					else FailModel = FM_MAXSTRESS;
				}

				if (MatModel == MDL_DATA) ValidateSSData();


				pXML->UpLevel();
			}
			break;
		case INTERNAL:
			if (!pXML->FindLoadElement("Name", &Name)) Name = "";
			if (!pXML->FindLoadElement("X_Offset", &X_Offset)) X_Offset = 0;
			if (!pXML->FindLoadElement("Y_Offset", &Y_Offset)) Y_Offset = 0;
			if (!pXML->FindLoadElement("Z_Offset", &Z_Offset)) Z_Offset = 0;
			if (!pXML->FindLoadElement("StructRotateAxis", &StructRotateAxis)) StructRotateAxis = RAX_X;
			if (!pXML->FindLoadElement("StructRotateAmount", &StructRotateAmount)) StructRotateAmount = RAM_0;


			if (pXML->FindElement("Structure")){
				AllocateIntStruct();
				pStructure->ReadXML(pXML, Version);
				pXML->UpLevel();
			}
			break;
		case DITHER:
			if (!pXML->FindLoadElement("RandIndex1", &RandIndex1)) RandIndex1 = 0;
			if (!pXML->FindLoadElement("RandIndex2", &RandIndex2)) RandIndex2 = 0;
			if (!pXML->FindLoadElement("PercIndex1", &PercIndex1)) PercIndex1 = 0;
			break;
		case EXTERNAL:
			std::string Filename;
			if (!pXML->FindLoadElement("File", &Filename)) Filename = "";
			if (RetMessage) *RetMessage += "External VXC/DMF references are deprecated. Please manually import" + Filename + " to the palette.\n";
			break;
	}
}

vfloat CVXC_Material::GetModelStiffness(vfloat StrainIn) //returns the stiffness of this material at a given strain
{
	switch (MatModel){
	case MDL_LINEAR: return Elastic_Mod;
	case MDL_LINEAR_FAIL: return Elastic_Mod;
	case MDL_BILINEAR:
		if (StrainIn > Yield_Stress/Elastic_Mod) return Plastic_Mod; //if past the linear region...
		else return Elastic_Mod;
	case MDL_DATA:{
		int NumPts = (int)DStrain.size();

		if (StrainIn<0) return Elastic_Mod; //before data
		else if (StrainIn<Fail_Strain){ //in data
			for (int i=0; i<NumPts-1; i++){
				if (DStrain[i+1] > StrainIn){ //if the [i, i+1] segment containts our current strain...
					return (DStress[i+1] - DStress[i])/(DStrain[i+1] - DStrain[i]);
				}
			}
		}
		else { //after data
			if (DStress[NumPts-1]<DStress[NumPts-2]) return 0;  //if the slope was decreasing output the last value (so we don't go negative stiffness eventually)
			else return (DStress[NumPts-1] - DStress[NumPts-2])/(DStrain[NumPts-1] - DStrain[NumPts-2]);
		}
		}
	}

	return Elastic_Mod; //default
}

vfloat CVXC_Material::GetModelStress(const vfloat StrainIn, bool* const pIsPastYielded, bool* const pIsPastFail, float voxelEmod) const //returns the stiffness and stress of the current material model at the given strain. Also returns flags for if this is yielded and/or failed.
{

	float true_Elastic_Mod = (voxelEmod > 0) ? voxelEmod : Elastic_Mod; // we need to use the evolvedStiffness for the stress computation, not material's ElasticMod

	//assume linear case, neither yielded nor failed unless we set otherwise...
	*pIsPastYielded = false;
	*pIsPastFail = false;
	vfloat CalcStress = true_Elastic_Mod*StrainIn;

	switch (MatModel){
	case MDL_LINEAR:
		//we're all good!
		return CalcStress;
	case MDL_LINEAR_FAIL:
		if (FailModel == FM_MAXSTRESS && StrainIn > Fail_Stress/true_Elastic_Mod){ //if past failure, we've yielded and failed at once...
			*pIsPastYielded = true;
			*pIsPastFail = true;
		}
		else if (FailModel == FM_MAXSTRAIN && StrainIn > Fail_Strain){ //if past failure, we've yielded and failed at once...
			*pIsPastYielded = true;
			*pIsPastFail = true;
		}
		//still all linear...
		return CalcStress;
	case MDL_BILINEAR:
		if (StrainIn > Yield_Stress/true_Elastic_Mod){ //if past the linear region...
			*pIsPastYielded = true;
			CalcStress = Yield_Stress + Plastic_Mod*(StrainIn-Yield_Stress/true_Elastic_Mod);
			if (FailModel == FM_MAXSTRESS && StrainIn > Fail_Stress/true_Elastic_Mod) *pIsPastFail = true; //if past fail, we've failed...
			else if (FailModel == FM_MAXSTRAIN && StrainIn > Fail_Strain) *pIsPastFail = true; 
			return CalcStress;
		}

		break;
	case MDL_DATA: {
		int NumPts = (int)DStrain.size();
		//if (StrainIn < 0){	} //default linear elastic mod for compression (for now...)
		if (StrainIn < Fail_Strain){ //Fail Strain should always be the end of the data
			for (int i=0; i<NumPts-1; i++){
				if (DStrain[i+1] > StrainIn){ //if the [i, i+1] segment containts our current strain...
					vfloat Perc = (StrainIn-DStrain[i])/(DStrain[i+1]-DStrain[i]);
					CalcStress = DStress[i] + Perc*(DStress[i+1]-DStress[i]);
					if (CalcStress > Yield_Stress)
						*pIsPastYielded = true;

					break;
				}
			}
		}
		else { //if past the end of the data...
			*pIsPastYielded = true;
			*pIsPastFail = true;
			if (DStress[NumPts-1]<DStress[NumPts-2]){ //if the slope was decreasing output the last value (so we don't go negative stiffness eventually)
				CalcStress = DStress[NumPts-1];
			}
			else {
				vfloat Slope = (DStress[NumPts-1] - DStress[NumPts-2])/(DStrain[NumPts-1] - DStrain[NumPts-2]);
				CalcStress = DStress[NumPts-1] + Slope*(StrainIn-Fail_Strain);
			}
		}
		return CalcStress; 
		}
	}
	return CalcStress;
}

bool CVXC_Material::SetSSData(std::vector<vfloat>* pStrain, std::vector<vfloat>* pStress, std::string* RetMsg) //imports point data to use for tension sterss/strain curve...
{
	DStrain = *pStrain;
	DStress = *pStress;
	return ValidateSSData(RetMsg);
}

bool CVXC_Material::ValidateSSData(std::string* RetMsg) //Does all the checks in the imported stress/strain data to make sure its suitable for simulating
{
	if (DStrain.size() == 0 || DStress.size() == 0){ //zero length
		if (RetMsg) *RetMsg += "Error: Zero length stress or strain vector.";
		return false;
	}
	if (DStrain.size() == 1 || DStress.size() == 1){ //single point
		if (RetMsg) *RetMsg += "Error: Need more than one point in series!";
		return false;
	}

	if (DStrain.size() != DStress.size()){	//make sure the vectors are the same length!
		if (RetMsg) *RetMsg += "Error: Strain and stress vectors are unqual length!";
		return false;
	}
	int NumPts = DStrain.size();

	if (DStrain[0] != 0 || DStrain[0] != 0){ //make sure first point is at 0,0:
		if (RetMsg) *RetMsg += "Error: Initial point in series must be zero stress, zero strain.";
		return false;
	}

	for (int i=0; i<NumPts-1; i++){ //Ensure strain values only ever increase
		if (DStrain[i] >= DStrain[i+1]){
			if (RetMsg) *RetMsg += "Error: strain values must be in increasing order from zero without repeated values.";
			return false;
		}
	}

	Elastic_Mod = DStress[1]/DStrain[1]; //set the elastic modulus to the first segment. This value will be used for compressive stiffness.
	Plastic_Mod = Elastic_Mod; //Plastic_Mod is meaningless...
	Fail_Stress = 0;
	Fail_Strain = DStrain[NumPts-1]; //the last point in the series will be taken as failure...
	SetFailModel(FM_MAXSTRAIN); 
	
	//.2% (0.002) offset to find a good yield point...
	vfloat Mo = Elastic_Mod; //the offset line
	vfloat Bo = (vfloat)(-0.002*Elastic_Mod);
	vfloat Mt, Bt; //temporary... for line segments we're checking.
	vfloat XInt, PercBetPts;

	//assume yield stress is last stress value unless we find a better one...
	Yield_Stress = DStress[NumPts-1];
	for (int i=1; i<NumPts-1; i++){
		vfloat X1=DStrain[i];
		vfloat X2=DStrain[i+1];
		vfloat Y1=DStress[i];
		vfloat Y2=DStress[i+1];

		Mt = (Y2-Y1)/(X2-X1);
		Bt = Y1-Mt*X1;

		if (Mo!=Mt){ //if not parallel lines...
			XInt = (Bt-Bo)/(Mo-Mt);
			if (XInt>X1 && XInt<X2){ //if intersects at this segment...
				PercBetPts = (XInt-X1)/(X2-X1);
				Yield_Stress = Y1+PercBetPts*(Y2-Y1);
				break;
			}
		}
	}
	return true;
}



////////////////////////////////STRUCTURE////////////////////////////////

CVXC_Structure& CVXC_Structure::operator=(const CVXC_Structure& RefStruct)
{
	Compression = RefStruct.Compression;
	X_Voxels = RefStruct.X_Voxels;
	Y_Voxels = RefStruct.Y_Voxels;
	Z_Voxels = RefStruct.Z_Voxels;

	if (RefStruct.DataInit){ 
		IniData(RefStruct.GetArraySize());
		for (int i=0; i<RefStruct.GetArraySize(); i++)
			SetData(i, RefStruct.GetData(i));
	}

	return *this;
}

void CVXC_Structure::DeleteData(void) //sandbox the creation and destruction...
{
	DataInit = false;
	m_SizeOfArray = 0;
	if (pData != NULL){ //clear current array
		delete [] pData;
		pData = NULL;
	}
}

void CVXC_Structure::IniData(int Size)
{
	DeleteData();
	if (Size > 0){
		pData = new char[Size];
		m_SizeOfArray = Size;
		DataInit = true;
	}
}

bool CVXC_Structure::WriteXML(CXML_Rip* pXML, int Compression, std::string* RetMessage)
{
	std::string Compress;
	switch (Compression){
		case CP_RAWDATA: Compress = "RAW_DATA"; break;
		case CP_ASCIIREADABLE: Compress = "ASCII_READABLE"; break;
		case CP_QTZLIB: Compress = "QT_ZLIB"; break;
		case CP_BASE64: Compress = "BASE64"; break;
		case CP_ZLIB: Compress = "ZLIB"; break;

		default: Compress = "CP_ASCIIREADABLE"; break;
	}

	pXML->DownLevel("Structure");
	pXML->SetElAttribute("Compression", Compress);

	pXML->Element("X_Voxels", X_Voxels);
	pXML->Element("Y_Voxels", Y_Voxels);
	pXML->Element("Z_Voxels", Z_Voxels);

	pXML->DownLevel("Data");

	std::string RawData;
	std::string WriteData;

	for (int i=0; i<Z_Voxels; i++){
		RawData.clear();
		for (int j=0; j<X_Voxels*Y_Voxels; j++){
			RawData.push_back(GetData(i*X_Voxels*Y_Voxels + j));
		} //convert to QByteArray to use qcompress
		
		if (Compress == "ASCII_READABLE"){ for (int k=0; k < X_Voxels*Y_Voxels; k++){ WriteData.resize(X_Voxels*Y_Voxels); WriteData[k] = RawData[k]+48;}}
		else if (Compress == "ZLIB"){
			#ifdef USE_ZLIB_COMPRESSION
				unsigned long DataSize = 10+2*X_Voxels*Y_Voxels;
				std::string tmpData;
				tmpData.resize(DataSize);
				int ErrorReturn = compress((unsigned char*)tmpData.c_str(), &DataSize, (unsigned char*)RawData.c_str(), RawData.size());
				WriteData = ToBase64((unsigned char*)tmpData.c_str(), DataSize); //DataSize changed by compress to compressed size...
			#else
				if (RetMessage) *RetMessage += "ZLib is not enabled. #define USE_ZLIB_COMPRESSION to use.\n";
				return false;
			#endif
		}
		else if (Compress == "QT_ZLIB"){
			#ifdef QT_CORE_LIB
				QString DataInString(RawData.c_str());
				QByteArray QRawData = DataInString.toAscii();
				QByteArray QWriteData = qCompress(QRawData).toBase64();
				WriteData = QString(QWriteData).toStdString();
			#else
				if (RetMessage) *RetMessage += "QT ZLib version of VXC is not suported in this build. Please select another type.\n";
				return false;
			#endif
		}
		else if (Compress == "RAW_DATA"){if (RetMessage) *RetMessage += "Raw data version of VXC is deprecated. Please select another type."; }
		else {WriteData = ToBase64((unsigned char*)RawData.c_str(), RawData.length());} //Compress == BASE64 
		

		pXML->Element("Layer", WriteData, true);
	}
	pXML->UpLevel();
	pXML->UpLevel();
	return true;
}

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) 
{
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) 
    {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, char delim) 
{
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

bool CVXC_Structure::ReadXML(CXML_Rip* pXML, std::string Version, std::string* RetMessage)
{
	// Default values
	MaxAdaptationRate = 1.0;
	MAX_STIFFNESS_VARIATION_STEP = 0.0;
	growthModel = 0.0;	
	//MinElasticMod = 
	//MaxElasticMod = 

	std::string Compression;
	pXML->GetElAttribute("Compression", &Compression);

	if (!pXML->FindLoadElement("X_Voxels", &X_Voxels)) X_Voxels = 1;
	if (!pXML->FindLoadElement("Y_Voxels", &Y_Voxels)) Y_Voxels = 1;
	if (!pXML->FindLoadElement("Z_Voxels", &Z_Voxels)) Z_Voxels = 1;
	if (!pXML->FindLoadElement("numForwardModelSynapses", &numForwardModelSynapses)) numForwardModelSynapses = 0;
	if (!pXML->FindLoadElement("numControllerSynapses", &numControllerSynapses)) numControllerSynapses = 0;
	if (!pXML->FindLoadElement("numRegenerationModelSynapses", &numRegenerationModelSynapses)) numRegenerationModelSynapses = 0;

	// // nac: for neural net:
	// if (!pXML->FindLoadElement("NumNeurons", &NumNuerons)) NumNuerons = 0;

	CreateStructure(X_Voxels, Y_Voxels, Z_Voxels); //create the space for the structure

	if (Version == "" || Version == "0.93" || Version == "0.94"){ //newest version
		pXML->FindElement("Data");
		for (int i=0; i<Z_Voxels; i++)
		{
			std::string DataIn;
			std::string RawData;
			pXML->FindLoadElement("Layer", &RawData, true, true);
		
			//different compression types
			if (Compression == "QT_ZLIB") //DEPRECATED
			{
				#ifdef QT_CORE_LIB
					QString DataInString(RawData.c_str());
					QByteArray QRawData = DataInString.toAscii();
					QByteArray test = QByteArray::fromBase64(QRawData);
					QByteArray QDataIn = qUncompress(test); //if compressed using this scheme
					DataIn.resize(QDataIn.size());
					for (int k=0; k<(int)QDataIn.size(); k++)
						DataIn[k] = QDataIn[k];
				#else
					if (RetMessage) *RetMessage += "QT ZLib version of VXC is not suported in this build. Please convert or re-save file first.";
					return false;
				#endif
			}
			else if (Compression == "ZLIB")
			{
				#ifdef USE_ZLIB_COMPRESSION
					unsigned long DestLength = X_Voxels*Y_Voxels;
					DataIn.resize(DestLength);

					std::string tmpData = FromBase64(RawData);
					int ErrorReturn = uncompress((unsigned char*)DataIn.c_str(), &DestLength, (unsigned char*)tmpData.c_str(), (unsigned long)tmpData.size());
				#else
					if (RetMessage) *RetMessage += "ZLib version of VXC is not suported in this build. Please convert or re-save file first.";
					return false;
				#endif
			}
			else if (Compression == "ASCII_READABLE") 
			{
				DataIn.resize(RawData.size());
				for (int i=0; i<(int)RawData.size(); i++)
				{
					DataIn[i] = RawData[i]-48; //if compressed using this scheme
				}
			}
			else if (Compression == "RAW_DATA") //DEPRECATED!!!
			{	
				DataIn = RawData; 
			}
			else { //if Compression == BASE64
				DataIn = FromBase64(RawData); //otherwise uncompressed
			}

			if (DataIn.length() != X_Voxels*Y_Voxels){
				if (RetMessage) *RetMessage += "Voxel layer data not present or does not match expected size.";
				return false;
			}

			for(int k=0; k<X_Voxels*Y_Voxels; k++){
				//the object's internal representation at this stage is as a long array, starting at (x0,xy,z0), proceeding to (xn,y0,z0), next to (xn,yn,z0), and on to (xn,yn,zn)
				SetData(X_Voxels*Y_Voxels*i + k, DataIn[k]); //pDataIn[k];
			}

		}
		pXML->UpLevel(); //Layer
		pXML->UpLevel(); //Data

	}
	else if (Version == "0.92") { //legacy version
		std::string tmpData;
		pXML->FindLoadElement("Data", &tmpData);
		const char *pDataIn = tmpData.c_str(); //get pointer to characters

		int IteratorIn = 0; //iterator for input file
		int Iterator = 0; //iterator for output file

		for (int k=0; k<X_Voxels*Y_Voxels*Z_Voxels + 2*Z_Voxels; k++){ //specific to old system where each ascii line was a layer...
			if (pDataIn[IteratorIn] != 0x0A && pDataIn[IteratorIn] != 0x0D){ //ignore carraige returns and line feeds
				char tmp = pDataIn[IteratorIn];
				SetData(Iterator++, pDataIn[IteratorIn]);
			}
			IteratorIn++;
		}
	}

	// // nac: load neural net weights
	// if (pXML->FindElement("Weights")){ 
	// 	// std::cout << "found weights!" << std::endl;
	// 	InitSynpaseArray(pow(NumNuerons,2));
	// 	for (int i=0; i<NumNuerons; i++)
	// 	{
	// 		std::string DataIn;
	// 		std::string RawData;
	// 		// std::string thisValue;
	// 		pXML->FindLoadElement("Layer", &RawData, true, true);
		
	// 		std::vector<std::string> dataArray;
	// 		dataArray = split(RawData,',',dataArray);
	// 		// std::cout << "rawData, Layer " << i << ": " << RawData << std::endl;
	// 		// std::cout << "rawData, Layer " << i << ": " << dataArray[0] << std::endl;
	// 		// DataIn.resize(RawData.size());
	// 		// for (int k=0; k<(int)RawData.size(); k++)
	// 		// std::istringstream iss(RawData);
	// 		for (int k=0; k<NumNuerons; k++)
	// 		{
	// 			// std::getline(iss,thisValue,",");
	// 			// std::cout << thisValue << std::endl;
	// 			// DataIn[k] = RawData[k]-48; 
	// 			SetSynapseWeight(NumNuerons*i+k,atof(dataArray[k].c_str()));
	// 		}

	// 		// for(int j=0; j<NumNuerons; j++)
	// 		// {
	// 		// 	SetData(j, atof(DataIn[j]); 
	// 		// }

	// 	}
	// 	pXML->UpLevel(); //Layer
	// 	pXML->UpLevel(); //Weights

	// 	// for (int i=0; i<NumNuerons; i++)
	// 	// {
	// 	// 	for (int j=0; j<NumNuerons; j++)
	// 	// 	{
	// 	// 		std::cout << GetSynapseWeight(i*NumNuerons+j) << " ";
	// 	// 	}
	// 	// 	std::cout << std::endl;
	// 	// }

	// }

	// nac: load phase offset
	if (pXML->FindElement("ControllerSynapseWeights")){
		int voxCounter = 0;
		// std::cout << "here1" << std::endl;
		// InitSynapseWeightArray(X_Voxels*Y_Voxels*Z_Voxels,numSynapses); //nac: hard coded... for now
		for (int i=0; i<Z_Voxels; i++)
		{
			// std::cout << "here2" << std::endl;
			std::string DataIn;
			std::string RawData;
			pXML->FindLoadElement("Layer", &RawData, true, true);

			std::vector<std::string> dataArray;
			// dataArray = split(RawData,'|',dataArray);
			dataArray = split(RawData,',',dataArray);

			// for (int k=0; k<X_Voxels*Y_Voxels; k++)	{ std::cout << dataArray[k] << std::endl; } // debugging

			for (int k=0; k<X_Voxels*Y_Voxels; k++)
			{
				// std::cout << "k: " << k << ",layer: " << dataArray[k] << std::endl;
				// std::cout << "here3" << std::endl;
				if (pData[X_Voxels*Y_Voxels*i+k] > 0)
				{
					// std::cout << "here4" << std::endl;
					// std::vector<std::string> dataArraySynapse;
					// dataArray = split(dataArray[k],',',dataArraySynapse);
					for (int s=0; s<numControllerSynapses; s++)
					{
						// std::cout << "here5, vox: " << voxCounter << ", s: " << s << std::endl;
						// std::cout << "data: " <<  atof(dataArray[numSynapses*k+s].c_str()) << std::endl;
						SetControllerSynapseWeight(voxCounter,s,atof(dataArray[numControllerSynapses*k+s].c_str()));
				// 		std::cout << "here6" << std::endl;
					}
					voxCounter++;
				}
			}
		}
		pXML->UpLevel(); //Layer
		pXML->UpLevel(); //Weights
	}

	if (pXML->FindElement("ForwardModelSynapseWeights")){
		int voxCounter = 0;
		// std::cout << "here1" << std::endl;
		// InitSynapseWeightArray(X_Voxels*Y_Voxels*Z_Voxels,numSynapses); //nac: hard coded... for now
		for (int i=0; i<Z_Voxels; i++)
		{
			// std::cout << "here2" << std::endl;
			std::string DataIn;
			std::string RawData;
			pXML->FindLoadElement("Layer", &RawData, true, true);

			std::vector<std::string> dataArray;
			// dataArray = split(RawData,'|',dataArray);
			dataArray = split(RawData,',',dataArray);

			// for (int k=0; k<X_Voxels*Y_Voxels; k++)	{ std::cout << dataArray[k] << std::endl; } // debugging

			for (int k=0; k<X_Voxels*Y_Voxels; k++)
			{
				// std::cout << "k: " << k << ",layer: " << dataArray[k] << std::endl;
				// std::cout << "here3" << std::endl;
				if (pData[X_Voxels*Y_Voxels*i+k] > 0)
				{
					// std::cout << "here4" << std::endl;
					// std::vector<std::string> dataArraySynapse;
					// dataArray = split(dataArray[k],',',dataArraySynapse);
					for (int s=0; s<numForwardModelSynapses; s++)
					{
						// std::cout << "here5, vox: " << voxCounter << ", s: " << s << std::endl;
						// std::cout << "data: " <<  atof(dataArray[numSynapses*k+s].c_str()) << std::endl;
						SetForwardModelSynapseWeight(voxCounter,s,atof(dataArray[numForwardModelSynapses*k+s].c_str()));
				// 		std::cout << "here6" << std::endl;
					}
					voxCounter++;
				}
			}
		}
		pXML->UpLevel(); //Layer
		pXML->UpLevel(); //Weights
	}


	if (pXML->FindElement("RegenerationModelSynapseWeights")){
		int voxCounter = 0;
		for (int i=0; i<Z_Voxels; i++)
		{
			std::string DataIn;
			std::string RawData;
			pXML->FindLoadElement("Layer", &RawData, true, true);

			std::vector<std::string> dataArray;
			dataArray = split(RawData,',',dataArray);

			for (int k=0; k<X_Voxels*Y_Voxels; k++)
			{
				if (pData[X_Voxels*Y_Voxels*i+k] > 0)
				{
					for (int s=0; s<numRegenerationModelSynapses; s++)
					{
						SetRegenerationModelSynapseWeight(voxCounter,s,atof(dataArray[numRegenerationModelSynapses*k+s].c_str()));
					}
					voxCounter++;
				}
			}
		}
		pXML->UpLevel(); //Layer
		pXML->UpLevel(); //Weights
	}


	if (pXML->FindElement("PhaseOffset")){ 
		usingPhaseOffset = true;
		int voxCounter = 0;
		// std::cout << "found weights!" << std::endl;
		InitPhaseOffsetArray(X_Voxels*Y_Voxels*Z_Voxels);
		for (int i=0; i<Z_Voxels; i++)
		{
			std::string DataIn;
			std::string RawData;
			// std::string thisValue;
			pXML->FindLoadElement("Layer", &RawData, true, true);
		
			std::vector<std::string> dataArray;
			dataArray = split(RawData,',',dataArray);
			// std::cout << "rawData, Layer " << i << ": " << RawData << std::endl;
			// std::cout << "rawData, Layer " << i << ": " << dataArray[0] << std::endl;
			// DataIn.resize(RawData.size());
			// for (int k=0; k<(int)RawData.size(); k++)
			// std::istringstream iss(RawData);
			for (int k=0; k<X_Voxels*Y_Voxels; k++)
			{
				// std::getline(iss,thisValue,",");
				// std::cout << thisValue << std::endl;
				// DataIn[k] = RawData[k]-48; 
				// SetPhaseOffset((X_Voxels*Y_Voxels)*i+k,atof(dataArray[k].c_str()));
				if (pData[X_Voxels*Y_Voxels*i+k] > 0)
				{
					SetPhaseOffset(voxCounter,atof(dataArray[k].c_str()));
					voxCounter++;
				}
			}

			// for(int j=0; j<NumNuerons; j++)
			// {
			// 	SetData(j, atof(DataIn[j]); 
			// }

		}
		pXML->UpLevel(); //Layer
		pXML->UpLevel(); //Weights

		// for (int i=0; i<NumNuerons; i++)
		// {
		// 	for (int j=0; j<NumNuerons; j++)
		// 	{
		// 		std::cout << GetSynapseWeight(i*NumNuerons+j) << " ";
		// 	}
		// 	std::cout << std::endl;
		// }

	}
	else
	{
		usingPhaseOffset = false;
	}


	if (pXML->FindElement("FinalPhaseOffset")){
		usingFinalPhaseOffset = true;
		int voxCounter = 0;

		InitFinalPhaseOffsetArray(X_Voxels*Y_Voxels*Z_Voxels);
		for (int i=0; i<Z_Voxels; i++)
		{
			std::string DataIn;
			std::string RawData;
			pXML->FindLoadElement("Layer", &RawData, true, true);

			std::vector<std::string> dataArray;
			dataArray = split(RawData,',',dataArray);
			for (int k=0; k<X_Voxels*Y_Voxels; k++)
			{
				if (pData[X_Voxels*Y_Voxels*i+k] > 0)
				{
					SetFinalPhaseOffset(voxCounter,atof(dataArray[k].c_str()));
					voxCounter++;
				}
			}
		}
		pXML->UpLevel(); //Layer
		pXML->UpLevel(); //Weights

	}
	else
	{
		usingFinalPhaseOffset = false;
	}


	if (pXML->FindElement("InitialVoxelSize"))
	{
		usingInitialVoxelSize = true;
		int voxCounter = 0;

		InitInitialVoxelSizeArray(X_Voxels*Y_Voxels*Z_Voxels);
		for (int i=0; i<Z_Voxels; i++)
		{
			std::string DataIn;
			std::string RawData;
			pXML->FindLoadElement("Layer", &RawData, true, true);

			std::vector<std::string> dataArray;
			dataArray = split(RawData,',',dataArray);
			for (int k=0; k<X_Voxels*Y_Voxels; k++)
			{
				if (pData[X_Voxels*Y_Voxels*i+k] > 0)
				{
					SetInitialVoxelSize(voxCounter,atof(dataArray[k].c_str()));
					voxCounter++;
				}
			}
		}
		pXML->UpLevel(); //Layer
		pXML->UpLevel(); //Weights

	}
	else
	{
		usingInitialVoxelSize = false;
	}


	if (pXML->FindElement("FinalVoxelSize"))
	{
		usingFinalVoxelSize = true;
		int voxCounter = 0;

		InitFinalVoxelSizeArray(X_Voxels*Y_Voxels*Z_Voxels);
		for (int i=0; i<Z_Voxels; i++)
		{
			std::string DataIn;
			std::string RawData;
			pXML->FindLoadElement("Layer", &RawData, true, true);

			std::vector<std::string> dataArray;
			dataArray = split(RawData,',',dataArray);
			for (int k=0; k<X_Voxels*Y_Voxels; k++)
			{
				if (pData[X_Voxels*Y_Voxels*i+k] > 0)
				{
					SetFinalVoxelSize(voxCounter,atof(dataArray[k].c_str()));
					voxCounter++;
				}
			}
		}
		pXML->UpLevel(); //Layer
		pXML->UpLevel(); //Weights

	}
	else
	{
		usingFinalVoxelSize = false;
	}


	if (pXML->FindElement("VestibularContribution"))
	{
		usingVestibularContribution = true;
		int voxCounter = 0;

		InitVestibularContributionArray(X_Voxels*Y_Voxels*Z_Voxels);
		for (int i=0; i<Z_Voxels; i++)
		{
			std::string DataIn;
			std::string RawData;
			pXML->FindLoadElement("Layer", &RawData, true, true);

			std::vector<std::string> dataArray;
			dataArray = split(RawData,',',dataArray);
			for (int k=0; k<X_Voxels*Y_Voxels; k++)
			{
				if (pData[X_Voxels*Y_Voxels*i+k] > 0)
				{
					SetVestibularContribution(voxCounter,atof(dataArray[k].c_str()));
					voxCounter++;
				}
			}
		}
		pXML->UpLevel(); //Layer
		pXML->UpLevel(); //Weights

	}
	else
	{
		usingVestibularContribution = false;
	}
	
	
	if (pXML->FindElement("PreDamageRoll"))
	{
		usingPreDamageRoll = true;
		int voxCounter = 0;

		InitPreDamageRollArray(X_Voxels*Y_Voxels*Z_Voxels);
		for (int i=0; i<Z_Voxels; i++)
		{
			std::string DataIn;
			std::string RawData;
			pXML->FindLoadElement("Layer", &RawData, true, true);

			std::vector<std::string> dataArray;
			dataArray = split(RawData,',',dataArray);
			for (int k=0; k<X_Voxels*Y_Voxels; k++)
			{
				if (pData[X_Voxels*Y_Voxels*i+k] > 0)
				{
					SetPreDamageRoll(voxCounter,atof(dataArray[k].c_str()));
					voxCounter++;
				}
			}
		}
		pXML->UpLevel(); //Layer
		pXML->UpLevel(); //Weights

	}
	else
	{
		usingPreDamageRoll = false;
	}
	
	
	if (pXML->FindElement("PreDamagePitch"))
	{
		usingPreDamagePitch = true;
		int voxCounter = 0;

		InitPreDamagePitchArray(X_Voxels*Y_Voxels*Z_Voxels);
		for (int i=0; i<Z_Voxels; i++)
		{
			std::string DataIn;
			std::string RawData;
			pXML->FindLoadElement("Layer", &RawData, true, true);

			std::vector<std::string> dataArray;
			dataArray = split(RawData,',',dataArray);
			for (int k=0; k<X_Voxels*Y_Voxels; k++)
			{
				if (pData[X_Voxels*Y_Voxels*i+k] > 0)
				{
					SetPreDamagePitch(voxCounter,atof(dataArray[k].c_str()));
					voxCounter++;
				}
			}
		}
		pXML->UpLevel(); //Layer
		pXML->UpLevel(); //Weights

	}
	else
	{
		usingPreDamagePitch = false;
	}
	
	
	if (pXML->FindElement("PreDamageYaw"))
	{
		usingPreDamageYaw = true;
		int voxCounter = 0;

		InitPreDamageYawArray(X_Voxels*Y_Voxels*Z_Voxels);
		for (int i=0; i<Z_Voxels; i++)
		{
			std::string DataIn;
			std::string RawData;
			pXML->FindLoadElement("Layer", &RawData, true, true);

			std::vector<std::string> dataArray;
			dataArray = split(RawData,',',dataArray);
			for (int k=0; k<X_Voxels*Y_Voxels; k++)
			{
				if (pData[X_Voxels*Y_Voxels*i+k] > 0)
				{
					SetPreDamageYaw(voxCounter,atof(dataArray[k].c_str()));
					voxCounter++;
				}
			}
		}
		pXML->UpLevel(); //Layer
		pXML->UpLevel(); //Weights

	}
	else
	{
		usingPreDamageYaw = false;
	}


	if (pXML->FindElement("StressContribution"))
	{
		usingStressContribution = true;
		int voxCounter = 0;

		InitStressContributionArray(X_Voxels*Y_Voxels*Z_Voxels);
		for (int i=0; i<Z_Voxels; i++)
		{
			std::string DataIn;
			std::string RawData;
			pXML->FindLoadElement("Layer", &RawData, true, true);

			std::vector<std::string> dataArray;
			dataArray = split(RawData,',',dataArray);
			for (int k=0; k<X_Voxels*Y_Voxels; k++)
			{
				if (pData[X_Voxels*Y_Voxels*i+k] > 0)
				{
					SetStressContribution(voxCounter,atof(dataArray[k].c_str()));
					voxCounter++;
				}
			}
		}
		pXML->UpLevel(); //Layer
		pXML->UpLevel(); //Weights

	}
	else
	{
		usingStressContribution = false;
	}


	if (pXML->FindElement("PreDamageStress"))
	{
		usingPreDamageStress = true;
		int voxCounter = 0;

		InitPreDamageStressArray(X_Voxels*Y_Voxels*Z_Voxels);
		for (int i=0; i<Z_Voxels; i++)
		{
			std::string DataIn;
			std::string RawData;
			pXML->FindLoadElement("Layer", &RawData, true, true);

			std::vector<std::string> dataArray;
			dataArray = split(RawData,',',dataArray);
			for (int k=0; k<X_Voxels*Y_Voxels; k++)
			{
				if (pData[X_Voxels*Y_Voxels*i+k] > 0)
				{
					SetPreDamageStress(voxCounter,atof(dataArray[k].c_str()));
					voxCounter++;
				}
			}
		}
		pXML->UpLevel(); //Layer
		pXML->UpLevel(); //Weights

	}
	else
	{
		usingPreDamageStress = false;
	}


	if (pXML->FindElement("PressureContribution"))
	{
		usingPressureContribution = true;
		int voxCounter = 0;

		InitPressureContributionArray(X_Voxels*Y_Voxels*Z_Voxels);
		for (int i=0; i<Z_Voxels; i++)
		{
			std::string DataIn;
			std::string RawData;
			pXML->FindLoadElement("Layer", &RawData, true, true);

			std::vector<std::string> dataArray;
			dataArray = split(RawData,',',dataArray);
			for (int k=0; k<X_Voxels*Y_Voxels; k++)
			{
				if (pData[X_Voxels*Y_Voxels*i+k] > 0)
				{
					SetPressureContribution(voxCounter,atof(dataArray[k].c_str()));
					voxCounter++;
				}
			}
		}
		pXML->UpLevel(); //Layer
		pXML->UpLevel(); //Weights

	}
	else
	{
		usingPressureContribution = false;
	}


	if (pXML->FindElement("PreDamagePressure"))
	{
		usingPreDamagePressure = true;
		int voxCounter = 0;

		InitPreDamagePressureArray(X_Voxels*Y_Voxels*Z_Voxels);
		for (int i=0; i<Z_Voxels; i++)
		{
			std::string DataIn;
			std::string RawData;
			pXML->FindLoadElement("Layer", &RawData, true, true);

			std::vector<std::string> dataArray;
			dataArray = split(RawData,',',dataArray);
			for (int k=0; k<X_Voxels*Y_Voxels; k++)
			{
				if (pData[X_Voxels*Y_Voxels*i+k] > 0)
				{
					SetPreDamagePressure(voxCounter,atof(dataArray[k].c_str()));
					voxCounter++;
				}
			}
		}
		pXML->UpLevel(); //Layer
		pXML->UpLevel(); //Weights

	}
	else
	{
		usingPreDamagePressure = false;
	}


	if (pXML->FindElement("Stiffness"))
	{ 
		evolvingStiffness = true;

		pXML->FindLoadElement("MinElasticMod", &MinElasticMod);
		pXML->FindLoadElement("MaxElasticMod", &MaxElasticMod);		

		int voxCounter = 0;

		InitStiffnessArray(X_Voxels*Y_Voxels*Z_Voxels);
		for (int i=0; i<Z_Voxels; i++)
		{
			std::string DataIn;
			std::string RawData;
			pXML->FindLoadElement("Layer", &RawData, true, true);
		
			std::vector<std::string> dataArray;
			dataArray = split(RawData,',',dataArray);
			for (int k=0; k<X_Voxels*Y_Voxels; k++)
			{
				if (pData[X_Voxels*Y_Voxels*i+k] > 0)
				{
					SetStiffness(voxCounter,atof(dataArray[k].c_str()));
					voxCounter++;
				}
			}
		}
		pXML->UpLevel(); //Layer
		pXML->UpLevel(); //Weights

	}
	else
	{
		evolvingStiffness = false;
	}


	// Now fetching parameters associated with stiffness plasticity
	if (pXML->FindElement("StressAdaptationRate"))
	{ 
		usingStressAdaptationRate = true;

		pXML->FindLoadElement("MinDevo", &MinDevo);
		pXML->FindLoadElement("MinElasticMod", &MinElasticMod);
		pXML->FindLoadElement("MaxElasticMod", &MaxElasticMod);
		pXML->FindLoadElement("MaxAdaptationRate", &MaxAdaptationRate);
		pXML->FindLoadElement("MaxStiffnessChange", &MAX_STIFFNESS_VARIATION_STEP);		
		pXML->FindLoadElement("GrowthModel", &growthModel);		
		

		int voxCounter = 0;

		InitStressAdaptationRateArray(X_Voxels*Y_Voxels*Z_Voxels);
		for (int i=0; i<Z_Voxels; i++)
		{
			std::string DataIn;
			std::string RawData;
			pXML->FindLoadElement("Layer", &RawData, true, true);
		
			std::vector<std::string> dataArray;
			dataArray = split(RawData,',',dataArray);
			for (int k=0; k<X_Voxels*Y_Voxels; k++)
			{
				if (pData[X_Voxels*Y_Voxels*i+k] > 0)
				{
					SetStressAdaptationRate(voxCounter,atof(dataArray[k].c_str()));
					voxCounter++;
				}
			}
		}
		pXML->UpLevel(); //Layer
		pXML->UpLevel(); //Weights

	}
	else
	{
		usingStressAdaptationRate = false;
	}


	if (pXML->FindElement("PressureAdaptationRate"))
	{ 
		usingPressureAdaptationRate = true;

        pXML->FindLoadElement("MinDevo", &MinDevo);
		pXML->FindLoadElement("MinElasticMod", &MinElasticMod);
		pXML->FindLoadElement("MaxElasticMod", &MaxElasticMod);
		pXML->FindLoadElement("MaxAdaptationRate", &MaxAdaptationRate);
		pXML->FindLoadElement("MaxStiffnessChange", &MAX_STIFFNESS_VARIATION_STEP);		
		pXML->FindLoadElement("GrowthModel", &growthModel);		

		int voxCounter = 0;

		InitPressureAdaptationRateArray(X_Voxels*Y_Voxels*Z_Voxels);
		for (int i=0; i<Z_Voxels; i++)
		{
			std::string DataIn;
			std::string RawData;
			pXML->FindLoadElement("Layer", &RawData, true, true);
		
			std::vector<std::string> dataArray;
			dataArray = split(RawData,',',dataArray);
			for (int k=0; k<X_Voxels*Y_Voxels; k++)
			{
				if (pData[X_Voxels*Y_Voxels*i+k] > 0)
				{
					SetPressureAdaptationRate(voxCounter,atof(dataArray[k].c_str()));

					voxCounter++;
				}
			}
		}
		pXML->UpLevel(); //Layer
		pXML->UpLevel(); //Weights

	}
	else
	{
		usingPressureAdaptationRate = false;
	}

	//std::cout << "DEBUGMSG VX_Object.cpp: Max Stiffness Change: "<< MAX_STIFFNESS_VARIATION_STEP << std::endl;

	return true;
}

std::string CVXC_Structure::ToBase64(unsigned char const* bytes_to_encode, unsigned int in_len) // Ren� Nyffenegger http://www.adp-gmbh.ch/cpp/common/base64.html
{
	std::string ret;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	while (in_len--) {
		char_array_3[i++] = *(bytes_to_encode++);
		if (i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;
			for(i = 0; i<4; i++) ret += base64_chars[char_array_4[i]];
			i = 0;
		}
	}

	if (i){
		for(j = i; j < 3; j++) char_array_3[j] = '\0';
		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;
		for (j = 0; (j<i+1); j++) ret += base64_chars[char_array_4[j]];
		while((i++ < 3))
		ret += '=';
	}
	return ret;
}

std::string CVXC_Structure::FromBase64(std::string const& encoded_string) // Ren� Nyffenegger http://www.adp-gmbh.ch/cpp/common/base64.html
{
	int in_len = encoded_string.size();
	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];
	std::string ret;

	while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
		char_array_4[i++] = encoded_string[in_]; in_++;
		if (i ==4) {
			for (i = 0; i <4; i++) char_array_4[i] = base64_chars.find(char_array_4[i]);
			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++) ret += char_array_3[i];
			i = 0;
		}
	}

	if (i) {
		for (j = i; j <4; j++) char_array_4[j] = 0;
		for (j = 0; j <4; j++) char_array_4[j] = base64_chars.find(char_array_4[j]);
		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
		for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
	}

  return ret;
}

void CVXC_Structure::ClearStructure() //completey erases, frees, and destroys the voxel array
{
	DeleteData();

	Compression = "";
	X_Voxels = 0;
	Y_Voxels = 0;
	Z_Voxels = 0;
}

void CVXC_Structure::CreateStructure(int xV, int yV, int zV) //creates empty structure with these dimensions
{
	IniData(xV*yV*zV);

	Compression = "";
	X_Voxels = xV;
	Y_Voxels = yV;
	Z_Voxels = zV;

	ResetStructure(); //sets to zeros!
}

void CVXC_Structure::ResetStructure() //erases all voxel imformation within voxel array
{
	for (int i=0; i<GetArraySize(); i++)
		SetData(i, 0);
}

void CVXC_Structure::Resize(int xS, int yS, int zS) //resizes a structure, preserving voxels in correct locations
{
	CVXC_Structure LStruct;
	LStruct.CreateStructure(xS, yS, zS);

	int tX=0;
	int tY=0;
	int tZ=0;
	int NewIndex = 0;

	//Populate the Temporary new array
	for (int i=0; i<GetArraySize(); i++){ //go through the existing array
		if (GetData(i) != 0){ //If there is a voxel present
			GetXYZNom(&tX, &tY, &tZ, i); //sets the current XYZ indices 
			if (tX < LStruct.GetVXDim() && tY < LStruct.GetVYDim() && tZ < LStruct.GetVZDim()){ //if its within the new area... (X, Y, Z are totals- based from 1)
				NewIndex = tX + LStruct.GetVXDim()*tY + LStruct.GetVXDim()*LStruct.GetVYDim()*tZ;
				if (NewIndex <= LStruct.GetArraySize()) LStruct[NewIndex] = GetData(i);
			}
		}
	}
	*this = LStruct;
}

void CVXC_Structure::ReplaceMaterial(int Matindex, int ReplaceWith, bool ShiftDown)
{
	//replace all voxels in the matrix made of this material and (optionally) shift down the rest...
	for (int i=0; i<GetArraySize(); i++){
		if (GetData(i) == Matindex) SetData(i, ReplaceWith);
		if (ShiftDown && (int)GetData(i) > Matindex) SetData(i, abs(GetData(i))-1);
	}
}


int CVXC_Structure::GetIndex(int x, int y, int z) const //returns the index of the array from xyz indices
{
	if (x<0 || x >= X_Voxels || y<0 || y >= Y_Voxels || z<0 || z >= Z_Voxels) { //if this XYZ is out of the area
		return -1;
	}
	else {
		return x + X_Voxels*y + X_Voxels*Y_Voxels*z;
	}
}

bool CVXC_Structure::GetXYZNom(int* x, int* y, int* z, int index) const //gets the physical position of the voxels from index
{
	if (index<0 || index > X_Voxels*Y_Voxels*Z_Voxels){
		*x = -1;
		*y = -1;
		*z = -1;
		return false;
	}
	else {
		*z = (int)((vfloat)index / (X_Voxels*Y_Voxels)); //calculate the indices in x, y, z directions
		*y = (int)(((vfloat)index - *z*X_Voxels*Y_Voxels)/X_Voxels);
		*x = index - *z*X_Voxels*Y_Voxels - *y*X_Voxels;
		return true;
	}
}



////////////////////////////////VOXEL////////////////////////////

CVXC_Voxel& CVXC_Voxel::operator=(const CVXC_Voxel& RefVox)
{

	Vox_Name = RefVox.Vox_Name;
	File = RefVox.File;
	X_Squeeze = RefVox.X_Squeeze;
	Y_Squeeze = RefVox.Y_Squeeze;
	Z_Squeeze = RefVox.Z_Squeeze;

	return *this;

}

void CVXC_Voxel::ReadXML(CXML_Rip* pXML)
{
	std::string Name;
	if (pXML->FindLoadElement("File", &File)) {Vox_Name = VS_VOXFILE; }
	else if (pXML->FindLoadElement("Vox_Name", &Name)){
		if (Name == "SPHERE") Vox_Name = VS_SPHERE;
		else if (Name == "BOX") Vox_Name = VS_BOX;
		else if (Name == "CYLINDER") Vox_Name = VS_CYLINDER;
		else Vox_Name = VS_BOX; //if not these three...
		File = "";
	}
	else { //default
		Vox_Name = VS_BOX; //default
		File = "";
	}

	if (!pXML->FindLoadElement("X_Squeeze", &X_Squeeze)) X_Squeeze = 1.0;
	if (!pXML->FindLoadElement("Y_Squeeze", &Y_Squeeze)) Y_Squeeze = 1.0;
	if (!pXML->FindLoadElement("Z_Squeeze", &Z_Squeeze)) Z_Squeeze = 1.0;
}

void CVXC_Voxel::WriteXML(CXML_Rip* pXML)
{
	pXML->DownLevel("Voxel");
		switch (Vox_Name){
			case VS_VOXFILE: pXML->Element("File", File); break;
			case VS_SPHERE: pXML->Element("Vox_Name", "SPHERE"); break;
			case VS_BOX: pXML->Element("Vox_Name", "BOX"); break;
			case VS_CYLINDER: pXML->Element("Vox_Name", "CYLINDER"); break;
		}
		pXML->Element("X_Squeeze", X_Squeeze);
		pXML->Element("Y_Squeeze", Y_Squeeze);
		pXML->Element("Z_Squeeze", Z_Squeeze);
	pXML->UpLevel();
}

void CVXC_Voxel::ClearVoxel(void)
{
	Vox_Name = VS_SPHERE; //default to sphere!
	File = "";
	X_Squeeze = 1.0;
	Y_Squeeze = 1.0;
	Z_Squeeze = 1.0;
}

#ifdef USE_OPEN_GL
void CVXC_Voxel::DrawVoxel(Vec3D<>* Center, vfloat Lat_Dim)
{
	Vec3D<> Squeeze(X_Squeeze, Y_Squeeze, Z_Squeeze);

	switch (Vox_Name){
		case VS_SPHERE:
			CGL_Utils::DrawSphere(*Center, Lat_Dim/2, Squeeze);
			break;
		case VS_BOX:
			CGL_Utils::DrawCube(*Center, Lat_Dim, Squeeze, true, true, 1.0);
			break;
		case VS_CYLINDER:{
			Vec3D<> V2 = *Center + Vec3D<>(0, 0, Lat_Dim/2);
			Vec3D<> V1 = *Center - Vec3D<>(0, 0, Lat_Dim/2);
			CGL_Utils::DrawCylinder(V1, V2, (float)Lat_Dim/2, Squeeze, true, true, 1.0);
			break;}
		default:
			break;
	}
}

void CVXC_Voxel::DrawVoxel2D(Vec3D<>* Center, vfloat Lat_Dim, Vec3D<>* Normal, bool Fill)
{
	Vec3D<> Squeeze(X_Squeeze, Y_Squeeze, Z_Squeeze);

	switch (Vox_Name){
		case VS_SPHERE:
			CGL_Utils::DrawCircle(*Center, Lat_Dim/2, *Normal, Squeeze, Fill, 1.0);
			break;
		case VS_BOX:
			CGL_Utils::DrawRectangle(*Center, Lat_Dim, *Normal, Squeeze, Fill, 1.0);
			break;
		case VS_CYLINDER:
			if (Normal->z > Normal->x && Normal->z > Normal->y){ //if viewing from above (ie draw a circle)
				CGL_Utils::DrawCircle(*Center, Lat_Dim/2, *Normal, Squeeze, Fill, 1.0);	
			}
			else
				CGL_Utils::DrawRectangle(*Center, Lat_Dim, *Normal, Squeeze, Fill, 1.0);
			break;
		default:
			break;
	}
}
#endif



unsigned long int rand_seed(unsigned long int x) {
	return (1664525*x+1013904223) & 0x7fffffffUL;
}

unsigned long int taus_get(taus_state* state){
    unsigned long b;
    b = (((state->s1 << 13UL) & 0xffffffffUL) ^ state->s1) >> 19UL;
    state->s1 = (((state->s1 & 0xfffffffeUL) << 12UL) & 0xffffffffUL) ^ b;
    b = (((state->s2 << 2UL) & 0xffffffffUL) ^ state->s2) >> 25UL;
    state->s2 = (((state->s2 & 0xfffffff8UL) << 4UL) & 0xffffffffUL) ^ b;
    b = (((state->s3 << 3UL) & 0xffffffffUL) ^ state->s3) >> 11UL;
    state->s3 = (((state->s3 & 0xfffffff0UL) << 17UL) & 0xffffffffUL) ^ b;
    return (state->s1 ^ state->s2 ^ state->s3);
}

vfloat prsm(vfloat x, vfloat y, vfloat z, int k)
{
    taus_state state;

    float tx, ty, tz;
    tx = (float) x;
    ty = (float) y;
    tz = (float) z;

    /* Convert floating point numbers to ints*/
    unsigned long int ts1 = *(unsigned int*)&tx;
    unsigned long int ts2 = *(unsigned int*)&ty;
    unsigned long int ts3 = *(unsigned int*)&tz;

    /* Convert coordinates to random seeds */
    state.s1 = rand_seed(ts1);
    state.s2 = rand_seed(ts2);
    state.s3 = rand_seed(ts3);

    state.s1 = rand_seed(state.s1 ^ state.s3);
    state.s2 = rand_seed(state.s2 ^ state.s1);
    state.s3 = rand_seed(state.s3 ^ state.s2);

    state.s1 = rand_seed(state.s1 ^ state.s3);
    state.s2 = rand_seed(state.s2 ^ state.s1);
    state.s3 = rand_seed(state.s3 ^ state.s2);

    /* "warm up" generator and generate k-th number */
    for (int i=0; i<k+9; i++) {taus_get(&state);}

    return ((vfloat)taus_get(&state)/UINT_MAX);  
}
