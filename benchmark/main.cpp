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

#include "../src/containers/ImageContainer.hpp"
#include "../src/containers/ImageContainerSaver.hpp"

#include "../src/aligners/RecursiveAligner.hpp"
#include "../src/aligners/AverageAligner.hpp"
#include "../src/renders/AverageRender.hpp"
#include "../src/renders/FloatRender.hpp"

#include <QCoreApplication>
#include <QFileInfo>
#include <QDebug>
#include <QTime>

int main( int argc, char *argv[] ){
	QCoreApplication a( argc, argv );
	ImageContainer images;
	
	auto args = a.arguments();
	args.removeFirst();
	if( args.size() == 0 ){
		qDebug() << "Needs files to load";
		return -1;
	}
	
	QTime t;
	t.start();
	
	for( auto arg : args ){
		if( QFileInfo( arg ).completeSuffix() == "xml.overmix" )
			ImageContainerSaver::load( images, arg );
		else
			images.addImage( ImageEx::fromFile( arg ), -1, -1, arg );
	}
	
	qDebug() << "Loading took: " << t.restart();
	
	//*
	RecursiveAligner aligner( images, AImageAligner::ALIGN_VER, 1 );
	//AverageAligner aligner( images, AImageAligner::ALIGN_VER, 1 );
	aligner.addImages();
	qDebug() << "Preparing alignment took: " << t.restart();
	//return 0;
	aligner.align();
	qDebug() << "Aligning took: " << t.restart();
	//return 0;
	
	auto img( AverageRender().render( images ) );
	//auto img( FloatRender().render( images ) );
	
	qDebug() << "Rendering took: " << t.restart();
	
	img.to_qimage( ImageEx::SYSTEM_REC709, ImageEx::SETTING_GAMMA | ImageEx::SETTING_DITHER );
	qDebug() << "to_qimage() took: " << t.restart();
	
	qDebug() << "Done";
	
	return 0;
}
