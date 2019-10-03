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
Instructions 
*******************************************************************************/

/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * The name if its contributors may not be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#ifndef QSIMPLEPLOT_H
#define QSIMPLEPLOT_H

#include <qwidget.h>
#include <deque>
#include <qmutex.h>

class QSimplePlot : public QWidget
{
	Q_OBJECT

public:
	QSimplePlot();
	~QSimplePlot();

	void AddPoint(double XIn, double YIn);
	void SetMaxToShow(int MaxIn) {MaxPoints = MaxIn;}
	void Reset();
	void SaveData();

 protected:
     void paintEvent(QPaintEvent *event);
	 virtual QSize sizeHint() const;

	 QMutex DataMutex;

	 int MaxPoints; //the maximum number of points to keep...
	 std::deque<double> X, Y;

	 double MinX, MaxX, MinY, MaxY;
	 int PlotLeftMarg, PlotRightMarg, PlotUpMarg, PlotDownMarg; //in pixels
	 double PlotUpper() {return 0;}
	 double PlotLower() {return height()-1;}
	 double PlotLeft() {return PlotLeftMarg-1;}
	 double PlotRight() {return width()-1;}


	 QPoint PlotUL() {return QPoint(PlotLeft(), PlotUpper());}
	 QPoint PlotUR() {return QPoint(PlotRight(), PlotUpper());}
	 QPoint PlotLL() {return QPoint(PlotLeft(), PlotLower());}
	 QPoint PlotLR() {return QPoint(PlotRight(), PlotLower());}


	 double FindMax(std::deque<double>* pData);
	 double FindMin(std::deque<double>* pData);

	 double ToPlotXCoord(double DataXVal) {return (DataXVal-MinX)/(MaxX-MinX)*(width()-PlotLeftMarg-PlotRightMarg)+PlotLeftMarg;}
	 double ToPlotYCoord(double DataYVal) {return PlotUpMarg + (height()-PlotUpMarg-PlotDownMarg)-((DataYVal-MinY)/(MaxY-MinY)*(height()-PlotUpMarg-PlotDownMarg));}



};

#endif // QSIMPLEPLOT_H
