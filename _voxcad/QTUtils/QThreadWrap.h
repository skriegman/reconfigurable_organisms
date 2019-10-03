/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * The name if its contributors may not be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/


/*******************************************************************************
Instructions on implementing QThreadWrap Thread:
1) Create an instance of this class as a member variable of the class "A" containing the function you want to thread
Thread SimThread;

2) Create a QT slot function an class "A" that calls the desired function to thread with a single QString* parameter
public slots:
void SimLoop(QString* pSimMessage = NULL);

3) Connect the thread to this function in the constructor of class "A":
connect(&SimThread, SIGNAL(CallFunc(QString*)), this, SLOT(SimLoop(QString*)), Qt::DirectConnection);

4) (Optional) Link parameters to make a progress bar and have ability to cancel thread:
4a) in class "A" header, add member variables:
	int CurTick, TotalTick;
	bool CancelFlag;
	std::string ProgressMessage;

4b) in class "A" constructor, intialize variables and link them to thread object
	CurTick = 0;
	TotalTick = 1;
	ProgressMessage = "Initializing...";
	CancelFlag = false;
	TensileThread.LinkProgress(&CurTick, &TotalTick, &ProgressMessage, &CancelFlag);

Update these parameters during the course of the threaded function to supply information to the user and allow the user to cancel (threaded function should end if Cancelflag is set to true...)

5) Execute the thread. A parameter of true will not return from Execute() until the thread exits.
SimThread.Execute(false);
*******************************************************************************/

#ifndef THREAD_H
#define THREAD_H

#include <QThread>

class Thread : public QThread
{
	Q_OBJECT

public:
	Thread();
	Thread(QString* SArg1In, bool WantTimedIn = false);
	~Thread();


signals:
	void CallFunc(QString* SArg1); //signal to call whatever we want from this thread...


public:
	void ClearAll(){disconnect(this); SArg1 = NULL; pCancelFlag = NULL; pCurTick = NULL; pCurMaxTick = NULL; pCurMessage = NULL; WantProgress = false; WantTimed = false;};
	
	void LinkProgress(int* pCurTickIn, int* pCurMaxTickIn, std::string* pCurMessageIn, bool* pCancelFlagIn) {WantProgress = true; pCurTick = pCurTickIn; *pCurTick = 0; pCurMaxTick = pCurMaxTickIn; *pCurMaxTick = 0; pCurMessage = pCurMessageIn; *pCurMessage = ""; pCancelFlag = pCancelFlagIn; *pCancelFlag = false;};

	QString* SArg1;

	bool *pCancelFlag; //pointer to a bool to set to cancel the process
	int *pCurTick, *pCurMaxTick; //pointers to progress bar statistics
	std::string *pCurMessage; //pointer to current message to display in progress window
	bool WantProgress, WantTimed;

	void Execute(bool WaitTilDone = true); //starts the thread, waits til complete; Updates progress bar, watches for cancel.

protected: 
	void run(); //called automatically when we start the thread

};

#endif // THREAD_H
