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

#ifndef IMAGE_CONTAINER_HPP
#define IMAGE_CONTAINER_HPP

#include "ImageGroup.hpp"
#include "../comparators/GradientComparator.hpp"

namespace Overmix{

class ImageContainer : public AContainer{
	private:
		GradientComparator comparator; //TODO: Make virtual?
	public:
		void setComparator( GradientComparator );
		
	private:
		/** A index to an ImageItem */
		struct ImagePosition{
			unsigned group;
			unsigned index;
			ImagePosition( unsigned group, unsigned index ) : group(group), index(index) { }
		};
		
		class IndexCache{
			private:
				std::vector<ImagePosition> indexes;
				std::vector<std::vector<ImageOffset>> offsets;
				
			public: //Modify
				void reserve( unsigned amount );
				void push_back( ImagePosition position );
				void setOffset( unsigned, unsigned, ImageOffset );
				void invalidate( const std::vector<ImageGroup>& groups );
				void clear(){
					indexes.clear();
					offsets.clear();
				}
				
			public: //Accessors
				ImagePosition getImage( unsigned index ) const{ return indexes[index]; }
				bool hasOffset( unsigned, unsigned ) const;
				ImageOffset getOffset( unsigned, unsigned ) const;
				auto size() const{ return indexes.size(); }
		};
		
	private:
		
	private:
		std::vector<ImageGroup> groups;
		std::vector<Plane> masks;
		IndexCache index_cache;
		
		bool aligned{ false };
		
	public:
		unsigned groupAmount() const{ return groups.size(); }
		const ImageGroup& getConstGroup( unsigned index ) const{ return groups[index]; }
		ImageGroup& getGroup( unsigned index ){ return groups[index]; }
		
		void clear(){
			groups.clear();
			masks.clear();
			index_cache.clear();
		}
		void clearMasks(){
			for( unsigned i=0; i<count(); i++ )
				setMask( i, -1 );
			masks.clear();
		}
		
		std::vector<ImageGroup>::iterator        begin()       { return groups.begin(); }
		std::vector<ImageGroup>::const_iterator  begin()  const{ return groups.begin(); }
		std::vector<ImageGroup>::iterator        end()         { return groups.end();   }
		std::vector<ImageGroup>::const_iterator  end()    const{ return groups.end();   }
		
	public: //AContainer implementation
		virtual       unsigned     count()                 const override;
		virtual const ImageEx&     image( unsigned index ) const override;
		virtual       ImageEx&  imageRef( unsigned index ) override;
		virtual const Plane&       alpha( unsigned index ) const override;
		virtual       int      imageMask( unsigned index ) const override;
		virtual       void       setMask( unsigned index, int id ) override;
		virtual const Plane&        mask( unsigned index ) const override{ return masks[index]; }
		virtual       unsigned maskCount()              const override{ return masks.size(); }
		virtual       Point<double>  pos( unsigned index ) const override;
		virtual       void        setPos( unsigned index, Point<double> newVal ) override;
		virtual       int          frame( unsigned index ) const override;
		virtual       void      setFrame( unsigned index, int newVal ) override;
		
	public: //AContainer comparators implementation
		const AComparator* getComparator() const override{ return &comparator; }
		virtual bool        hasCachedOffset( unsigned, unsigned ) const override;
		virtual ImageOffset getCachedOffset( unsigned, unsigned ) const override;
		virtual void        setCachedOffset( unsigned, unsigned, ImageOffset ) override;
		
	public:
		bool isAligned() const{ return aligned; }
		void setUnaligned(){ aligned = false; }
		void setAligned(){ aligned = true; }
		
		void prepareAdds( unsigned amount );
		ImageItem& addImage( ImageEx&& img, int mask=-1, int group=-1, QString filepath="" );
		
		int addMask( Plane&& mask ){
			masks.emplace_back( mask );
			return masks.size() - 1;
		}
		const std::vector<Plane>& getMasks() const{ return masks; }	
		
		void addGroup( QString name ){ groups.emplace_back( &comparator, name, masks ); }
		void addGroup( QString name, unsigned group, unsigned from, unsigned to );
		
		void moveImage( unsigned from_group, unsigned from_img
			,	unsigned to_group, unsigned to_img );
		void moveGroup( unsigned from, unsigned to );
		
		void removeImage( unsigned group, unsigned img );
		
		bool removeGroups( unsigned from, unsigned amount );
		void rebuildIndexes(){ index_cache.invalidate( groups ); } //TODO: This is used in JpegRender of some reason, it should not be needed!
		
		void onAllItems( void update( ImageItem& item ) );

		template<typename T>
		void onAllItems( T update ){
			for( auto& group : groups )
				for( auto& item : group.items )
					update( item );
		}
		template<typename T>
		void onAllItems( T update ) const{
			for( auto& group : groups )
				for( auto& item : group.items )
					update( item );
		}
		
		//TODO: Get group thing
		
		//TODO: Get render thing
};

}

#endif
