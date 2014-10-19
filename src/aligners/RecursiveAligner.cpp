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
	container.getGroup( 0 ).items[1].offset = offset_f;
	
	//Render it
	ImageEx merged = SimpleRender( SimpleRender::FOR_MERGING ).render( container );
	
	return make_pair( merged, offset_f );
}

ImageEx RecursiveAligner::align( AProcessWatcher* watcher, unsigned begin, unsigned end ){
	auto amount = end - begin;
	switch( amount ){
		case 0: qFatal( "No images to align!" );
		case 1: return image( begin )[0]; //Just return this one
		case 2: { //Optimization for two images
				ImageEx first ( image( begin   )[0], alpha( begin   ) );
				ImageEx second( image( begin+1 )[0], alpha( begin+1 ) );
				auto offset = combine( first, second );
				setPos( begin+1, offset.second );
				return offset.first;
			}
		default: { //More than two images
				//Reset position
				for( unsigned i=begin; i<end; i++ )
					setPos( i, Point<double>() );
				
				//Solve sub-areas recursively
				unsigned middle = amount / 2 + begin;
				auto first = align( watcher, begin, middle );
				auto second = align( watcher, middle, end );
				
				//Find the offset between these two images
				auto offset = combine( first, second );
				
				//Find top-left corner of first
				auto corner1 = Point<double>( numeric_limits<double>::max(), numeric_limits<double>::max() );
				auto corner2 = corner1;
				for( unsigned i=begin; i<middle; i++ ){
					corner1.x = min( corner1.x, pos(i).x );
					corner1.y = min( corner1.y, pos(i).y );
				}
				//Find top-left corner of second
				for( unsigned i=middle; i<end; i++ ){
					corner2.x = min( corner2.x, pos(i).x );
					corner2.y = min( corner2.y, pos(i).y );
				}
				
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
	
	raw = true;
	
	align( watcher, 0, count() );
	
	raw = false;
}

