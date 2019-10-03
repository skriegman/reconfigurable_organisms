/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef VX_SIMGLUTILS_H
#define VX_SIMGLUTILS_H

#include "VX_Sim.h"

#ifdef QT_GUI_LIB
#include <qgl.h>
#else
#include "OpenGLInclude.h" //If not using QT's openGL system, make a header file "OpenGLInclude.h" that includes openGL library functions 
#endif
#include <iostream>

//! Defines the type of view
enum ViewMode {
	RVM_NONE, //!< Disable drawing for maximum simulation speed
	RVM_VOXELS, //!< Draws the actual voxels of the simulation
	RVM_BONDS //!< Draws the bonds underlying the simulation
};
//! Defines the color to display each voxel
enum ViewColor {
	RVC_TYPE, //!< Defined material color for voxels, type of bond for bonds
	RVC_KINETIC_EN, //!< Kinetic energy
	RVC_DISP, //!< Color coded displacement from inital position.
	RVC_STATE, //Color coded information about the state of each voxel (reserved for future implementation)
	RVC_STRAIN_EN, //!< Strain energy
	RVC_STRAIN, //!< Color coded engineering srain, or internal deformation percent.
	RVC_STRESS, //!< Color coded engineering stress, or internal force
	RVC_PRESSURE, //!< Color coded volumetric pressure
	RVC_STIFFNESS //!< Color coded stiffness
};
//! Defines the way  to display each voxel
enum ViewVoxel {
	RVV_DISCRETE, //!< Draws discrete voxels with position and orientation
	RVV_DEFORMED, //!< Draws deformed voxels
	RVV_SMOOTH //!< Draws a smooth mesh, whether provided or generated with marching cubes
};

//! Defines which vectors to plot in VoxCad
enum PlotVectors {
	PLOT_DRAG, 
	PLOT_SPEEDS,
	PLOT_NORMALS 
};

class CVXS_SimGLView
{
public:
	CVXS_SimGLView(CVX_Sim* pSimIn); //!< Constructor, links to simulation
	~CVXS_SimGLView(void); //!< Destructor
	CVXS_SimGLView& operator=(const CVXS_SimGLView& rGlView); //!< Overload "=" 


	CVX_Sim* pSim;

	void Draw(int Selected = -1, bool ViewSection=false, int SectionLayer=0);
//	void DrawOverlay(void);

	void SetCurViewMode(ViewMode CurViewModeIn){CurViewMode=CurViewModeIn; NeedStatsUpdate=true;}
	void SetCurViewCol(ViewColor CurViewColIn){CurViewCol=CurViewColIn; NeedStatsUpdate=true;}
	void SetCurViewVox(ViewVoxel CurViewVoxIn){CurViewVox=CurViewVoxIn;}
	void SetVectorsScalingView(float valueIn){  vectorsScalingView = valueIn; }
	float GetVectorsScalingView(){return vectorsScalingView;}
	void SetViewForce(PlotVectors curVectPlotIn){ curVectPlot = curVectPlotIn; }
	void SetPlottingForces(bool plotIn){ plottingForces = plotIn; }

	ViewMode GetCurViewMode(void) {return CurViewMode;}
	ViewColor GetCurViewCol(void) {return CurViewCol;}
	ViewVoxel GetCurViewVox(void) {return CurViewVox;}

	void SetViewForce(bool Enabled) {ViewForce=Enabled;}
	void SetViewAngles(bool Enabled) {ViewAngles=Enabled;}
	bool GetViewForce() {return ViewForce;}
	bool GetViewAngles() {return ViewAngles;}

	int StatRqdToDraw(); //returns the stats bitfield that we need to calculate to draw the current view.

//move these to private eventually
	CVX_MeshUtil SmoothMesh; //marching cubes-generated mesh
	CVX_MeshUtil VoxMesh; //deformable voxel geometry mesh (generated internally)
	bool NeedStatsUpdate; //do we need to re-calculate statistics relevant to the opengl view? i.e. maximums...
	CColor GetJet(vfloat val){if (val > 0.75) return CColor(1, 4-val*4, 0, 1.0);	else if (val > 0.5) return CColor(val*4-2, 1, 0, 1.0); else if (val > 0.25) return CColor(0, 1, 2-val*4, 1.0); else return CColor(0, val*4, 1, 1.0);};
	CColor GetCurVoxColor(int SIndex, int Selected);

	void ChangeSkyColor(float r, float g, float b);

	CColor skyColor;
private:

	PlotVectors curVectPlot;
	float vectorsScalingView;
	bool plottingForces;

	bool ViewForce; //look at force vectors?
	bool ViewAngles; //look at axes for each point?



	CColor GetInternalBondColor(CVXS_BondInternal* pBond);
	CColor GetCollisionBondColor(CVXS_BondCollision* pBond);


	//Drawing
	void DrawGeometry(int Selected = -1, bool ViewSection=false, int SectionLayer=0, vfloat ScaleVox = 1.0);
	void DrawSurfMesh(int Selected = -1);
	void DrawVoxMesh(int Selected = -1);
	void DrawVoxHandles(int Selected = -1);
	void DrawFloor();
	void DrawBonds();
	void DrawForce();
	void DrawAngles();
	void DrawStaticFric();



	ViewMode CurViewMode;
	ViewColor CurViewCol;
	ViewVoxel CurViewVox;
};

#endif //VX_SIMGLUTILS_H