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

#include <cmath>
#include <QPen>

unsigned MultiImage::diff_amount = 0;

void MultiImage::clear(){
	imgs.clear();
	pos.clear();
	viewer->change_image( NULL, true );
	temp = NULL;
}

void MultiImage::add_image( QString path ){
	QImage img( path );
	QPoint p( 0,0 );
	QRect box = get_size();
	
	if( imgs.size() > 0 ){
		bool use_average = false;
		QImage img1;
		if( use_average )
			img1 = image_average();
		else{
			img1 = imgs[imgs.size()-1];
			p.setX( pos[imgs.size()-1].first );
			p.setY( pos[imgs.size()-1].second );
			if( p.x() < 0 )
				p.setX( 0 );
			if( p.y() < 0 )
				p.setY( 0 );
		}
		
		double diff = 99999;
		int level = 1;
		while( diff > 24 && level < 6 ){
			pair<QPoint,double> result = best_round( img1, img, level, 0.5 );
			level++;
			p += result.first;
			diff = result.second;
		}
	//	int py = best_vertical_slow( imgs[imgs.size()-1], img );
	//	if( y!=py ){
	//		qDebug( "Missed alignment: %d where should be %d", y, py );
	//		qDebug( "with file: %s", path.toLocal8Bit().data() );
	//		y = py;
	//	}
		p += box.topLeft();
	}
	
	//TODO: what to do with zeros
	bool no_zero = true;
	imgs.push_back( img );
	pos.push_back( pair<int,int>( p.x(), p.y() ) );
	size_cache = QRect();
}

void MultiImage::save( QString path ) const{
	if( temp )
		temp->save( path );
}


QImage MultiImage::image_average(){
	QRect box = get_size();
	QImage img( box.width(), box.height(), QImage::Format_ARGB32 );
	
	for( int iy = 0; iy < box.height(); iy++ ){
		QRgb* row = (QRgb*)img.scanLine( iy );
		for( int ix = 0; ix < box.width(); ix++ ){
				
			int r, g, b, a, amount;
			r = g = b = a = amount = 0;
			for( unsigned i=0; i<imgs.size(); i++ ){
				pair<int,int> p = pos[i];
				QRect placement( QPoint( p.first, p.second ), imgs[i].size() );
				if( placement.contains( QPoint( ix+box.x(),iy+box.y() ) ) ){
					QRgb px = imgs[i].pixel( ix-p.first+box.x(), iy-p.second+box.y() );
					r += qRed( px );
					g += qGreen( px );
					b += qBlue( px );
					a += qAlpha( px );
					amount++;
				}
			}
			if( amount )
				row[ix] = qRgba( r / amount, g / amount, b / amount, a / amount );
			else
				row[ix] = qRgba( 255, 255, 255, 0 );
		}
	}
	
	return img;
}


void MultiImage::draw(){
	QRect box = get_size();
	color *line = new color[ box.width()+1 ];
	
	temp = new QImage( /*image_average() );/*/box.width(), box.height(), QImage::Format_ARGB32 );
	temp->fill(0);
	for( int iy = 0; iy < box.height(); iy++ ){
		QRgb* row = (QRgb*)temp->scanLine( iy );
		for( int ix = 0; ix < box.width(); ix++ ){
			color c = get_color( ix+box.x(), iy+box.y() );
			
			if( do_dither )
				c += line[ix];
			
			color rounded = (c) / 256;
			
			if( do_dither ){
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
	delete line;//*/
	viewer->change_image( temp, true );
}

QRect MultiImage::get_size(){
	if( size_cache.isValid() )
		return size_cache;
	
	int ymin, ymax, xmin, xmax;
	ymin = xmin = 9999; //Bah...
	ymax = xmax = 0;
	
	if( pos.size() && pos.size() == imgs.size() ){
		for( unsigned i=0; i<pos.size(); i++ ){
			if( xmin > pos[i].first )
				xmin = pos[i].first;
			if( xmax < (pos[i].first + imgs[i].width()) )
				xmax = pos[i].first + imgs[i].width();
				
			if( ymin > pos[i].second )
				ymin = pos[i].second;
			if( ymax < (pos[i].second + imgs[i].height()) )
				ymax = pos[i].second + imgs[i].height();
		}
	}
	else{
		ymin = xmin = 0;
	}
	
	qDebug( "Size: %d %d %d %d", xmin, ymin, xmax-xmin, ymax-ymin );
	return size_cache = QRect( xmin, ymin, xmax-xmin, ymax-ymin );
}

color MultiImage::get_color( int x, int y ) const{
	//Find all pixels on this spot
	vector<color> pixels;
	for( unsigned i=0; i<imgs.size(); i++ ){
		pair<int,int> p = pos[i];
		QRect placement( QPoint( p.first, p.second ), imgs[i].size() );
		if( placement.contains( QPoint( x,y ) ) )
			pixels.push_back( color( imgs[i].pixel( x-p.first, y-p.second ) ) );
	}
	
	//If none was found, make it transparent
	int size = pixels.size();
	if( size < 1 )
		return color( 0,0,0,0 );
	
	//*
	
	//Find the one where threshould will give most samples
	unsigned best = 0;
	color best_color;
	for( unsigned i=0; i<pixels.size(); i++ ){
		unsigned amount = 0;
		color avg;
		
		for( unsigned j=0; j<pixels.size(); j++ ){
			color d( pixels[i] );
			d.diff( pixels[j] );
			
			unsigned max = d.r > d.g ? d.r : d.g;
			max = d.b > max ? d.b : max;
			if( max <= threshould ){
				amount++;
				avg += pixels[j];
			}
		}
		
		if( amount > best ){
			best = amount;
			best_color = avg / amount;
		}
	}
	
	
	if( best ){
		if( do_diff )
			best_color.r = best_color.g = best_color.b = 255*256 * best / pixels.size();
		return best_color;
	}
	else
		return color( 0,0,255*256 );
	
	/*/
	//Calculate average:
	color avg;
	for( unsigned i=0; i<pixels.size(); i++ )
		avg += pixels[i];
	avg /= size;
	
	//Calculate value
	color r;
	unsigned amount = 0;
	int threshould = 16*256;
	for( unsigned i=0; i<pixels.size(); i++ ){
		//Find difference from average
		color d( pixels[i] );
		d.diff( avg );
		
		//Find the largest difference
		unsigned max = d.r > d.g ? d.r : d.g;
		max = d.b > max ? d.b : max;
		
		//Only apply if below threshould
		if( max <= threshould ){
			r += pixels[i];
			amount++;
		}
	}
	
	//Let it be transparent if no amount
	if( amount ){
		r.a *= amount;
		return r / amount;
	}
	else
		return color( 0,0,255*256 );
	
	//*/
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
	
	diff_amount++;
	return amount ? ((double)diff / (double)amount) : 999999;
}


int MultiImage::best_vertical_slow( QImage img1, QImage img2 ){
	double min = 99999;
	int y = 0;
	
	int height = img1.height();
	for( int i=1-height; i<height; i++ ){
		double diff = img_diff( 0, i, img1, img2 );
//		qDebug( "Diff: %d at %d", diff, i );
		if( diff < min ){
			min = diff;
			y = i;
		}
	}
	
	return y;
}


int MultiImage::best_vertical( QImage img1, QImage img2 ){
	diff_amount = 0;
	unsigned height = img1.height();
	
	double diff = img_diff( 0,0, img1, img2 );
	int y = best_round_sub( img1, img2, 1, 0,0,0, -height+1, height, 0, diff ).first.y();
	
	qDebug( "Diff performance: %d", diff_amount );
	
	return y;
}

int MultiImage::best_horizontal( QImage img1, QImage img2 ){
	diff_amount = 0;
	unsigned width = img1.width();
	
	double diff = img_diff( 0,0, img1, img2 );
	int x = best_round_sub( img1, img2, 1, -width+1, width, 0, 0,0,0, diff ).first.x();
	
	qDebug( "Diff performance: %d", diff_amount );
	
	return x;
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
	
	virtual std::pair<QPoint,double> result(){ return std::pair<QPoint,double>(QPoint( h_middle, v_middle ),diff); }
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
	
	std::pair<QPoint,double> result(){
		return MultiImage::best_round_sub( img1, img2, level, left, right, h_middle, top, bottom, v_middle, diff );
	}
	void debug(){
		qDebug( "img_comp_round %d: (%d,%d,%d) - (%d,%d,%d) at %.2f", level, left, h_middle, right, top, v_middle, bottom, diff );
	}
};

std::pair<QPoint,double> MultiImage::best_round( QImage img1, QImage img2, int level, double range ){
	//Bail if invalid settings
	if(	level < 1
		||	range < 0.0
		||	range > 1.0
		||	img1.isNull()
		||	img2.isNull()
		)
		return std::pair<QPoint,double>(QPoint(),99999);
	
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

std::pair<QPoint,double> MultiImage::best_round_sub( QImage img1, QImage img2, int level, int left, int right, int h_middle, int top, int bottom, int v_middle, double diff ){
	qDebug( "Round %d: %d,%d,%d x %d,%d,%d at %.2f", level, left, h_middle, right, top, v_middle, bottom, diff );
	vector<img_comp*> comps;
	int amount = level*2 + 2;
	double h_offset = (double)(right - left) / amount;
	double v_offset = (double)(bottom - top) / amount;
	qDebug( "offsets: %.2f, %.2f", h_offset, v_offset );
	level = level > 1 ? level-1 : 1;
	
	if( h_offset < 1 && v_offset < 1 ){
		qDebug( "\tstarting trivial step" );
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
				if( x == right || y == bottom )
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
			//	t->debug();
			}
	}
	/*
	{
		//Handle interation step
		//Horizontal movement
		if( h_offset >= 1 ){
			img_comp* l = new img_comp_round( img1, img2, left, h_middle, left+h_offset,      top+v_offset, v_middle+v_offset, v_middle );
			img_comp* r = new img_comp_round( img1, img2, h_middle, right, h_middle+h_offset, top+v_offset, v_middle+v_offset, v_middle );
			l->do_diff_center();
			r->do_diff_center();
			qDebug( "\tlr: %.2f %.2f", l->diff, r->diff );
			comps.push_back( l );
			comps.push_back( r );
		}
		
		//Vertical movement
		if( v_offset >= 1 ){
			img_comp* t = new img_comp_round( img1, img2, left+h_offset, h_middle+h_offset, h_middle, top, v_middle, top+v_offset );
			img_comp* b = new img_comp_round( img1, img2, left+h_offset, h_middle+h_offset, h_middle, v_middle, bottom, v_middle+v_offset );
			t->do_diff_center();
			b->do_diff_center();
			qDebug( "\ttb: %.2f %.2f", t->diff, b->diff );
			comps.push_back( t );
			comps.push_back( b );
		}
		
		//Dirgonal movement
		if( v_offset >= 1 && h_offset >= 1 ){
			img_comp* tl = new img_comp_round( img1, img2, left, h_middle, left+h_offset,      top, v_middle, top+v_offset );
			img_comp* tr = new img_comp_round( img1, img2, h_middle, right, h_middle+h_offset, top, v_middle, top+v_offset );
			img_comp* bl = new img_comp_round( img1, img2, left, h_middle, left+h_offset,      v_middle, bottom, v_middle+v_offset );
			img_comp* br = new img_comp_round( img1, img2, h_middle, right, h_middle+h_offset, v_middle, bottom, v_middle+v_offset );
			
			tl->do_diff_center();
			tr->do_diff_center();
			bl->do_diff_center();
			br->do_diff_center();
			qDebug( "\ttl,tr,bl,br: %.2f %.2f %.2f %.2f", tl->diff, tr->diff, bl->diff, br->diff );
			
			comps.push_back( tl );
			comps.push_back( tr );
			comps.push_back( bl );
			comps.push_back( br );
		}
		
		//Center
		img_comp* c = new img_comp_round( img1, img2, left+h_offset, h_middle+h_offset, h_middle, top+v_offset, v_middle+v_offset, v_middle );
		c->diff = diff;
		comps.push_back( c );
	}
	*/
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
		return std::pair<QPoint,double>(QPoint(),99999);
	}
	
	//Calculate result, delete and return
	qDebug( "\tbest: %d,%d at %.2f", best->h_middle, best->v_middle, best->diff );
	std::pair<QPoint,double> result = best->result();
	
	for( unsigned i=0; i<comps.size(); i++ )
		delete comps[i];
	
	return result;
}

