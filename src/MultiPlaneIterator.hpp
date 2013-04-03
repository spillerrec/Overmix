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

#ifndef MULTI_PLANE_ITERATOR_H
#define MULTI_PLANE_ITERATOR_H

#include <vector>

#include "Plane.hpp"

struct PlaneItInfo{
	Plane *p;
	int x;
	int y;
	color_type* row_start;
	color_type* row;
	
	PlaneItInfo( Plane *p, int x, int y ){
		this->p = p;
		this->x = x;
		this->y = y;
		row_start = 0;
		row = 0;
	}
	
	bool check_x( int x ){
		return x >= 0 && (unsigned)x < p->get_width();
	}
	bool check_y( int y ){
		return y >= 0 && (unsigned)y < p->get_height();
	}
};

class MultiPlaneIterator{
	private:
		std::vector<PlaneItInfo> infos;
		int current_x;
		int current_y;
		unsigned height;
		unsigned width;
		int left;
		
	private:
		void new_y( int y );
		void new_x( int x );
		
		void row_starts( int y );
	
	public:
		MultiPlaneIterator( std::vector<PlaneItInfo> info, unsigned w, unsigned h, int x=0, int y=0 );
		
		void next_y(){ new_y( current_y + 1 ); }
		void next_x(){ new_x( current_x + 1 ); }
		void next_line(){
			next_y();
			new_x( left );
		}
		void next(){
			if( current_x+1 < width )
				next_x();
			else
				next_line();
		}
		bool valid() const{
			return current_y < height && current_x < width;
		}
		
	public:
		color_type& operator[]( const int index ){
			return *infos[index].row;
		}
		
		color_type diff( unsigned i1, unsigned i2 ){
			color_type c1 = (*this)[i1], c2 = (*this)[i2];
			return c2 > c1 ? c2-c1 : c1-c2;
		}
		
		void write_average();
};

#endif