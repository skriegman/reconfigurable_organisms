#ifndef QDM_EDIT_H
#define QDM_EDIT_H

#include "QVX_Interfaces.h"

enum DrawTool {DT_PEN, DT_BOX, DT_ELLIPSE, DT_BUCKET}; //current drawing tool for modifying the 2D view

class CQDM_Edit : public QVX_Object
{
	Q_OBJECT

public:
	CQDM_Edit();
	~CQDM_Edit();

signals:
	void UpdateGLWindows(void);
	void GetCurMaterial(int* MatIndex);
	void GetCurGLSelected(int* CurSel);
	void ModelChanged(void); //whenever the model (voxels) get edited

public slots:
	void DrawSceneView(bool FastMode = false) {Draw3D(FastMode);}
	void DrawSceneEdit(bool FastMode = false) {Draw2D(FastMode);}

//	void SelectedNew(int index);

	//mouse handlers:
	void HoverMove(Vec3D<> P);
	void LMouseDown(Vec3D<> P, bool IsCtrlDown);
	void LMouseUp(Vec3D<> P);
	void LMouseDownMove(Vec3D<> P);
	void PressedEscape(void);
	void CtrlMouseRoll(bool Positive) {if (Positive) LayerForward(); else LayerBack();};

	void SetV2DTop(void) {CurSecAxis = AXIS_Z; CurSecFromNeg = false; EnforceLayer();};
	void SetV2DBottom(void) {CurSecAxis = AXIS_Z; CurSecFromNeg = true; EnforceLayer();};
	void SetV2DLeft(void) {CurSecAxis = AXIS_Y; CurSecFromNeg = true; EnforceLayer();};
	void SetV2DRight(void) {CurSecAxis = AXIS_Y; CurSecFromNeg = false; EnforceLayer();};
	void SetV2DFront(void) {CurSecAxis = AXIS_X; CurSecFromNeg = false; EnforceLayer();};
	void SetV2DBack(void) {CurSecAxis = AXIS_X; CurSecFromNeg = true; EnforceLayer();};

	void LayerBack(void);
	void LayerForward(void);

	void SetDrawPencil(void) {CurDrawTool = DT_PEN;};
	void SetDrawRectangle(void) {CurDrawTool = DT_BOX;};
	void SetDrawCircle(void) {CurDrawTool = DT_ELLIPSE;};
	void SetDrawBucket(void) {CurDrawTool = DT_BUCKET;};

//	void GetCurIndex(int* CurIndex) {*CurIndex = CurSelected;};


public:
//	void DrawFast(int Selected = -1, bool FastMode = false); //Draws the entire object, but very efficiently (use for non-editing large models) No picking, no transparency, composite materials not evaluated.

	void DrawMe(bool FastMode = false, bool ShowSelected = true, int NumBehindSection = LAYERMAX);

	void Draw3D(bool FastMode = false);
	void Draw2D(bool FastMode = false);

//	void DrawSecX(int Layer, int NumBehind = LAYERMAX, bool FromNeg = false, int Selected = -1);
//	void DrawSecY(int Layer, int NumBehind = LAYERMAX, bool FromNeg = false, int Selected = -1);
//	void DrawSecZ(int Layer, int NumBehind = LAYERMAX, bool FromNeg = false, int Selected = -1);

//	void DrawXLayer(int Layer, int Selected = -1, double FadeOutPerc = 0.0); //draws a single X (YZ) plane layer faded to the specified percentage of the background r, g, b (BG_R, BG_G, BG_B)
//	void DrawYLayer(int Layer, int Selected = -1, double FadeOutPerc = 0.0); //draws a single Y (XZ) plane layer faded to the specified percentage of the background r, g, b (BG_R, BG_G, BG_B)
//	void DrawZLayer(int Layer, int Selected = -1, double FadeOutPerc = 0.0); //draws a single Z (XY) plane layer faded to the specified percentage of the background r, g, b (BG_R, BG_G, BG_B)

	void SetCurGLColor(CVXC_Material* pMat, bool Selected, float FadeOutPerc = 0.0);


//	void Draw2DTransLayer(int LayerOffset, float Opacity = 0.7f); //opacity zero to 1 range
	void Draw2DOverlay();
	void DrawSectionPlane(bool FastMode = false);

	void SetSectionView(bool State = true);
	bool ViewSection; //flag that determines whether we are viewing a section as defined by Axis, Layer, FromNeg
	Axis CurSecAxis;
	int CurSecLayer;
	bool CurSecFromNeg;
	bool ViewTiled; //display multiple unit cells?
	void SetTiledView(bool WantTiled) {ViewTiled = WantTiled;}

	//to implement:
	int FadeAwayDist; //when veiwing a section, how many layers behind do we want to see faded out...
	//bool

	void EnforceLayer(); //makes sure current layer is valid given the axis...
	int GetCurSel(void){int tmp; emit GetCurGLSelected(&tmp); return tmp;};
	DrawTool GetCurDrawTool(void) {return CurDrawTool;};

	void ExtractCurLayer(CVXC_Structure* pOutput); //returns structure with ZDim of 1 of current slice...
	void ImposeLayerCur(CVXC_Structure* pInput); //pastes the z=0 layer of this structure onto current slice

private:
//	int CurSelected; //index of currently selected voxel
	std::vector<int> CurHighlighted; //index of voxel we are hovering over (currently used in 2D view only...)
	int V2DFindVoxelUnder(Vec3D<> Coord); //returns the index of a voxel under the coordinates
	bool IsInHighlighted(int index) {for (int i=0; i<(int)CurHighlighted.size(); i++) if (CurHighlighted[i] == index) return true; return false;} //returns true if this voxel is already in the highlighted array...

	bool V2DHighlightMe(int index); //highlight this voxel in 2D overlay layer?
	bool LMBDown; //flag for whether out left mouse button is down or not...
	Vec3D<> CurCoord; //current mouse coords in real units (not pixels)
	Vec3D<> DownCoord; //mouse coordinates in real numbers where mouse was pressed down

//	float CurMCX, CurMCY; //current mouse coords in real units (not pixels)
//	float DownMCX, DownMCY; //mouse coordinates in real numbers where mouse was pressed down

	DrawTool CurDrawTool;
	void FillHighlighted(bool CtrlDown = false); //fill the CurHighlighted array based on current drawing tool and coodrinates
};

#endif // QDM_EDIT_H
