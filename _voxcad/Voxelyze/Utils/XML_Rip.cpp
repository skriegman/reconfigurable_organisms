/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "XML_Rip.h"
#ifdef QT_XML_LIB
#include <QTextStream>
#endif

CXML_Rip::CXML_Rip(void)
{
#ifndef QT_XML_LIB
	TiXmlDeclaration* declaration = new TiXmlDeclaration( "1.0", "", "" );
	doc.LinkEndChild(declaration);
#endif

	LastTagSearched = "";
}

CXML_Rip::~CXML_Rip(void)
{
}

void CXML_Rip::SaveFile(std::string filename)
{
#ifdef QT_XML_LIB
	const int Indent = 2;
	QFile OutFile(filename.c_str());
	if (OutFile.open(QFile::WriteOnly | QIODevice::Text | QFile::Truncate)) { //QIODevice::Text to see line breaks!!!
		QTextStream out(&OutFile);
		QDomNode xmlNode = doc.createProcessingInstruction("xml","version=\"1.0\" encoding=\"ISO-8859-1\"");
		doc.insertBefore(xmlNode, doc.firstChild());
		doc.save(out, Indent);
	}
#else //TINY_XML
	doc.SaveFile(filename);
#endif

}

void CXML_Rip::toXMLText(std::string* Text)
{
#ifdef QT_XML_LIB
	const int Indent = 2;
	QDomNode xmlNode = doc.createProcessingInstruction("xml","version=\"1.0\" encoding=\"ISO-8859-1\"");
	doc.insertBefore(xmlNode, doc.firstChild());
	QString tmp = doc.toString(Indent);
	*Text = tmp.toStdString();
#else //TINY_XML
	//implement me!
#endif
}

bool CXML_Rip::LoadFile(std::string filename, std::string* pRetMsg) 
{
#ifdef QT_XML_LIB
	file.setFileName(filename.c_str());
	QString ErrorMsg;
	if (!doc.setContent(&file, true, &ErrorMsg)){
		if (pRetMsg) *pRetMsg += "Xml read error: " + ErrorMsg.toStdString() + "\n";
		return false;
	}
	ElStack.clear();
	ElStack.push_back(doc.documentElement()); //start with the root element!
#else //TINY_XML
	if (!doc.LoadFile(filename)){
		if (pRetMsg) *pRetMsg += "Xml read error\n";
		return false;
	}
	ElStack.clear();
	ElStack.push_back(doc.RootElement()); //start with the root element!
#endif

	StrStack.clear(); //Maybe don't want for qt xml?
	StrStack.push_back("Root");

	return true;
}

bool CXML_Rip::fromXMLText(std::string* Text) 
{
#ifdef QT_XML_LIB
	if (!doc.setContent(QString(Text->c_str()), true)) return false;
	ElStack.clear();
	ElStack.push_back(doc.documentElement()); //start with the root element!
#else //TINY_XML
	//ImplementMe
#endif

	StrStack.clear(); //Maybe don't want for qt xml?
	StrStack.push_back("Root");

	return true;
}

void CXML_Rip::DownLevel(std::string tag)
{
	int ElStackSize = (int)ElStack.size();
	
#ifdef QT_XML_LIB
	QDomElement tmpEl = doc.createElement(tag.c_str());
	if (ElStackSize == 0) doc.appendChild(tmpEl); //if root level, add to doc
	else ElStack[ElStackSize-1].appendChild(tmpEl); //if not root level, add to current level of stack
#else //TINY_XML
	TiXmlElement * tmpEl = new TiXmlElement(tag);
	if (ElStackSize == 0) doc.LinkEndChild(tmpEl); //if root level, add to doc
	else ElStack[ElStackSize-1]->LinkEndChild(tmpEl);//if not root level, add to current level of stack
#endif

	ElStack.push_back(tmpEl);
	StrStack.push_back(tag); //not really necessary, but keep track of it anyway...
}

void CXML_Rip::UpLevel(void)
{
	ElStack.pop_back();
	StrStack.pop_back();
}

void CXML_Rip::Element(std::string tag, std::string data, bool CDATA)
{
	if (CDATA)
		int test = data.size();
	DownLevel(tag);

#ifdef QT_XML_LIB
	if (CDATA){
		int test = data.length();
		QDomCDATASection tmpData = doc.createCDATASection(data.c_str());
		ElStack.back().appendChild(tmpData);
	}
	else {
		QDomText tmpText = doc.createTextNode(data.c_str());
		ElStack.back().appendChild(tmpText);
	}
#else //TINY_XML
	TiXmlText * tmpText = new TiXmlText(data);
	tmpText->SetCDATA(CDATA);
	ElStack[ElStack.size()-1]->LinkEndChild(tmpText);
#endif

	UpLevel();
}


void CXML_Rip::SetElAttribute(std::string Att, std::string Value){
#ifdef QT_XML_LIB
	ElStack.back().setAttribute(Att.c_str(), Value.c_str());
#else //TINY_XML
	ElStack.back()->SetAttribute(Att, Value);
#endif
}


bool CXML_Rip::FindElement(std::string tag) //finds element if it exists and appends ptr to stack. if called subsequently with the same tag, looks for siblings, not children.
{
#ifdef QT_XML_LIB
	QDomElement StartElement;
	QDomElement IterElement;
	bool IsSameTag = (tag == StrStack.back()); //flag to see if we just searched for this one

	if (IsSameTag) StartElement = ElStack.back().nextSiblingElement();//if this IS the last tag we searched for...
	else StartElement = ElStack.back().firstChildElement();//if this is the first time we've searched for the tag

	for (IterElement = StartElement; !IterElement.isNull(); IterElement = IterElement.nextSiblingElement()){
		if (tag == IterElement.tagName().toStdString()){
			if (IsSameTag) ElStack.back() = IterElement; //move the bottom element of the stack to next sibling
			else {
				ElStack.push_back(IterElement); //if first element of this type
				StrStack.push_back(tag);
			}
			return true; 
		}
	}
#else //TINY_XML
	TiXmlElement* StartElement;
	TiXmlElement* IterElement;
	bool IsSameTag = (tag == StrStack.back()); //flag to see if we just searched for this one
	
	if (IsSameTag) StartElement = (TiXmlElement*) ElStack.back()->NextSibling(); //if this IS the last tag we searched for...
	else StartElement = (TiXmlElement*) ElStack.back()->FirstChild(); //if this is the first time we've searched for the tag

	for (IterElement = StartElement; IterElement != 0; IterElement = (TiXmlElement*)IterElement->NextSibling()){
		if (tag == IterElement->Value()){
			if (IsSameTag) ElStack.back() = IterElement; //move the bottom element of the stack to next sibling
			else {
				ElStack.push_back(IterElement); //if first element of this type
				StrStack.push_back(tag);
			}
			return true; 
		}
	}

#endif

	if (IsSameTag) UpLevel(); //idea: if've we've been searching for multiple tags and we reach the end, automatically go back up a level

	return false;
}

bool CXML_Rip::FindLoadElement(std::string tag, std::string* pString, bool StayDown, bool CDATA)
{
	if (!FindElement(tag)) return false;

#ifdef QT_XML_LIB
	if (CDATA){
		QDomCDATASection childData = (ElStack.back().firstChild()).toCDATASection();
		if (childData.isNull()) return false;
		*pString = childData.data().toStdString();
	}
	else {
		QDomText childText = (ElStack.back().firstChild()).toText();
		if (childText.isNull()) return false;
		*pString = childText.data().toStdString();
	}
#else //TINY_XML
	TiXmlText* pText = ElStack.back()->FirstChild()->ToText();
	if (pText == 0) return false;
	*pString = pText->Value();

#endif

	if (!StayDown) UpLevel(); //if we're not expecting to load more tags of the same name...

	return true;
}

void CXML_Rip::GetElAttribute(std::string Att, std::string* pString) {
#ifdef QT_XML_LIB
	*pString = ElStack.back().attribute(Att.c_str()).toStdString();
#else //TINY_XML
	*pString = ElStack.back()->Attribute(Att.c_str());
#endif
}
