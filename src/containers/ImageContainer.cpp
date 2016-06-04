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

using namespace Overmix;

void ImageContainer::IndexCache::reserve( unsigned amount ){
	indexes.reserve( amount );
	offsets.resize( amount );
	for( auto& row : offsets )
		row.reserve( amount );
}

void ImageContainer::IndexCache::push_back( ImagePosition position ){
	indexes.push_back( position );
	offsets.emplace_back();
	for( auto& row : offsets )
		row.resize( indexes.size() );
}

void ImageContainer::IndexCache::invalidate( const std::vector<ImageGroup>& groups ){
	clear();
	for( unsigned ig=0; ig<groups.size(); ig++ )
		for( unsigned ii=0; ii<groups[ig].items.size(); ii++ )
			push_back( {ig, ii} );
}
void ImageContainer::IndexCache::setOffset( unsigned index1, unsigned index2, ImageOffset offset )
	{ offsets[index1][index2] = { offset }; }

bool ImageContainer::IndexCache::hasOffset( unsigned index1, unsigned index2 ) const
	{ return offsets[index1][index2].isValid(); }

ImageOffset ImageContainer::IndexCache::getOffset( unsigned index1, unsigned index2 ) const
	{ return offsets[index1][index2]; }


void ImageContainer::prepareAdds( unsigned amount ){
	if( groups.size() == 0 )
		addGroup( "Auto group" ); //TODO: do not repeat this!
	auto& items = groups.back().items;
	items.reserve( items.size() + amount );
}

ImageItem& ImageContainer::addImage( ImageEx&& img, int mask, int group, QString filepath ){
	if( groups.size() == 0 )
		addGroup( "Auto group" );
	
	unsigned index = ( group >= 0 ) ? group : groups.size()-1;
	groups[index].items.emplace_back( filepath, std::move(img) );
	groups[index].items.back().setSharedMask( mask );
	index_cache.push_back( {index, unsigned(groups[index].count()-1) } );
	
	setUnaligned();
	return groups[index].items.back();
}

void ImageContainer::addGroup( QString name, unsigned group, unsigned from, unsigned to ){
	//Create group and put it after this
	addGroup( name );
	addGroup( groups[group].name ); //TODO: if to != groups.size()
	auto& from_group  = groups[group].items;
	auto& to_group    = groups[groups.size()-2].items;
	auto& after_group = groups[groups.size()-1].items;
	//TODO: move them to be behind this group
	//TODO: initialize vector sizes for item
	
	//Move items to new group
	for( unsigned i=from; i<to; i++ )
		to_group.emplace_back( std::move( from_group[i] ) );
	
	//Move rest to after group
	for( unsigned i=to; i<from_group.size(); i++ )
		after_group.emplace_back( std::move( from_group[i] ) );
	
	//Resize first group
	from_group.resize( from );
	
	index_cache.invalidate( groups );
}

bool ImageContainer::hasCachedOffset( unsigned index1, unsigned index2 ) const
	{ return index_cache.hasOffset( index1, index2 ); }

ImageOffset ImageContainer::getCachedOffset( unsigned index1, unsigned index2 ) const {
	if( index_cache.hasOffset( index1, index2 ) )
		return index_cache.getOffset( index1, index2 );
	else
		throw std::logic_error( "ImageContainer::getCachedOffset - cache not available" );
}

void ImageContainer::setCachedOffset( unsigned index1, unsigned index2, ImageOffset offset )
	{ index_cache.setOffset( index1, index2, offset ); }

bool ImageContainer::removeGroups( unsigned from, unsigned amount ){
	if( !util::removeItems( groups, from, amount ) )
		return false;
	setUnaligned();
	index_cache.invalidate( groups );
	return true;
}

unsigned ImageContainer::count() const{ return index_cache.size(); }

const ImageEx& ImageContainer::image( unsigned index ) const{
	auto pos = index_cache.getImage( index );
	return groups[pos.group].image( pos.index );
}

ImageEx& ImageContainer::imageRef( unsigned index ){
	auto pos = index_cache.getImage( index );
	return groups[pos.group].imageRef( pos.index );
}


int ImageContainer::imageMask( unsigned index ) const{
	auto pos = index_cache.getImage( index );
	return groups[pos.group].imageMask( pos.index );
}

void ImageContainer::setMask( unsigned index, int id ){
	auto pos = index_cache.getImage( index );
	groups[pos.group].setMask( pos.index, id );
}

const Plane& ImageContainer::alpha( unsigned index ) const{
	auto pos = index_cache.getImage( index );
	return groups[pos.group].alpha( pos.index );
}

Point<double> ImageContainer::pos( unsigned index ) const{
	auto pos = index_cache.getImage( index );
	return groups[pos.group].pos( pos.index );
}

void ImageContainer::setPos( unsigned index, Point<double> newVal ){
	auto pos = index_cache.getImage( index );
	groups[pos.group].setPos( pos.index, newVal );
	aligned = true;
}

int ImageContainer::frame( unsigned index ) const{
	auto pos = index_cache.getImage( index );
	return groups[pos.group].frame( pos.index );
}
void ImageContainer::setFrame( unsigned index, int newVal ){
	auto pos = index_cache.getImage( index );
	groups[pos.group].setFrame( pos.index, newVal );
}

