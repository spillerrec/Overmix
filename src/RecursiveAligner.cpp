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
#include "FakeAligner.hpp"
#include "SimpleRender.hpp"

#include <limits>
using namespace std;

/*const*/ Plane* RecursiveAligner::prepare_plane( /*const*/ Plane* p ){
	if( use_edges )
		return p->edge_sobel();
	else
		return p;
}

QPointF RecursiveAligner::min_point() const{
	if( images.size() == 0 )
		return QPointF(0,0);
	
	QPointF min = images[0].pos;
	for( auto img : images ){
		if( img.pos.x() < min.x() )
			min.setX( img.pos.x() );
		if( img.pos.y() < min.y() )
			min.setY( img.pos.y() );
	}
	
	return min;
}

pair<Plane*,QPointF> RecursiveAligner::combine( const Plane& first, const Plane& second ) const{
	ImageOffset offset = find_offset( first, second );
	QPointF offset_f( offset.distance_x, offset.distance_y );
	
	//Wrap planes in ImageEx*
	ImageEx img1( ImageEx::GRAY );
	ImageEx img2( ImageEx::GRAY );
	img1.replace_plane( 0, new Plane( first ) );
	img2.replace_plane( 0, new Plane( second ) );
	
	//Make aligner for rendering the result
	FakeAligner aligner;
	aligner.set_raw( true );
	aligner.add_image( &img1 );
	aligner.add_image( &img2 );
	aligner.setPos( 1, offset_f );
	
	//Render it
	ImageEx* merged = SimpleRender( SimpleRender::FOR_MERGING ).render( aligner );
	Plane *out = new Plane( *((*merged)[0]) );
	delete merged;
	
	return make_pair( out, offset_f );
}

Plane* RecursiveAligner::align( AProcessWatcher* watcher, unsigned begin, unsigned end ){
	auto amount = end - begin;
	switch( amount ){
		case 0: qFatal( "No images to align!" );
		case 1: return new Plane( *(images[begin].image) ); //Just return this one
		default: { //More than two images
				//Solve sub-areas recursively
				unsigned middle = amount / 2 + begin;
				Plane* first = align( watcher, begin, middle );
				Plane* second = align( watcher, middle, end );
				
				//Find the offset between these two images
				auto offset = combine( *first, *second );
				
				//Find top-left corner of first
				QPointF corner1( numeric_limits<double>::max(), numeric_limits<double>::max() );
				for( unsigned i=begin; i<middle; i++ ){
					corner1.setX( min( corner1.x(), images[i].pos.x() ) );
					corner1.setY( min( corner1.y(), images[i].pos.y() ) );
				}
				//Find top-left corner of second
				QPointF corner2( numeric_limits<double>::max(), numeric_limits<double>::max() );
				for( unsigned i=middle; i<end; i++ ){
					corner2.setX( min( corner2.x(), images[i].pos.x() ) );
					corner2.setY( min( corner2.y(), images[i].pos.y() ) );
				}
				
				//move all in "middle to end" using the offset
				for( unsigned i=middle; i<end; i++ )
					images[i].pos += corner1 + offset.second - corner2;
				
				delete first;
				delete second;
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

