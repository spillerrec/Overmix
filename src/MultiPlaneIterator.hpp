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
#include <QImage>

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
		int x;
		int y;
		int right;
		int bottom;
		int left;
		int top;
		
	private:
		void new_y( int y );
		void new_x( int x );
	
	public:
		MultiPlaneIterator( std::vector<PlaneItInfo> info ) :	infos( info ){
			x = y = left = top = 0;
			right = bottom = -1;
		}
		
		bool valid() const{ return y <= bottom && x <= right; }
		unsigned width(){ return right - left + 1; }
		unsigned height(){ return bottom - top + 1; }
		
		void iterate_all();
		void iterate_shared();
		
		void next_y(){ new_y( y + 1 ); }
		void next_x(){ new_x( x + 1 ); }
		void next_line(){
			next_y();
			new_x( left );
		}
		void next(){
			if( x < right )
				next_x();
			else
				next_line();
		}
		
	public:
		color_type& operator[]( const int index ){
			return *infos[index].row;
		}
		
		color_type diff( unsigned i1, unsigned i2 ){
			color_type c1 = (*this)[i1], c2 = (*this)[i2];
			return c2 > c1 ? c2-c1 : c1-c2;
		}
		
		QRgb gray_to_qrgb(){
			int val = (*this)[0]/256;
			return qRgb( val, val, val );
		}
		
		QRgb rgb_to_qrgb(){
			return qRgb( (*this)[0]/256, (*this)[1]/256, (*this)[2]/256 );
		}
		
		QRgb yuv_to_qrgb();
		
		void write_average();
};

#endif