/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef CXML_RIP_H
#define CXML_RIP_H


#ifdef QT_XML_LIB
#include <QDomDocument>
#include <QFile>
#else
#include "tinyxml.h"
#endif

#include <vector>
#include <sstream>

//for quick run-through xml encoding
class CXML_Rip
{
public:
	CXML_Rip(void);
	~CXML_Rip(void);

	//for saving
#ifdef QT_XML_LIB
	QDomDocument doc;
	std::vector <QDomElement> ElStack;
	QFile file;
#else //TinyXML
	TiXmlDocument doc;
	std::vector <TiXmlElement*> ElStack;

#endif

	std::string tmp; //temporary string (to avoid creating a bunch of these...)
	std::vector <std::string> StrStack; //used on loading to keep track of which tags were last looked for


	void SaveFile(std::string filename);
	void toXMLText(std::string* Text);
	bool LoadFile(std::string filename, std::string* pRetMsg = NULL);
	bool fromXMLText(std::string* Text);


	void DownLevel(std::string tag);
	void UpLevel(void);
//	void Element(std::string tag, std::string data, bool CDATA = false);
	void Element(std::string tag, std::string data, bool CDATA = false);
	void Element(std::string tag, double data) {std::ostringstream tmp; tmp << data; Element(tag, tmp.str());};
	void Element(std::string tag, float data) {std::ostringstream tmp; tmp << data; Element(tag, tmp.str());};
	void Element(std::string tag, int data) {std::ostringstream tmp; tmp << data; Element(tag, tmp.str());};

	void SetElAttribute(std::string Att, std::string Value); //adds an attribute to the current element in the stack
	void SetElAttribute(std::string Att, double Value) {std::ostringstream tmp; tmp << Value; SetElAttribute(Att, tmp.str());}; //adds an attribute to the current element in the stack
	void SetElAttribute(std::string Att, float Value) {std::ostringstream tmp; tmp << Value; SetElAttribute(Att, tmp.str());}; //adds an attribute to the current element in the stack
	void SetElAttribute(std::string Att, int Value) {std::ostringstream tmp; tmp << Value; SetElAttribute(Att, tmp.str());};  //adds an attribute to the current element in the stack

	//for loading
	std::string LastTagSearched; //keep track of which tag we last looks for. if new query same as last, look for siblings.
	//caution... this function can either go down a level or not!
	bool FindElement(std::string tag); //finds element if it exists and appends ptr to stack. if called subsequently with the same tag, looks for siblings, not children.

	//these functions do not change the level of the stack by default.
	bool FindLoadElement(std::string tag, std::string* pString, bool StayDown = false, bool CDATA = false); 
	bool FindLoadElement(std::string tag, double* pDouble, bool StayDown = false) {if (FindLoadElement(tag, &tmp, StayDown)){*pDouble = atof(tmp.c_str()); return true;} return false;};
	bool FindLoadElement(std::string tag, float* pFloat, bool StayDown = false) {if (FindLoadElement(tag, &tmp, StayDown)){*pFloat = (float)atof(tmp.c_str()); return true;} return false;};
	bool FindLoadElement(std::string tag, long int* pLong, bool StayDown = false) {if (FindLoadElement(tag, &tmp, StayDown)){*pLong = atol(tmp.c_str()); return true;} return false;};
	bool FindLoadElement(std::string tag, int* pInt, bool StayDown = false) {if (FindLoadElement(tag, &tmp, StayDown)){*pInt = atoi(tmp.c_str()); return true;} return false;};
	bool FindLoadElement(std::string tag, bool* pBool, bool StayDown = false) {int tmpInt; if (!FindLoadElement(tag, &tmpInt, StayDown)) return false; *pBool=(tmpInt == 0)?false:true; return true;};
	bool FindLoadElement(std::string tag, char* pChar, bool StayDown = false) {if (FindLoadElement(tag, &tmp, StayDown)){*pChar = tmp[0]; return true;} return false;};

	void GetElAttribute(std::string Att, std::string* pString);
	void GetElAttribute(std::string Att, double* pDouble) {GetElAttribute(Att, &tmp); *pDouble = atof(tmp.c_str());};
	void GetElAttribute(std::string Att, float* pFloat) {GetElAttribute(Att, &tmp); *pFloat = (float)atof(tmp.c_str());};
	void GetElAttribute(std::string Att, int* pInt) {GetElAttribute(Att, &tmp); *pInt = atoi(tmp.c_str());};

};

#endif //CXML_RIP_H