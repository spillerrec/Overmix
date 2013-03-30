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

#ifndef IMAGE_EX_HPP
#define IMAGE_EX_HPP

#include "Plane.hpp"
#include <cstdio>

class image;

struct PlaneInfo{
	public:
		Plane &p;
		double offset_x;
		double offset_y;
		int spacing_x;
		int spacing_y;
		
		PlaneInfo( Plane &p, double off_x, double off_y, int spacing ) : p( p ){
			offset_x = off_x;
			offset_y = off_y;
			spacing_x = spacing_y = spacing;
		}
};

class ImageEx{
	private:
		bool initialized;
		Plane **planes;
		PlaneInfo **infos;
		bool initialize_size( unsigned size );
		bool read_dump_plane( FILE *f, unsigned index );
		bool from_dump( const char* path );
		bool from_png( const char* path );
		
	public:
		ImageEx(){
			initialized = false;
			planes = 0;
			infos = 0;
		}
		~ImageEx(){
			if( planes )
				delete[] planes; //TODO: fix
			if( infos )
				delete[] infos;
		}
		
		bool read_file( const char* path );
		
		image* to_image();
};

#endif