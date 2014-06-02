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
#include "../AImageAligner.hpp"
#include "../color.hpp"

#include <vector>
using namespace std;

#include <QSize>

class StaticDiff{
	private:
		const AImageAligner& aligner;
		const ImageEx& reference;
		
		unsigned amount{ 0 };
		precision_color_type sum{ color::BLACK };
		
		QPoint offset;
		QPoint absolute;
		
	public:
		StaticDiff( const AImageAligner& aligner, const ImageEx& ref, unsigned x, unsigned y )
			: aligner(aligner), reference(ref) {
			offset = QPoint( x, y );
			absolute = aligner.size().topLeft();
		}
		
		void add_image( unsigned index ){
			//Get the actual color
			color_type actual = aligner.plane( index, 0 ).pixel( offset.x(), offset.y() );
			
			//Find the expected color
			QPoint pos = (aligner.pos( index ) + offset - absolute).toPoint();
			color_type expected = reference[0].pixel( pos.x(), pos.y() );
			
			//Add it to the sum
			sum += abs( actual - expected );
			amount++;
		}
		
		color_type result() const{
			return amount ? color::truncate( sum / amount ) : color::BLACK;
		}
};

ImageEx DiffRender::render( const AImageAligner& aligner, unsigned max_count, AProcessWatcher* watcher ) const{
	if( max_count > aligner.count() )
		max_count = aligner.count();
	
	//Normal render
	ImageEx avg = SimpleRender( SimpleRender::FOR_MERGING ).render( aligner, max_count );
	
	//Find the smallest shared size
	QSize size = aligner.size().size(); //No image is larger than the final result
	for( unsigned i=0; i<max_count; i++ ){
		size.setWidth( min( (unsigned)size.width(), aligner.image(i).get_width() ) );
		size.setHeight( min( (unsigned)size.height(), aligner.image(i).get_height() ) );
	}
	
	//Create final output image based on the smallest size
	ImageEx img( ImageEx::GRAY );
	img.create( size.width(), size.height() );
	Plane& output = img[0];
	
	if( watcher )
		watcher->set_total( 1000 );
	
	//Iterate over each pixel in the output image
	for( unsigned iy=0; iy<output.get_height(); iy++ ){
		if( watcher )
			watcher->set_current( iy * 1000 / output.get_height() );
		
		color_type* out = output.scan_line( iy );
		for( unsigned ix=0; ix<output.get_width(); ix++ ){
			//Set the pixel to the static difference of all the images until max_count
			StaticDiff diff( aligner, avg, ix, iy );
			
			for( unsigned j=0; j<max_count; j++ )
				diff.add_image( j );
			
			out[ix] = diff.result();
		}
	}
	
	img.apply( &Plane::normalize );
	return img;
}



