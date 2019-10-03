/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller (Cornell University)
If used in publication cite "J. Hiller and H. Lipson "Dynamic Simulation of Soft Heterogeneous Objects" In press. (2011)"

This file is part of Voxelyze.
Voxelyze is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
Voxelyze is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#include "VXS_SimGLView.h"
#include "Utils/GL_Utils.h"
#include <iostream>

CVXS_SimGLView::CVXS_SimGLView(CVX_Sim* pSimIn)
{
	pSim = pSimIn;

	CurViewMode = RVM_VOXELS;
	CurViewCol = RVC_TYPE;
	CurViewVox = RVV_DEFORMED;

	ViewForce = false;
	ViewAngles = false;

	NeedStatsUpdate=true;
	vectorsScalingView = 5.0;
	plottingForces = false;

	skyColor = CColor(0.52,0.80,1.0);

}

CVXS_SimGLView::~CVXS_SimGLView(void)
{
}

CVXS_SimGLView& CVXS_SimGLView::operator=(const CVXS_SimGLView& rGlView) //overload "=" 
{
	pSim = rGlView.pSim;
	NeedStatsUpdate = rGlView.NeedStatsUpdate;
	ViewForce = rGlView.ViewForce;
	ViewAngles = rGlView.ViewAngles;
	CurViewMode = rGlView.CurViewMode;
	CurViewCol = rGlView.CurViewCol;
	CurViewVox = rGlView.CurViewVox;

	return *this;
}

void CVXS_SimGLView::Draw(int Selected, bool ViewSection, int SectionLayer)
{
	if (!pSim->IsInitalized()) return;

	if (CurViewMode == RVM_NONE) return;
	else if (CurViewMode == RVM_VOXELS){ 
		switch (CurViewVox){
		case RVV_DISCRETE: DrawGeometry(Selected, ViewSection, SectionLayer); break; //section view only currently enabled in voxel view mode
		case RVV_DEFORMED: DrawVoxMesh(Selected); break;
		case RVV_SMOOTH: DrawSurfMesh(); break;
		}
	}
	else { //CurViewMode == RVT_BONDS
		vfloat VoxScale=0.2;
		if (ViewForce){
			DrawForce();
			DrawGeometry(Selected, ViewSection, SectionLayer, VoxScale);

		}
		else {
			if (CurViewVox == RVV_SMOOTH) VoxScale=0.1;
			DrawBonds();
			DrawGeometry(Selected, ViewSection, SectionLayer, VoxScale);
		}
		DrawStaticFric();
	}
	if (ViewAngles)	DrawAngles();
	if (pSim->IsFeatureEnabled(VXSFEAT_FLOOR)) DrawFloor(); //draw the floor if its in use
//	if (pSim->IsFeatureEnabled(VXSFEAT_FLOOR) and not pSim->fluidEnvironment) DrawFloor(); //draw the floor if its in use
	
	if(pSim->fluidEnvironment) // Let's color the environment if we are in water!
//		glClearColor(0.0, 0.5, 1.0, 1.0f);
		skyColor = CColor(0.0, 0.5, 1.0, 1.0f);
	else
		skyColor = CColor(0.52,0.80,1.0);

    glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);
    //glClearColor(0.8, 1.0, 1.0, 1.0f);

	// if (pSim->IsFeatureEnabled(VXSFEAT_FLOOR)) DrawFloor(); //draw the floor if its in use
//	if (pEnv->IsFloorEnabled()) DrawFloor(); //draw the floor if its in use

	NeedStatsUpdate=true;
}

void CVXS_SimGLView::DrawForce(void)
{
	CVXS_Voxel* pVox;

	float PrevLineWidth;
	glGetFloatv(GL_LINE_WIDTH, &PrevLineWidth);
	glLineWidth(1.0);
	glDisable(GL_LIGHTING);

	vfloat MaxForce = 0;
	int iT = pSim->NumBond();
	int NumVox = pSim->NumVox();
	if (NumVox <=0) return;
	vfloat VSize = pSim->VoxArray[0].GetNominalSize();

	for (int i = 0; i<iT; i++){ //go through all the bonds...
		vfloat ThisMag = pSim->BondArrayInternal[i].GetForce1().Length();
		if (ThisMag > MaxForce) MaxForce = ThisMag;
	}

	vfloat ForceScale = 0.3*VSize/MaxForce; //Vox size / max Force

//	int x, y, z;
	glBegin(GL_LINES);
	glLoadName (-1); //to disable picking
	for (int i = 0; i<NumVox; i++) //go through all the voxels...
	{
		//pSim->pEnv->pObj->GetXYZNom(&x, &y, &z, pSim->StoXIndexMap[i]);
		//if (ViewSection && z>SectionLayer) continue; //exit if obscured in a section view!

		pVox = &pSim->VoxArray[i];

		Vec3D<> Center = pVox->GetCurPos();
		CQuat<> Angle = pVox->GetCurAngle();
		Vec3D<> PointToDrawFrom;
		for (int i=0; i<6; i++) //for each potential bond
		{
			switch (i){
			case BD_PX: PointToDrawFrom = Center + Angle.RotateVec3D(Vec3D<>(0.2*VSize, 0, 0)); break;
			case BD_NX: PointToDrawFrom = Center + Angle.RotateVec3D(Vec3D<>(-0.2*VSize, 0, 0)); break;
			case BD_PY: PointToDrawFrom = Center + Angle.RotateVec3D(Vec3D<>(0, 0.2*VSize, 0)); break;
			case BD_NY: PointToDrawFrom = Center + Angle.RotateVec3D(Vec3D<>(0, -0.2*VSize, 0)); break;
			case BD_PZ: PointToDrawFrom = Center + Angle.RotateVec3D(Vec3D<>(0, 0, 0.2*VSize)); break;
			case BD_NZ: PointToDrawFrom = Center + Angle.RotateVec3D(Vec3D<>(0, 0, -0.2*VSize)); break;
			};

			CVXS_BondInternal* pBond = pVox->GetpInternalBond((BondDir)i);
			if (pBond){
				glColor4d(1.0, 0.0, 0.0, 1.0); //red
				Vec3D<> PointToDrawTo;
	
				//Total Force
				if (pVox->IAmInternalVox2(i)) PointToDrawTo = PointToDrawFrom+ForceScale*pBond->GetForce2();
				else PointToDrawTo = PointToDrawFrom+ForceScale*pBond->GetForce1();
				CGL_Utils::DrawLineArrowD(PointToDrawFrom, PointToDrawTo, 1.0, CColor(1, 0, 0)); //Red

				//Axial Force
				if (pVox->IAmInternalVox2(i)) PointToDrawTo = PointToDrawFrom+ForceScale*pBond->AxialForce2;
				else PointToDrawTo = PointToDrawFrom+ForceScale*pBond->AxialForce1;
				CGL_Utils::DrawLineArrowD(PointToDrawFrom, PointToDrawTo, 1.0, CColor(0.2, 0.2, 0.7)); //Blue

				//Bending Force
				if (pVox->IAmInternalVox2(i)) PointToDrawTo = PointToDrawFrom+ForceScale*pBond->BendingForce2;
				else PointToDrawTo = PointToDrawFrom+ForceScale*pBond->BendingForce1;
				CGL_Utils::DrawLineArrowD(PointToDrawFrom, PointToDrawTo, 1.0, CColor(0.2, 0.7, 0.2)); //Green

				//Shear Force
				if (pVox->IAmInternalVox2(i)) PointToDrawTo = PointToDrawFrom+ForceScale*pBond->ShearForce2;
				else PointToDrawTo = PointToDrawFrom+ForceScale*pBond->ShearForce1;
				CGL_Utils::DrawLineArrowD(PointToDrawFrom, PointToDrawTo, 1.0, CColor(0.7, 0.2, 0.7)); //Purple

			}
		}
	}
	glEnd();


	glLineWidth(PrevLineWidth);
	glEnable(GL_LIGHTING);

}

void CVXS_SimGLView::DrawFloor(void)
{

	//TODO: build an openGL list 
	vfloat Size = pSim->LocalVXC.GetLatticeDim()*4;
	vfloat sX = 1.5*Size;
	vfloat sY = .866*Size;

	glEnable(GL_LIGHTING);

	glLoadName (-1); //never want to pick floor

	vfloat floorHeight = 0.0; // should fix shadow rendering
	vfloat floorSlope = pSim->pEnv->GetFloorSlope()*PI/180.0;


	//std::cout << "Floor slope is " << floorSlope << std::endl;

	glNormal3d(0.0, 0.0, 1.0);
	for (int i=-20; i <=30; i++){
		for (int j=-40; j <=60; j++){
			
//			glColor4d(0.6, 0.7+0.2*((int)(1000*sin((float)(i+110)*(j+106)*(j+302)))%10)/10.0, 0.6, 1.0);
			 glColor4d(1.0, 1.0, 1.0, 1.0);
			// glColor4d(1.0, 0.8, 0.6, 1.0);

			glBegin(GL_TRIANGLE_FAN);
			glVertex3d(i*sX, j*sY, floorHeight+((i*sX)*tan(floorSlope)));
			glVertex3d(i*sX+0.5*Size,  j*sY, floorHeight+((i*sX+0.5*Size)*tan(floorSlope)));
			glVertex3d(i*sX+0.25*Size, j*sY+0.433*Size, floorHeight+((i*sX+0.25*Size)*tan(floorSlope)));
			glVertex3d(i*sX-0.25*Size, j*sY+0.433*Size, floorHeight+((i*sX-0.25*Size)*tan(floorSlope)));
			glVertex3d(i*sX-0.5*Size,  j*sY, floorHeight+((i*sX-0.5*Size)*tan(floorSlope)));
			glVertex3d(i*sX-0.25*Size, j*sY-0.433*Size, floorHeight+((i*sX-0.25*Size)*tan(floorSlope)));
			glVertex3d(i*sX+0.25*Size, j*sY-0.433*Size, floorHeight+((i*sX+0.25*Size)*tan(floorSlope)));
			glVertex3d(i*sX+0.5*Size,  j*sY, floorHeight+((i*sX+0.5*Size)*tan(floorSlope)));
			glEnd();

			// COLORED HEXAGON FLOOR
//            glColor4d(0.6, 0.7+0.2*((int)(1000*sin((float)(i+100)*(j+103)*(j+369)))%10)/10.0, 0.6, 1.0);
			 glColor4d(1.0, 1.0, 1.0, 1.0);
			// glColor4d(1.0, 0.8, 0.6, 1.0);

			glBegin(GL_TRIANGLE_FAN);
			glVertex3d(i*sX+.75*Size, j*sY+0.433*Size, floorHeight+((i*sX+.75*Size)*tan(floorSlope)));
			glVertex3d(i*sX+1.25*Size, j*sY+0.433*Size, floorHeight+((i*sX+1.25*Size)*tan(floorSlope)));
			glVertex3d(i*sX+Size, j*sY+0.866*Size, floorHeight+((i*sX+Size)*tan(floorSlope)));
			glVertex3d(i*sX+0.5*Size, j*sY+0.866*Size, floorHeight+((i*sX+0.5*Size)*tan(floorSlope)));
			glVertex3d(i*sX+0.25*Size, j*sY+0.433*Size, floorHeight+((i*sX+0.25*Size)*tan(floorSlope)));
			glVertex3d(i*sX+0.5*Size, j*sY, floorHeight+((i*sX+0.5*Size)*tan(floorSlope)));
			glVertex3d(i*sX+Size, j*sY, floorHeight+((i*sX+Size)*tan(floorSlope)));
			glVertex3d(i*sX+1.25*Size, j*sY+0.433*Size, floorHeight+((i*sX+1.25*Size)*tan(floorSlope)));
			glEnd();
		}
	}
}

void CVXS_SimGLView::DrawGeometry(int Selected, bool ViewSection, int SectionLayer, vfloat ScaleVox)
{
	Vec3D<> Center;
	Vec3D<> tmp(0,0,0);

	int iT = pSim->NumVox();
	int x, y, z;
	CColor ThisColor;
	for (int i = 0; i<iT; i++) //go through all the voxels...
	{
		pSim->pEnv->pObj->GetXYZNom(&x, &y, &z, pSim->StoXIndexMap[i]);
		if (ViewSection && z>SectionLayer) continue; //exit if obscured in a section view!


		Center = pSim->VoxArray[i].GetCurPos();

		ThisColor = GetCurVoxColor(i, Selected);
		glColor4d(ThisColor.r, ThisColor.g, ThisColor.b, ThisColor.a);
		
		// nac: debug: color surface voxels black
		// if (pSim->VoxArray[i].IsSurfaceVoxel())
		// {
		// 	glColor4d(0.0, 0.0, 0.0, 1.0);
		// }
		
		Vec3D<> CenterOff = ScaleVox*(pSim->VoxArray[i].GetCornerPos() + pSim->VoxArray[i].GetCornerNeg())/2;


		glPushMatrix();
		glTranslated(Center.x + CenterOff.x, Center.y + CenterOff.y, Center.z + CenterOff.z);

		glLoadName (pSim->StoXIndexMap[i]); //to enable picking

		//generate rotation matrix here!!! (from quaternion)
		Vec3D<> Axis;
		vfloat AngleAmt;
		CQuat<>(pSim->VoxArray[i].GetCurAngle()).AngleAxis(AngleAmt, Axis);
		glRotated(AngleAmt*180/3.1415926, Axis.x, Axis.y, Axis.z);
	
		Vec3D<> CurrentSizeDisplay = pSim->VoxArray[i].GetSizeCurrent();
		glScaled(CurrentSizeDisplay.x, CurrentSizeDisplay.y, CurrentSizeDisplay.z); 

		pSim->LocalVXC.Voxel.DrawVoxel(&Center, ScaleVox); //draw unit size since we scaled just now
		
		glPopMatrix();
	}

	// //------------------------------------------------------------------------------------------------
	// // nac: draw shadow:
	// for (int i = 0; i<iT; i++) //go through all the voxels...
	// {
	// 	pSim->pEnv->pObj->GetXYZNom(&x, &y, &z, pSim->StoXIndexMap[i]);
	// 	if (ViewSection && z>SectionLayer) continue; //exit if obscured in a section view!


	// 	Center = pSim->VoxArray[i].GetCurPos();

	// 	ThisColor = GetCurVoxColor(i, Selected);
	// 	glColor4d(ThisColor.r, ThisColor.g, ThisColor.b, ThisColor.a);
		
	// 	// nac: debug: color surface voxels black
	// 	// if (pSim->VoxArray[i].IsSurfaceVoxel())
	// 	// {
	// 	// 	glColor4d(0.0, 0.0, 0.0, 1.0);
	// 	// }
		
	// 	Vec3D<> CenterOff = ScaleVox*(pSim->VoxArray[i].GetCornerPos() + pSim->VoxArray[i].GetCornerNeg())/2;


	// 	glPushMatrix();
	// 	// glTranslated(Center.x + CenterOff.x, Center.y + CenterOff.y, Center.z + CenterOff.z);
	// 	glTranslated(Center.x + CenterOff.x, Center.y + CenterOff.y, 0);

	// 	glLoadName (pSim->StoXIndexMap[i]); //to enable picking

	// 	//generate rotation matrix here!!! (from quaternion)
	// 	Vec3D<> Axis;
	// 	vfloat AngleAmt;
	// 	CQuat<>(pSim->VoxArray[i].GetCurAngle()).AngleAxis(AngleAmt, Axis);
	// 	glRotated(AngleAmt*180/3.1415926, Axis.x, Axis.y, Axis.z);
	
	// 	Vec3D<> CurrentSizeDisplay = pSim->VoxArray[i].GetSizeCurrent();
	// 	glScaled(CurrentSizeDisplay.x, CurrentSizeDisplay.y, CurrentSizeDisplay.z);

	// 	pSim->LocalVXC.Voxel.DrawVoxel(&Center, ScaleVox); //draw unit size since we scaled just now
		
	// 	glPopMatrix();
	// }
	// //------------------------------------------------------------------------------------------------

}

CColor CVXS_SimGLView::GetCurVoxColor(int SIndex, int Selected)
{
	if (pSim->StoXIndexMap[SIndex] == Selected) return CColor(1.0f, 0.0f, 1.0f, 1.0f); //highlight selected voxel (takes precedence...)

	switch (CurViewCol) {
		case RVC_TYPE:
		{
		    if (pSim->pEnv->getGrowthAmplitude() > 0)  // color difference from nominal
            {
                double maxSize = (1 + pSim->pEnv->getGrowthAmplitude())*pSim->VoxArray[SIndex].GetNominalSize();
                double minSize = pSim->getMinTempFact() * pSim->VoxArray[SIndex].GetNominalSize();
                double curSize = pSim->VoxArray[SIndex].currSize;
                double normColor = (curSize - minSize) / (maxSize - minSize);
                return GetJet(normColor);
            }
            // forward model loss
//            return GetJet(4*pSim->VoxArray[SIndex].currentForwardModelError);
//            return GetJet(pSim->VoxArray[SIndex].StressContribution);

		}
		break;
//				float R, G, B, A;
//	//			LocalVXC.GetLeafMat(VoxArray[SIndex].GetVxcIndex())->GetColorf(&R, &G, &B, &A);
//				pSim->VoxArray[SIndex].GetpMaterial()->GetColorf(&R, &G, &B, &A);
//				return CColor(R, G, B, A);
//            break;

		case RVC_KINETIC_EN: // Quickly remapped on delta stiffness  --> electricity
			//if (pSim->SS.MaxVoxKinE == 0) return GetJet(0);
			//return GetJet(pSim->VoxArray[SIndex].GetCurKineticE() / pSim->SS.MaxVoxKinE);
			//break;
//			{
//				double maxE = pSim->pEnv->pObj->Structure.GetMaxElasticMod()*0.5;
//				double minE = pSim->pEnv->pObj->Structure.GetMinElasticMod();
//				double deltaStiff = pSim->VoxArray[SIndex].GetEMod() - pSim->VoxArray[SIndex].evolvedStiffness; // current Emod - initial (evolved) Emod
//				double normE = deltaStiff / (maxE - minE); // norm w.r.t. max excursion
//				//double normE = deltaStiff / pSim->VoxArray[SIndex].evolvedStiffness; // % w.r.t. evolvedStiff
//
//				return GetJet((normE+1)/2); // bringing from [-1,1] to [0,1] for coloring
//				//return GetJet((pSim->VoxArray[SIndex].getCurStiffnessChange()+1.0)/2);//CColor(0.0f, normE+0.3f, 0.0f, 1.0f); // 1.3f-normE
//			}
            {
//                if (pSim->VoxArray[SIndex].ElectricallyActiveNew) {return CColor(1.0f, 0.0f, 0.0f, 1.0f);}
//                return CColor(0.0f, 1.0f, 1.0f, 1.0f);

                 return GetJet(pSim->VoxArray[SIndex].Voltage);

//                return GetJet(pSim->VoxArray[SIndex].PressureContribution);
//                return GetJet(pSim->VoxArray[SIndex].VestibularContribution);

            }
            break;

		case RVC_STATE:
			// if (pSim->VoxArray[SIndex].GetBroken()) return CColor(1.0f, 0.0f, 0.0f, 1.0f);
			// else if (pSim->VoxArray[SIndex].GetYielded()) return CColor(1.0f, 1.0f, 0.0f, 1.0f);
			// else return CColor(1.0f, 1.0f, 1.0f, 1.0f);
			// nac: island fitness function:
			// if (pSim->VoxArray[SIndex].inRing) { return CColor(0.5,0.0,1.0,1.0); }
			// else {return CColor(0.9, 1.0, 0.8, 1.0);}
			// if (pSim->VoxArray[SIndex].GetCurPos().z <= pSim->LocalVXC.GetLatticeDim()*.75) { return CColor(0.5,0.0,1.0,1.0); }
			// else {return CColor(0.9, 1.0, 0.8, 1.0);}
			// return CColor(pSim->VoxArray[SIndex].Scale, pSim->VoxArray[SIndex].Scale, pSim->VoxArray[SIndex].Scale, 1.0);
			if (pSim->VoxArray[SIndex].GetMaterialIndex() == 3 and pSim->CurTime > pSim->GetInitCmTime())
			{
				return CColor(0.5-(0.5*sin(2*3.1415926f * (pSim->CurTime/pSim->VoxArray[SIndex].TempPeriod + pSim->VoxArray[SIndex].phaseOffset))),0.5+(0.5*sin(2*3.1415926f * (pSim->CurTime/pSim->VoxArray[SIndex].TempPeriod + pSim->VoxArray[SIndex].phaseOffset))),0.0+(0.0*sin(2*3.1415926f * (pSim->CurTime/pSim->VoxArray[SIndex].TempPeriod + pSim->VoxArray[SIndex].phaseOffset))), 1.0);//
			}
			else
			{
				float R, G, B, A;
				pSim->VoxArray[SIndex].GetpMaterial()->GetColorf(&R, &G, &B, &A);
				return CColor(R, G, B, A);
			}
			break;

		case RVC_STRAIN:
			if (pSim->SS.MaxBondStrain == 0) return GetJet(0);
			return GetJet(pSim->VoxArray[SIndex].GetMaxBondStrain() / pSim->SS.MaxBondStrain);
			break;
		case RVC_STRESS:
			if (pSim->SS.MaxBondStress == 0) return GetJet(0);
			return GetJet(pSim->VoxArray[SIndex].GetMaxBondStress() / pSim->SS.MaxBondStress);
			break;
		case RVC_PRESSURE:{
			vfloat MaxP = pSim->SS.MaxPressure, MinP = pSim->SS.MinPressure;
			if (MaxP <= MinP) return GetJet(0);

			vfloat Mag = MaxP;
			if (-MinP>Mag) Mag = -MinP;
			//vfloat ThisP = pSim->VoxArray[SIndex].GetPressure();
			return GetJet(0.5-pSim->VoxArray[SIndex].GetPressure()/(2*Mag));
			break;
						  }

		case RVC_STIFFNESS:
		{
			double maxE = pSim->pEnv->pObj->Structure.GetMaxElasticMod()*0.5;
			double minE = pSim->pEnv->pObj->Structure.GetMinElasticMod();
			double normE = (pSim->VoxArray[SIndex].GetEMod() - minE ) / (maxE - minE);
			return GetJet(normE);//CColor(0.0f, normE+0.3f, 0.0f, 1.0f); // 1.3f-normE

			//return GetJet( pSim->VoxArray[SIndex].GetMaxBondStress() / pSim->VoxArray[SIndex].PreDamageStress );
			//return GetJet( pSim->VoxArray[SIndex].CalcVoxelPressure() / pSim->VoxArray[SIndex].PreDamagePressure );
//			return GetJet(
//                    0.33 * fabs(pSim->VoxArray[SIndex].GetRoll() / pSim->VoxArray[SIndex].PreDamageRoll) +
//                    0.33 * fabs(pSim->VoxArray[SIndex].GetPitch() / pSim->VoxArray[SIndex].PreDamagePitch) +
//                    0.33 * fabs(pSim->VoxArray[SIndex].GetYaw() / pSim->VoxArray[SIndex].PreDamageYaw)
////			    (pSim->VoxArray[SIndex].GetRoll() + pSim->VoxArray[SIndex].GetPitch() + pSim->VoxArray[SIndex].GetYaw()) /
////			    (pSim->VoxArray[SIndex].PreDamageRoll + pSim->VoxArray[SIndex].PreDamagePitch + pSim->VoxArray[SIndex].PreDamageYaw)
//			);
//			return GetJet(fabs(pSim->VoxArray[SIndex].GetMaxBondStress() / pSim->VoxArray[SIndex].PreDamageStress));
		}

		case RVC_DISP:
		{
			if (pSim->SS.MaxVoxDisp == 0) return GetJet(0);
			return GetJet(pSim->VoxArray[SIndex].GetCurAbsDisp() / pSim->SS.MaxVoxDisp);
			break;
//			double maxS = pSim->pEnv->pObj->Structure.GetMaxAdaptationRate();
//			double minS = -pSim->pEnv->pObj->Structure.GetMaxAdaptationRate();
//			double normStressAdaptationRate = (pSim->VoxArray[SIndex].stressAdaptationRate - minS ) / (maxS - minS);
//			return GetJet(normStressAdaptationRate);
//			break;
//            return GetJet(fabs(pSim->VoxArray[SIndex].CalcVoxelPressure() / pSim->VoxArray[SIndex].PreDamagePressure));
//			break;
		}

		case RVC_STRAIN_EN:  // Now: light intensity or pitch
		{
			//if (pSim->SS.MaxBondStrainE == 0) return GetJet(0);
			//return GetJet(pSim->VoxArray[SIndex].GetMaxBondStrainE() / pSim->SS.MaxBondStrainE);
			//break;

//			double maxP = pSim->pEnv->pObj->Structure.GetMaxAdaptationRate();
//			double minP = -pSim->pEnv->pObj->Structure.GetMaxAdaptationRate();
//			double normPressureAdaptationRate = (pSim->VoxArray[SIndex].pressureAdaptationRate - minP ) / (maxP - minP);
//			return GetJet(normPressureAdaptationRate);
//			break;
//            double maxDist = pSim->pEnv->getLightSource().Length();
//            double thisDist = pSim->pEnv->getLightSource().Dist(pSim->VoxArray[SIndex].GetCurPos());
            if ( (pSim->pEnv->getLightSource().x != 0) && (pSim->pEnv->getLightSource().y != 0) && (pSim->pEnv->getLightSource().z != 0) )
            {
                if (pSim->VoxArray[SIndex].LightIntensity > 0)
                {
                    return CColor(1.0f, 1.0f, 0.0f, 1.0f);
                }
                return CColor(1.0f, 1.0f, 1.0f, 1.0f);
                break;
			}
		}
		default:
			return GetJet(0.5*pSim->VoxArray[SIndex].GetPitch()/PI+0.5);
			break;
	}
}

CColor CVXS_SimGLView::GetInternalBondColor(CVXS_BondInternal* pBond)
{
	switch (CurViewCol) {
		case RVC_TYPE:
			if (pBond->IsSmallAngle()) return CColor(0.3, 0.7, 0.3, 1.0);
			else return CColor(0.0, 0.0, 0.0, 1.0);
			break;
		case RVC_KINETIC_EN:
			if (pSim->SS.MaxVoxKinE == 0) return GetJet(0);
			return GetJet(pBond->GetMaxVoxKinE() / pSim->SS.MaxVoxKinE);
			break;
		case RVC_DISP:
			if (pSim->SS.MaxVoxDisp == 0) return GetJet(0);
			return GetJet(pBond->GetMaxVoxDisp() / pSim->SS.MaxVoxDisp);
			break;
		case RVC_STATE:
			if (pBond->IsBroken()) return CColor(1.0f, 0.0f, 0.0f, 1.0f);
			else if (pBond->IsYielded()) return CColor(1.0f, 1.0f, 0.0f, 1.0f);
			else return CColor(1.0f, 1.0f, 1.0f, 1.0f);
			break;
		case RVC_STRAIN_EN:
			if (pSim->SS.MaxBondStrainE == 0) return GetJet(0);
			return GetJet(pBond->GetStrainEnergy() / pSim->SS.MaxBondStrainE);
			break;
		case RVC_STRAIN:
			if (pSim->SS.MaxBondStrain == 0) return GetJet(0);
			return GetJet(pBond->GetEngStrain() / pSim->SS.MaxBondStrain);
			break;
		case RVC_STRESS:
			if (pSim->SS.MaxBondStress == 0) return GetJet(0);
			return GetJet(pBond->GetEngStress() / pSim->SS.MaxBondStress);
			break;
		case RVC_PRESSURE:
			return GetJet(0); //for now. Pressure of a bond doesn't really make sense
			break;
		default:
			return CColor(0.0f,0.0f,0.0f,1.0f);
			break;
	}
}

CColor CVXS_SimGLView::GetCollisionBondColor(CVXS_BondCollision* pBond)
{
	if (!pSim->IsFeatureEnabled(VXSFEAT_COLLISIONS)) return CColor(0.0, 0.0, 0.0, 0.0); //Hide me
	vfloat Force = pBond->GetForce1().Length(); //check which force to use!
	if (Force == 0.0) return CColor(0.3, 0.3,1.0, 1.0);
	else return CColor(1.0, 0.0, 0.0, 1.0);
}


void CVXS_SimGLView::DrawSurfMesh(int Selected)
{
	//if simulation has a mesh, draw it...

	//otherwise
	SmoothMesh.UpdateMesh(Selected); //updates the generated mesh
	SmoothMesh.Draw();

	PlotVectors curVectPlot;
	float vectorsScalingView;
	bool plottingForces;

}

void CVXS_SimGLView::DrawVoxMesh(int Selected)
{
	// nac, this is the mesh we want
	VoxMesh.UpdateMesh(Selected); //updates the generated mesh
	VoxMesh.Draw(plottingForces, curVectPlot, vectorsScalingView);
}


void CVXS_SimGLView::DrawBonds(void)
{
//	bool DrawInputBond = true;

	Vec3D<> P1, P2, Axis;
	CVXS_Voxel* pV1, *pV2;
	vfloat AngleAmt;
	int NumSegs = 12; //number segments for smooth bonds


	float PrevLineWidth;
	glGetFloatv(GL_LINE_WIDTH, &PrevLineWidth);
	glLineWidth(3.0);
	glDisable(GL_LIGHTING);

	int iT = pSim->NumBond();

	for (int i = 0; i<iT; i++) //go through all the bonds...
	{
		CVXS_BondInternal* pBond = &pSim->BondArrayInternal[i];
		pV1 = pBond->GetpV1(); pV2 = pBond->GetpV2();

		//set color
		CColor ThisColor = GetInternalBondColor(pBond);
		glColor4f(ThisColor.r, ThisColor.g, ThisColor.b, ThisColor.a);

		P1 = pV1->GetCurPos();
		P2 = pV2->GetCurPos();

		if (CurViewVox == RVV_SMOOTH){
			CQuat<>A1 = pV1->GetCurAngle();
			CQuat<>A2 = pV2->GetCurAngle();
			A1.AngleAxis(AngleAmt, Axis); //get angle/axis for A1

			Vec3D<> Pos2L = A1.RotateVec3DInv(P2-P1); //Get PosDif in local coordinate system
			CQuat<> Angle2L = A2*A1.Conjugate(); //rotate A2 by A1
			pBond->ToXDirBond(&Pos2L); //swing bonds in the +Y and +Z directions to +X
			pBond->ToXDirBond(&Angle2L);

			vfloat L = Pos2L.x; //pV1->GetNominalSize();
			Vec3D<> Angle2LV = Angle2L.ToRotationVector();
			vfloat ay = (Angle2LV.z*L-2*Pos2L.y)/(L*L*L);
			vfloat by = (3*Pos2L.y-Angle2LV.z*L)/(L*L);
			vfloat az = (-Angle2LV.y*L-2*Pos2L.z)/(L*L*L);
			vfloat bz = (3*Pos2L.z+Angle2LV.y*L)/(L*L);

			glPushMatrix();
			glTranslated(P1.x, P1.y, P1.z);
			glRotated(AngleAmt*180/3.1415926, Axis.x, Axis.y, Axis.z); //rotate to voxel 1's coordinate system
			glBegin(GL_LINE_STRIP);
			glLoadName (-1); //to disable picking

			for (int i=0; i<=NumSegs; i++){
				vfloat iL = ((float)i)/NumSegs*L;
				Vec3D<> ThisPoint = Vec3D<>(iL, ay*iL*iL*iL + by*iL*iL, az*iL*iL*iL + bz*iL*iL);
				pBond->ToOrigDirBond(&ThisPoint);
				glVertex3d(ThisPoint.x, ThisPoint.y, ThisPoint.z);
			}

			glEnd();
			glPopMatrix();
		}
		else { //straight lines (faster)
			glBegin(GL_LINES);
			glLoadName (-1); //to disable picking

			if (ThisColor.a != 0.0) {glVertex3f((float)P1.x, (float)P1.y, (float)P1.z); glVertex3f((float)P2.x, (float)P2.y, (float)P2.z);}

			glEnd();

		}
	}

	iT = pSim->NumColBond();
	glBegin(GL_LINES);
	glLoadName (-1); //to disable picking
	for (int i = 0; i<iT; i++) //go through all the bonds...
	{
		pV1 = pSim->BondArrayCollision[i].GetpV1(); pV2 = pSim->BondArrayCollision[i].GetpV2();

		CColor ThisColor = GetCollisionBondColor(&pSim->BondArrayCollision[i]);
		P1 = pV1->GetCurPos();
		P2 = pV2->GetCurPos();

		glColor4f(ThisColor.r, ThisColor.g, ThisColor.b, ThisColor.a);
			if (ThisColor.a != 0.0) {glVertex3f((float)P1.x, (float)P1.y, (float)P1.z); glVertex3f((float)P2.x, (float)P2.y, (float)P2.z);}
	}


	////input bond
	//if (DrawInputBond && BondInput->GetpV1() && BondInput->GetpV2()){
	//	glColor4f(1.0, 0, 0, 1.0);
	//	P1 = BondInput->GetpV1()->GetCurPos();
	//	P2 = BondInput->GetpV2()->GetCurPos();
	//	glVertex3f((float)P1.x, (float)P1.y, (float)P1.z); glVertex3f((float)P2.x, (float)P2.y, (float)P2.z);
	//}

	glEnd();

	glLineWidth(PrevLineWidth);
	glEnable(GL_LIGHTING);


}
//
//void CVXS_SimGLView::DrawMiniVoxels() //draws grab-able mini voxels with space to show bonds or forces
//{
//	Vec3D<> Center;
//	iT = pSim->NumVox();
//	glPointSize(5.0);
//	Vec3D<> tmp(0,0,0);
//	for (int i = 0; i<iT; i++) //go through all the voxels...
//	{
//		//mostly copied from Voxel drawing function!
//		Center = pSim->VoxArray[i].GetCurPos();
//		glColor4d(0.2, 0.2, 0.2, 1.0);
//	//	glLoadName (StoXIndexMap[i]); //to enable picking
//
//		glPushMatrix();
//		glTranslated(Center.x, Center.y, Center.z);
//		glLoadName (pSim->StoXIndexMap[i]); //to enable picking
//
//		//generate rotation matrix here!!! (from quaternion)
//		Vec3D<> Axis;
//		vfloat AngleAmt;
//		CQuat<>(pSim->VoxArray[i].GetCurAngle()).AngleAxis(AngleAmt, Axis);
//		glRotated(AngleAmt*180/3.1415926, Axis.x, Axis.y, Axis.z);
//	
//		vfloat Scale = pSim->VoxArray[i].GetCurScale(); //show deformed voxel size
//		glScaled(Scale, Scale, Scale);
//
//		//LocalVXC.Voxel.DrawVoxel(&tmp, LocalVXC.Lattice.Lattice_Dim*(1+0.5*CurTemp * pMaterials[CVoxelArray[i].MatIndex].CTE), LocalVXC.Lattice.Z_Dim_Adj);
//		pSim->LocalVXC.Voxel.DrawVoxel(&tmp, 0.2); //LocalVXC.GetLatticeDim()); //[i].CurSize.x); //, LocalVXC.Lattice.Z_Dim_Adj);
//		
//		glPopMatrix();
//	}
//
//
//	glLineWidth(PrevLineWidth);
//	glEnable(GL_LIGHTING);
//}

void CVXS_SimGLView::DrawAngles(void)
{
	//draw directions
	float PrevLineWidth;
	glGetFloatv(GL_LINE_WIDTH, &PrevLineWidth);
	glLineWidth(2.0);
	glDisable(GL_LIGHTING);

	glBegin(GL_LINES);

	for (int i = 0; i < pSim->NumVox(); i++){ //go through all the voxels... (GOOD FOR ONLY SMALL DISPLACEMENTS, I THINK... think through transformations here!)
		glColor3f(1,0,0); //+X direction
		glVertex3d(pSim->VoxArray[i].GetCurPos().x, pSim->VoxArray[i].GetCurPos().y, pSim->VoxArray[i].GetCurPos().z);
		Vec3D<> Axis1(pSim->LocalVXC.GetLatticeDim()/4,0,0);
		Vec3D<> RotAxis1 = (pSim->VoxArray[i].GetCurAngle()*CQuat<>(Axis1)*pSim->VoxArray[i].GetCurAngle().Conjugate()).ToVec();
		glVertex3d(pSim->VoxArray[i].GetCurPos().x + RotAxis1.x, pSim->VoxArray[i].GetCurPos().y + RotAxis1.y, pSim->VoxArray[i].GetCurPos().z + RotAxis1.z);

		glColor3f(0,1,0); //+Y direction
		glVertex3d(pSim->VoxArray[i].GetCurPos().x, pSim->VoxArray[i].GetCurPos().y, pSim->VoxArray[i].GetCurPos().z);
		Axis1 = Vec3D<>(0, pSim->LocalVXC.GetLatticeDim()/4,0);
		RotAxis1 = (pSim->VoxArray[i].GetCurAngle()*CQuat<>(Axis1)*pSim->VoxArray[i].GetCurAngle().Conjugate()).ToVec();
		glVertex3d(pSim->VoxArray[i].GetCurPos().x + RotAxis1.x, pSim->VoxArray[i].GetCurPos().y + RotAxis1.y, pSim->VoxArray[i].GetCurPos().z + RotAxis1.z);

		glColor3f(0,0,1); //+Z direction
		glVertex3d(pSim->VoxArray[i].GetCurPos().x, pSim->VoxArray[i].GetCurPos().y, pSim->VoxArray[i].GetCurPos().z);
		Axis1 = Vec3D<>(0,0, pSim->LocalVXC.GetLatticeDim()/4);
		RotAxis1 = (pSim->VoxArray[i].GetCurAngle()*CQuat<>(Axis1)*pSim->VoxArray[i].GetCurAngle().Conjugate()).ToVec();
		glVertex3d(pSim->VoxArray[i].GetCurPos().x + RotAxis1.x, pSim->VoxArray[i].GetCurPos().y + RotAxis1.y, pSim->VoxArray[i].GetCurPos().z + RotAxis1.z);

	}
	glEnd();

	glLineWidth(PrevLineWidth);
	glEnable(GL_LIGHTING);
}

void CVXS_SimGLView::DrawStaticFric(void)
{
	//draw triangle for points that are stuck via static friction
	glBegin(GL_TRIANGLES);
	glColor4f(255, 255, 0, 1.0);
	vfloat dist = pSim->VoxArray[0].GetNominalSize()/3; //needs work!!
	int iT = pSim->NumVox();
	Vec3D<> P1;
	for (int i = 0; i<iT; i++){ //go through all the voxels...
		if (pSim->VoxArray[i].GetCurStaticFric()){ //draw point if static friction...
			P1 = pSim->VoxArray[i].GetCurPos();
			glVertex3f((float)P1.x, (float)P1.y, (float)P1.z); 
			glVertex3f((float)P1.x, (float)(P1.y - dist/2), (float)(P1.z + dist));
			glVertex3f((float)P1.x, (float)(P1.y + dist/2), (float)(P1.z + dist));
		}
	}
	glEnd();
}

int CVXS_SimGLView::StatRqdToDraw() //returns the stats bitfield that we need to calculate to draw the current view.
{
	if (CurViewMode == RVM_NONE) return CALCSTAT_NONE;
	switch (CurViewCol){
	case RVC_KINETIC_EN: return CALCSTAT_KINE; break;
	case RVC_DISP: return CALCSTAT_DISP; break;
	case RVC_STRAIN_EN: return CALCSTAT_STRAINE; break;
	case RVC_STRAIN: return CALCSTAT_ENGSTRAIN; break;
	case RVC_STRESS: return CALCSTAT_ENGSTRESS; break;
	case RVC_PRESSURE: return CALCSTAT_PRESSURE; break;
	default: return CALCSTAT_NONE;
	}
}

void CVXS_SimGLView::ChangeSkyColor(float r, float g, float b)
{
	skyColor = CColor(r,g,b);
}
//
//void CVXS_SimGLView::DrawOverlay(void)
//{
//	if (CurViewCol == RVC_KINETIC_EN || CurViewCol == RVC_DISP || CurViewCol == RVC_STRAIN_EN || CurViewCol == RVC_STRAIN || CurViewCol == RVC_STRESS){
//		CColor Tmp;
//		int XOff = 10;
//		int YOff = 10;
//		int XWidth = 30;
//		int YHeight = 200;
//		int NumChunks = 4;
//		int TextXOff = 10;
//
//		glBegin(GL_QUAD_STRIP);
//		
//		for (int i=0; i<=NumChunks; i++){
//			double Perc = ((double)i)/NumChunks;
//			Tmp = GetJet(1.0-Perc);
//			glColor4f(Tmp.r, Tmp.g, Tmp.b, Tmp.a);
//
//			glVertex2f(XOff,YOff+Perc*YHeight);
//			glVertex2f(XOff+XWidth,YOff+Perc*YHeight);
//
//		}
//		glEnd();
//
//		//draw the labels...
//		double MaxVal = 1.0;
//		QString Units = "";
//		switch(CurViewCol){
//			case RVC_KINETIC_EN: MaxVal = pSim->SS.MaxVoxKinE*1000; Units = "mJ"; break;
//			case RVC_DISP: MaxVal = pSim->SS.MaxVoxDisp*1000; Units = "mm"; break;
//			case RVC_STRAIN_EN: MaxVal = pSim->SS.MaxBondStrainE*1000; Units = "mJ"; break;
//			case RVC_STRAIN: MaxVal = pSim->SS.MaxBondStrain; break;
//			case RVC_STRESS: MaxVal = pSim->SS.MaxBondStress/1000000; Units = "MPa"; break;
//		}
//
//		glColor4f(0, 0, 0, 1.0);
//		QString ScaleNumber;
//
//		for (int i=0; i<=NumChunks; i++){
//			double Perc = ((double)i)/NumChunks;
//			ScaleNumber = QString::number((1-Perc) * MaxVal, 'g', 3) + Units;
//			pGLWin->renderText(XOff + XWidth + TextXOff, YOff + Perc*YHeight+5, ScaleNumber);
//		}
//
//
//		
//	}
//
//
//}