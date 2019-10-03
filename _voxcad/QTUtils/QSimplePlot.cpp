/*******************************************************************************
Copyright (c) 2010, Jonathan Hiller
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * The name if its contributors may not be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#include <math.h>
#include "QSimplePlot.h"
#include <qpainter.h>
#include <qfiledialog.h>
#include <qtextstream.h>

QSimplePlot::QSimplePlot()
{
	MaxPoints=500;
	MinX = 0.0;
	MaxX = 0.0;
	MinY = 0.0;
	MaxY = 0.0;

	PlotLeftMarg=41;
	PlotRightMarg=1;
	PlotUpMarg=10;
	PlotDownMarg=1;

//	LeftMargin = 50;

}

QSimplePlot::~QSimplePlot()
{

}

void QSimplePlot::paintEvent(QPaintEvent *)
{
	QPainter painter(this);

	painter.fillRect(QRect(PlotUL(), PlotLR()), QColor(240, 240, 240));

	//draw frame
	painter.setPen(QColor(0,0,0));
	painter.drawLine(PlotUL(), PlotUR());
	painter.drawLine(PlotUR(), PlotLR());
	painter.drawLine(PlotLR(), PlotLL());
	painter.drawLine(PlotLL(), PlotUL());

	//Grid
	painter.setPen(QColor(200,200,200));

	double NumGridLines = 4;
	double ApproxGridSpace = (MaxX-MinX)/NumGridLines;
	ApproxGridSpace = QString::number(ApproxGridSpace, 'g', 1).toDouble();

	double R = fmod(MinX, ApproxGridSpace);

	for (int i=0; i<2*NumGridLines; i++){
		double Gridline = MinX + i*ApproxGridSpace - R;
		if (Gridline<=MinX || Gridline >=MaxX) continue;

		int PixGridLine = ToPlotXCoord(Gridline);
		painter.drawLine(PixGridLine, PlotUpper(), PixGridLine, PlotLower());
	}

	//Axes
	painter.setPen(QColor(0,0,0));
	painter.drawLine(PlotLeft()-3, PlotUpper(), PlotLeft()-8, PlotUpper());
	painter.drawLine(PlotLeft()-3, PlotLower(), PlotLeft()-8, PlotLower());

	painter.drawText(QPoint(0, height()-2), QString::number(MinY, 'g', 2));
	painter.drawText(QPoint(0, 10), QString::number(MaxY, 'g', 2));


	//draw line
	painter.setRenderHint(QPainter::Antialiasing);

	painter.setPen(QColor(0,0,0));

	if (X.size() > 1){
		DataMutex.lock();
		for (int i=0; i<X.size()-1; i++){
			painter.drawLine(ToPlotXCoord(X[i]), ToPlotYCoord(Y[i]), ToPlotXCoord(X[i+1]), ToPlotYCoord(Y[i+1]));
		}
		DataMutex.unlock();
	}


	painter.rotate(-90);
	painter.drawText(QPoint(-(height()/2+40), 20), QString("(Range: " + QString::number(MaxY-MinY, 'g', 2)) + ")");


}

 QSize QSimplePlot::sizeHint() const
 {
	 return QSize(100, 100);
 }

void QSimplePlot::AddPoint(double XIn, double YIn)
{
	DataMutex.lock();
	X.push_back(XIn);
	Y.push_back(YIn);


	if (X.size() == 1) { //if this is the first point
		MinX = MaxX = XIn;
		MinY = MaxY = YIn;
	}
	else {
		if (XIn>MaxX) MaxX = XIn;
		if (YIn>MaxY) MaxY = YIn;
		if (XIn<MinX) MinX = XIn;
		if (YIn<MinY) MinY = YIn;
	}

	//keep within limits
	while (X.size() > MaxPoints){
		double RemovedVal=X.front();
		X.pop_front();
		if (RemovedVal >= MaxX) MaxX = FindMax(&X);
		if (RemovedVal <= MinX) MinX = FindMin(&X);

	}
	while (Y.size() > MaxPoints){
		double RemovedVal=Y.front();
		Y.pop_front();
		if (RemovedVal >= MaxY) MaxY = FindMax(&Y);
		if (RemovedVal <= MinY) MinY = FindMin(&Y);
	}

	DataMutex.unlock();

//	MaxX = FindMax(&X);
//	MinX = FindMin(&X);
//	MaxY = FindMax(&Y);
//	MinY = FindMin(&Y);
}

double QSimplePlot::FindMax(std::deque<double>* pData)
{
	double CurMax = pData->front();
	for (std::deque<double>::iterator it = pData->begin(); it != pData->end(); it++){
		if (*it > CurMax) CurMax = *it;
	}
	return CurMax;
}

double QSimplePlot::FindMin(std::deque<double>* pData)
{
	double CurMin = pData->front();
	for (std::deque<double>::iterator it = pData->begin(); it != pData->end(); it++){
		if (*it < CurMin) CurMin = *it;
	}
	return CurMin;
}

void QSimplePlot::Reset()
{
	DataMutex.lock();
	X.clear();
	Y.clear();
	DataMutex.unlock();

	MinX=0;
	MaxX=0;
	MinY=0;
	MaxY=0;
}

void QSimplePlot::SaveData()
{
	QFile data(QFileDialog::getSaveFileName(NULL, "Save trace data", "", "Text files (*.txt)"));
	if (data.open(QFile::WriteOnly | QFile::Truncate)) {
		QTextStream out(&data);
		for (int i=0; i<X.size(); i++) {
			out << X[i] << "\t" << Y[i] << "\n";
		}
	}
	data.close();
}
