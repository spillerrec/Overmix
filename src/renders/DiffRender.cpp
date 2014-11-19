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
#include "SimpleRender.hpp"
#include "../containers/AContainer.hpp"
#include "../containers/DelegatedContainer.hpp"
#include "../color.hpp"
#include "../planes/ImageEx.hpp"

#include <vector>
using namespace std;

class StaticDiff{
	private:
		const AContainer& aligner;
		const ImageEx& reference;
		
		double amount{ 0.0 };
		precision_color_type sum{ color::BLACK };
		
		Point<double> offset;
		Point<double> absolute;
		
	public:
		StaticDiff( const AContainer& aligner, Point<double> absolute, const ImageEx& ref, unsigned x, unsigned y )
			: aligner(aligner), reference(ref), offset( x, y ), absolute(absolute) { }
		
		void add_image( unsigned index ){
			//Get the actual color
			color_type actual = aligner.image( index )[0].pixel( offset );
			auto& alpha = aligner.alpha( index );
			auto a = alpha ? color::fromDouble( alpha.pixel( offset ) ) : 1.0;
			
			//Find the expected color
			auto pos = (aligner.pos( index ) + offset - absolute).round();
			color_type expected = reference[0].pixel( pos );
			
			//Add it to the sum
			sum += abs( actual - expected ) * a;
			amount += a;
		}
		
		color_type result() const{
			return amount ? color::truncate( color::WHITE - sum / amount ) : color::WHITE;
		}
};

class FakeMask : public ConstDelegatedContainer{
	private:
		vector<Plane> masks;
		
	public:
		void setMask( const Plane& fakemask );
		FakeMask( const AContainer& con, const Plane& mask ) : ConstDelegatedContainer(con)
			{ setMask( mask ); }
		
		virtual const Plane& mask( unsigned index ) const override{ return masks[index]; }
		virtual const Plane& alpha( unsigned index ) const override{ return masks[imageMask(index)]; }
		virtual int imageMask( unsigned index ) const override{
			auto pos = ConstDelegatedContainer::imageMask( index );
			return (pos < 0) ? 0 : pos;
		}
		virtual unsigned maskCount() const override{ return masks.size(); }
};

void FakeMask::setMask( const Plane& fakemask ){
	masks.clear();
	auto amount = ConstDelegatedContainer::maskCount();
	
	if( amount == 0 )
		masks.push_back( fakemask );
	else
		for( unsigned i=0; i<amount; i++ )
			masks.push_back( fakemask.minPlane( ConstDelegatedContainer::mask(i)) );
}

Plane DiffRender::iteration( const AContainer& aligner, const AContainer& real, unsigned max_count, Size<unsigned> size ) const{
	//Normal render
	ImageEx avg = SimpleRender( SimpleRender::FOR_MERGING ).render( aligner, max_count );
	auto real_size = real.size().topLeft();
	
	//Create final output image based on the smallest size
	Plane output( size );
	
	//Iterate over each pixel in the output image
	for( unsigned iy=0; iy<output.get_height(); iy++ ){
		auto out = output.scan_line( iy );
		for( unsigned ix=0; ix<output.get_width(); ix++ ){
			//Set the pixel to the static difference of all the images until max_count
			StaticDiff diff( real, real_size, avg, ix, iy );
			
			for( unsigned j=0; j<max_count; j++ )
				diff.add_image( j );
			
			out[ix] = diff.result();
		}
	}
	
	return output.normalize();
}

ImageEx DiffRender::render( const AContainer& aligner, unsigned max_count, AProcessWatcher* watcher ) const{
	if( max_count > aligner.count() )
		max_count = aligner.count();
	
	//Find the smallest shared size
	Size<unsigned> size = aligner.size().size(); //No image is larger than the final result
	for( unsigned i=0; i<max_count; i++ )
		size = size.min( aligner.image(i).getSize() );
	
	Plane init( size );
	init.fill( color::WHITE );
	FakeMask fake( aligner, init );
	
	for( int i=0; i<2; i++ ){
		init.binarize_threshold( color::WHITE / 2 );
		init = init.dilate( 10 );
		fake.setMask( init );
		init = iteration( fake, aligner, max_count, size );
		//ImageEx( init ).to_qimage( ImageEx::SYSTEM_KEEP, ImageEx::SETTING_NONE ).save( "staticdiff" + QString::number( i ) + ".png" );
	}
	
	//Combine masks
	//TODO: we need some way of returning them all individually!
	for( unsigned i=0; i<aligner.maskCount(); ++i )
		init = init.minPlane( aligner.mask( i ) );
	
	//Create output image
	return init;
}



