/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/


#ifndef ARRAY3D_H
#define ARRAY3D_H

#include "XML_Rip.h"
//#include <fstream>


class CArray3Df
{
public:
	CArray3Df(void);
	CArray3Df(int x, int y, int z);
	~CArray3Df(void);

	CArray3Df(const CArray3Df& rArray); //copy constructor
	CArray3Df& operator=(const CArray3Df& rArray); //overload "=" 
	const float& operator [](int i) const { return pData[i]; } //overload index operator
		  float& operator [](int i)       { return pData[i]; }
	
	float& operator ()(int i, int j, int k) { return pData[GetIndex(i, j, k)]; }


	void WriteXML(CXML_Rip* pXML);
	void ReadXML(CXML_Rip* pXML);

	void IniSpace(int x, int y, int z, float IniVal = 0.0f);
	void ResetSpace(float IniVal = 0.0f);
	void DeleteSpace();
	float GetMaxValue(); //returns the maximum value anywhere in the array.
	int GetIndex(int x, int y, int z); //returns the index  or -1 if invalid
	bool GetXYZ(int* x, int* y, int* z, int Index); //returns the xyz indicies
	int GetFullSize(void) {return FullSize;};
	int GetXSize(void) {return XSize;};
	int GetYSize(void) {return YSize;};
	int GetZSize(void) {return ZSize;};



protected:
	float* pData; 
	int XSize;
	int YSize;
	int ZSize;
	int FullSize;

};

#endif
