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


#include "Deteleciner.hpp"
#include "planes/ImageEx.hpp"

using namespace Overmix;

ImageEx Deteleciner::addInterlaced( ImageEx image ){
	if( !frame.is_valid() ){
		frame = image;
		interlaced = true;
		return ImageEx();
	}
	
	if( interlaced ){
		interlaced = true;
		frame.replace_line( image, true );
		ImageEx temp = frame;
		frame = image;
		return temp;
	}
	else{
		frame.combine_line( image, true );
		ImageEx temp = frame;
		frame = image;
		interlaced = true;
		return temp;
	}
}

ImageEx Deteleciner::addProgressive( ImageEx image ){
	if( !frame.is_valid() ){
		frame = image;
		interlaced = false;
		return ImageEx();
	}
	
	if( interlaced ){
		image.combine_line( frame, false );
		clear();
		return image;
	}
	else{
		ImageEx temp = frame;
		frame = image;
		return temp;
	}
}

ImageEx Deteleciner::process( ImageEx image ){
	if( !active )
		return image;
	
	if( image.is_interlaced() )
		return addInterlaced( image );
	else
		return addProgressive( image );
}

