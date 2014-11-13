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


#include "RecursiveAligner.hpp"
#include "../containers/ImageContainer.hpp"
#include "../renders/SimpleRender.hpp"

#include <limits>
using namespace std;

pair<ImageEx,Point<double>> RecursiveAligner::combine( const ImageEx& first, const ImageEx& second ) const{
	ImageOffset offset = findOffset( first, second );
	Point<double> offset_f( offset.distance_x, offset.distance_y );
	
	//Wrap planes in ImageContainer
	//TODO: Optimize this
	ImageContainer container;
	container.addImage( ImageEx( first ) ); //We are having copies here!!
	container.addImage( ImageEx( second ) );
	container.setPos( 1, offset_f );
	
	//Render it
	return { SimpleRender( SimpleRender::FOR_MERGING ).render( container ), offset_f };
}

static void addToWatcher( AProcessWatcher* watcher, int add ){
	if( watcher )
		watcher->setCurrent( watcher->getCurrent() + add );
}

ImageEx RecursiveAligner::align( AProcessWatcher* watcher, unsigned begin, unsigned end ){
	auto amount = end - begin;
	switch( amount ){
		case 0: qFatal( "No images to align!" );
		case 1: addToWatcher( watcher, 1 );
				return { image( begin )[0], alpha( begin) }; //Just return this one
		case 2: { //Optimization for two images
				ImageEx first ( image( begin   )[0], alpha( begin   ) );
				ImageEx second( image( begin+1 )[0], alpha( begin+1 ) );
				auto offset = combine( first, second );
				setPos( begin+1, pos(begin) + offset.second );
				addToWatcher( watcher, 2 );
				return offset.first;
			}
		default: { //More than two images
				//Solve sub-areas recursively
				unsigned middle = amount / 2 + begin;
				auto offset = combine( align( watcher, begin, middle ), align( watcher, middle, end ) );
				
				//Find top-left corner of first
				auto corner1 = Point<double>( numeric_limits<double>::max(), numeric_limits<double>::max() );
				auto corner2 = corner1;
				for( unsigned i=begin; i<middle; i++ )
					corner1 = corner1.min( pos(i) );
				//Find top-left corner of second
				for( unsigned i=middle; i<end; i++ )
					corner2 = corner2.min( pos(i) );
				
				//move all in "middle to end" using the offset
				for( unsigned i=middle; i<end; i++ )
					setPos( i, pos( i ) + corner1 + offset.second - corner2 );
				
				return offset.first; //Return the combined image
			}
	}
}

void RecursiveAligner::align( AProcessWatcher* watcher ){
	if( count() == 0 )
		return;
	
	ProgressWrapper( watcher ).setTotal( count() );
	
	raw = true;
	align( watcher, 0, count() );
	raw = false;
}

