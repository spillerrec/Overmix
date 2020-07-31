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


#include "DiffRender.hpp"
#include "AverageRender.hpp"
#include "../containers/AContainer.hpp"
#include "../containers/DelegatedContainer.hpp"
#include "../color.hpp"
#include "../planes/ImageEx.hpp"

#include <set>
#include <vector>
#include <stdexcept>
using namespace std;
using namespace Overmix;


class StaticDiff{
	private:
		const ImageEx& reference;
		
		double amount{ 0.0 };
		precision_color_type sum{ color::BLACK };
		
		Point<unsigned> offset;
		Point<double> absolute;
		
	public:
		StaticDiff( Point<double> absolute, const ImageEx& ref, unsigned x, unsigned y )
			: reference(ref), offset( x, y ), absolute(absolute) { }
		
		void add_image( AContainer::ConstRef img ){
			//Get the actual color
			color_type actual = img.image()[0].pixel( offset );
			auto alpha = img.alpha() ? color::fromDouble( img.alpha().pixel( offset ) ) : 1.0;
			
			//Find the expected color
			auto pos = (img.pos() + offset - absolute).round();
			color_type expected = reference[0].pixel( pos );
			
			//Add it to the sum
			sum += abs( actual - expected ) * alpha;
			amount += alpha;
		}
		
		color_type result() const{
			return amount ? color::truncate( color::WHITE - sum / amount ) : color::WHITE;
		}
};

class FakeMask : public ConstDelegatedContainer{
	private:
		vector<Plane> masks;
		
	public:
		void setFakeMask( const Plane& fakemask );
		FakeMask( const AContainer& con, const Plane& mask ) : ConstDelegatedContainer(con)
			{ setFakeMask( mask ); }
		
		virtual const Plane& mask( unsigned index ) const override{ return masks[index]; }
		virtual const Plane& alpha( unsigned index ) const override{ return masks[imageMask(index)]; }
		virtual int imageMask( unsigned index ) const override{
			auto pos = ConstDelegatedContainer::imageMask( index );
			return (pos < 0) ? 0 : pos;
		}
		virtual unsigned maskCount() const override{ return masks.size(); }
};

void FakeMask::setFakeMask( const Plane& fakemask ){
	masks.clear();
	
	//Combine with the fake mask
	auto amount = ConstDelegatedContainer::maskCount();
	for( unsigned i=0; i<amount; i++ )
		masks.push_back( fakemask.minPlane( ConstDelegatedContainer::mask(i)) );
		
	//Make sure we have at least one mask
	//TODO: why?
	if( masks.size() == 0 )
		masks.push_back( fakemask );
}

Plane DiffRender::iteration( const AContainer& aligner, const AContainer& real, Size<unsigned> size ) const{
	//Normal render
	ImageEx avg = AverageRender( false, true ).render( aligner );
	auto real_size = real.size().pos.round();
	
	//Create final output image based on the smallest size
	Plane output( size );
	
	//Iterate over each pixel in the output image
	for( unsigned iy=0; iy<output.get_height(); iy++ ){
		auto out = output.scan_line( iy );
		for( unsigned ix=0; ix<output.get_width(); ix++ ){
			//Set the pixel to the static difference of all the images
			StaticDiff diff( real_size, avg, ix, iy );
			
			for( auto image : real )
				diff.add_image( image );
			
			out[ix] = diff.result();
		}
	}
	
	return output.normalize();
}

static std::set<int> usedMasks( const AContainer& container ){
	std::set<int> ids;
	for( unsigned i=0; i<container.count(); i++ )
		if( container.imageMask(i) >= 0 )
			ids.insert( container.imageMask(i) );
	return ids;
}

ImageEx DiffRender::render( const AContainer& aligner, AProcessWatcher* watcher ) const{
	//Find the smallest shared size
	auto size = aligner.size().size; //No image is larger than the final result
	for( unsigned i=0; i<aligner.count(); i++ )
		size = size.min( aligner.image(i).getSize() );
	
	Plane init( size );
	init.fill( color::WHITE );
	FakeMask fake( aligner, init );
	
	for( int i=0; i<iteration_count; i++ ){
		init.binarize_threshold( color::WHITE * threshold );
		init = init.dilate( dilate_size );
		fake.setFakeMask( init );
		init = iteration( fake, aligner, size );
		//ImageEx( init ).to_qimage( ImageEx::SYSTEM_KEEP, ImageEx::SETTING_NONE ).save( "staticdiff" + QString::number( i ) + ".png" );
	}
	
	//Combine masks
	//TODO: we need some way of returning them all individually!
	for( auto i : usedMasks(aligner) )
		init = init.minPlane( aligner.mask( i ) );
	
	//Create output image
	return ImageEx{ init };
}



