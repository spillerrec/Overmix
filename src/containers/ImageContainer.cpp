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


#include "ImageContainer.hpp"

void ImageContainer::addImage( ImageEx&& img, int group ){
	if( groups.size() == 0 )
		groups.emplace_back( ImageGroup( "" ) );
	
	unsigned index = ( group >= 0 ) ? group : groups.size()-1;
	groups[index].items.emplace_back( ImageItem( "", std::move(img) ) );
}


ImageContainer::ImagePosition ImageContainer::fromLinear( unsigned index ) const{
	for( unsigned i=0; i<groups.size(); i++ ){
		if( index < groups[i].items.size() )
			return { i, index };
		else
			index -= groups[i].items.size();
	}
	
	qWarning( "ImageContainer::fromLinear(), out-of-bounds" );
	return { 0, 0 }; //Oh no...
}

unsigned ImageContainer::count() const{
	unsigned sum=0;
	for( auto group : groups )
		sum += group.items.size();
	return sum;
	//TODO: accumulate
}

const ImageEx& ImageContainer::image( unsigned index ) const{
	auto pos = fromLinear( index );
	return groups[pos.group].image( pos.index );
}

QPointF ImageContainer::pos( unsigned index ) const{
	auto pos = fromLinear( index );
	return groups[pos.group].pos( pos.index );
}

void ImageContainer::setPos( unsigned index, QPointF newVal ){
	auto pos = fromLinear( index );
	groups[pos.group].setPos( pos.index, newVal );
}

