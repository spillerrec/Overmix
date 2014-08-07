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


#include "PlaneRender.hpp"
#include "../containers/AContainer.hpp"
#include "../ImageEx.hpp"
#include "../color.hpp"

#include <QTime>
#include <QRect>
#include <vector>
using namespace std;


ImageEx PlaneRender::render( const AContainer& aligner, unsigned max_count, AProcessWatcher* watcher ) const{
	if( max_count > aligner.count() )
		max_count = aligner.count();
	
	//Abort if no images
	if( max_count == 0 ){
		qWarning( "No images to render!" );
		return ImageEx();
	}
	qDebug( "render_image: image count: %d", (int)max_count );
	
	//Do iterator
	QRect full = aligner.size();
	ImageEx img( ImageEx::GRAY );
	img.create( 1, 1 ); //TODO: set as initialized
	
	
	//Create output plane
	Plane out( full.width(), full.height() );
	out.fill( color::BLACK );
	img[0] = out;
	
	//Initialize PlaneItInfos
	vector<PlaneItInfo> info;
	info.push_back( PlaneItInfo( img[0], full.x(),full.y() ) );
	
	for( unsigned i=0; i<max_count; i++ )
		info.push_back( PlaneItInfo(
				aligner.plane( i, 0 )
			,	round( aligner.pos(i).x() )
			,	round( aligner.pos(i).y() )
			) );
	
	if( watcher )
		watcher->setTotal( 1000 );
	
	//Execute
	MultiPlaneIterator it( info );
	it.data = data();
	it.iterate_all();
	it.for_all_pixels( pixel(), watcher );
	
	return img;
}



