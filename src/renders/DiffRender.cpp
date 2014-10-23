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

#include <QSize>

class StaticDiff{
	private:
		const AContainer& aligner;
		const ImageEx& reference;
		
		unsigned amount{ 0 };
		precision_color_type sum{ color::BLACK };
		
		Point<double> offset;
		Point<double> absolute;
		
	public:
		StaticDiff( const AContainer& aligner, const ImageEx& ref, unsigned x, unsigned y )
			: aligner(aligner), reference(ref) {
			offset = QPoint( x, y );
			absolute = aligner.size().topLeft();
		}
		
		void add_image( unsigned index ){
			//Get the actual color
			color_type actual = aligner.image( index )[0].pixel( offset.x, offset.y );
			
			//Find the expected color
			auto pos = (aligner.pos( index ) + offset - absolute).round();
			color_type expected = reference[0].pixel( pos.x, pos.y );
			
			//Add it to the sum
			sum += abs( actual - expected );
			amount++;
		}
		
		color_type result() const{
			return amount ? color::truncate( color::WHITE - sum / amount ) : color::WHITE;
		}
};

class FakeMask : public DelegatedContainer{
	public:
		Plane fakemask;
		FakeMask( const AContainer& con, Plane mask ) : DelegatedContainer(con), fakemask(mask) { }
		
		virtual const Plane& mask( unsigned ) const override{ return fakemask; }
		virtual const Plane& alpha( unsigned ) const override{ return fakemask; }
		virtual int imageMask( unsigned ) const override{ return 0; }
		virtual unsigned maskCount() const override{ return 1; }
};

Plane DiffRender::iteration( const AContainer& aligner, unsigned max_count, Size<unsigned> size ) const{
	//Normal render
	ImageEx avg = SimpleRender( SimpleRender::FOR_MERGING ).render( aligner, max_count );
	
	//Create final output image based on the smallest size
	Plane output( size );
	
	//Iterate over each pixel in the output image
	for( unsigned iy=0; iy<output.get_height(); iy++ ){
		auto out = output.scan_line( iy );
		for( unsigned ix=0; ix<output.get_width(); ix++ ){
			//Set the pixel to the static difference of all the images until max_count
			StaticDiff diff( aligner, avg, ix, iy );
			
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
	for( unsigned i=0; i<max_count; i++ ){
		size.width() = min( size.width(), aligner.image(i).get_width() );
		size.height() = min( size.height(), aligner.image(i).get_height() );
	}
	
	Plane init( size );
	init.fill( color::WHITE );
	FakeMask fake( aligner, init );
	
	for( int i=0; i<2; i++ ){
		fake.fakemask.binarize_threshold( color::WHITE / 2 );
		fake.fakemask = fake.fakemask.dilate( 10 );
		fake.fakemask = iteration( fake, max_count, size );
	}
	
	//Create output image
	ImageEx img( ImageEx::GRAY );
	img.create( size.width(), size.height() );
	img[0] = fake.fakemask;
	return img;
}



