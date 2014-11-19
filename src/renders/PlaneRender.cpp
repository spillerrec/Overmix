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
#include "../planes/ImageEx.hpp"
#include "../color.hpp"

#include <QTime>
#include <QRect>
#include <vector>
using namespace std;

Plane PlaneRender::renderPlane( const AContainer& aligner, int plane, unsigned max_count, AProcessWatcher* watcher ) const{
	auto scale = aligner.image( 0 )[plane].getSize().to<double>() / aligner.image( 0 )[0].getSize().to<double>();
	
	//Create output plane
	Plane out( (Point<>( aligner.size().size() ) * scale).round() );
	out.fill( color::BLACK );
	
	//Initialize PlaneItInfos
	auto full = (Point<>( aligner.size().topLeft() ) * scale ).round();
	vector<PlaneItInfo> info;
	info.push_back( PlaneItInfo( out, full.x,full.y ) );
	
	for( unsigned i=0; i<max_count; i++ )
		info.push_back( PlaneItInfo(
				const_cast<Plane&>( aligner.image( i )[plane] ) //TODO: FIX!!!
			,	round( aligner.pos(i).x * scale.x )
			,	round( aligner.pos(i).y * scale.y )
			) );
	
	//Execute
	MultiPlaneIterator it( info );
	it.data = data();
	it.iterate_all();
	it.for_all_pixels( pixel(), watcher, 1000 * plane );
	
	return out;
}

ImageEx PlaneRender::render( const AContainer& aligner, unsigned max_count, AProcessWatcher* watcher ) const{
	if( max_count > aligner.count() )
		max_count = aligner.count();
	
	//Abort if no images
	if( max_count == 0 ){
		qWarning( "No images to render!" );
		return ImageEx();
	}
	qDebug( "render_image: image count: %d", (int)max_count );
	
	
	unsigned planes_amount = aligner.image(0).size();
	if( watcher )
		watcher->setTotal( 1000 * planes_amount );
	
	//Render all planes
	ImageEx img( aligner.image(0).get_system() );
	for( unsigned c=0; c<planes_amount; ++c )
		img.addPlane( renderPlane( aligner, c, max_count, watcher ) );
	return img;
}

