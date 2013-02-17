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
		for( unsigned ix=0; ix<width; ix++ ){
			data[ ix + iy*width] = color( row[ix] );
		}
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
			if( p1.a > 127 && p2.a > 127 ){
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


MergeResult image::best_vertical( const image& img, int level, double range ) const{
	int y = ( (int)height - img.get_height() ) / 2;
	double difference = diff( img, 0,y );
	return best_round_sub(
			img, level
		,	0,0,0
		,	((int)1 - (int)img.get_height()) * range, ((int)height - 1) * range, y
		,	difference
		);
}

MergeResult image::best_horizontal( const image& img, int level, double range ) const{
	int x = ( (int)width - img.get_width() ) / 2;
	double difference = diff( img, x,0 );
	return best_round_sub(
			img, level
		,	((int)1 - (int)img.get_width()) * range, ((int)width - 1) * range, x
		,	0,0,0
		,	difference
		);
}


struct img_comp{
	const image& img1;
	const image& img2;
	int h_middle;
	int v_middle;
	double diff;
	
	img_comp( const image& image1,  const image& image2, int hm, int vm ) : img1( image1 ), img2( image2 ){
		h_middle = hm;
		v_middle = vm;
		diff = -1;
	}
	void do_diff( int x, int y ){
		if( diff < 0 )
			diff = img1.diff( img2, x, y );
	}
	
	virtual MergeResult result(){ return std::pair<QPoint,double>(QPoint( h_middle, v_middle ),diff); }
	virtual void debug(){
		qDebug( "img_comp (%d,%d) at %.2f", h_middle, v_middle, diff );
	}
};
void do_diff_center( img_comp* comp ){
	comp->do_diff( comp->h_middle, comp->v_middle );
}

struct img_comp_round : img_comp{
	int level;
	int left;
	int right;
	int top;
	int bottom;
	
	img_comp_round( const image& image1, const image& image2, int lvl, int l, int r, int hm, int t, int b, int vm )
		:	img_comp( image1, image2, hm, vm )
		{
		level = lvl;
		left = l;
		right = r;
		top = t;
		bottom = b;
	}
	
	MergeResult result(){
		return img1.best_round_sub( img2, level, left, right, h_middle, top, bottom, v_middle, diff );
	}
	void debug(){
		qDebug( "img_comp_round %d: (%d,%d,%d) - (%d,%d,%d) at %.2f", level, left, h_middle, right, top, v_middle, bottom, diff );
	}
};

MergeResult image::best_round( const image& img, int level, double range ) const{
	//Bail if invalid settings
	if(	level < 1
		||	range < 0.0
		||	range > 1.0
		||	is_invalid()
		||	img.is_invalid()
		)
		return MergeResult(QPoint(),99999);
	
	//Starting point is the one where both images are centered on each other
	int x = ( (int)width - img.get_width() ) / 2;
	int y = ( (int)height - img.get_height() ) / 2;
	
	return best_round_sub(
			img, level
		,	((int)1 - (int)img.get_width()) * range, ((int)width - 1) * range, x
		,	((int)1 - (int)img.get_height()) * range, ((int)height - 1) * range, y
		,	diff( img, x,y )
		);
}

MergeResult image::best_round_sub( const image& img, int level, int left, int right, int h_middle, int top, int bottom, int v_middle, double diff ) const{
	qDebug( "Round %d: %d,%d,%d x %d,%d,%d at %.2f", level, left, h_middle, right, top, v_middle, bottom, diff );
	QList<img_comp*> comps;
	int amount = level*2 + 2;
	double h_offset = (double)(right - left) / amount;
	double v_offset = (double)(bottom - top) / amount;
	level = level > 1 ? level-1 : 1;
	
	if( h_offset < 1 && v_offset < 1 ){
		//Handle trivial step
		//Check every diff in the remaining area
		for( int ix=left; ix<=right; ix++ )
			for( int iy=top; iy<=bottom; iy++ ){
				img_comp* t = new img_comp( *this, img, ix, iy );
				if( ix == h_middle && iy == v_middle )
					t->diff = diff;
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
				img_comp* t = new img_comp_round(
						*this, img, level
					,	floor( ix - h_offset ), ceil( ix + h_offset ), x
					,	floor( iy - v_offset ), ceil( iy + v_offset ), y
					);
				
				if( x == h_middle && y == v_middle )
					t->diff = diff; //Reuse old diff
				
				comps << t;
			}
	}
	
	//Calculate diffs
	QFuture<void> progress = QtConcurrent::map( comps, do_diff_center );
	progress.waitForFinished();
	
	//Find best comp
	img_comp* best = NULL;
	double best_diff = 99999;
	
	for( unsigned i=0; i<comps.size(); i++ ){
		if( comps.at(i)->diff < best_diff ){
			best = comps.at(i);
			best_diff = best->diff;
		}
	}
	
	if( !best ){
		qDebug( "ERROR! no result to continue on!!" );
		return MergeResult(QPoint(),99999);
	}
	
	//Calculate result, delete and return
	qDebug( "\tbest: %d,%d at %.2f", best->h_middle, best->v_middle, best->diff );
	MergeResult result = best->result();
	
	for( unsigned i=0; i<comps.size(); i++ )
		delete comps.at(i);
	
	return result;
}


