#ifndef QVX_INTERFACES_H
#define QVX_INTERFACES_H

//wraps classes with QT slots, dialog interfaces, etc:

#include <QWidget>
#include "../Voxelyze/VX_Object.h"
#include "../Voxelyze/VX_FEA.h"
#include "../Voxelyze/VX_Environment.h"
#include "../Voxelyze/VX_Sim.h"
#include "../Voxelyze/VXS_SimGLView.h"
#include <QFileDialog>
#include "../QTUtils/QThreadWrap.h"
#include <qsettings.h>
#include "../QTUtils/QOpenGL.h"

//for multithreading
#include <QtConcurrentMap>

class QVX_Object : public QWidget, public CVX_Object
{
	Q_OBJECT

public:
	
	QVX_Object(QWidget *parent = 0) {Path = "";};
	~QVX_Object(){};
	QVX_Object& operator=(const QVX_Object& RefObj); //overload "=" 

//	Path = RefObj.Path;


public slots:
	void GetDim(Vec3D<>* pVec, Vec3D<>* pOff) {GetWorkSpace(pVec); *pOff = Vec3D<>(0,0,0);};

	//high level file I/O functions
	void New(void) {Close(); InitializeMatter(0.001, 10, 10, 10);};
	void Save(int Compression = CP_ASCIIREADABLE, bool NewLoc = false, QString* pNewFilenameOut = NULL);
	void SaveZLib(void) {Save(CP_ZLIB);};
	void SaveAsZLib(QString* pFilenameOut = NULL) {Save(CP_ZLIB, true, pFilenameOut);};
	void SaveAsBase64() {Path = ""; Save(CP_BASE64);};
	void SaveAsAsciiReadable() {Path = ""; Save(CP_ASCIIREADABLE);};
	void ExportSTL(void);
	void ExportXYZ(void);
	void ExportKV6(void);

	bool Open(QString* pFilenameOut = NULL);
	void Close(void) {ClearMatter(); Path = "";};
	
	bool OpenPal(void); //open material palette
	bool SavePal(void); //save material palette

	void GetVXCInfoString(QString* pString) {std::string tmp; GetVXCInfoStr(&tmp); *pString = QString(tmp.c_str());};
	void GetVoxInfoString(int Index, QString* pString) {std::string tmp; GetVoxInfoStr(Index, &tmp); *pString = QString(tmp.c_str());};

//	QIcon GenerateMatIcon(int MatIndex);

protected:
	std::string Path; //current file path

private:
	QString GetLastDir(){return QSettings().value("CurrentDir", "").toString();}
	void SetLastDir(QString FullPath){QString SelDir = QFileInfo(FullPath).path(); QSettings().setValue("CurrentDir", SelDir);}
};

class QVX_FEA : public QWidget, public CVX_FEA
{
	Q_OBJECT

public:
	QVX_FEA(QWidget *parent = 0){};
	~QVX_FEA(){};

	int GetCurSel(void) {int tmp; emit GetCurGLSelected(&tmp); return tmp;};
signals:
	void SolveResult(bool);
	void GetCurGLSelected(int* CurSel);

public slots:

	void RequestSolveSystem(void);
	void ExecuteSolveSystem(QString* Param = NULL);
	void DrawScene(void) {DrawFEA(GetCurSel());}
//	void SetViewConstraints(bool View) {ViewConstraints = View;}
	void SetViewDisplaced(bool View) {if (View) ViewDefPerc = 2.0; else ViewDefPerc = 0.0;}
	void SetViewModeDisplacement(void) {ViewMode = VIEW_DISP;}
	void SetViewModeForce(void) {ViewMode = VIEW_FORCE;}
	void SetViewModeStrain(void) {ViewMode = VIEW_STRAIN;}
	void SetViewModeReaction(void) {ViewMode = VIEW_REACTION;}



	void GetFEAInfoString(QString* pString) {std::string tmp; GetFEAInfoStr(&tmp); *pString = QString(tmp.c_str());};
	void GetFEAInfoString(int VoxIndex, QString* pString) {std::string tmp; GetFEAInfoStr(VoxIndex, &tmp); *pString = QString(tmp.c_str());};

private:
	
};


class QVX_Environment : public QWidget, public CVX_Environment
{
	Q_OBJECT

public:
	QVX_Environment(QWidget *parent = 0){};
	~QVX_Environment(){};

signals:
	void BCsChanged();

public slots:
	bool OpenBCs(void);
	void SaveBCs(void);

private:
	QString GetLastDir(){return QSettings().value("CurrentDir", "").toString();}
	void SetLastDir(QString FullPath){QString SelDir = QFileInfo(FullPath).path(); QSettings().setValue("CurrentDir", SelDir);}

};

class QVX_Sim : public QWidget, public CVX_Sim
{
	Q_OBJECT

public:
	
//	QVX_Sim(CVX_Environment* pSimEnvIn, CMesh* pSimMeshIn = 0, QWidget *parent = 0);
	QVX_Sim(QWidget *parent = 0);
	~QVX_Sim(){EndSim();}

	QGLWidget* pGLWin; //need this pointer to render text... :P
	CVXS_SimGLView* pSimView;

	Thread SimThread; //the simulation
	QString SimMessage; //simulation has access to change this...

	bool LogEvery; //do we want to log every data point, or at fixed (real) time intervals?
	float ApproxMSperLog;

//	CVX_Environment* pSimEnv;
//	CMesh* pSimMesh;
	
	bool Running; //simulation running?
	bool Paused; //sim paused?
	bool StopSim; //Stop Simulation?

	bool LockCoMToCenter; //keep the view centered on the center of mass
	bool HideBoundingBox;

	//reimplemented timestep and integration functions to multithread this baby with QT threadpool!
	//bool TimeStepMT(std::string* pRetMessage = NULL);
	//void IntegrateMT(IntegrationType Integrator = I_EULER);

	//Input voxel (moved by user interactively or otherwise
	int CurXSel; //current selection... (VXC index...)

	//Overlay Drawing
	void DrawOverlay(void);

	//video capture
	bool Recording; //are we currently dumping frames to video?
	void BeginRecording(void); //opens record dialog and begins recording
	void EndRecording(void); //stops recording

	int GLUpdateEveryNFrame; //updates GL window every N frames. -1 is off. If on, ensure StopExternalGLUpdate();

signals:
	void BCsChanged();
	void IsStatusTextVisible(bool* pIsVisible);
	void IsPlotVisible(bool* pIsVisible);
	void GetPlotRqdStats(char* pStatsRequired); //returns the stats the ui needs (i.e. plot data)
	void ReqAddPlotPoint(double time); //adds a point to the plot...
	void UpdateText(QString);
	void SimEndedInternally(QString); //emitted when simulation loop kicks out (unexpectedly)
	void ReqGLUpdate(void);
	void ReqUiUpdate(void);
	void ReqGLDrawingStatus(bool*);
	void StartExternalGLUpdate(int);
	void StopExternalGLUpdate();
	void ResizeGLWindow(int, int);
	void ResetGLWindow();


public slots:
	void SaveVXA(QString* pFilenameOut = NULL);
	bool OpenVXA(QString* pFileNameOut = NULL);

	void ExportDeformedSTL(void);


	void RequestBeginSim();
	void SimLoop(QString* pSimMessage = NULL);
	void SimPauseToggle();
	void EndSim();
	void ResetSim();
	void CatchInternalSimEnding(QString Msg);

	void LMouseDown(Vec3D<> P);
	void LMouseUp(Vec3D<> P);
	void LMouseDownMove(Vec3D<> P);

	void GetCoM(Vec3D<>* pCoM) {if (Running && LockCoMToCenter) *pCoM = SS.CurCM;}

	void WantFramesAutoSaved(bool* YN){*YN = Recording;}
	void AutoSavePath(QString* pFilePath) {
		*pFilePath = VideoOutputFolder + QString::number(CurVideoFrameNumber).rightJustified(6, '0', false) +".jpg";
		CurVideoFrameNumber++;
	}

public:
//	void BondCalcForce(std::vector<CVXS_Bond>::iterator Bond) {(*Bond).CalcForce();}

//?	static void BondCalcForce(CVXS_Bond &Bond) {Bond.UpdateBond();}
//	static void VoxEulerStep(CVXS_Voxel &Voxel) {Voxel.EulerStep();}

private: 
//	void SetGlUpdate();
	bool ActuallyPaused;
	QString VideoOutputFolder;
	int CurVideoFrameNumber;

	QString GetLastDir(){return QSettings().value("CurrentDir", "").toString();}
	void SetLastDir(QString FullPath){QString SelDir = QFileInfo(FullPath).path(); QSettings().setValue("CurrentDir", SelDir);}

	//for interacting with structure
//	bool Dragging; //Is the input voxel active
	CVXS_Voxel* DraggingVoxel;
	vfloat DraggingStiffness;
	Vec3D<> DraggingOffset;
	Vec3D<> InputPoint;
};



#endif // QVX_INTERFACES_H
