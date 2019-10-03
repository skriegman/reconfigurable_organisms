#include "QThreadWrap.h"
#include <QProgressDialog>
#include <QApplication>
#include <time.h>
#include <QMessageBox>
#include <sstream>


Thread::Thread()
{
	ClearAll();
}

Thread::Thread(QString* SArg1In, bool WantTimedIn)
{
	ClearAll();
	SArg1 = SArg1In;
	WantTimed = WantTimedIn;
}

Thread::~Thread()
{

}

void Thread::run() //called automatically when we start the thread
{
	emit CallFunc(SArg1); //must be directly connected to thread for progress bar to work! (
}

void Thread::Execute(bool WaitTilDone) { //starts the thread, waits til complete;
	clock_t startTime, finishTime;
	startTime = clock();

	QProgressDialog progress;
	progress.setModal(true);
	if (pCancelFlag) *pCancelFlag = false; //haven't canceled before we started!

	start(); 

	if (WantProgress) progress.show();

	if (WantProgress || WantTimed || WaitTilDone){ //if we want to stick around until the thread is finished...
		while (isRunning()){
			if (WantProgress){
				progress.setLabelText(QString((*pCurMessage).c_str()));
				progress.setRange(0, *pCurMaxTick);
				progress.setValue(*pCurTick);
				QApplication::processEvents();
				if (progress.wasCanceled()){
					*pCancelFlag = true;
					wait();
					return; //returns
				}
			}
			msleep(50);
		}
	}

	finishTime = clock();
	double duration = (double)(finishTime - startTime) / CLOCKS_PER_SEC;
	std::ostringstream os;
	os << "Elapsed time: " << duration << " Sec";
	if (WantTimed) QMessageBox::warning(NULL, "Warning", (os.str()).c_str());
}


