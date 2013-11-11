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
#include "ImageEx.hpp"

ImageEx* Deteleciner::addInterlaced( ImageEx* image ){
	if( !frame ){
		frame = image;
		interlaced = true;
		return nullptr;
	}
	
	if( interlaced ){
		interlaced = true;
		frame->replace_line( *image, true );
		ImageEx* temp = frame;
		frame = image;
		return temp;
	}
	else{
		frame->combine_line( *image, true );
		frame = image;
		interlaced = true;
		return nullptr;
	}
}

ImageEx* Deteleciner::addProgressive( ImageEx* image ){
	if( !frame ){
		frame = image;
		interlaced = false;
		return nullptr;
	}
	
	if( interlaced ){
		image->combine_line( *frame, false );
	//	delete frame;
		frame = nullptr;
		return image;
	}
	else{
		ImageEx* done = frame;
		frame = image;
		return done;
	}
}

void Deteleciner::clear(){
	delete frame;
	frame = nullptr;
}

ImageEx* Deteleciner::process( ImageEx* image ){
	if( image->is_interlaced() )
		return addInterlaced( image );
	else
		return addProgressive( image );
}

