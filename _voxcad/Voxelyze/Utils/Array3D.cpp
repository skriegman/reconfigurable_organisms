/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/


#include "Array3D.h"

CArray3Df::CArray3Df(void)
{
	pData = NULL; //dynamic array
	XSize = 0;
	YSize = 0;
	ZSize = 0;
	FullSize = 0;
}

CArray3Df::CArray3Df(int x, int y, int z)
{
	pData = NULL; //dynamic array
	IniSpace(x, y, z);
}


CArray3Df::~CArray3Df(void)
{
	DeleteSpace();
}

CArray3Df::CArray3Df(const CArray3Df& rArray) //copy constructor
{
	XSize = rArray.XSize;
	YSize = rArray.YSize;
	ZSize = rArray.ZSize;
	FullSize = XSize*YSize*ZSize;

	pData = new float[FullSize];
	for(int i=0; i<FullSize; i++)
		pData[i] = rArray[i];
}

CArray3Df& CArray3Df::operator=(const CArray3Df& rArray) //overload "=" 
{
	DeleteSpace();

	XSize = rArray.XSize;
	YSize = rArray.YSize;
	ZSize = rArray.ZSize;
	FullSize = XSize*YSize*ZSize;

	pData = new float[FullSize];
	for(int i=0; i<FullSize; i++)
		pData[i] = rArray[i];

	return *this;
}


//void CArray3Df::Write(ofstream* pFile)
//{
//	*pFile << "XSize: "<< XSize << "\n";
//	*pFile << "YSize: "<< YSize << "\n";
//	*pFile << "ZSize: "<< ZSize << "\n";
//
//	for (int i=0; i<FullSize; i++)
//		*pFile << pData[i] << "\n";
//
//}

void CArray3Df::WriteXML(CXML_Rip* pXML)
{
	pXML->DownLevel("Array3Df");
		pXML->Element("XSize", XSize);
		pXML->Element("YSize", YSize);
		pXML->Element("ZSize", ZSize);
		pXML->DownLevel("Elements");
			for (int i=0; i<FullSize; i++)
				pXML->Element("El", pData[i]);
		pXML->UpLevel();
	pXML->UpLevel();
}

void CArray3Df::ReadXML(CXML_Rip* pXML)
{
	if (!pXML->FindLoadElement("XSize", &XSize)) XSize = 0;
	if (!pXML->FindLoadElement("YSize", &YSize)) YSize = 0;
	if (!pXML->FindLoadElement("ZSize", &ZSize)) ZSize = 0;

	IniSpace(XSize, YSize, ZSize);

	if (pXML->FindElement("Elements")){
		int iterator = 0;
		float tmpNum;
		while(pXML->FindLoadElement("El", &tmpNum, true)){ //have to go through them all to pop back up the level...
			if(iterator < FullSize) pData[iterator++] = tmpNum;
			//else ignore...
		}
		pXML->UpLevel();
	}

}

//void CArray3Df::Read(CStdioFile* pFile)
//{
//	std::string line;
//
//
//	(*pFile).ReadString(line);
//	sscanf_s(line, "XSize: %d", &XSize);
//	(*pFile).ReadString(line);
//	sscanf_s(line, "YSize: %d", &YSize);
//	(*pFile).ReadString(line);
//	sscanf_s(line, "ZSize: %d", &ZSize);
//
//	IniSpace(XSize, YSize, ZSize);
//
//	for (int i=0; i<FullSize; i++){
//		(*pFile).ReadString(line);
//		sscanf_s(line, "%f", &pData[i]);
//	}
//}


void CArray3Df::IniSpace(int x, int y, int z, float IniVal)
{
	if (pData != NULL)
		DeleteSpace();

	XSize = x;
	YSize = y;
	ZSize = z;
	FullSize = x*y*z;

	pData = new float[FullSize];

	ResetSpace(IniVal);
}

void CArray3Df::ResetSpace(float IniVal)
{
	if (pData != NULL)
		for(int i=0; i<FullSize; i++)
			pData[i] = IniVal;

}

void CArray3Df::DeleteSpace()
{
	if (pData != NULL) delete[] pData;
	pData = NULL;
}

float CArray3Df::GetMaxValue() //returns the maximum value anywhere in the array.
{
	float MaxVal = -9e9f;
	for(int i=0; i<FullSize; i++) if (pData[i] > MaxVal) MaxVal = pData[i];
	return MaxVal;
}

//Index calculating!
int CArray3Df::GetIndex(int x, int y, int z) //returns the index in the frequency space or -1 if invalid
{
	if (x<0 || x >= XSize || y<0 || y >= YSize || z<0 || z >= ZSize) //if this XYZ is out of the area
		return -1;
	else
		return x + XSize*y + XSize*YSize*z;
}

bool CArray3Df::GetXYZ(int* x, int* y, int* z, int Index) //returns the index in the frequency space or -1 if invalid
{
	if (Index<0 || Index > XSize*YSize*ZSize){ *x = -1; *y = -1;	*z = -1; return false;}
	else {
		*z = (int)((float)Index / (XSize*YSize)); //calculate the indices in x, y, z directions
		*y = (int)(((float)Index - *z*XSize*YSize)/XSize);
		*x = Index - *z*XSize*YSize - *y*XSize;
		return true;
	}
}
