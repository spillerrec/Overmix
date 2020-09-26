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
void ImageContainer::IndexCache::setOffset( unsigned index1, unsigned index2, const ImageOffset& offset ) {
	if( index2 > index1 )
		setOffset( index2, index1, offset.reverse() );
	else
		offsets[index1][index2] = offset;
}

bool ImageContainer::IndexCache::hasOffset( unsigned index1, unsigned index2 ) const{
	if( index2 > index1 )
		return offsets[index2][index1].isValid();
	return offsets[index1][index2].isValid();
}

ImageOffset ImageContainer::IndexCache::getOffset( unsigned index1, unsigned index2 ) const{
	if( index2 > index1 )
		return getOffset( index2, index1 ).reverse();
	else
		return offsets[index1][index2];
}

class TempComparator : public AComparator{
	private:
		const AComparator* parent;
		
	public:
		explicit TempComparator(const AComparator* parent) : parent(parent) {}
		Size<double> scale() const override{ return parent->scale(); }
		ModifiedPlane process( const Plane& plane ) const override { return parent->process(plane); }
		ModifiedPlane processAlpha( const Plane& plane ) const override { return parent->processAlpha(plane); }
		
		ImageOffset findOffset( const Plane& img1, const Plane& img2, const Plane& a1, const Plane& a2, Point<double> hint ) const
			{ return parent->findOffset(img1, img2, a1, a2, hint); }
		
		double findError( const Plane& img1, const Plane& img2, const Plane& a1, const Plane& a2, double x, double y ) const override
			{ return parent->findError(img1, img2, a1, a2, x, y); }
};

void ImageContainer::setComparator( const AComparator* comp ){
	setComparator( std::make_unique<TempComparator>( comp ) );
}

void ImageContainer::setComparator( std::unique_ptr<AComparator> g ){
	comparator = std::move( g );
	index_cache.invalidate( groups );
}


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
	//Extract the group to split
	auto original = std::move( groups[group] );
	util::removeItems( groups, group, 1 );
	
	//Create new groups
	ImageGroup before( comparator.get(), original.name, masks );
	ImageGroup added(  comparator.get(), name,          masks );
	ImageGroup after(  comparator.get(), original.name, masks );
	
	for( unsigned i=0; i<from; i++ )
		before.items.emplace_back( std::move( original.items[i] ) );
	
	for( unsigned i=from; i<to; i++ )
		added.items.emplace_back( std::move( original.items[i] ) );
	
	for( unsigned i=to; i<original.items.size(); i++ )
		after.items.emplace_back( std::move( original.items[i] ) );
	
	
	//Add the groups if the contain items
	auto insert_it = groups.begin() + group;
	auto insert = [&]( auto& group ){
		if( group.items.size() > 0 )
			insert_it = groups.insert( insert_it, std::move( group ) ) + 1;
	};
	
	insert( before );
	insert( added  );
	insert( after  );
	
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
	return groups.at(pos.group).image( pos.index );
}

ImageEx& ImageContainer::imageRef( unsigned index ){
	auto pos = index_cache.getImage( index );
	return groups.at(pos.group).imageRef( pos.index );
}


int ImageContainer::imageMask( unsigned index ) const{
	auto pos = index_cache.getImage( index );
	return groups.at(pos.group).imageMask( pos.index );
}

void ImageContainer::setMask( unsigned index, int id ){
	auto pos = index_cache.getImage( index );
	groups.at(pos.group).setMask( pos.index, id );
}

void ImageContainer::removeMask( int mask ){
	if( mask < 0 || unsigned(mask) >= masks.size() )
		throw std::runtime_error( "ImageContainer::removeMask - invalid mask ID" );
	
	//Remove mask
	util::removeItems( masks, mask, 1 );
	
	//Remove references to mask
	for( auto& group : groups )
		for( auto& item : group )
			if( item.maskId() == mask )
				item.setSharedMask( -1 );
	
	//Move all references above removed mask id
	for( auto& group : groups )
		for( auto& item : group )
			if( item.maskId() > mask )
				item.setSharedMask( item.maskId()-1 );
}

const Plane& ImageContainer::alpha( unsigned index ) const{
	auto pos = index_cache.getImage( index );
	return groups.at(pos.group).alpha( pos.index );
}

Point<double> ImageContainer::pos( unsigned index ) const{
	auto pos = index_cache.getImage( index );
	return groups.at(pos.group).pos( pos.index );
}

void ImageContainer::setPos( unsigned index, Point<double> newVal ){
	auto pos = index_cache.getImage( index );
	groups.at(pos.group).setPos( pos.index, newVal );
	aligned = true;
}

int ImageContainer::frame( unsigned index ) const{
	auto pos = index_cache.getImage( index );
	return groups.at(pos.group).frame( pos.index );
}
void ImageContainer::setFrame( unsigned index, int newVal ){
	auto pos = index_cache.getImage( index );
	groups.at(pos.group).setFrame( pos.index, newVal );
}

const AComparator* ImageContainer::getComparator() const{
	if( !comparator )
		throw std::runtime_error( "ImageContainer::getComparator - No comparator!" );
	return comparator.get();
}

