#ifndef QVX_TENSILETEST_H
#define QVX_TENSILETEST_H

#include "../Voxelyze/VX_Sim.h"
#include "../QTUtils/QThreadWrap.h"
#include <qwidget.h>
#include "../Voxelyze/VXS_SimGLView.h"
#include <qsettings.h>

//class QVX_Environment;
class QVX_Sim;

class QVX_TensileTest : public QWidget, public CVX_Sim
{
	Q_OBJECT

public:
	QVX_TensileTest();
	~QVX_TensileTest();

	CVXS_SimGLView* pTensileView;
	Thread TensileThread;
	QString OutFilePath;

	bool DoBCChecks(void); //returns true if passes all checks
	bool IsBasicTensile; //true if two BC, one fixed, one displaced axially at two ends.
	double CSArea; //cross sectional area of the tensile specimen.
	double IniLength; //initial length of the tensile specimen.
	Axis TensileAxis;
	bool TestRunning;

	void EnableAutoConverge(double ConvExp){AutoConverge=true; AutoConvergeExp=ConvExp;}
	void DisableAutoConverge() {AutoConverge=false;}

public slots:
	void BeginTensileTest(QVX_Sim* pSim, int NumStepIn, double ConvThreshIn, Vec3D<double> MixRadIn = Vec3D<>(0,0,0), MatBlendModel ModelIn = MB_LINEAR, double PolyExpIn = 1.0);
//	void BeginTensileTest(QVX_Environment* pEnvIn, int NumStepIn, double ConvThreshIn, double MixRadiusIn = 0.0, MatBlendModel ModelIn = MB_LINEAR, double PolyExpIn = 1.0);
	void RunTensileTest(QString* pMessage = NULL);

signals:
	void StartExternalGLUpdate(int);
	void StopExternalGLUpdate();

private:
	void RenderMixedObject(CVX_Object* pSrcObj, CVX_Object* pDestObj, Vec3D<> MixRadius);

	int NumStep;
	bool AutoConverge;
	double AutoConvergeExp; //in orders of magnitude below the maximum voxel motion at the beginning of the simulation
	double ConvThresh;
//	double MixRadius;

	//for threading:
	int CurTick, TotalTick;
	bool CancelFlag;
	std::string ProgressMessage;

	CVX_Environment LocEnv; //local objects to run the test on so we don't modify the main object..
	CVX_Object LocObj;
	QString GetLastDir(){return QSettings().value("CurrentDir", "").toString();}

};

#endif // QVX_TENSILETEST_H
