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

#include "image.h"

#include <QRect>
#include <vector>
#include <QtConcurrentMap>

using namespace std;

image::image( unsigned w, unsigned h ){
	height = h;
	width = w;
	data = new color[ h * w ];
}
image::image( QImage img ){
	height = img.height();
	width = img.width();
	data = new color[ height * width ];
	
	for( unsigned iy=0; iy<height; iy++ ){
		const QRgb *row = (const QRgb*)img.constScanLine( iy );
		color* row2 = scan_line( iy );
		for( unsigned ix=0; ix<width; ++ix, ++row, ++row2 )
			*row2 = color( *row );
	}
}

image::~image(){
	qDebug( "deleting image %p", this );
	if( data )
		delete[] data;
}


double image::diff( const image& img, int x, int y ) const{
	unsigned long long difference = 0;
	unsigned amount = 0;
	
	QRect r1( QPoint(0,0), QSize( width, height ) );
	QRect r2( QPoint(x,y), QSize( img.get_width(), img.get_height() ) );
	QRect common = r1.intersected( r2 );
	
	for( int iy=common.y(); iy<common.height()+common.y(); iy++ ){
		const color* row1 = scan_line( iy );
		const color* row2 = img.scan_line( iy-y );
		
		row2 -= x;
		for( int ix=common.x(); ix<common.width()+common.x(); ix++ ){
			color p1 = row1[ix];
			color p2 = row2[ix];
			if( p1.a > 127*256 && p2.a > 127*256 ){
				color d = p1.difference( p2 );
				difference += d.r;
				difference += d.g;
				difference += d.b;
				amount++;
			}
		}
	}
	//qDebug( "%d %d - %d %d %d %d - %d",x,y, common.x(), common.y(), common.width(), common.height(), difference  / amount );
	
	return amount ? ((double)difference / (double)amount / 256.0) : 999999;
}


struct img_comp{
	image* img1;
	image* img2;
	int h_middle;
	int v_middle;
	double diff;
	int level;
	int left;
	int right;
	int top;
	int bottom;
	
	img_comp( image& image1, image& image2, int hm, int vm, int lvl=0, int l=0, int r=0, int t=0, int b=0 ){
		img1 = &image1;
		img2 = &image2;
		h_middle = hm;
		v_middle = vm;
		diff = -1;
		level = lvl;
		left = l;
		right = r;
		top = t;
		bottom = b;
	}
	void do_diff( int x, int y ){
		if( diff < 0 )
			diff = img1->diff( *img2, x, y );
	}
	
	MergeResult result() const{
		if( level > 0 )
			return img1->best_round_sub( *img2, level, left, right, h_middle, top, bottom, v_middle, diff );
		else
			return MergeResult(QPoint( h_middle, v_middle ),diff);
	}
};
void do_diff_center( img_comp& comp ){
	comp.do_diff( comp.h_middle, comp.v_middle );
}

MergeResult image::best_round( image& img, int level, double range_x, double range_y ){
	//Bail if invalid settings
	if(	level < 1
		||	( range_x < 0.0 || range_x > 1.0 )
		||	( range_y < 0.0 || range_y > 1.0 )
		||	is_invalid()
		||	img.is_invalid()
		)
		return MergeResult(QPoint(),99999);
	
	//Starting point is the one where both images are centered on each other
	int x = ( (int)width - img.get_width() ) / 2;
	int y = ( (int)height - img.get_height() ) / 2;
	
	return best_round_sub(
			img, level
		,	((int)1 - (int)img.get_width()) * range_x, ((int)width - 1) * range_x, x
		,	((int)1 - (int)img.get_height()) * range_y, ((int)height - 1) * range_y, y
		,	diff( img, x,y )
		);
}

MergeResult image::best_round_sub( image& img, int level, int left, int right, int h_middle, int top, int bottom, int v_middle, double diff ){
//	qDebug( "Round %d: %d,%d,%d x %d,%d,%d at %.2f", level, left, h_middle, right, top, v_middle, bottom, diff );
	QList<img_comp> comps;
	int amount = level*2 + 2;
	double h_offset = (double)(right - left) / amount;
	double v_offset = (double)(bottom - top) / amount;
	level = level > 1 ? level-1 : 1;
	
	if( h_offset < 1 && v_offset < 1 ){
		//Handle trivial step
		//Check every diff in the remaining area
		for( int ix=left; ix<=right; ix++ )
			for( int iy=top; iy<=bottom; iy++ ){
				img_comp t( *this, img, ix, iy );
				if( ix == h_middle && iy == v_middle )
					t.diff = diff;
				comps << t;
			}
	}
	else{
		//Make sure we will not do the same position multiple times
		double h_add = ( h_offset < 1 ) ? 1 : h_offset;
		double v_add = ( v_offset < 1 ) ? 1 : v_offset;
		
		for( double iy=top+v_offset; iy<=bottom; iy+=v_add )
			for( double ix=left+h_offset; ix<=right; ix+=h_add ){
				int x = ( ix < 0.0 ) ? ceil( ix-0.5 ) : floor( ix+0.5 );
				int y = ( iy < 0.0 ) ? ceil( iy-0.5 ) : floor( iy+0.5 );
				
				//Avoid right-most case. Can't be done in the loop
				//as we always want it to run at least once.
				if( ( x == right && x != left ) || ( y == bottom && y != top ) )
					continue;
				
				//Create and add
				img_comp t(
						*this, img, x, y, level
					,	floor( ix - h_offset ), ceil( ix + h_offset )
					,	floor( iy - v_offset ), ceil( iy + v_offset )
					);
				
				if( x == h_middle && y == v_middle )
					t.diff = diff; //Reuse old diff
				
				comps << t;
			}
	}
	
	//Calculate diffs
	QtConcurrent::map( comps, do_diff_center ).waitForFinished();
	
	//Find best comp
	const img_comp* best = NULL;
	double best_diff = 99999;
	
	for( int i=0; i<comps.size(); i++ ){
		if( comps.at(i).diff < best_diff ){
			best = &comps.at(i);
			best_diff = best->diff;
		}
	}
	
	if( !best ){
		qDebug( "ERROR! no result to continue on!!" );
		return MergeResult(QPoint(),99999);
	}
	
	return best->result();
}


