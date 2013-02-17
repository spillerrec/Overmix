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
	threshould = 16*256;
	movement = 0.5;
	merge_method = 0;
	use_average = true;
}
MultiImage::~MultiImage(){
	clear();
}

void MultiImage::clear(){
	for( unsigned i=0; i<imgs.size(); i++ )
		delete imgs[i];
	imgs.clear();
	pos.clear();
	calculate_size();
}

void MultiImage::add_image( QString path ){
	image *img = new image( QImage( path ) );
	QPoint p( 0,0 );
	
	if( imgs.size() > 0 ){
		image *img1 = NULL;;
		if( use_average )
			img1 = render_image( FILTER_AVERAGE );
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
						result = img1->best_horizontal( *img, level, movement );
					break;
				case 2: 
						result = img1->best_vertical( *img, level, movement );
					break;
				
				case 0:
				default:
						result = img1->best_round( *img, level, movement );
					break;
			}
		}while( result.second > 24 && level++ < 6 );
		
		//Do sub-pixel align
	//	img_subv_diff( result.first.x(), result.first.y(), img1, img );
	//	QImage best_img = img;
	//	double best_diff = result.second;
		int y_add = 0;
/* 		for( int i=-19; i<20; i++ ){
			QImage sub = img_subv( i/20.0, img );
			double diff = img_diff( result.first.x(), result.first.y()+1, img1, sub );
			qDebug( "Diff at %d: %.2f", i, diff );
			
			if( diff < best_diff ){
				best_diff = diff;
				best_img = sub;
				y_add = 1;
			}
		}
		img = best_img; */
	/* 	
		img_subv( result.first.x(), result.first.y(), 0.1, img1, img ).save( "01.png" );
		img_subv( result.first.x(), result.first.y(), 0.2, img1, img ).save( "02.png" );
		img_subv( result.first.x(), result.first.y(), 0.3, img1, img ).save( "03.png" );
		img_subv( result.first.x(), result.first.y(), 0.4, img1, img ).save( "04.png" );
		img_subv( result.first.x(), result.first.y(), 0.5, img1, img ).save( "05.png" );
		img_subv( result.first.x(), result.first.y(), 0.6, img1, img ).save( "06.png" );
		img_subv( result.first.x(), result.first.y(), 0.7, img1, img ).save( "07.png" );
		img_subv( result.first.x(), result.first.y(), 0.8, img1, img ).save( "08.png" );
		img_subv( result.first.x(), result.first.y(), 0.9, img1, img ).save( "09.png" ); */
		
		p += get_size().topLeft() + result.first + QPoint( 0,y_add);
		delete img1;
	}
	
	//Add image
	imgs.push_back( img );
	pos.push_back( p );
	calculate_size();
}

image* MultiImage::render_image( filters filter ) const{
	QRect box = get_size();
	MultiImageIterator it( imgs, pos, box.x(),box.y() );
	
	image *img = new image( box.width(), box.height() );
	for( int iy = 0; iy < box.height(); iy++, it.next_line() ){
		color* row = img->scan_line( iy );
		for( int ix = 0; ix < box.width(); ix++, it.next_x() ){
			switch( filter ){
				case FILTER_DIFFERENCE:	row[ ix ] = it.difference(); break;
				case FILTER_SIMPLE:	row[ ix ] = it.simple_filter( threshould ); break;
				case FILTER_SIMPLE_SLIDE:	row[ ix ] = it.simple_slide( threshould ); break;
				default:
				case FILTER_AVERAGE:	row[ ix ] = it.average(); break;
			}
		}
	}
	
	return img;
}

QImage MultiImage::render( filters filter, bool dither ) const{
	image *img = render_image( filter );
	color *line = new color[ img->get_width()+1 ];
	
	QImage temp( img->get_width(), img->get_height(), QImage::Format_ARGB32 );
	temp.fill(0);
	for( unsigned iy = 0; iy < img->get_height(); iy++ ){
		QRgb* row = (QRgb*)temp.scanLine( iy );
		for( unsigned ix = 0; ix < img->get_width(); ix++ ){
			color c = img->pixel( ix, iy );
			c.sRgb();
			
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
	delete img;
	
	return temp;
}


void MultiImage::calculate_size(){
	size_cache = QRect();
	
	for( unsigned i=0; i<pos.size(); i++ )
		size_cache = size_cache.united( QRect( pos[i], QSize( imgs[i]->get_width(), imgs[i]->get_height() ) ) );
}


double MultiImage::img_subv_diff( int x, int y, QImage &img1, QImage &img2 ){
	double pos_align = 0;
	double neg_align = 0;
	double pos_amount = 0;
	double neg_amount = 0;
	
	//Find the common area, and remove one-pixel around the edge on both
	QRect r1( QPoint(0,1), img1.size() - QSize( 0,2 ) );
	QRect r2( QPoint(x,y+1), img2.size() - QSize( 0,2 ) );
	QRect common = r1.intersected( r2 );
	
	int pos[50];
	for( int i=0; i<50; i++ )
		pos[i] = 0;
	
	for( int iy=common.y(); iy<common.height()+common.y(); iy++ ){
		const QRgb* fixed = (const QRgb*)img1.constScanLine( iy );
		const QRgb* row2 = (const QRgb*)img2.constScanLine( iy-y-1 ) - x;
		const QRgb* row1 = (const QRgb*)img2.constScanLine( iy-y ) - x;
		const QRgb* row0 = (const QRgb*)img2.constScanLine( iy-y+1 ) - x;
		
		for( int ix=common.x(); ix<common.width()+common.x(); ix++ ){
			QRgb f = fixed[ix];
			QRgb r0 = row0[ix];
			QRgb r1 = row1[ix];
			QRgb r2 = row2[ix];
			if( qAlpha( f ) > 127 && qAlpha( r1 ) > 127 ){
				//Find positive align
				int y = qRed( f ) + qGreen( f ) + qBlue( f );
				int a = qRed( r0 ) + qGreen( r0 ) + qBlue( r0 );
				int b = qRed( r1 ) + qGreen( r1 ) + qBlue( r1 );
				int c = qRed( r2 ) + qGreen( r2 ) + qBlue( r2 );
			int diff = abs( y-b );
				double pos_g = abs(c-b)>4 ? (double)( y-b ) / (double)( c-b ) : -1;
				double neg_g = abs(a-b)>4 ? (double)( y-b ) / (double)( a-b ) : 1;
				
				if( pos_g > 0 && pos_g < 1 ){
					pos_align += pos_g * diff * diff;
					pos_amount += diff * diff;
					pos[ (int)std::floor( pos_g * 50 ) ]++;
				}
				if( neg_g < 0 && neg_g > -1 ){
					neg_align += neg_g * diff * diff;
					neg_amount += diff * diff;
				}
			}
		}
	}
	
	qDebug( "Pos: %.2f / %.2f = %.4f", pos_align, pos_amount, pos_align / pos_amount );
	qDebug( "Neg: %.2f / %.2f = %.4f", neg_align, neg_amount, neg_align / neg_amount );
	for(int i=0; i<50; i++ )
		qDebug( "pos[%.2f]: %d", i / 50.0, pos[i] );
	
	return pos_amount > neg_amount ? pos_align : neg_align;
}


QImage MultiImage::img_subv( double v_diff, QImage &img ){
	QImage img_new( img.width(), img.height()-2, QImage::Format_ARGB32 );
	
	for( int iy=0; iy<img_new.height(); iy++ ){
		const QRgb* row2 = (const QRgb*)img.constScanLine( iy );
		const QRgb* row1 = (const QRgb*)img.constScanLine( iy+1 );
		const QRgb* row0 = (const QRgb*)img.constScanLine( iy+2 );
		QRgb* row_new = (QRgb*)img_new.scanLine( iy );
		
		for( int ix=0; ix<img_new.width(); ix++ ){
			QRgb r0 = row0[ix];
			QRgb r1 = row1[ix];
			QRgb r2 = row2[ix];
			if( v_diff > 0 ){
				int r = ( qRed(r2) - qRed(r1) )*v_diff + qRed(r1);
				int g = ( qGreen(r2) - qGreen(r1) )*v_diff + qGreen(r1);
				int b = ( qBlue(r2) - qBlue(r1) )*v_diff + qBlue(r1);
				int a = ( qAlpha(r2) - qAlpha(r1) )*v_diff + qAlpha(r1);
				
				row_new[ix] = qRgba( r, g, b, a );
			}
			else{
				int r = ( qRed(r0) - qRed(r1) )*v_diff + qRed(r1);
				int g = ( qGreen(r0) - qGreen(r1) )*v_diff + qGreen(r1);
				int b = ( qBlue(r0) - qBlue(r1) )*v_diff + qBlue(r1);
				int a = ( qAlpha(r0) - qAlpha(r1) )*v_diff + qAlpha(r1);
				
				row_new[ix] = qRgba( r, g, b, a );
			}
		}
	}
	
	return img_new;
}




