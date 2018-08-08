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

#ifndef MANIPULATORS_WAIFU_HPP
#define MANIPULATORS_WAIFU_HPP

struct W2XConv;

namespace Overmix{
class Plane;
class ImageEx;
	
class Waifu{
	private:
		W2XConv* conv;
		
		ImageEx processRgb( const ImageEx& );
		ImageEx processYuv( const ImageEx& );
		
	public:
		const double scale;
		const int denoise;
	
	public:
		Waifu( double scale, int denoise, const char* model_dir );
		~Waifu();
	
		Plane process( const Plane& p );
		ImageEx process( const ImageEx& input );
	
};


}

#endif

