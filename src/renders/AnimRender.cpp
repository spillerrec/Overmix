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


#include "AnimRender.hpp"
#include "../color.hpp"
#include "../planes/PlaneBase.hpp"


#include "../renders/AverageRender.hpp"
#include "../containers/FrameContainer.hpp"
#include "../planes/ImageEx.hpp"

#include <QTime>
#include <vector>
using namespace std;
using namespace Overmix;

static Plane expand( const Plane& p, Size<unsigned> size, Point<unsigned> pos ){
	if( !p )
		return {};
	
	Plane out( size );
	out.fill( 0 );
	out.copy( p, {0,0}, p.getSize(), pos );
	return out;
}

static Plane reduce( const Plane& p, Size<unsigned> size, Point<unsigned> pos ){
	Plane out( size );
	out.copy( p, pos, size, {0,0} );
	return out;
}

template<typename Func>
ImageEx modify( const ImageEx& img, Size<unsigned> size, Point<unsigned> pos, Func f ){
	ImageEx out( img.getColorSpace() );
	
	auto max_size = img.getSize();
	for( auto i=0u; i<img.size(); i++ ){
		auto scale = img[i].getSize().to<double>() / max_size.to<double>();
		out.addPlane( f( img[i], (size.to<double>()*scale).round(), (pos.to<double>()*scale).round() ) );
	}
	out.alpha_plane() = f( img.alpha_plane(), size, pos );
	
	return out;
}

AnimRender::AnimRender( const AContainer& aligner, ARender& render, AProcessWatcher* watcher ){
	auto min_point = aligner.minPoint();
	auto size = aligner.size().size;
	
	for( auto frame : aligner.getFrames() ){
		FrameContainer frame_con( const_cast<AContainer&>(aligner), frame ); //TODO: const_cast
		auto pos = frame_con.minPoint() - min_point;
		auto img = render.render( frame_con );
		frames.addImage( modify( img, size, pos, expand ) );
		old.emplace_back( pos, img.getSize() );
	}
}

Plane difference( Plane base, Plane other ){
	//Create mask
	base.difference( other );
	base.binarize_threshold( color::from8bit( 1 ) ); //TODO: constant as setting
	
	//Dilate needs the colors reversed
	base = base.level( color::BLACK, color::WHITE, color::WHITE, color::BLACK, 1.0 );
	base = base.dilate( 3 ); //TODO: constant as setting
	
	return base;
}

Plane difference( const ImageEx& base, const ImageEx& other ){
	Plane mask( base.getSize() );
	mask.fill( color::WHITE );
	
	for( unsigned i=0; i<base.size(); i++ ){
		auto img = difference( base[i], other[i] );
		if( mask.getSize() == img.getSize() )
			mask = mask.minPlane( img );
		else
			mask = mask.minPlane( img.scale_cubic( mask.getSize() ) );
	}
	mask = mask.minPlane( other.alpha_plane() );
	mask = mask.minPlane( base.alpha_plane() );
	
	return mask;
}

ImageEx AnimRender::render( int frame, AProcessWatcher* watcher ){
	if( frame < 0 )
		return AverageRender().render( frames, watcher );
	
	//TODO: remove old masks
	
	//Create masks
	auto& base = frames.image( frame );
	for( unsigned i=0; i<frames.count(); i++ )
		frames.setMask( i, frames.addMask( difference( frames.image( i ), base ) ) );
	
	//TODO: reduce
	return modify( AverageRender().render( frames, watcher ), old[frame].size, old[frame].pos, reduce );
}



