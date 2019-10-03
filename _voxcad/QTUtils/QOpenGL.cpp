#include "QOpenGL.h"
#include <QMouseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QTime>
#include <vector>
#include <math.h>

#include "GL/glu.h"

#include "../Voxelyze/Utils/GL_Utils.h"

#if defined(_WIN32) || defined(_WIN64) //to get fmax, fmin to work on Windows/Visual Studio
#define fmax max
#define fmin min
#endif

int CQOpenGL::TotalID = 0;

CQOpenGL::CQOpenGL(const QGLFormat &format, QWidget *parent) : QGLWidget(format, parent)
{

	MyID = TotalID++;
//	if (format == 0) setFormat(QGLFormat(QGL::DoubleBuffer | QGL::DepthBuffer)); //This causes an assert on destruction in some cases. switched to recomended passing in QGLFormat through constructor.
//	setSizePolicy(2);
	SetView(VPERSPECTIVE);
	WindowSize = QRect(0, 0, 0, 0);
	CurEnv = Vec3D<>(0, 0, 0);
	CurEnvOff = Vec3D<>(0, 0, 0);

    setSizePolicy( QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
	setFocusPolicy (Qt::ClickFocus);
	setMouseTracking(true); //turns on mouse events even if no button pressed...

	lastPos = QPoint(0,0);
//	LMBDownPos = QPoint(0,0);

	//default view settings:
	bDrawBounds = true;
	bDrawAxes = true;
	IniViewSet = false;

	AskedAboutFastMode = false;

	DrawTimer = new QTimer(this);
	connect(DrawTimer, SIGNAL(timeout()), this, SLOT(updateGL()));

	AutoRedraw = false;
	Drawing = false;

	//set up some lights!
	QColor a(90, 90, 90), b(40, 40, 40); 
	Lights.push_back(GL_Light(GL_LIGHT0, a, b, Vec3D<>(-0.5, 0.5, 2.0)));
	QColor c(60, 60, 60), d(60, 20, 20);
	Lights.push_back(GL_Light(GL_LIGHT1, c, d, Vec3D<>(-2.0, -0.5, 1.0)));
	QColor e(90, 90, 90), f(20, 20, 60);
	Lights.push_back(GL_Light(GL_LIGHT2, e, f, Vec3D<>(1.0, -1.0, -1.0)));
}


void CQOpenGL::initializeGL()
{
	EnterFastMode(false);

	GLSetBG(true);
	glClearDepth(1.0f);

	// Turn on backface culling
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);

	// Turn on depth testing
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);


	glLineWidth (1.0);
	GLSetLighting();

}

void CQOpenGL::EnterFastMode(bool entering) //cuts the fat of rendering for large scenes
{
	emit FastModeChanged(entering); //let the ui know we've changed!

	if (entering){ //if going to fast mode
		
		FastMode = true;
		glDisable(GL_BLEND);
		glHint(GL_POLYGON_SMOOTH_HINT, GL_FASTEST);
		glHint(GL_LINE_SMOOTH_HINT, GL_FASTEST);
		glDisable(GL_LINE_SMOOTH);
		glDisable(GL_POLYGON_SMOOTH);
		glDisable(GL_NORMALIZE);
		glDisable(GL_POLYGON_OFFSET_FILL);

	}
	else { //if going to High quality mode
		FastMode = false;
		glEnable(GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	
		glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
//		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
//		glEnable(GL_LINE_SMOOTH);
		glDisable(GL_LINE_SMOOTH);
		glEnable(GL_POLYGON_SMOOTH);
		glEnable(GL_NORMALIZE);
		glPolygonOffset(1.0, 2);
		glEnable(GL_POLYGON_OFFSET_FILL);
	}
}

void CQOpenGL::resizeGL(int width, int height)
{
	glViewport(0, 0, width, height);
	WindowSize = QRect(0, 0, width, height); //store current window size
	GLSetPersp(); //set current view perspective
}

void CQOpenGL::paintGL()
{
	if (Drawing) 
		return;
	
	Drawing = true;
	makeCurrent();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	if (!IniViewSet) GLCenterView(); //set the view appropriately the first time through
	GLDrawScene(); // Draw OpenGL scene

	//output frames for video
	bool AutoSaveFrame = false;
	emit WantAutoSaveFrames(&AutoSaveFrame);
	if (AutoSaveFrame){
		QString SavePath = "";
		emit GetFrameFilePath(&SavePath);
		if (SavePath != "") GlSaveScreenShot(SavePath);
	}

	Drawing = false;

}

void CQOpenGL::mousePressEvent(QMouseEvent *event)
{
	lastPos = event->pos();

	if (event->buttons() & Qt::LeftButton){
		bool EmitCoord3D, EmitGLIndex;
		emit WantCoord3D(&EmitCoord3D); //do we want to calculate and emit the 3D coordinates?
		emit WantGLIndex(&EmitGLIndex); //do we want to calculate and emit the 3D coordinates?

		if (EmitGLIndex){
			emit MousePressIndex(GLPickScene(event->x(), event->y())); //broadcast that we've clicked a voxel index
		}
		if (EmitCoord3D){
			Vec3D<> Coord;
			GLScreenToCoord3D(event->pos(), Coord);
			bool IsCtrl = event->modifiers() & Qt::ControlModifier;
			emit LMouseDown(Coord.x, Coord.y, Coord.z, IsCtrl);
			LastPickedPoint = Coord;

		}

/*		

		bool ExtScene2DProcess;
		emit IsExtScene2D(&ExtScene2DProcess); //do we want to emit the mouse handle signals?

		if (ExtScene2DProcess && CurView != VPERSPECTIVE){ //if its a 2D view we can process faster without using OpenGL (using signals...)
			float XCoord, YCoord;
			GLScreenToCoord(event->x(), event->y(), XCoord, YCoord);

			Vec3D tmp;
			GLScreenToCoord3D(event->pos(), tmp);
			emit LMouseDown(XCoord, YCoord); //broadcast the coordinates of this click
		}
		else if (!ExtScene2DProcess){ //if its a 3D view, we have no (reasonable) choice but to proces via openGL
			int index = GLPickScene(event->x(), event->y());
			emit MousePressIndex(index); //broadcast that we've clicked a voxel index

		}
		
		updateGL(); //get rid of this one I think!!
		*/
	}
}

void CQOpenGL::mouseReleaseEvent(QMouseEvent *event)
{
//	if (LMBDownPos != QPoint(0,0)){ //if lmb WAS down... (can't tell from event, since we released already...
		bool EmitCoord3D;
		emit WantCoord3D(&EmitCoord3D); //do we want to calculate and emit the 3D coordinates?
	//	emit IsEmitScreenCoords(&EmitScreenCoords);

		if (EmitCoord3D){ //if its a 2D view we can process faster without using OpenGL (using signals...)
			Vec3D<> Coord;
			GLScreenToCoord3D(event->pos(), Coord, false);
			emit LMouseUp(Coord.x, Coord.y, Coord.z);

			//REPLACE THESE!!!
//			float XCoord, YCoord;
//			GLScreenToCoord(event->x(), event->y(), XCoord, YCoord);
//			emit LMouseUp(XCoord, YCoord); //broadcast the coordinates of this click
		}
//}
}

void CQOpenGL::mouseDoubleClickEvent(QMouseEvent *event)
{

}

void CQOpenGL::keyPressEvent(QKeyEvent *event)
{
//	bool ExtScene2DProcess;
//	emit IsExtScene2D(&ExtScene2DProcess); //do we want to emit the mouse handle signals?

	switch (event->key()){
		case Qt::Key_Escape:
			emit PressedEscape();
//			if (ExtScene2DProcess) emit PressedEscape();
//			else GLCenterView();
//			updateGL();
			break;
//		default: CQOpenGL::keyPressEvent(event); //pass along if we don't use it...
		
		
		case Qt::Key_Z:
			m_Cam.XPos += 0.00025f * m_Cam.Zoom;
		break;

		case Qt::Key_C:
		m_Cam.XPos -= 0.00025f * m_Cam.Zoom;
		break;

		case Qt::Key_X:
		m_Cam.YPos += 0.00025f * m_Cam.Zoom;
		break;

		case Qt::Key_S:
		m_Cam.YPos -= 0.00025f * m_Cam.Zoom;
		break;

		case Qt::Key_Right:
		m_Cam.YRot -= 2.0f;	
		if (m_Cam.YRot > 360.0f) m_Cam.YRot -= 360.0f;
		if (m_Cam.YRot < 360.0f) m_Cam.YRot += 360.0f;
		break;

		case Qt::Key_Left:
		m_Cam.YRot += 2.0f;	
		if (m_Cam.YRot > 360.0f) m_Cam.YRot -= 360.0f;
		if (m_Cam.YRot < 360.0f) m_Cam.YRot += 360.0f;
		break;

		case Qt::Key_Down:
		m_Cam.XRot -= 0.5f;	
		if (m_Cam.XRot > 360.0f) m_Cam.XRot -= 360.0f;
		if (m_Cam.XRot < 360.0f) m_Cam.XRot += 360.0f;
		break;

		case Qt::Key_Up:
		m_Cam.XRot += 0.5f;	
		if (m_Cam.XRot > 360.0f) m_Cam.XRot -= 360.0f;
		if (m_Cam.XRot < 360.0f) m_Cam.XRot += 360.0f;
		break;

		case Qt::Key_PageDown:
		m_Cam.Zoom *= (float)1.05;
		break;

		case Qt::Key_PageUp:
		m_Cam.Zoom /= (float)1.05;
		break;
	}

}

void CQOpenGL::mouseMoveEvent(QMouseEvent *event)
{
	bool EmitCoord3D;
	emit WantCoord3D(&EmitCoord3D); //do we want to calculate and emit the 3D coordinates?

	if (event->buttons() & Qt::LeftButton) { //If left mouse button is down:
		if (EmitCoord3D){ //if its a 2D view we can process faster without using OpenGL (using signals...)
			Vec3D<> Coord;
			GLScreenToCoord3D(event->pos(), Coord, false);
			emit LMouseMovePressed(Coord.x, Coord.y, Coord.z);

			LastPickedPoint = Coord;

//			float XC, YC;
//			GLScreenToCoord(event->x(), event->y(), XC, YC);
//			emit LMouseMovePressed(XC, YC);
		}
	}
	else if (event->buttons() & Qt::MidButton || event->buttons() & Qt::RightButton) { //If middle mouse button is down: (manipulate the view!)
		GLfloat dX = GLfloat(event->x() - lastPos.x()) / width();
		GLfloat dY = GLfloat(event->y() - lastPos.y()) / height();

		if (event->modifiers() & Qt::ControlModifier){ //Control key down (translate)
			if (CurView == VPERSPECTIVE){ //if 3D view currently
				m_Cam.XPos += 0.025f * m_Cam.Zoom * dX;
				m_Cam.YPos -= 0.025f * m_Cam.Zoom * dY;
			}
			else { //if 2D view currently
				m_Cam.XPos += 4 * m_Cam.Zoom * dX; //change these factors below, as well...
				m_Cam.YPos -= 4 * m_Cam.Zoom * dY;
			}
		}
		else if (event->modifiers() & Qt::ShiftModifier){ //Shift key down (change perspective)
			m_Cam.Persp += 50.0 * dY;
			if (m_Cam.Persp < 5.0f) m_Cam.Persp = 5.0f;
			if (m_Cam.Persp > 90.0f) m_Cam.Persp = 90.0f;
		}
		else if (event->modifiers() & Qt::AltModifier){ //Shift key down (change zoom)
			m_Cam.Zoom *= 1.0f+1.5*dY;

		}
		else { //No modifier down (rotate view in 3D, pan in 2D)
			if (CurView == VPERSPECTIVE){ //if 3D view 
				m_Cam.XRot += 300.0f * dY;
				if (m_Cam.XRot > 360.0f) m_Cam.XRot = 360.0f;
				if (m_Cam.XRot < 180.0f) m_Cam.XRot = 180.0f;
//				if (m_Cam.XRot > 360.0f) m_Cam.XRot -= 360.0f; //continuous end-overend rotation
//				if (m_Cam.XRot < 360.0f) m_Cam.XRot += 360.0f;

				m_Cam.YRot += 300.0f * dX;

			
				if (m_Cam.YRot > 360.0f) m_Cam.YRot -= 360.0f;
				if (m_Cam.YRot < 360.0f) m_Cam.YRot += 360.0f;
			}
			else { //if 2D view currently
				m_Cam.XPos += 4 * m_Cam.Zoom * dX; //change these factors below, as well...
				m_Cam.YPos -= 4 * m_Cam.Zoom * dY;
			}
		}

		bool SavingFrames; emit WantAutoSaveFrames(&SavingFrames); //are we recording video?
		if (!AutoRedraw && !SavingFrames) updateGL();
		lastPos = event->pos();
	}
	else { //If no other button we care about is down:
		if (EmitCoord3D){ //if its a 2D view we can process faster without using OpenGL (using signals...)
			Vec3D<> Coord;
			GLScreenToCoord3D(event->pos(), Coord, false);
			emit MouseMoveHover(Coord.x, Coord.y, Coord.z);

//			float XC, YC;
//			GLScreenToCoord(event->x(), event->y(), XC, YC);
//			emit MouseMoveHover(XC, YC);
		}
	}


}

void CQOpenGL::wheelEvent(QWheelEvent *event)
{
	if (event->modifiers() & Qt::ControlModifier){ //if its a 2D view we can process faster without using OpenGL (using signals
		if (event->delta() > 0) emit CtrlWheelRoll(true);
		if (event->delta() < 0) emit CtrlWheelRoll(false);
	}
	else {
		if (event->delta() > 0)	m_Cam.Zoom *= (float)1.05; //zoom zoom!
		if (event->delta() < 0) m_Cam.Zoom /= (float)1.05;
		if (!AutoRedraw) updateGL();
	}
}

void CQOpenGL::GLSetPersp()
{
	float aspect = (float)WindowSize.width() / (float)WindowSize.height();
	float Scale = CurEnv.Length();
	if (CurView == VPERSPECTIVE){ //if 3D view
		gluPerspective(m_Cam.Persp, aspect, Scale/100, Scale*100); //set reasonable viewing distances
	}
	else {
		glOrtho(-aspect*m_Cam.Zoom, aspect*m_Cam.Zoom, -m_Cam.Zoom, m_Cam.Zoom, -Scale*10, Scale*10);
//		glOrtho(-aspect*m_Cam.Zoom, aspect*m_Cam.Zoom, -m_Cam.Zoom, m_Cam.Zoom, Scale/10, Scale*10);
	}
}

void CQOpenGL::GLDrawScene()
{
	CGL_Utils::CurContextID = MyID;
	glRenderMode(GL_RENDER);
	
	glMatrixMode(GL_PROJECTION); // Select the Projection Matrix
	glLoadIdentity();
	GLSetPersp(); //dynamic control over the perspective

	//move the view to follow something if this signal is hooked up to something that changes its value
	Vec3D<> CurTarget(m_Cam.TargetX, m_Cam.TargetY, m_Cam.TargetZ);
	emit FindCamTarget(&CurTarget);
	m_Cam.TargetX = CurTarget.x;
	m_Cam.TargetY = CurTarget.y;
	m_Cam.TargetZ = CurTarget.z;

	// Set camera view
	glMatrixMode(GL_MODELVIEW); //Back to model view
	glLoadIdentity();
	GLTranslateCam();

	GLSetLighting(); //Enable to have specular highlight in accurate place
	//End Bonuses


	glPushMatrix();
	switch (CurView){ //this makes sure the axes and bounds are out front of drawing...
	case VTOP: glTranslated(0, 0, CurEnv.z); break;
	case VBOTTOM: glTranslated(0, 0, -CurEnv.z); break;
	case VLEFT: glTranslated(0, -CurEnv.y, 0); break;
	case VRIGHT: glTranslated(0, CurEnv.y, 0); break;
	case VFRONT: glTranslated(CurEnv.x, 0, 0); break;
	case VBACK: glTranslated( -CurEnv.x, 0, 0); break;
	}

	if (bDrawAxes)
		GLDrawAxes();

	glPopMatrix();

	if (bDrawBounds)
		GLDrawBounds();

//	CGL_Utils::DrawSphere(LastPickedPoint, 0.0002, Vec3D<>(1,1,1), CColor(.5, .5, .5));

	QTime t;
	t.start();
	//draw geometry:
	emit DrawGL(FastMode); //Draw anything connected to this!

	int MStoDraw = t.elapsed();
	if (MStoDraw > 200 && !IsFastMode() && !AskedAboutFastMode){
		AskedAboutFastMode = true;
		if (QMessageBox::question(NULL, "Enter fast draw mode?", "Do you want to enter fast drawing mode?", QMessageBox::Yes | QMessageBox::No)==QMessageBox::Yes)
			EnterFastMode(true);
	}

//	glPushMatrix();
	//draw 2D overlay!

	
	glPushAttrib(GL_TRANSFORM_BIT | GL_VIEWPORT_BIT);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	  glLoadIdentity();
	  glOrtho(0, WindowSize.width(), WindowSize.height(), 0, -1, 1);
	  glMatrixMode(GL_MODELVIEW);
	  glPushMatrix();
	    glLoadIdentity();
	    glDisable(GL_LIGHTING);
	    emit DrawGLOverlay(); //draw any 2D connected to this!
	  glPopMatrix();
    glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glPopAttrib();

}

int CQOpenGL::GLPickScene(int x, int y) //"draws" a section the scene in memory to see what we clicked on. returns name index
{
	makeCurrent(); //make sure we're using the right gl window information...
	GLuint selectBuf[BUFSIZE];
	GLint hits;

	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport); //gets current viewport info
	glSelectBuffer(BUFSIZE, selectBuf);
	glRenderMode(GL_SELECT);
	glInitNames();
	glPushName(-1);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	/*  create 5x5 pixel picking region near cursor location      */
	gluPickMatrix ((GLdouble) x, (GLdouble) (viewport[3] - y), 1.0, 1.0, viewport);
	GLSetPersp();																				

	glMatrixMode(GL_MODELVIEW); //Back to model view
	glLoadIdentity();
	GLTranslateCam();
	emit DrawGL(FastMode); //Draw anything connected to this!

	glMatrixMode (GL_PROJECTION);
	glPopMatrix();
	glFlush();

	hits = glRenderMode (GL_RENDER);
	//processHits (hits, selectBuf);

	int CurClosest = -1;
	uint MinDist = UINT_MAX;
	for (int i = 0; i < hits; i++){
		if (selectBuf[4*i+1] < MinDist){
			MinDist = selectBuf[4*i+1];
			CurClosest = selectBuf[4*i+3];
		}
	}
	return CurClosest;
}

void CQOpenGL::GLScreenToCoord3D(QPoint p, Vec3D<>& RetVec, bool SetZDepthHere)
{
	makeCurrent(); //make sure we're using the right gl window information...

	GLdouble Cx, Cy, Cz;
	GLdouble modelMatrix[16];
	GLdouble projMatrix[16];
	GLint viewport[4];

	glGetIntegerv(GL_VIEWPORT, viewport);
	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix); //unit matrix, but there shouldn't be much overhead in this call since we need for unproject() anyway
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);

	if (SetZDepthHere) glReadPixels(p.x(), viewport[1] + viewport[3] - p.y(), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &lastZDepth);
	if (lastZDepth < 0) lastZDepth = 0;
	else if (lastZDepth >= 1.0){
		lastZDepth = 1.0; //at the back extreme
		RetVec = Vec3D<>(0,0,0);
		return;
	}

	gluUnProject(p.x(), viewport[1] + viewport[3] - p.y(), lastZDepth, modelMatrix, projMatrix, viewport, &Cx, &Cy, &Cz);


	RetVec = Vec3D<>(Cx, Cy, Cz);

}


void CQOpenGL::GLScreenToCoord(int x, int y, float& pXD, float& pYD) //for 2D views returns the world coordinates where we clicked
{
	//replaced by GLScreenToCoord3D();
	if (CurView == VPERSPECTIVE) return; //meaningless if we are not in an orthographic projection

	float WinWidth = (float)WindowSize.width();
	float WinHeight = (float)WindowSize.height();
	float aspect = WinWidth / WinHeight;

	float XTarget =  m_Cam.TargetX;
	if (CurView == VFRONT || CurView == VBACK) XTarget = m_Cam.TargetY;
	float YTarget = m_Cam.TargetZ;
	if (CurView == VTOP || CurView == VBOTTOM) YTarget = m_Cam.TargetY;

	if (CurView == VBOTTOM || CurView == VRIGHT || CurView == VBACK) //deal with the fact that the origin is on the left in negative axis views...
		pXD = -aspect*m_Cam.Zoom*(x*2/WinWidth - 1.0) + XTarget + m_Cam.XPos;
	else 
		pXD = aspect*m_Cam.Zoom*(x*2/WinWidth - 1.0) + XTarget - m_Cam.XPos;

	pYD = m_Cam.Zoom*(2.0*(WinHeight-y)/WinHeight - 1.0) + YTarget - m_Cam.YPos;
}


void CQOpenGL::GLTranslateCam(void)
{
	glTranslatef(m_Cam.XPos, m_Cam.YPos, -m_Cam.Zoom/m_Cam.Persp);
	glRotatef(m_Cam.XRot, 1.0f, 0.0f, 0.0f);
	glRotatef(m_Cam.YRot, 0.0f, 0.0f, 1.0f);
	glTranslatef(-m_Cam.TargetX, -m_Cam.TargetY, -m_Cam.TargetZ);
}

void CQOpenGL::GLDrawBounds() //draws bounding box with size determined by GLGetDim()
{
	//if (!GLGetDim(&CurEnv))
	//	return;

	//float Off = CurEnv.Length()/1000.0; //offset to draw lines outside of body
 //   Vec3D a(-Off, -Off, -Off), b(CurEnv.x + Off, CurEnv.y + Off, CurEnv.z + Off);
	//CGL_Utils::DrawCube(a, b, false, true, 2.0);


	if (!GLGetDim(&CurEnv, &CurEnvOff))
		return;

	float Off = CurEnv.Length()/1000.0; //offset to draw lines outside of body
    Vec3D<> a = CurEnvOff-Vec3D<>(Off, Off, Off), b = CurEnvOff+CurEnv+Vec3D<>(Off, Off, Off);
	CGL_Utils::DrawCube(a, b, false, true, 2.0);
}

void CQOpenGL::GLDrawAxes() //draws axes
{
	if (!GLGetDim(&CurEnv, &CurEnvOff))
		return;

	float AxisScale = (float)((CurEnv.x+CurEnv.y+CurEnv.z)/15);
	float AxisRad = AxisScale*0.02f;

//	glColor3d(0.6,0.6,0.6);
	CGL_Utils::DrawSphere(Vec3D<>(0,0, 0), AxisScale*0.04, Vec3D<>(1,1,1), CColor(0.6, 0.6, 0.6));
	CGL_Utils::DrawArrowD(Vec3D<>(0,0,AxisScale*0.0305), Vec3D<>(0, 0, AxisScale), CColor(0.4,0.4,0.8));
	CGL_Utils::DrawArrowD(Vec3D<>(0,AxisScale*0.0305,0), Vec3D<>(0, AxisScale, 0), CColor(0.25,0.5,0.25));
	CGL_Utils::DrawArrowD(Vec3D<>(AxisScale*0.0305, 0,0), Vec3D<>(AxisScale, 0, 0), CColor(0.7,0.3,0.3));

}

bool CQOpenGL::GLGetDim(Vec3D<>* pDim, Vec3D<>* pOff)
{
	//*pDim = Vec3D(0.0, 0.0, 0.0);
	//emit FindDims(pDim); //emits the signal, should store result in pDim

	//if (pDim->x == 0.0 || pDim->y == 0.0 || pDim->z == 0.0) return false;
	//return true;

	*pDim = Vec3D<>(0.0, 0.0, 0.0);
	*pOff = Vec3D<>(0.0, 0.0, 0.0);

	emit FindDims(pDim, pOff); //emits the signal, should store result in pDim and pOff
//	emit FindDims(pDim); //emits the signal, should store result in pDim and pOff

	if (pDim->x == 0.0 || pDim->y == 0.0 || pDim->z == 0.0) return false;
	return true;
}

void CQOpenGL::GLCenterView(bool ReZoom, Vec3D<>* pTargetPos)
{
	IniViewSet = true; //we've set the view!
	Vec3D<> Target;
	GLGetDim(&CurEnv, &CurEnvOff); //get the size of bounding box...
//	GLGetDim(&CurEnv); //get the size of bounding box...

	if (pTargetPos) Target = *pTargetPos; //if we've passed a center point in the arguments, set to this
	else Target = CurEnvOff + CurEnv/2;
//	else Target = CurEnv / 2;


	m_Cam.TargetX = Target.x;
	m_Cam.TargetY = Target.y;
	m_Cam.TargetZ = Target.z;

	QSize CurSize = size(); //current window size...
	if (ReZoom && !CurSize.isNull()){
		float WinAspect = ((float)CurSize.width())/CurSize.height(); //aspect ratio of the window
		m_Cam.XPos = 0.0f;
		m_Cam.YPos = 0.0f;

		switch (CurView){
		case VPERSPECTIVE:
			float ToZoomH, ToZoomV;
			ToZoomH = 100 * fmax(CurEnv.x, CurEnv.y);
			ToZoomV = 100 * CurEnv.z;
			m_Cam.Zoom = fmax(ToZoomH/WinAspect, ToZoomV);

		break;
		case VTOP: case VBOTTOM: m_Cam.Zoom = fmax(CurEnv.x/WinAspect, CurEnv.y)/1.95; break;
		case VRIGHT: case VLEFT: m_Cam.Zoom = fmax(CurEnv.x/WinAspect, CurEnv.z)/1.95; break;
		case VFRONT: case VBACK: m_Cam.Zoom = fmax(CurEnv.y/WinAspect, CurEnv.z)/1.95; break;

		}
	}
}

void CQOpenGL::GLSetLighting()
{
	if (FastMode){
		glDisable(GL_LIGHTING); //global lighting
	}
	else {
		glShadeModel(GL_SMOOTH); //smooth surfaces
		glEnable(GL_LIGHTING); //global lighting

		float AmbientLight[] = { 0.5f, 0.5f, 0.5f, 1.0f };
		glLightfv(GL_LIGHT0, GL_AMBIENT, AmbientLight); //Doesn't do anything unless we have at least one light!
	//	for (int i=0; i<(int)Lights.size(); i++){ Lights[i].SetLight(100*fmax(CurEnv.x, fmax(CurEnv.y, CurEnv.z)));}
		for (int i=0; i<(int)Lights.size(); i++){ Lights[i].SetLight(100*fmax(CurEnv.x, fmax (CurEnv.y, CurEnv.z)), &CurEnvOff);}


		//Global scene lighing setup
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); //default, but verify
		
		GLfloat mat_specular[] = {1.0f, 1.0f, 1.0f, 1.0f};  //Specular (highlight)
		GLfloat mat_shininess[] = {70};						//Shininess (size of highlight)
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, mat_shininess);

		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE); //Use ambient and diffuse
		glEnable(GL_COLOR_MATERIAL); //enable color tracking
		
		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE); //for accurate light reflections, mu
	}
}

//Functions to set view!
void CQOpenGL::SetView(int ViewIndex)
{
	if (CurView == (ViewType)ViewIndex) return; //don't reset a view (use GLCenterView if just want to see everything
	else CurView = (ViewType)ViewIndex;

	switch (CurView){
		case VPERSPECTIVE: m_Cam = Camera(0.0f, 0.0f, 280.0f, 210.0f, 0.0f, 0.0f, 0.0f, 30.0f, 1.0f); break;//initialize the camera to default angle
		case VTOP: m_Cam.XRot = 0.0f; m_Cam.YRot = 0.0f; break;
		case VBOTTOM: m_Cam.XRot = 180.0f; m_Cam.YRot = 180.0f; break;
		case VLEFT: m_Cam.XRot = 270.0f; m_Cam.YRot = 0.0f; break;
		case VRIGHT: m_Cam.XRot = 270.0f; m_Cam.YRot = 180.0f; break;
		case VFRONT: m_Cam.XRot = 270.0f; m_Cam.YRot = 270.0f; break;
		case VBACK: m_Cam.XRot = 270.0f; m_Cam.YRot = 90.0f; break;
	}
	GLCenterView();

//	updateGL();

}

void CQOpenGL::GLSaveScreenShot(void)
{ //save a screenshot of the scene
	GlSaveScreenShot(QFileDialog::getSaveFileName(this, "Save Screenshot (jpg)", "", "JPEG File (*.jpg)"));
}

void CQOpenGL::GlSaveScreenShot(QString FilePath)
{
	QImage Image;
	Image = grabFrameBuffer();
	Image.save(FilePath, 0, 95);
}
