/*
	This file is part of Overmix.

	Overmix is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Overmix is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Overmix.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "MainWindow.hpp"

#include "Slide.hpp"
#include "ConfusionMatrix.hpp"

#include <QApplication>
#include <QMessageBox>
#include <QDebug>

int main( int argc, char *argv[] ){
	QApplication a( argc, argv );
	
	try{
		auto args = a.arguments();
		args.removeFirst();
		if( args.size() > 0 ){
			Overmix::ConfusionMatrix matrix;
			
			for( auto arg : args ){
				Overmix::Slide s;
				s.loadXml( arg );
				
				matrix += s.evaluateInterlaze();
			}
			
			qDebug() << "Results:";
			qDebug() << "tp:" << matrix.tp;
			qDebug() << "fp:" << matrix.fp;
			qDebug() << "tn:" << matrix.tn;
			qDebug() << "fn:" << matrix.fn;
			
			return 0;
		}
		else{
			Overmix::MainWindow w;
			w.show();
			return a.exec();
		}
	}
	catch( std::exception& e ){
		QMessageBox::critical( nullptr, "An uncaught error occurred", e.what() );
		return -1;
	}
}
