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


#include "AContainer.hpp"

#include "../planes/ImageEx.hpp"


const Plane& AContainer::alpha( unsigned index ) const{
	return image( index ).alpha_plane();
}

//NOTE: this makes no sense, throw something instead?
const Plane& AContainer::mask( unsigned index ) const{ return alpha( index ); }

void AContainer::cropImage( unsigned index, unsigned left, unsigned top, unsigned right, unsigned bottom ){
	auto& img = imageRef( index );
	auto offset = img.crop( left, top, right, bottom );
	setPos( index, pos( index ) + offset );
}

void AContainer::scaleImage( unsigned index, Point<double> scale, ScalingFunction scaling ){
	setPos( index, pos( index ) * scale );
	
	auto& img = imageRef( index );
	if( img.getSize() == (img.getSize() * scale).round() )
		return; //Don't attempt to scale, if it will not change the size
	img.scale( scale, scaling );
}

QRect AContainer::size() const{
	QRectF total;
	
	for( unsigned i=0; i<count(); i++ )
		total = total.united( QRectF( { pos(i).x, pos(i).y }, QSizeF( image(i).get_width(), image(i).get_height() ) ) );
	
	//Round so that we only increase size
	total.setLeft( floor( total.left() ) );
	total.setTop( floor( total.top() ) );
	total.setRight( ceil( total.right() ) );
	total.setBottom( ceil( total.bottom() ) );
	//TODO: just return a QRectF and let the caller deal with the rounding
	
	return total.toRect();
}

Point<double> AContainer::minPoint() const{
	if( count() == 0 )
		return { 0, 0 };
	
	Point<double> min = pos( 0 );
	for( unsigned i=0; i<count(); i++ )
		min = min.min( pos(i) );
	
	return min;
}

void AContainer::resetPosition(){
	for( unsigned i=0; i<count(); i++ )
		setPos( i, { 0.0, 0.0 } );
}

void AContainer::offsetAll( double dx, double dy ){
	Point<double> offset( dx, dy );
	for( unsigned i=0; i<count(); i++ )
		setPos( i, pos( i ) + offset );
}

static void addToFrames( std::vector<int>& frames, int frame ){
	for( auto& item : frames )
		if( item == frame )
			return;
	frames.push_back( frame );
}

std::vector<int> AContainer::getFrames() const{
	std::vector<int> frames;
	for( unsigned i=0; i<count(); ++i )
			if( frame( i ) >= 0 )
				addToFrames( frames, frame( i ) );
	
	//Make sure to include at least include
	if( frames.size() == 0 )
		frames.push_back( -1 );
	
	return frames;
}
