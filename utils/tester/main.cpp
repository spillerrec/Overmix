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

using namespace std;

using Points = vector<Point<double>>;

Points extractAlignment( const AContainer& container ){
	Points points( container.count() );
	for( unsigned i=0; i<points.size(); ++i )
		points[i] = container.pos( i );
	return points;
}

Size<double> difference( const Points& p1, const Points& p2 ){
	Size<double> error( 0, 0 );
	for( unsigned i=0; i<p1.size(); ++i ){
		qDebug() << "x: " << p1[i].y << " - " << p2[i].y;
		error += (p1[i] - p2[i]).abs();
	}
	return error;
}

Points align( AImageAligner&& aligner ){
	aligner.addImages();
	aligner.align();
	//Make sure the first image starts on (0,0)
	aligner.offsetAll( Point<double>() - aligner.minPoint() ); //TODO: define -
	return extractAlignment( aligner );
}

int main( int argc, char *argv[] ){
	QCoreApplication a( argc, argv );
	ImageContainer images;
	
	auto args = a.arguments();
	args.removeFirst();
	if( args.size() == 0 ){
		qDebug() << "Needs files to load";
		return -1;
	}
	
	for( auto arg : args ){
		if( QFileInfo( arg ).completeSuffix() == "xml.overmix" )
			ImageContainerSaver::load( images, arg );
		else
			images.addImage( ImageEx::fromFile( arg ), -1, -1, arg );
	}
	
	auto rec_align = align( RecursiveAligner( images, AImageAligner::ALIGN_VER, 2 ) );
	auto avg_align = align( AverageAligner( images, AImageAligner::ALIGN_VER, 2 ) );
	
	auto result = difference( avg_align, rec_align ) / images.count();
	qDebug() << result.x << " - " << result.y;
	
	return 0;
}
