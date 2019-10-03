//*********************************************************************
// Directions to implement:

// The class which contains drawing information must 
// 1) #include <QGLWidget>
// 2) inherit "[classname] : public QGLWidget"
// 3) Be declared "Q_OBJECT"
// 4) Contain two slot functions:
//  public slots:
//	    void GetDim(Vec3D* pVec);
//	    void DrawScene();

// MainWindow.h File:
//	*Create a CQOpenGL pointer
//		CQOpenGL* GLWindow;

// MainWindow.cpp File
// Constructor:
//  *Make a QGLFormat object. (Defaults should do)
//		QGLFormat format;
//	*In the Main window constructor, create a CQOpenGL instance...
//		GLWindow = new CQOpenGL(format);
//	*Add the widget in the constructor somehow. Usually to an existing layout (with spacer to aid resizing)
//		ui.horizontalLayout->addWidget(GLWindow);
//	*connect the draw and getDim signals:
//	connect(GLWindow, SIGNAL(FindDims(Vec3D*, Vec3D*)), &classname, SLOT(GetDim(Vec3D*, Vec3D*)));
//	connect(GLWindow, SIGNAL(DrawGL()), &classname, SLOT(DrawScene()));

// Optional: connect view setting functions:
//	connect(ui.Custom1Button, SIGNAL(clicked()), GLWindow, SLOT(SetViewCustom1()));
//
//
//*********************************************************************


#ifndef QOpenGL_H 
#define QOpenGL_H

#include <qgl.h>
#include <QTimer>
#include "../Voxelyze/Utils/Vec3D.h"

//Predefined views
enum ViewType {VTOP, VBOTTOM, VLEFT, VRIGHT, VFRONT, VBACK, VPERSPECTIVE}; 
#define BUFSIZE 512

struct Camera //structure representing a camera view of the scene
{
	Camera(void){};
	Camera(float XPosIn, float YPosIn, float XRotIn, float YRotIn, float TargetXIn, float TargetYIn, float TargetZIn, float PerspIn, float ZoomIn){XPos = XPosIn; YPos = YPosIn; XRot = XRotIn; YRot = YRotIn; TargetX = TargetXIn; TargetY = TargetYIn; TargetZ = TargetZIn; Persp = PerspIn; Zoom = ZoomIn;};
	GLfloat	XPos, YPos, XRot, YRot, TargetX, TargetY, TargetZ, Persp, Zoom;
};
struct GL_Light //An OpenGL light structure
{
	GL_Light(void){};
	GL_Light(int LightInd, QColor& DifIn, QColor& SpecIn, Vec3D<> PosIn){	D[0] = DifIn.redF();	D[1] = DifIn.greenF();	D[2] = DifIn.blueF();	D[3] = 1.0f;
																		S[0] = SpecIn.redF();	S[1] = SpecIn.greenF();	S[2] = SpecIn.blueF();	S[3] = 1.0f;
																		P[0] = PosIn.x;			P[1] = PosIn.y;			P[2] = PosIn.z;			P[3] = 1.0f; Index = LightInd;};
	float D[4], S[4], P[4]; //position is scaled by the size of the Envelope
	int Index; //the index of this light
	void SetLight(float Scale, Vec3D<>* Offset) {
		float sP[4] = {P[0]*Scale+Offset->x, P[1]*Scale+Offset->y, P[2]*Scale+Offset->z, P[3]};
		glEnable(Index); 
		glLightfv(Index, GL_DIFFUSE, D); 
		glLightfv(Index, GL_SPECULAR, S); 
		glLightfv(Index, GL_POSITION, sP);};
};

class CQOpenGL : public QGLWidget
{
	Q_OBJECT

signals:
	void FindDims(Vec3D<>* pVec, Vec3D<>* pOff); //signal to ask for the dimensions of the object to draw
	void FindCamTarget(Vec3D<>* pTarget);
	void DrawGL(bool FastMode); //signal to ask for whatever objecs connected to this class to be drawn
	void DrawGLOverlay(); //signal to ask for whatever 2D objecs connected to this class to be drawn

	//signals that broadcast when stuff has been clicked or changed...
	void MousePressIndex(int NameIndex); //we've clicked on something
	void FastModeChanged(bool Entering); //entering or exiting fast mode

	//get information necessary to know how to handle input:
	void WantGLIndex(bool* YN);
	void WantCoord3D(bool* YN);

	void WantAutoSaveFrames(bool* YN); //should we save every frame? (to GetFrameFilePath location)
	void GetFrameFilePath(QString* pFilePath); //where should and autocaptured frame be saved?

	//generic event handler signals
	void CtrlWheelRoll(bool Positive); //mouse wheel rolled with control held down
	void MouseMoveHover(float X, float Y, float Z); //mouse move if no mouse buttons pressed
	void LMouseMovePressed(float X, float Y, float Z); //mouse move if lmb is down
	void LMouseDown(float X, float Y, float Z, bool IsCtrl = false); //left mouse button pressed
	void LMouseUp(float X, float Y, float Z); //left mouse button released
	void PressedEscape(void); //escape key pressed

public slots:
	void ZoomExtents() {GLCenterView(); updateGL();}
	void ReqGLUpdateThis() {updateGL();}
	void SetView(int ViewIndex); //really a ViewType, but for qt's sake we'll keep it an int.
	void SetViewTop(){SetView(VTOP);}
	void SetViewBottom(){SetView(VBOTTOM);}
	void SetViewFront(){SetView(VFRONT);}
	void SetViewBack(){SetView(VBACK);}
	void SetViewRight(){SetView(VRIGHT);}
	void SetViewLeft(){SetView(VLEFT);}
	void SetViewCustom1(){SetView(VPERSPECTIVE);}

	void SetFastMode(bool IsFastMode){FastMode = IsFastMode;}
	void SetAxesVisible(bool IsVisible){bDrawAxes = IsVisible;}

	void StartAutoRedraw(int ms = 33) {AutoRedraw = true; DrawTimer->start(ms);}
	void StopAutoRedraw(){AutoRedraw = false; DrawTimer->stop();}
	void IsDrawing(bool* pIsDrawing){*pIsDrawing = Drawing;}

	void EnterFastMode(bool entering); //cuts the fat of rendering for large scenes
	void GLCenterView(bool ReZoom = true, Vec3D<>* pTargetPos = NULL); //centers the view

	void GLSaveScreenShot(); //save a screenshot of the scene (brings up save dlg)
	void GlSaveScreenShot(QString FilePath);

public:
	CQOpenGL(const QGLFormat &format, QWidget *parent = 0);
	static int TotalID; //keep track of how many openGL windows we've created in this app
	int MyID; //keep track of which OpenGL window this is...
	bool bDrawBounds;
	bool bDrawAxes;

	void GLSetBG(QColor Color) {glClearColor(Color.redF(), Color.greenF(), Color.blueF(), 1.0f);};
	void GLSetBG(bool White = false) {if (White) GLSetBG(QColor(255, 255, 255)); else GLSetBG(QColor(0,0,0));};
	int GetCurView(){return (int)CurView;};


protected: //functions and variables only needed internally
	QSize minimumSizeHint() const { return QSize(200, 200); }; //overload the sizing instructions
	QSize sizeHint() const { return QSize(1000, 1000); };

	//QT gl overloaded functions
	void initializeGL();
	void resizeGL(int width, int height);
	void paintGL();
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void wheelEvent(QWheelEvent *event);
	void mouseDoubleClickEvent(QMouseEvent *event);
	void keyPressEvent(QKeyEvent *event);

	QTimer* DrawTimer;
	bool AutoRedraw; //is the view being updated externally (IE timer)? if not, update it when we change the view...
	bool Drawing; //flag to ignore draw commands if we're still drawing a previous frame...

	Vec3D<> CurEnv; //current envelope
	Vec3D<> CurEnvOff; //current envelope offset
	bool GLGetDim(Vec3D<>* pDim, Vec3D<>* pOff);

	void GLSetPersp(void);
	void GLSetLighting(void);
	void GLDrawScene();
	void GLTranslateCam(void); //translates the camera to the current location

	//Drawing functions:
	void GLDrawBounds(void);
	void GLDrawAxes(void);

	std::vector <GL_Light> Lights;
	QRect WindowSize; //store the current window size so we can update perpective dynamically
	ViewType CurView; //current preset view
	Camera m_Cam; //camera view
	QPoint lastPos; //last point storages for mouse moving
	float lastZDepth; //zvalue for where we last sampled (stored for 3D click and drag...)
	bool IniViewSet; //so we can see if this is the first time through and set view accordingly...

	bool FastMode; //flag for if we want to draw things super fast...
	bool AskedAboutFastMode; //don't keep pestering the user if we've already asked...
	inline bool IsFastMode(void) {return FastMode;};

	//Picking functions and variable:
	int GLPickScene(int x, int y); //"draws" a section the scene in memory to see what we clicked on. returns name index
	void GLScreenToCoord(int x, int y, float& pXD, float& pYD); //for 2D views returns the world coordinates where we clicked, returns to pXD and pYD.
	void GLScreenToCoord3D(QPoint p, Vec3D<>& RetVec, bool SetZDepthHere = true);

	Vec3D<> LastPickedPoint;

};

#endif // QOpenGL_H
