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
#include "../renders/AverageRender.hpp"
#include "../utils/AProcessWatcher.hpp"
#include "../utils/PlaneUtils.hpp"

#include <limits>
#include <stdexcept>
using namespace std;
using namespace Overmix;

static void copyLine( Plane& plane_out, const Plane& plane_in, unsigned y_out, unsigned y_in ){
	for( auto val : makeZipRowIt( plane_out.scan_line( y_out ), plane_in.scan_line( y_in  ) ) )
		val.first = val.second;
}

static void mergeLine( Plane& plane_out, const Plane& plane_in1, const Plane& plane_in2, unsigned y_out, unsigned y_in1, unsigned y_in2 ){
	auto out = plane_out.scan_line( y_out );
	auto in1 = plane_in1.scan_line( y_in1 );
	auto in2 = plane_in2.scan_line( y_in2 );
	for( unsigned ix=0; ix<plane_out.get_width(); ++ix )
		out[ix] = (in1[ix] + in2[ix]) / 2; //TODO: use precisison_color_type
}

/** Efficient rendering of the mean of two planes, assuming only a vertical offset */
static Plane mergeVertical( const Plane& p1, const Plane& p2, int offset ){
	auto& top    = offset < 0 ? p2 : p1;
	auto& bottom = offset < 0 ? p1 : p2;
	offset = abs(offset);
	unsigned height = max( top.get_height(), bottom.get_height() + offset );
	
	Plane out( p1.get_width(), height );
	
	//Copy top part
	for( int iy=0; iy < abs(offset); ++iy )
		copyLine( out, top, iy, iy );
	
	//Merge the shared middle part
	auto shared_height = min( top.get_height()-offset, bottom.get_height() );
	for( unsigned iy=0; iy<shared_height; ++iy )
		mergeLine( out, top, bottom, iy+offset, iy+offset, iy );
	
	//Copy bottom part, might be in the top plane, if it is larger than the bottom plane
	if( top.get_height() > bottom.get_height() + offset )
		for( unsigned iy=offset+shared_height; iy<height; ++iy )
			copyLine( out, top, iy, iy );
	else
		for( unsigned iy=shared_height; iy<bottom.get_height(); ++iy )
			copyLine( out, bottom, iy+offset, iy );
	
	return out;
}

/** Refers to a plane+alpha in a container, or contains an instance of a plane+alpha */
struct Overmix::ImageGetter{
	ModifiedPlane plane, alpha;
	Point<double> center_left{0.0,0.0}; ///< The offset off the center for the left most image
	Point<double> center_right{0.0,0.0}; ///< The offset off the center for the right most image
	ImageGetter( ModifiedPlane p, ModifiedPlane a ) : plane(p), alpha(a) { }
	ImageGetter( ModifiedPlane p, ModifiedPlane a, Point<double> left, Point<double> right ) :	ImageGetter(p, a) {
		center_left = left;
		center_right = right;
	}
};

/** Creates a ImageGetter from this container with the specified index */
ImageGetter RecursiveAligner::getGetter( const AContainer& container, unsigned index ) const {
	auto comparator = container.getComparator();
	return { comparator->process( container.image(index)[0] ), comparator->processAlpha( container.alpha(index) ) };
}

/** Aligns two ImageGetters, and renders the average. Returns the render and the offset betten the getters */
pair<ImageGetter,Point<double>> RecursiveAligner::combine( const AContainer& container, const ImageGetter& first, const ImageGetter& second ) const{
	auto comparator = container.getComparator();
	
	auto hint = first.center_right - second.center_left;
	auto offset = comparator->findOffset( first.plane(), second.plane(), first.alpha(), second.alpha(), hint ).distance;
	
	//TODO: calculate new center points
	Point<double> center_left = first.center_left - offset/2;
	Point<double> center_right = second.center_right + offset/2;
	
	if( offset.x == 0
		&&	!first.alpha() && !second.alpha()
		&&	first.plane().get_width() == second.plane().get_width() )
		return { ImageGetter{ mergeVertical( first.plane(), second.plane(), offset.y ), Plane(), center_left, center_right }, offset };
	else{
		//Wrap planes in ImageContainer
		//TODO: Optimize this
		ImageContainer local_container;
		local_container.addImage( ImageEx( Plane( first.plane()), Plane( first.alpha()) ) ); //We are having copies here!!
		local_container.addImage( ImageEx( Plane(second.plane()), Plane(second.alpha()) ) );
		local_container.setRawPos( 1, offset );
		
		//Render it
		auto img = AverageRender( false, true ).render( local_container );
		return { { std::move(img[0]), std::move(img.alpha_plane()), center_left, center_right }, offset };
	}
}

/** Internal implementation of align, supporting a recursive interface */
ImageGetter RecursiveAligner::align( AContainer& container, Progress& progress, unsigned begin, unsigned end ) const{
	if( begin >= end )
		throw invalid_argument( "Invalid image range" );
	
	auto amount = end - begin;
	switch( amount ){
		case 1: progress.add( 1 );
				return getGetter( container, begin ); //Just return this one
		case 2: { //Optimization for two images
				auto offset = combine( container, getGetter( container, begin ), getGetter( container, begin+1 ) );
				container.setRawPos( begin+1, container.rawPos(begin) + offset.second );
				progress.add( 2 );
				return std::move( offset.first );
			}
		default: { //More than two images
				//Solve sub-areas recursively
				unsigned middle = amount / 2 + begin;
				auto offset = combine( container, align( container, progress, begin, middle ), align( container, progress, middle, end ) );
				
				//Find top-left corner of first
				auto corner1 = Point<double>( numeric_limits<double>::max(), numeric_limits<double>::max() );
				auto corner2 = corner1;
				for( unsigned i=begin; i<middle; i++ )
					corner1 = corner1.min( container.rawPos(i) );
				//Find top-left corner of second
				for( unsigned i=middle; i<end; i++ )
					corner2 = corner2.min( container.rawPos(i) );
				
				//move all in "middle to end" using the offset
				for( unsigned i=middle; i<end; i++ )
					container.setRawPos( i, container.rawPos( i ) + corner1 + offset.second - corner2 );
				
				return std::move( offset.first ); //Return the combined image
			}
	}
}

void RecursiveAligner::align( AContainer& container, AProcessWatcher* watcher ) const{
	if( container.count() == 0 )
		return;
	
	Progress progress( "RecursiveAligner", container.count(), watcher );
	align( container, progress, 0, container.count() );
	
	auto scale = container.getComparator()->scale();
	for( unsigned i=0; i<container.count(); i++ )
		container.setRawPos( i, container.rawPos( i ) / scale );
}

