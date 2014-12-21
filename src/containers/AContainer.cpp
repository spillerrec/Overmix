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
	if( img.getSize() == (img.getSize() * scale).round().to<unsigned>() )
		return; //Don't attempt to scale, if it will not change the size
	img.scaleFactor( scale, scaling );
}

Rectangle<double> AContainer::size() const{
	auto min = minPoint();
	auto max = min;
	for( unsigned i=0; i<count(); ++i )
		max = max.max( image(i)[0].getSize().to<double>() + pos(i) );
	return { min, max-min };
}

Point<double> AContainer::minPoint() const{
	if( count() == 0 )
		return { 0, 0 };
	
	Point<double> min = pos( 0 );
	for( unsigned i=0; i<count(); i++ )
		min = min.min( pos(i) );
	
	return min;
}

Point<double> AContainer::maxPoint() const{
	if( count() == 0 )
		return { 0, 0 };
	
	Point<double> max = pos( 0 );
	for( unsigned i=0; i<count(); i++ )
		max = max.max( pos(i) );
	
	return max;
}

std::pair<bool,bool> AContainer::hasMovement() const{
	std::pair<bool,bool> movement( false, false );
	if( count() == 0 )
		return movement;
	
	auto fixed = pos( 0 );
	for( unsigned i=1; i<count() && (!movement.first || !movement.second); ++i ){
		auto current = pos( i );
		movement.first  = movement.first  || current.x != fixed.x;
		movement.second = movement.second || current.y != fixed.y;
	}
	
	return movement;
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
