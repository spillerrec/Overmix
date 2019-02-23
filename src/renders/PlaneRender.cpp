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

#include "../MultiPlaneIterator.hpp"

#include <QTime>
#include <vector>
using namespace std;
using namespace Overmix;

Plane PlaneRender::renderPlane( const AContainer& aligner, int plane, AProcessWatcher* watcher ) const{
	auto scale = aligner.image( 0 )[plane].getSize().to<double>() / aligner.image( 0 )[0].getSize().to<double>();
	
	//Create output plane
	Plane out( (Point<>( aligner.size().size ) * scale).round() );
	out.fill( color::BLACK );
	
	//Initialize PlaneItInfos
	vector<PlaneItInfo> info;
	info.emplace_back( out, (aligner.size().pos * scale).round() );
	
	for( auto align : aligner )
		info.emplace_back( const_cast<Plane&>( align.image()[plane] ), (align.pos() * scale).round() );
		//TODO: FIX!!!
	
	//Execute
	MultiPlaneIterator it( info );
	it.data = data();
	it.iterate_all();
	it.for_all_pixels( pixel(), watcher, 1000 * plane );
	
	return out;
}

ImageEx PlaneRender::render( const AContainer& aligner, AProcessWatcher* watcher ) const{
	unsigned planes_amount = aligner.image(0).size();
	planes_amount = min( planes_amount, max_planes );
	Progress progress( "PlaneRender", planes_amount, watcher );
	
	//Render all planes
	auto color_space = aligner.image(0).getColorSpace();
	ImageEx img( (planes_amount==1) ? color_space.changed( Transform::GRAY ) : color_space );
	for( unsigned c=0; c<planes_amount; ++c ){
		if( progress.shouldCancel() )
			return {};
		img.addPlane( renderPlane( aligner, c, progress.makeSubtask() ) );
	}
	return img;
}

