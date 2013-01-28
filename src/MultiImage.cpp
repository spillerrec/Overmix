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


#include "MultiImage.h"
#include "MultiImageIterator.h"

#include <cmath>

using namespace std;

MultiImage::MultiImage(){
	do_diff = false;
	threshould = 16*256;
	movement = 0.5;
	merge_method = 0;
	use_average = true;
}

void MultiImage::clear(){
	imgs.clear();
	pos.clear();
	calculate_size();
}

void MultiImage::add_image( QString path ){
	QImage img( path );
	QPoint p( 0,0 );
	
	if( imgs.size() > 0 ){
		QImage img1;
		if( use_average )
			img1 = render( FILTER_AVERAGE );
		else{
			img1 = imgs[imgs.size()-1];
			p = pos[imgs.size()-1];
			if( p.x() < 0 )
				p.setX( 0 );
			if( p.y() < 0 )
				p.setY( 0 );
		}
		
		//Keep repeating with higher levels until it drops
		//below threshould
		int level = 1;
		pair<QPoint,double> result;
		do{
			switch( merge_method ){
				case 1: 
						result = best_horizontal( img1, img, level, movement );
					break;
				case 2: 
						result = best_vertical( img1, img, level, movement );
					break;
				
				case 0:
				default:
						result = best_round( img1, img, level, movement );
					break;
			}
		}while( result.second > 24 && level++ < 6 );
		p += get_size().topLeft() + result.first;
	}
	
	//Add image
	imgs.push_back( img );
	pos.push_back( p );
	calculate_size();
}

QImage MultiImage::render( filters filter, bool dither ) const{
	QRect box = get_size();
	MultiImageIterator it( imgs, pos, box.x(),box.y() );
	color *line = new color[ box.width()+1 ];
	
	QImage temp( box.width(), box.height(), QImage::Format_ARGB32 );
	temp.fill(0);
	for( int iy = 0; iy < box.height(); iy++, it.next_line() ){
		QRgb* row = (QRgb*)temp.scanLine( iy );
		for( int ix = 0; ix < box.width(); ix++, it.next_x() ){
			color c;
			switch( filter ){
				case FILTER_SIMPLE:	c = it.simple_filter( threshould ); break;
				case FILTER_SIMPLE_SLIDE:	c = it.simple_slide( threshould ); break;
				default:
				case FILTER_AVERAGE:	c = it.average(); break;
			}
			
			if( dither )
				c += line[ix];
			
			color rounded = (c) / 256;
			
			if( dither ){
				color err = c - ( rounded * 256 );
				line[ix] = err / 4;
				line[ix+1] += err / 2;
				if( ix )
					line[ix-1] += err / 4;
			}
			
			//rounded.trunc( 255 );
			row[ix] = qRgba( rounded.r, rounded.g, rounded.b, rounded.a );
		}
	}
	delete line;
	
	return temp;
}


void MultiImage::calculate_size(){
	size_cache = QRect();
	
	for( unsigned i=0; i<pos.size(); i++ )
		size_cache = size_cache.united( QRect( pos[i], imgs[i].size() ) );
}

double MultiImage::img_diff( int x, int y, QImage &img1, QImage &img2 ){
	unsigned diff = 0;
	unsigned amount = 0;
	
	QRect r1( QPoint(0,0), img1.size() );
	QRect r2( QPoint(x,y), img2.size() );
	QRect common = r1.intersected( r2 );
	
	for( int iy=common.y(); iy<common.height()+common.y(); iy++ ){
		const QRgb* row1 = (const QRgb*)img1.constScanLine( iy );
		const QRgb* row2 = (const QRgb*)img2.constScanLine( iy-y );
		
		row2 -= x;
		for( int ix=common.x(); ix<common.width()+common.x(); ix++ ){
			QRgb p1 = row1[ix];
			QRgb p2 = row2[ix];
			if( qAlpha( p1 ) > 127 && qAlpha( p2 ) > 127 ){
				
				int r = abs( qRed( p2 ) - qRed( p1 ) );
				int g = abs( qGreen( p2 ) - qGreen( p1 ) );
				int b = abs( qBlue( p2 ) - qBlue( p1 ) );
				
				diff += r + g +b;
				amount++;
			}
		}
	}
//	qDebug( "%d %d - %d %d %d %d - %d",x,y, common.x(), common.y(), common.width(), common.height(), diff * 2 / amount );
	
	return amount ? ((double)diff / (double)amount) : 999999;
}


MergeResult MultiImage::best_vertical( QImage img1, QImage img2, int level, double range ){
	int y = ( img1.height() - img2.height() ) / 2;
	double diff = img_diff( 0,y, img1, img2 );
	return best_round_sub(
			img1, img2, level
		,	0,0,0
		,	(1 - img2.height()) * range, (img1.height() - 1) * range, y
		,	diff
		);
}

MergeResult MultiImage::best_horizontal( QImage img1, QImage img2, int level, double range ){
	int x = ( img1.width() - img2.width() ) / 2;
	double diff = img_diff( x,0, img1, img2 );
	return best_round_sub(
			img1, img2, level
		,	(1 - img2.width()) * range, (img1.width() - 1) * range, x
		,	0,0,0
		,	diff
		);
}


struct img_comp{
	QImage img1;
	QImage img2;
	int h_middle;
	int v_middle;
	double diff;
	
	img_comp( QImage image1, QImage image2, int hm, int vm ){
		img1 = image1;
		img2 = image2;
		h_middle = hm;
		v_middle = vm;
		diff = -1;
	}
	void do_diff( int x, int y ){
		diff = MultiImage::img_diff( x, y, img1, img2 );
	}
	void do_diff_center(){
		do_diff( h_middle, v_middle );
	}
	
	virtual MergeResult result(){ return std::pair<QPoint,double>(QPoint( h_middle, v_middle ),diff); }
	virtual void debug(){
		qDebug( "img_comp (%d,%d) at %.2f", h_middle, v_middle, diff );
	}
};

struct img_comp_round : img_comp{
	int level;
	int left;
	int right;
	int top;
	int bottom;
	
	img_comp_round( QImage image1, QImage image2, int lvl, int l, int r, int hm, int t, int b, int vm )
		:	img_comp( image1, image2, hm, vm )
		{
		level = lvl;
		left = l;
		right = r;
		top = t;
		bottom = b;
	}
	
	MergeResult result(){
		return MultiImage::best_round_sub( img1, img2, level, left, right, h_middle, top, bottom, v_middle, diff );
	}
	void debug(){
		qDebug( "img_comp_round %d: (%d,%d,%d) - (%d,%d,%d) at %.2f", level, left, h_middle, right, top, v_middle, bottom, diff );
	}
};

MergeResult MultiImage::best_round( QImage img1, QImage img2, int level, double range ){
	//Bail if invalid settings
	if(	level < 1
		||	range < 0.0
		||	range > 1.0
		||	img1.isNull()
		||	img2.isNull()
		)
		return MergeResult(QPoint(),99999);
	
	//Starting point is the one where both images are centered on each other
	int x = ( img1.width() - img2.width() ) / 2;
	int y = ( img1.height() - img2.height() ) / 2;
	
	return best_round_sub(
			img1, img2, level
		,	(1 - img2.width()) * range, (img1.width() - 1) * range, x
		,	(1 - img2.height()) * range, (img1.height() - 1) * range, y
		,	img_diff( x,y, img1, img2 )
		);
}

MergeResult MultiImage::best_round_sub( QImage img1, QImage img2, int level, int left, int right, int h_middle, int top, int bottom, int v_middle, double diff ){
	qDebug( "Round %d: %d,%d,%d x %d,%d,%d at %.2f", level, left, h_middle, right, top, v_middle, bottom, diff );
	vector<img_comp*> comps;
	int amount = level*2 + 2;
	double h_offset = (double)(right - left) / amount;
	double v_offset = (double)(bottom - top) / amount;
	level = level > 1 ? level-1 : 1;
	
	if( h_offset < 1 && v_offset < 1 ){
		//Handle trivial step
		//Check every diff in the remaining area
		for( int ix=left; ix<=right; ix++ )
			for( int iy=top; iy<=bottom; iy++ ){
				img_comp* t = new img_comp( img1, img2, ix, iy );
				if( ix == h_middle && iy == v_middle )
					t->diff = diff;
				else
					t->do_diff_center();
				comps.push_back( t );
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
						img1, img2, level
					,	floor( ix - h_offset ), ceil( ix + h_offset ), x
					,	floor( iy - v_offset ), ceil( iy + v_offset ), y
					);
				
				if( x == h_middle && y == v_middle )
					t->diff = diff; //Reuse old diff
				else
					t->do_diff_center(); //Calculate new
				
				comps.push_back( t );
			}
	}
	
	//Find best comp
	img_comp* best = NULL;
	double best_diff = 99999;
	
	for( unsigned i=0; i<comps.size(); i++ ){
		if( comps[i]->diff < best_diff ){
			best = comps[i];
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
		delete comps[i];
	
	return result;
}

