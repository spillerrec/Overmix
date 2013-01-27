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

#ifndef MULTI_IMAGE_H
#define MULTI_IMAGE_H

#include "imageViewer.h"

#include <QString>
#include <QImage>
#include <QRect>

#include <utility>
#include <vector>
using namespace std;

struct color{
	unsigned r;
	unsigned b;
	unsigned g;
	unsigned a;
	
	void clear(){
		r = b = g = a = 0;
	}
	
	void trunc( unsigned max ){
		r = ( r > max ) ? max : r;
		g = ( g > max ) ? max : g;
		b = ( b > max ) ? max : b;
		a = ( a > max ) ? max : a;
	}
	
	void diff( color c ){
		r = ( c.r > r ) ? c.r - r : r - c.r;
		g = ( c.g > g ) ? c.g - g : g - c.g;
		b = ( c.b > b ) ? c.b - b : b - c.b;
		a = ( c.a > a ) ? c.a - a : a - c.a;
	}
	
	color(){
		clear();
	}
	color( unsigned r, unsigned g, unsigned b, unsigned a = 255*256 ){
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = a;
	}
	color( color* c ){
		r = c->r;
		g = c->g;
		b = c->b;
		a = c->a;
	}
	color( QRgb c ){
		r = qRed( c ) * 256;
		g = qGreen( c ) * 256;
		b = qBlue( c ) * 256;
		a = qAlpha( c ) * 256;
	}
	
	color& operator+=( const color &rhs ){
		r += rhs.r;
		g += rhs.g;
		b += rhs.b;
		a += rhs.a;
		return *this;
	}
	
	color& operator-=( const color &rhs ){
		r -= rhs.r;
		g -= rhs.g;
		b -= rhs.b;
		a -= rhs.a;
		return *this;
	}
	
	color& operator+=( const int &rhs ){
		r += rhs;
		g += rhs;
		b += rhs;
		a += rhs;
		return *this;
	}
	
	color& operator*=( const int &rhs ){
		r *= rhs;
		g *= rhs;
		b *= rhs;
		a *= rhs;
		return *this;
	}
	
	color& operator/=( const int &rhs ){
		r /= rhs;
		g /= rhs;
		b /= rhs;
		a /= rhs;
		return *this;
	}
	
	color& operator<<=( const int &rhs ){
		r <<= rhs;
		g <<= rhs;
		b <<= rhs;
		a <<= rhs;
		return *this;
	}
	
	color& operator>>=( const int &rhs ){
		r >>= rhs;
		g >>= rhs;
		b >>= rhs;
		a >>= rhs;
		return *this;
	}
	
	const color operator+( const color &other ) const{
		return color(*this) += other;
	}
	const color operator+( const int &other ) const{
		return color(*this) += other;
	}
	const color operator-( const color &other ) const{
		return color(*this) -= other;
	}
	const color operator*( const int &other ) const{
		return color(*this) *= other;
	}
	const color operator/( const int &other ) const{
		return color(*this) /= other;
	}
	const color operator<<( const int &other ) const{
		return color(*this) <<= other;
	}
	const color operator>>( const int &other ) const{
		return color(*this) >>= other;
	}
};


class MultiImage{
	private:
		vector<QImage> imgs;
		vector<pair<int,int> > pos;
		QRect size_cache;
		imageViewer* viewer;
		QImage* temp;
		bool do_dither;
		bool do_diff;
		unsigned threshould;
		double movement;
		int merge_method; //0 = both, 1 = hor, 2 = ver
		bool use_average;
		
		
		QImage image_average();
	
	public:
		static unsigned diff_amount;
		static double img_diff( int x, int y, QImage &img1, QImage &img2 );
		static int best_vertical_slow( QImage img1, QImage img2 );
		static std::pair<QPoint,double> best_vertical( QImage img1, QImage img2, int level, double range = 1.0 );
		static std::pair<QPoint,double> best_horizontal( QImage img1, QImage img2, int level, double range = 1.0 );
		
		static std::pair<QPoint,double> best_round( QImage img1, QImage img2, int level, double range = 1.0 );
		static std::pair<QPoint,double> best_round_sub( QImage img1, QImage img2, int level, int left, int right, int h_middle, int top, int bottom, int v_middle, double diff );
		
	public:
		MultiImage( imageViewer* view ){
			viewer = view;
			do_dither = false;
			do_diff = false;
			threshould = 16*256;
			temp = NULL;
			movement = 0.5;
			merge_method = 0;
			use_average = true;
		}
		
		void set_dither( bool value ){ do_dither = value; }
		void set_diff( bool value ){ do_diff = value; }
		void set_use_average( bool value ){ use_average = value; }
		void set_threshould( unsigned value ){ threshould = value * 256; }
		void set_movement( double value ){ movement = value; }
		void set_merge_method( int value ){ merge_method = value; }
		
		void clear();
		void add_image( QString path );
		void save( QString path ) const;
		
		void draw();
		
		unsigned get_count() const{ return imgs.size(); }
		QRect get_size();
		color get_color( int x, int y ) const;
	
};

#endif