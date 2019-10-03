#include "QVX_Edit.h"
#include "../Voxelyze/Utils/GL_Utils.h"
#include <qgl.h>

CQDM_Edit::CQDM_Edit() : QVX_Object()
{
	ViewSection = false;
	CurSecAxis = AXIS_Z;
	CurSecLayer = 0;
	FadeAwayDist = 2;
	CurSecFromNeg = false;
	LMBDown = false;
	CurDrawTool = DT_PEN;
	ViewTiled = false;
}

CQDM_Edit::~CQDM_Edit()
{

}

//void CQDM_Edit::SelectedNew(int index)
//{
//	CurSelected = index;
//}

void CQDM_Edit::LMouseDown(Vec3D<> P, bool IsCtrlDown)
{
	CurCoord = P;
	DownCoord = CurCoord;
	//CurMCX = XC;
	//CurMCY = YC;
	//DownMCX = XC;
	//DownMCY = YC;
	LMBDown = true;

	FillHighlighted(IsCtrlDown);
	emit UpdateGLWindows();
}

void CQDM_Edit::LMouseUp(Vec3D<> P)
{
	if (LMBDown){
		LMBDown = false;

		int MatIndex = 0;
		emit GetCurMaterial(&MatIndex);
		//apply the stuff here!

		for (int i=0; i<(int)CurHighlighted.size(); i++){
			SetMat(CurHighlighted[i], MatIndex);
		}
		if ((int)CurHighlighted.size()>0) emit ModelChanged();

		CurHighlighted.clear();
		emit UpdateGLWindows();
	}
	
}

void CQDM_Edit::LMouseDownMove(Vec3D<> P)
{
	if (LMBDown){
		CurCoord = P;
//		CurMCX = XC;
//		CurMCY = YC;
		FillHighlighted();
	}
	emit UpdateGLWindows();

}

void CQDM_Edit::HoverMove(Vec3D<> P)
{
	CurCoord = P;

//	CurMCX = XC;
//	CurMCY = YC;
	int tmpIndex =  V2DFindVoxelUnder(CurCoord);
	if (!(CurHighlighted.size() == 1 && CurHighlighted[0] == tmpIndex)){ //if there's more than one index in the array or the index has changed...
		CurHighlighted.clear();
		if (tmpIndex != -1)	CurHighlighted.push_back(tmpIndex);
		emit UpdateGLWindows();
	}
}

void CQDM_Edit::PressedEscape(void)
{
	if (LMBDown){
		CurHighlighted.clear();	
		LMBDown = false;
	}
	emit UpdateGLWindows();

}


int CQDM_Edit::V2DFindVoxelUnder(Vec3D<> Coord) //returns the index of a voxel under the coordinates
{
	//Could be rewritten better!!
	Vec3D<> Bounds = GetWorkSpace();
	
	switch(CurSecAxis){ //put the "undefined" coordinate from 2d projection to something reasonable...
		case AXIS_Z: Coord.z = GetXYZ(0,0,CurSecLayer).z; break;
		case AXIS_Y: Coord.y = GetXYZ(0,CurSecLayer,0).y; break;
		case AXIS_X: Coord.x = GetXYZ(CurSecLayer,0,0).x; break;
	}

	if (Coord.x<0.0 || Coord.x > Bounds.x || Coord.y<0.0 || Coord.y > Bounds.y || Coord.z<0.0 || Coord.z > Bounds.z) return -1; //do easy check to see if we're even within the area

	int tX, tY, tZ, tInd;
	Vec3D<> tVec;
	Vec3D<> BB = Vec3D<>(Lattice.GetXDimAdj(), Lattice.GetYDimAdj(), Lattice.GetZDimAdj())*GetLatticeDim()/2;
	tX = Coord.x/Bounds.x*GetVXDim(); //approximate...
	tY = Coord.y/Bounds.y*GetVYDim();
	tZ = Coord.z/Bounds.z*GetVZDim();
	
	//estimates
	for (int i=-1; i<=1; i++){ //for +/- 1 in each direction..
		for (int j=-1; j<=1; j++){
			switch(CurSecAxis){
				case AXIS_Z: tInd = GetIndex(tX+i, tY+j, CurSecLayer); break;
				case AXIS_Y: tInd = GetIndex(tX+i, CurSecLayer, tZ+j); break;
				case AXIS_X: tInd = GetIndex(CurSecLayer, tY+i, tZ+j); break;
			}
			tVec = GetXYZ(tInd);
			if ((CurSecAxis == AXIS_X || (Coord.x > tVec.x-BB.x && Coord.x < tVec.x+BB.x)) && 
				(CurSecAxis == AXIS_Y || (Coord.y > tVec.y-BB.y && Coord.y < tVec.y+BB.y)) &&
				(CurSecAxis == AXIS_Z || (Coord.z > tVec.z-BB.z && Coord.z < tVec.z+BB.z))) 
				return tInd;
		}
	}
	return -1;
}

bool CQDM_Edit::V2DHighlightMe(int index) //highlight this voxel in 2D overlay layer?
{
	//if (!LMBDown){ //if we're just hovering...
	//	Vec3D Pos = GetXYZ(index);
	//	double LatRad = GetLatticeDim()/2;
	//	switch(CurSecAxis){
	//		case AXIS_Z:
	//			if (CurMCX > Pos.x-LatRad && CurMCX < Pos.x+LatRad && CurMCY > Pos.y-LatRad && CurMCY < Pos.y+LatRad) 
	//				return true;
	//			else return false;
	//			break;
	//	}
	//}
	return false;
}

void CQDM_Edit::Draw3D(bool FastMode) //draws the digital part in initialized OpenGL window
{
	DrawMe(FastMode, true);
	if (ViewSection) DrawSectionPlane(FastMode);
	

}
//void CQDM_Edit::DrawFast(int Selected, bool FastMode)
//{
//
//
//}

void CQDM_Edit::SetCurGLColor(CVXC_Material* pMat, bool Selected, float FadeOutPerc)
{
	double r = pMat->GetRedf(); 
	double g = pMat->GetGreenf();
	double b = pMat->GetBluef();
	double a = pMat->GetAlphaf();

	if (Selected){ r = (1.0f-r)+0.5f; if (r>1.0f) r = 1.0f; g=0.8f-g; if (g<0) g = 0; b=1.0f-b;}
	if (FadeOutPerc != 0.0){
		r += (1.0f-r)*(float)FadeOutPerc;
		g +=(1.0f-g)*(float)FadeOutPerc;
		b +=(1.0f-b)*(float)FadeOutPerc;
		a -= a*(float)FadeOutPerc;
	}

	glColor4f(r, g, b, a);
}


void CQDM_Edit::DrawMe(bool FastMode, bool ShowSelected, int NumBehindSection) //draws the digital part in initialized OpenGL window
//Show Selected highlights current sleceted voxel, Section view uses the currently selected axis and direction to show section, FadeBehindSelection optionally fades out the layers (for 2D view)
{
	int Selected = GetCurSel();
	if (!ShowSelected) Selected = -1; //don't draw the selected one...
	if (FastMode && NumBehindSection != LAYERMAX) NumBehindSection = 0; //in fast mode don't draw faded out ones...

	CVXC_Material* pMaterial;
	bool IsVis;
	int aS = Structure.GetArraySize();
	int x, y, z, MatIndex;
	int XMin=0, YMin=0, ZMin=0, XMax=LAYERMAX, YMax=LAYERMAX, ZMax=LAYERMAX;
	if (ViewSection){
		if (CurSecAxis == AXIS_X){
			if (CurSecFromNeg){ XMin = CurSecLayer; XMax = CurSecLayer+NumBehindSection;}
			else {XMin = CurSecLayer-NumBehindSection; XMax = CurSecLayer;}
		}
		else if (CurSecAxis == AXIS_Y){
			if (CurSecFromNeg){ YMin = CurSecLayer; YMax = CurSecLayer+NumBehindSection;}
			else {YMin = CurSecLayer-NumBehindSection; YMax = CurSecLayer;}
		}
		else if (CurSecAxis == AXIS_Z){
			if (CurSecFromNeg){ ZMin = CurSecLayer; ZMax = CurSecLayer+NumBehindSection;}
			else {ZMin = CurSecLayer-NumBehindSection; ZMax = CurSecLayer;}
		}
	}

	Vec3D<> Center;
	Vec3D<> Squeeze(Voxel.GetXSqueeze(), Voxel.GetYSqueeze(), Voxel.GetZSqueeze());
	double Dim = GetLatticeDim();
	Vec3D<> VoxEnv = GetLatDimEnv();
	Vec3D<> WS(GetVXDim()*VoxEnv.x, GetVYDim()*VoxEnv.y, GetVZDim()*VoxEnv.z);
	int NumCellAround = 0;
	if (ViewTiled) NumCellAround = 1;

	for (int ix=-NumCellAround; ix<=NumCellAround; ix++){ //for showing repeated unit cells
		for (int jy=-NumCellAround; jy<=NumCellAround; jy++){
			for (int kz=-NumCellAround; kz<=NumCellAround; kz++){
				if(ViewSection){ //don't draw ones that aren't in current plane section
					if (CurSecAxis == AXIS_X && ix != 0) continue;
					else if (CurSecAxis == AXIS_Y && jy != 0) continue; 
					else if (CurSecAxis == AXIS_Z && kz != 0) continue; 
				}

				glPushMatrix();
				glTranslated(ix*WS.x, jy*WS.y, kz*WS.z);


				for (int i = 0; i<aS; i++){ //go through all the voxels...
					GetXYZNom(&x, &y, &z, i);
					
					if (ViewSection && (x<XMin || x>XMax || y<YMin || y>YMax || z<ZMin || z>ZMax)) continue; //jump ship here if we're outside of the section view:

					MatIndex = Structure[i];
					if (MatIndex <= 0) continue; //don't draw if nothing there
					if (FastMode){ //disable picking, show selected, and resolving composite materials
						glColor3f(Palette[MatIndex].GetRedf(), Palette[MatIndex].GetGreenf(), Palette[MatIndex].GetBluef());
					}
					else { //NOT fastmode: enable picking, show selected, and resolve composite materials
						glLoadName(i); //to enable picking

						bool IsVis;
						pMaterial = GetLeafMat(i, &IsVis);
						if (pMaterial == NULL || !IsVis) continue;
						if (pMaterial->GetAlphai() == 0) continue;

						double FadeOut = 0.0;
						if (ViewSection && NumBehindSection != LAYERMAX){ //if there's fading out to be done...
							if (CurSecAxis == AXIS_X) FadeOut = abs((double)(x - CurSecLayer))/(NumBehindSection+1);
							else if (CurSecAxis == AXIS_Y) FadeOut = abs((double)(y - CurSecLayer))/(NumBehindSection+1);
							else if (CurSecAxis == AXIS_Z) FadeOut = abs((double)(z - CurSecLayer))/(NumBehindSection+1);

						}
						SetCurGLColor(pMaterial, i==Selected, FadeOut);

					}

					GetXYZ(&Center, i); 

					glPushMatrix();
					glTranslated(Center.x, Center.y, Center.z);
					glScaled(Dim*Squeeze.x, Dim*Squeeze.y, Dim*Squeeze.z);
					switch (Voxel.GetVoxName()){
						case VS_SPHERE:	CGL_Utils::DrawSphereFace(); break;
						case VS_BOX: CGL_Utils::DrawCubeFace(); break;
						case VS_CYLINDER: CGL_Utils::DrawCylFace(); break;
						default: break;
					}
					glPopMatrix();
				}

				if (!FastMode && Voxel.GetVoxName() != VS_SPHERE){ //no lines in fast mode or for spheres!
					glDisable(GL_LIGHTING);
					glLineWidth(1.0);
					for (int i = 0; i<aS; i++){ //go through all the voxels...
						GetXYZNom(&x, &y, &z, i);
						if (ViewSection && (x<XMin || x>XMax || y<YMin || y>YMax || z<ZMin || z>ZMax)) continue; //jump ship here if we're outside of the section view:


						if (Structure[i] <= 0) continue; //don't draw if nothing there
						glColor3f(0, 0, 0);
						GetXYZ(&Center, i); 

						glPushMatrix();
						glTranslated(Center.x, Center.y, Center.z);
						glScaled(Dim*Squeeze.x, Dim*Squeeze.y, Dim*Squeeze.z);
						switch (Voxel.GetVoxName()){
							case VS_BOX: CGL_Utils::DrawCubeEdge(); break;
							case VS_CYLINDER: CGL_Utils::DrawCylEdge(); break;
							default: break;
						}
						glPopMatrix();
					}
					glEnable(GL_LIGHTING);
				}
				glPopMatrix();
			}
		}
	}

	//if (!ViewSection){//if not in section mode, draw it all!
	//	//if(FastMode) 
	//		DrawFast(Selctd, FastMode); 
	//	//else Draw(Selctd);
	//}
	//else { //otherwise draw the appropriate section view
	//	switch (CurSecAxis){
	//		case AXIS_Z: DrawSecZ(CurSecLayer, NumBehindSection, CurSecFromNeg, Selctd); break;
	//		case AXIS_Y: DrawSecY(CurSecLayer, NumBehindSection, CurSecFromNeg, Selctd); break;
	//		case AXIS_X: DrawSecX(CurSecLayer, NumBehindSection, CurSecFromNeg, Selctd); break;
	//	}
	//}
}

//void CQDM_Edit::DrawSecX(int Layer, int NumBehind, bool FromNeg, int Selected) //draw Z Section view
//{
//	int CurLayer = Layer;
//	while (CurLayer >= 0 && CurLayer < GetVXDim()){ //while within range
//		if (NumBehind == LAYERMAX) DrawXLayer(CurLayer, Selected, 0.0);
//		else {
//			if (CurLayer < Layer - NumBehind || CurLayer > Layer + NumBehind) break;
//			DrawXLayer(CurLayer, Selected, abs((double)(Layer - CurLayer))/(NumBehind+1));
//
//		}
//		if (FromNeg) CurLayer++;
//		else CurLayer--;
//	}
//
//} 

//void CQDM_Edit::DrawSecY(int Layer, int NumBehind, bool FromNeg, int Selected) //draw Z Section view
//{
//	int CurLayer = Layer;
//	while (CurLayer >= 0 && CurLayer < GetVYDim()){ //while within range
//		if (NumBehind == LAYERMAX) DrawYLayer(CurLayer, Selected, 0.0);
//		else {
//			if (CurLayer < Layer - NumBehind || CurLayer > Layer + NumBehind) break;
//			DrawYLayer(CurLayer, Selected, abs((double)(Layer - CurLayer))/(NumBehind+1));
//
//		}
//		if (FromNeg) CurLayer++;
//		else CurLayer--;
//	}
//
//} 

//void CQDM_Edit::DrawSecZ(int Layer, int NumBehind, bool FromNeg, int Selected) //draw Z Section view
//{
//	int CurLayer = Layer;
//	while (CurLayer >= 0 && CurLayer < GetVZDim()){ //while within range
//		if (NumBehind == LAYERMAX) DrawZLayer(CurLayer, Selected, 0.0);
//		else {
//			if (CurLayer < Layer - NumBehind || CurLayer > Layer + NumBehind) break;
//			DrawZLayer(CurLayer, Selected, abs((double)(Layer - CurLayer))/(NumBehind+1));
//
//		}
//		if (FromNeg) CurLayer++;
//		else CurLayer--;
//	}
//
//} 
//
//
//void CQDM_Edit::DrawXLayer(int Layer, int Selected, double FadeOutPerc) //draws a single X (YZ) plane layer faded to the specified percentage of the background r, g, b (BG_R, BG_G, BG_B)
//{
//	Vec3D VoxEnv = GetLatDimEnv();
//	Vec3D WS = Vec3D(GetVXDim()*VoxEnv.x, GetVYDim()*VoxEnv.y, GetVZDim()*VoxEnv.z);
//	int NumCellAround = 0;
//	if (ViewTiled) NumCellAround = 1;
//
//	for (int i=-NumCellAround; i<=NumCellAround; i++){
//		for (int j=-NumCellAround; j<=NumCellAround; j++){
//
//			glPushMatrix();
//			glTranslated(0, i*WS.y, j*WS.z);
//			int ThisIndex;
//			for (int y=0; y<GetVYDim(); y++){
//				for (int z=0; z<GetVZDim(); z++){
//					ThisIndex = GetIndex(Layer, y, z);
//					DrawSingleVoxel(ThisIndex); //, ThisIndex==Selected, FadeOutPerc);
//				}
//			}
//			glPopMatrix();
//		}
//	}
//}
//
//void CQDM_Edit::DrawYLayer(int Layer, int Selected, double FadeOutPerc) //draws a single X (YZ) plane layer faded to the specified percentage of the background r, g, b (BG_R, BG_G, BG_B)
//{
//	Vec3D VoxEnv = GetLatDimEnv();
//	Vec3D WS = Vec3D(GetVXDim()*VoxEnv.x, GetVYDim()*VoxEnv.y, GetVZDim()*VoxEnv.z);
//	int NumCellAround = 0;
//	if (ViewTiled) NumCellAround = 1;
//
//	for (int i=-NumCellAround; i<=NumCellAround; i++){
//		for (int j=-NumCellAround; j<=NumCellAround; j++){
//
//			glPushMatrix();
//			glTranslated(i*WS.x, 0, j*WS.z);
//			int ThisIndex;
//			for (int z=0; z<GetVZDim(); z++){
//				for (int x=0; x<GetVXDim(); x++){
//					ThisIndex = GetIndex(x, Layer, z);
//					DrawSingleVoxel(ThisIndex); //, ThisIndex==Selected, FadeOutPerc);
//				}
//			}
//			glPopMatrix();
//		}
//	}
//}
//
//
//void CQDM_Edit::DrawZLayer(int Layer, int Selected, double FadeOutPerc) //draws a single X (YZ) plane layer faded to the specified percentage of the background r, g, b (BG_R, BG_G, BG_B)
//{
//	Vec3D VoxEnv = GetLatDimEnv();
//	Vec3D WS = Vec3D(GetVXDim()*VoxEnv.x, GetVYDim()*VoxEnv.y, GetVZDim()*VoxEnv.z);
//	int NumCellAround = 0;
//	if (ViewTiled) NumCellAround = 1;
//
//	for (int i=-NumCellAround; i<=NumCellAround; i++){
//		for (int j=-NumCellAround; j<=NumCellAround; j++){
//
//			glPushMatrix();
//			glTranslated(i*WS.x, j*WS.y, 0);
//			int ThisIndex;
//			for (int y=0; y<GetVYDim(); y++){
//				for (int x=0; x<GetVXDim(); x++){
//					ThisIndex = GetIndex(x, y, Layer);
//					DrawSingleVoxel(ThisIndex);//, ThisIndex==Selected, FadeOutPerc);
//				}
//			}
//			glPopMatrix();
//		}
//	}
//}



void CQDM_Edit::DrawSectionPlane(bool FastMode)//draw a slice plane to reference where we're cutting
{
	Vec3D<> WS = GetWorkSpace();
	Vec3D<> v1, v2, v12, v21, ArrDir;
	CColor CutColor = CColor(0.3, 0.3, 0.5, 1.0);
	double PadOut = WS.Length()/10.0;
	double Plane;
	switch (CurSecAxis){
	case AXIS_Z:
//		DrawSecZ(CurSecLayer, LAYERMAX, CurSecFromNeg, GetCurSel());
		Plane = GetXYZ(0, 0, CurSecLayer).z;
		v1 = Vec3D<>(-PadOut, -PadOut, Plane);
		v2 = Vec3D<>(WS.x+PadOut, WS.y+PadOut, Plane);
		v12 = Vec3D<>(v1.x, v2.y, v1.z);
		v21 = Vec3D<>(v2.x, v1.y, v1.z);
		ArrDir = Vec3D<>(0,0, CurSecFromNeg?-PadOut:PadOut);
	break;
	case AXIS_Y:
//		DrawSecY(CurSecLayer, LAYERMAX, CurSecFromNeg, GetCurSel());
		Plane = GetXYZ(0, CurSecLayer, 0).y;
		v1 = Vec3D<>(-PadOut, Plane, -PadOut);
		v2 = Vec3D<>(WS.x+PadOut, Plane, WS.z+PadOut);
		v12 = Vec3D<>(v1.x, v1.y, v2.z);
		v21 = Vec3D<>(v2.x, v1.y, v1.z);
		ArrDir = Vec3D<>(0,CurSecFromNeg?-PadOut:PadOut, 0);
	break;
	case AXIS_X:
//		DrawSecX(CurSecLayer, LAYERMAX, CurSecFromNeg, GetCurSel());
		Plane = GetXYZ(CurSecLayer, 0, 0).x;
		v1 = Vec3D<>(Plane, -PadOut, -PadOut);
		v2 = Vec3D<>(Plane, WS.y+PadOut, WS.z+PadOut);
		v12 = Vec3D<>(v1.x, v1.y, v2.z);
		v21 = Vec3D<>(v1.x, v2.y, v1.z);
		ArrDir = Vec3D<>(CurSecFromNeg?-PadOut:PadOut,0,0);
	break;
	}


	CGL_Utils::DrawRectangle(v1, v2, !FastMode, 0, CColor(CutColor.r, CutColor.g, CutColor.b, 0.3));
	CGL_Utils::DrawArrow(v1, ArrDir, CutColor);
	CGL_Utils::DrawArrow(v2, ArrDir, CutColor);
	CGL_Utils::DrawArrow(v12, ArrDir, CutColor);
	CGL_Utils::DrawArrow(v21, ArrDir, CutColor);
}

void CQDM_Edit::Draw2D(bool FastMode)
{

//	int LayersBack = 2;
	float ThisOpacity = 0.5 + 0.3f/FadeAwayDist;

	DrawMe(FastMode, false, FadeAwayDist);

//	glDisable(GL_LIGHTING);
//
//	if (LayersBack > 0){ //if we want to fade out layers behind the section layer
//		if (!CurSecFromNeg){
//			for (int i = -(LayersBack-1); i<=0 ; i++){ Draw2DTransLayer(i, ThisOpacity);}
//		}
//		if (CurSecFromNeg){
//			for (int i = (LayersBack-1); i>=0 ; i--){ Draw2DTransLayer(i, ThisOpacity);}
//		}
//	}
//	glEnable(GL_LIGHTING);

	Draw2DOverlay();

}

/*
void CQDM_Edit::Draw2DTransLayer(int LayerOffset, float Opacity)
{
	int Layer = CurSecLayer + LayerOffset;
	float Pad = -1.0; //static padding
	Vec3D Workspace = GetWorkSpace();

	glColor4f(1.0, 1.0, 1.0, Opacity);
	//draw fadeout planes:
	glBegin(GL_TRIANGLE_FAN);

	switch (CurSecAxis){
		case AXIS_Z:{
			if (Layer < 0 || Layer >= GetVZDim()) break;
			Vec3D Center = GetXYZ(0,0,Layer);
			glVertex3d(-Pad, -Pad, Center.z);
			glVertex3d(-Pad, Workspace.y+Pad, Center.z);
			glVertex3d(Workspace.x+Pad, Workspace.y+Pad, Center.z);
			glVertex3d(Workspace.x+Pad, -Pad, Center.z);
			break;
		}
		case AXIS_Y:{
			if (Layer < 0 || Layer >= GetVYDim()) break;
			Vec3D Center1 = GetXYZ(0,Layer,0);
			Vec3D Center2 = GetXYZ(0,Layer,1);
			Vec3D Center3 = GetXYZ(0,Layer,2);
			double planeDepth;
			if (!CurSecFromNeg) planeDepth = min(min(Center1.y, Center2.y), Center3.y);
			else planeDepth = max(max(Center1.y, Center2.y), Center3.y);

			glVertex3d(-Pad, planeDepth, -Pad);
			glVertex3d(-Pad, planeDepth, Workspace.z+Pad);
			glVertex3d(Workspace.x+Pad, planeDepth, Workspace.z+Pad);
			glVertex3d(Workspace.x+Pad, planeDepth, -Pad);
			break;
		}
		case AXIS_X:{
			if (Layer < 0 || Layer >= GetVXDim()) break;
			Vec3D Center1 = GetXYZ(Layer,0,0);
			Vec3D Center2 = GetXYZ(Layer,0,1);
			Vec3D Center3 = GetXYZ(Layer,0,2);
			double planeDepth;
			if (!CurSecFromNeg) planeDepth = min(min(Center1.x, Center2.x), Center3.x);
			else planeDepth = max(max(Center1.x, Center2.x), Center3.x);

			glVertex3d(planeDepth, -Pad, -Pad);
			glVertex3d(planeDepth, -Pad, Workspace.z+Pad);
			glVertex3d(planeDepth, Workspace.y+Pad, Workspace.z+Pad);
			glVertex3d(planeDepth, Workspace.y+Pad, -Pad);
			break;
		}
	}


	glEnd();

}
*/

void CQDM_Edit::Draw2DOverlay()
{
	glPushMatrix();

	Vec3D<> Center;
	Vec3D<> Normal;
	int x, y, z;
	double OverlayAdvance = Lattice.GetLatticeDim()/1.95 ; //amount to draw the outlines above the actual voxel center point
//	if (Voxel.GetVoxName() == VS_SPHERE) OverlayAdvance /= 2.0; //don't want to offset too much for sphere to deal with layers in multiple planes

	switch (CurSecAxis){
		case AXIS_X:
			Normal = Vec3D<>(1.0, 0.0, 0.0); 
			if (CurSecFromNeg) glTranslated(-OverlayAdvance, 0, 0);
			else glTranslated(OverlayAdvance, 0, 0);
			break;
		case AXIS_Y:
			Normal = Vec3D<>(0.0, 1.0, 0.0); 
			if (CurSecFromNeg) glTranslated(0, -OverlayAdvance, 0);
			else glTranslated(0, OverlayAdvance, 0);
			break;
		default: //case AXIS_Z: 
			Normal = Vec3D<>(0.0, 0.0, 1.0); 
			if (CurSecFromNeg) glTranslated(0, 0, -OverlayAdvance);
			else glTranslated(0, 0, OverlayAdvance);
			break;
	}

	for (int i = 0; i<Structure.GetArraySize(); i++) //go through all the voxels...
	{
		GetXYZNom(&x, &y, &z, i);
		switch (CurSecAxis){
			case AXIS_Z: if (z != CurSecLayer) continue; break;
			case AXIS_Y: if (y != CurSecLayer) continue; break;
			case AXIS_X: if (x != CurSecLayer) continue; break;
		}

		GetXYZ(&Center, i);
		bool DrawHighlighted = false;
		for (int j=0; j<(int)CurHighlighted.size(); j++){
			if (i==CurHighlighted[j]){ 
				glColor4f(1.0f, 0.0f, 0.0f, 0.1f); 
				DrawHighlighted = true;
				continue;
			}
		}
		Voxel.DrawVoxel2D(&Center, Lattice.GetLatticeDim(), &Normal, DrawHighlighted); //draw this voxel if we got this far!
	}

	glPopMatrix();
	
}


void CQDM_Edit::LayerBack(void)
{
	if (CurSecFromNeg) CurSecLayer--;
	else CurSecLayer++;

	EnforceLayer();

	emit UpdateGLWindows();
}

void CQDM_Edit::LayerForward(void)
{
	if (!CurSecFromNeg) CurSecLayer--;
	else CurSecLayer++;

	EnforceLayer();

	emit UpdateGLWindows();

}

void CQDM_Edit::SetSectionView(bool State)
{
	if (State){
		ViewSection = true;
		CurSecAxis = AXIS_Z;
		CurSecFromNeg = false;
		CurSecLayer = 0;
	}
	else ViewSection = false;
}

void CQDM_Edit::EnforceLayer()
{
	//keep within bounds
	if (CurSecLayer < 0) CurSecLayer = 0;
	if (CurSecAxis == AXIS_X && CurSecLayer >= GetVXDim()) CurSecLayer = GetVXDim()-1;
	if (CurSecAxis == AXIS_Y && CurSecLayer >= GetVYDim()) CurSecLayer = GetVYDim()-1;
	if (CurSecAxis == AXIS_Z && CurSecLayer >= GetVZDim()) CurSecLayer = GetVZDim()-1;
}



void CQDM_Edit::FillHighlighted(bool CtrlDown) //fill the CurHighlighted array based on current drawing tool and coordinates
{
	switch (CurDrawTool){
		case DT_PEN:{
			int ToAdd = V2DFindVoxelUnder(CurCoord);
			if (!IsInHighlighted(ToAdd) && ToAdd != -1) CurHighlighted.push_back(ToAdd);
			break;
		}
		case DT_BOX: { //could be shortened a little...
			Vec3D<> Loc;
			int x, y, z;
			Vec3D<> BB = Vec3D<>(Lattice.GetXDimAdj(), Lattice.GetYDimAdj(), Lattice.GetZDimAdj())*GetLatticeDim()/2;

			Vec3D<> V1 = CurCoord.Min(DownCoord);
			Vec3D<> V2 = CurCoord.Max(DownCoord);

			CurHighlighted.clear();
			for (int i=0; i<Structure.GetArraySize(); i++){ //go through all voxels...
				GetXYZNom(&x, &y, &z, i);
				Loc = GetXYZ(i);
				//float LatDim = GetLatticeDim();
				switch(CurSecAxis){
					case AXIS_Z:
						if (CurSecLayer != z) continue; //don't care if its not in this layer...
						if (Loc.x>V1.x-BB.x && Loc.x<V2.x+BB.x && Loc.y>V1.y-BB.y && Loc.y<V2.y+BB.y) CurHighlighted.push_back(i);
					break;
					case AXIS_Y:
						if (CurSecLayer != y) continue; //don't care if its not in this layer...
						if (Loc.x>V1.x-BB.x && Loc.x<V2.x+BB.x && Loc.z>V1.z-BB.z && Loc.z<V2.z+BB.z) CurHighlighted.push_back(i);
					break;
					case AXIS_X:
						if (CurSecLayer != x) continue; //don't care if its not in this layer...
						if (Loc.y>V1.y-BB.y && Loc.y<V2.y+BB.y && Loc.z>V1.z-BB.z && Loc.z<V2.z+BB.z) CurHighlighted.push_back(i);
					break;
				}
			}
			break;
		}
		case DT_ELLIPSE: {
			Vec3D<> Offset, Off2, Size2;
			int x, y, z;
			Vec3D<> V1 = CurCoord.Min(DownCoord);
			Vec3D<> V2 = CurCoord.Max(DownCoord);

			int VoxN = V2DFindVoxelUnder(V1); //rounds to outside corners of voxels we've selected
			int VoxP = V2DFindVoxelUnder(V2);
			Vec3D<> LocN = GetXYZ(VoxN)-GetLatDimEnv()/2.0;
			Vec3D<> LocP = GetXYZ(VoxP)+GetLatDimEnv()/2.0;
			Vec3D<> Cen = (LocN+LocP)/2.0;
			Vec3D<> Size = (LocP-LocN)/2.0;

			CurHighlighted.clear();
			for (int i=0; i<Structure.GetArraySize(); i++){ //go through all voxels...
				GetXYZNom(&x, &y, &z, i);
				Offset = GetXYZ(i)-Cen;
				Off2 = Offset.Scale(Offset);
				Size2 = Size.Scale(Size);

				double LatDim = GetLatticeDim();
				switch(CurSecAxis){
					case AXIS_Z:
						if (CurSecLayer != z) continue; //don't care if its not in this layer...
						if (Off2.x/Size2.x + Off2.y/Size2.y<1.0001) CurHighlighted.push_back(i);
					break;
					case AXIS_Y:
						if (CurSecLayer != y) continue; //don't care if its not in this layer...
						if (Off2.x/Size2.x + Off2.z/Size2.z<1.0001) CurHighlighted.push_back(i);
					break;
					case AXIS_X:
						if (CurSecLayer != x) continue; //don't care if its not in this layer...
						if (Off2.y/Size2.y + Off2.z/Size2.z<1.0001) CurHighlighted.push_back(i);
					break;
				}
			}
			break;
		}
		case DT_BUCKET: {
			int CVox = V2DFindVoxelUnder(CurCoord);
			int ThisMat = GetMat(CVox);
			CurHighlighted.clear();
			CurHighlighted.push_back(CVox); //put the first index on the buffer...
//			if (ViewSection)

			int Iter = 0;
			int Xi, Yi, Zi, curInd, tmpInd;
			while (Iter != (int)CurHighlighted.size()){
				curInd = CurHighlighted[Iter];
				GetXYZNom(&Xi, &Yi, &Zi, curInd); //this location...

				
				for (int i=-1; i<=1; i++){for (int j=-1; j<=1; j++){for (int k=-1; k<=1; k++){
					if (i!=0 && !CtrlDown && (ViewSection && CurSecAxis == AXIS_X)) continue; //if we are out of plane of current viewing section
					if (j!=0 && !CtrlDown&& (ViewSection && CurSecAxis == AXIS_Y)) continue;
					if (k!=0 && !CtrlDown && (ViewSection && CurSecAxis == AXIS_Z)) continue;

					tmpInd = GetIndex(Xi+i, Yi+j, Zi+k);
					if (IsAdjacent(curInd, tmpInd, true) && GetMat(tmpInd) == ThisMat && !IsInHighlighted(tmpInd)) CurHighlighted.push_back(tmpInd);
				}}}

				Iter++;
			}

		}
	}
}

void CQDM_Edit::ExtractCurLayer(CVXC_Structure* pOutput) //returns structure with ZDim of 1 of current slice...
{
	if (!pOutput) return; //Also, some way to see if its a garbage pointer
	int NumX, NumY;

	switch(CurSecAxis){
		case AXIS_Z:
			NumX = GetVXDim();
			NumY = GetVYDim();
			pOutput->CreateStructure(NumX, NumY, 1);
			for (int i=0; i<NumX; i++){for (int j=0; j<NumY; j++){
				pOutput->SetData(pOutput->GetIndex(i, j, 0), Structure.GetData(Structure.GetIndex(i, j, CurSecLayer)));
			}}
		break;
		case AXIS_Y:
			NumX = GetVXDim();
			NumY = GetVZDim();
			pOutput->CreateStructure(NumX, NumY, 1);
			for (int i=0; i<NumX; i++){for (int j=0; j<NumY; j++){
				pOutput->SetData(pOutput->GetIndex(i, j, 0), Structure.GetData(Structure.GetIndex(i, CurSecLayer, j)));
			}}
		break;
		case AXIS_X:
			NumX = GetVYDim();
			NumY = GetVZDim();
			pOutput->CreateStructure(NumX, NumY, 1);
			for (int i=0; i<NumX; i++){for (int j=0; j<NumY; j++){
				pOutput->SetData(pOutput->GetIndex(i, j, 0), Structure.GetData(Structure.GetIndex(CurSecLayer, i, j)));
			}}
		break;
	}
}

void CQDM_Edit::ImposeLayerCur(CVXC_Structure* pInput) //pastes the z=0 layer of this structure onto current slice
{
	if (!pInput) return; //Also, some way to see if its a garbage pointer?
	int NumX = pInput->GetVXDim();
	int NumY = pInput->GetVYDim();
	int tmpInd;

	switch(CurSecAxis){
		case AXIS_Z:
			for (int i=0; i<NumX; i++){ for (int j=0; j<NumY; j++){
				tmpInd = Structure.GetIndex(i, j, CurSecLayer);
				if (tmpInd!=-1) Structure.SetData(tmpInd, pInput->GetData(pInput->GetIndex(i, j, 0)));
			}}
		break;
		case AXIS_Y:
			for (int i=0; i<NumX; i++){ for (int j=0; j<NumY; j++){
				tmpInd = Structure.GetIndex(i, CurSecLayer, j);
				if (tmpInd!=-1) Structure.SetData(tmpInd, pInput->GetData(pInput->GetIndex(i, j, 0)));
			}}
		break;
		case AXIS_X:
			for (int i=0; i<NumX; i++){ for (int j=0; j<NumY; j++){
				tmpInd = Structure.GetIndex(CurSecLayer, i, j);
				if (tmpInd!=-1) Structure.SetData(tmpInd, pInput->GetData(pInput->GetIndex(i, j, 0)));
			}}
		break;
	}
}
