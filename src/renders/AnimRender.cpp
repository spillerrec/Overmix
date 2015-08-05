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

AnimRender::AnimRender( const AContainer& aligner, ARender& render, AProcessWatcher* watcher ){
	for( auto frame : aligner.getFrames() )
		frames.addImage( render.render( FrameContainer( const_cast<AContainer&>(aligner), frame ) ) );
	//TODO: const_cast
}

Plane getGray( ImageEx img ){
	img.to_grayscale();
	return std::move( img[0] );
}

int ID = 0;
Plane difference( Plane base, Plane& other ){
	//Create mask
	base.difference( other );
	base.binarize_threshold( color::from8bit( 1 ) ); //TODO: constant as setting
	
	//Dilate needs the colors reversed
	base = base.level( color::BLACK, color::WHITE, color::WHITE, color::BLACK, 1.0 );
	base = base.dilate( 3 ); //TODO: constant as setting
	
	//ImageEx( base ).to_qimage( ImageEx::SYSTEM_REC709 ).save( "base" + QString::number( ID++ ) + ".png" );
	
	return base;
}

Plane difference( const ImageEx& base, /*const*/ ImageEx& other ){
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
	auto/*&*/ base = frames.image( frame );
	for( unsigned i=0; i<frames.count(); i++ )
		frames.setMask( i, frames.addMask( difference( frames.image( i ), base ) ) );
	
	return AverageRender().render( frames, watcher );
}



