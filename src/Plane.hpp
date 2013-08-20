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

#ifndef PLANE_HPP
#define PLANE_HPP

#include <QPoint>
#include <QList>
#include <cstdio>
#include <utility>

typedef unsigned short color_type;
typedef std::pair<QPoint,double> MergeResult;

class DiffCache{
	private:
		struct Cached{
			int x;
			int y;
			double diff;
		};
		QList<Cached> cache;
		
	public:
		double get_diff( int x, int y ) const;
		void add_diff( int x, int y, double diff );
};

class Plane{
	private:
		unsigned height;
		unsigned width;
		unsigned line_width;
		color_type *data;
		
	public:
		Plane( unsigned w, unsigned h );
		~Plane();
		
		static const unsigned long MAX_VAL;
		
	//Status
		bool is_invalid() const{ return data == 0; }
		unsigned get_height() const{ return height; }
		unsigned get_width() const{ return width; }
		
	//Pixel/Row query
		color_type& pixel( unsigned x, unsigned y ) const{ return data[ x + y*line_width ]; }
		color_type* scan_line( unsigned y ) const{ return data + y*line_width; }
		
	//Interlacing methods
		bool is_interlaced() const;
		void replace_line( Plane &p, bool top );
		void combine_line( Plane &p, bool top );
		
	//Difference
		double diff( const Plane& p, int x, int y, unsigned stride=1 ) const;
		MergeResult best_round_sub( const Plane& p, int level, int left, int right, int top, int bottom, DiffCache *cache ) const;
		
	//Scaling
		Plane* scale_nearest( unsigned wanted_width, unsigned wanted_height, double offset_x, double offset_y ) const;
		Plane* scale_linear( unsigned wanted_width, unsigned wanted_height, double offset_x, double offset_y ) const;
		Plane* scale_bilinear( unsigned wanted_width, unsigned wanted_height, double offset_x, double offset_y ) const;
		Plane* scale_cubic( unsigned wanted_width, unsigned wanted_height, double offset_x, double offset_y ) const;
		Plane* scale_lanczos( unsigned wanted_width, unsigned wanted_height, double offset_x, double offset_y ) const;
};

#endif