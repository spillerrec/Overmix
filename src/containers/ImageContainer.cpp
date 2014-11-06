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

#include "../utils/utils.hpp"

ImageItem& ImageContainer::addImage( ImageEx&& img, int mask, int group, QString filepath ){
	if( groups.size() == 0 )
		groups.emplace_back( "Auto group", masks );
	
	ImageItem item{ filepath, std::move(img) };
	item.setSharedMask( mask );
	
	unsigned index = ( group >= 0 ) ? group : groups.size()-1;
	groups[index].items.emplace_back( item );
	indexes.emplace_back( index, groups[index].count()-1 );
	
	setUnaligned();
	return groups[index].items.back();
}

bool ImageContainer::removeGroups( unsigned from, unsigned amount ){
	if( !util::removeItems( groups, from, amount ) )
		return false;
	setUnaligned();
	rebuildIndexes();
	return true;
}

void ImageContainer::rebuildIndexes(){
	indexes.clear();
	for( unsigned ig=0; ig<groups.size(); ig++ )
		for( unsigned ii=0; ii<groups[ig].items.size(); ii++ )
			indexes.emplace_back( ig, ii );
}

unsigned ImageContainer::count() const{ return indexes.size(); }

const ImageEx& ImageContainer::image( unsigned index ) const{
	auto pos = indexes[index];
	return groups[pos.group].image( pos.index );
}


int ImageContainer::imageMask( unsigned index ) const{
	auto pos = indexes[index];
	return groups[pos.group].imageMask( pos.index );
}

const Plane& ImageContainer::alpha( unsigned index ) const{
	auto pos = indexes[index];
	return groups[pos.group].alpha( pos.index );
}

Point<double> ImageContainer::pos( unsigned index ) const{
	auto pos = indexes[index];
	return groups[pos.group].pos( pos.index );
}

void ImageContainer::setPos( unsigned index, Point<double> newVal ){
	auto pos = indexes[index];
	groups[pos.group].setPos( pos.index, newVal );
	aligned = true;
}

int ImageContainer::frame( unsigned index ) const{
	auto pos = indexes[index];
	return groups[pos.group].frame( pos.index );
}
void ImageContainer::setFrame( unsigned index, int newVal ){
	auto pos = indexes[index];
	groups[pos.group].setFrame( pos.index, newVal );
}

static void addToFrames( std::vector<int>& frames, int frame ){
	for( auto& item : frames )
		if( item == frame )
			return;
	frames.push_back( frame );
}

std::vector<int> ImageContainer::getFrames() const{
	std::vector<int> frames;
	for( auto& group : groups )
		for( auto& item : group.items )
			if( item.frame >= 0 )
				addToFrames( frames, item.frame );
	
	//Make sure to include at least include
	if( frames.size() == 0 )
		frames.push_back( -1 );
	
	return frames;
}

