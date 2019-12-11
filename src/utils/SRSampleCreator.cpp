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

#include "SRSampleCreator.hpp"

#include "../planes/ImageEx.hpp"
#include "../containers/ImageContainer.hpp"
#include "../color.hpp"

using namespace Overmix;


void SRSampleCreator::render( ImageContainer& add_to, const ImageEx& img ) const{
	//TODO:
	Point<unsigned> size( (img.get_width()-3) / scale * scale, (img.get_height()-3) / scale * scale );
	Point<unsigned> size_lr( size.x / scale, size.y / scale );
	for( int x=0; x<scale; x++ )
		for( int y=0; y<scale; y++ ){
			auto sample = img;
			Point<double> offset( x / (double)scale, y / (double)scale );
			
			sample.crop( {unsigned(x), unsigned(y)}, size );
			sample.scale( size_lr );
			
			add_to.addImage( std::move(sample), -1, -1, QString::fromUtf8(("lr_" + std::to_string(x) + "_" + std::to_string(y)).c_str()) );
			add_to.setPos( add_to.count()-1, offset );
		}
}