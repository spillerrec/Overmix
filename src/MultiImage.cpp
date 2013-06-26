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


#include "MultiImage.hpp"
#include "ImageEx.hpp"

#include <cmath>
#include <QTime>
#include "MultiPlaneIterator.hpp"

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
	QTime t;
	t.start();
	ImageEx *img = new ImageEx();
	img->read_file( path.toLocal8Bit().constData() );
	QPoint p( 0,0 );
	qDebug( "image loaded: %d", t.elapsed() );
	
	if( imgs.size() > 0 ){
		ImageEx *img1 = NULL;
		bool destroy = true;
		if( use_average )
			img1 = render_image( FILTER_FOR_MERGING );
		else{
			img1 = imgs[imgs.size()-1];
			p = pos[imgs.size()-1];
			if( p.x() < 0 )
				p.setX( 0 );
			if( p.y() < 0 )
				p.setY( 0 );
			destroy = false;
		}
		
		qDebug( "image prepared: %d", t.elapsed() );
		
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
						result = img1->best_round( *img, level, movement, movement );
					break;
			}
		}while( result.second > 24*256 && level++ < 6 );
		
		p += get_size().topLeft() + result.first;
		if( destroy )
			delete img1;
		qDebug( "image compared: %d", t.elapsed() );
	}
	
	//Add image
	imgs.push_back( img );
	pos.push_back( p );
	calculate_size();
}

ImageEx* MultiImage::render_image( filters filter ) const{
	QTime t;
	t.start();
	
	qDebug( "render_image: image count: %d", imgs.size() );
	
	//TODO: check for first image!
	
	//Do iterator
	QRect box = get_size();
	ImageEx *img = new ImageEx( imgs[0]->get_system() );
	if( !img )
		return NULL;
	if( !img->create( box.width(), box.height() ) ){
		delete img;
		return NULL;
	}
	
	//TODO: determine amount of planes!
	unsigned planes_amount = 3; //TODO: alpha?
	if( filter == FILTER_FOR_MERGING && imgs[0]->get_system() == ImageEx::YUV )
		planes_amount = 1;
	
	ImageEx &first( *imgs[0] );
	int width = first[0].p.get_width();
	int height = first[0].p.get_height();
	
	for( unsigned i=0; i<planes_amount; i++ ){
		std::vector<PlaneItInfo> info;
		info.push_back( PlaneItInfo( &(*img)[i].p, box.x(),box.y() ) );
		
		int local_width = first[i].p.get_width();
		int local_height = first[i].p.get_height();
		std::vector<Plane*> temp;
		
		
		if( local_width == width && local_height == height ){
			for( unsigned j=0; j<imgs.size(); j++ )
				info.push_back( PlaneItInfo( &(*imgs[j])[i].p, pos[j].x(),pos[j].y() ) );
		}
		else{
			temp.reserve( imgs.size() );
			for( unsigned j=0; j<imgs.size(); j++ ){
				Plane *p = (*imgs[j])[i].p.scale_nearest( width, height, 0, 0 );
				if( !p )
					qDebug( "No plane :\\" );
				temp.push_back( p );
				info.push_back( PlaneItInfo( p, pos[j].x(),pos[j].y() ) );
			}
		}
		
		MultiPlaneIterator it( info );
		it.iterate_all();
		
		for( ; it.valid(); it.next() )
			it.write_average();
		
		//Remove scaled planes
		for( unsigned j=0; j<temp.size(); j++ )
			delete temp[j];
	}
	
	/* 
	for( int i=1; i<=2; i++ ){
		//TODO: add draw plane
		info.push_back( PlaneItInfo( &(*img)[i].p, box.x()/2,box.y()/2 ) );
		for( unsigned i=0; i<imgs.size(); i++ )
			info.push_back( PlaneItInfo( &(*imgs[i])[i].p, pos[i].x()/2,pos[i].y()/2 ) );
		
		MultiPlaneIterator it( info );
		it.iterate_all();
		
		for( ; it.valid(); it.next() )
			it.write_average();
	} */
	
	
	/* 
	MultiImageIterator it( imgs, pos, box.x(),box.y() );
	it.set_threshould( threshould );
	//Pointer to the function we want to use
	color (MultiImageIterator::*function)();
	switch( filter ){
		case FILTER_DIFFERENCE:   function = &MultiImageIterator::difference; break;
		case FILTER_SIMPLE:       function = &MultiImageIterator::simple_filter; break;
		case FILTER_SIMPLE_SLIDE: function = &MultiImageIterator::simple_slide; break;
		default:
		case FILTER_AVERAGE:      function = &MultiImageIterator::average; break;
	}
	
	image *img = new image( box.width(), box.height() );
	qDebug( "render init took: %d", t.elapsed() );
	for( int iy = 0; iy < box.height(); iy++, it.next_line() ){
		color* row = img->scan_line( iy );
		for( int ix = 0; ix < box.width(); ++ix, it.next_x(), ++row )
			*row = (it.*function)();
	} */
	qDebug( "render rest took: %d", t.elapsed() );
	
	return img;
}

QImage MultiImage::render( filters filter, bool dither ) const{
	ImageEx *img_org = render_image( filter );
	QTime t;
	t.start();
	QImage img = img_org->to_qimage();
	delete img_org;
	/* 
	color *line = new color[ img->get_width()+1 ];
	
	QImage temp( img->get_width(), img->get_height(), QImage::Format_ARGB32 );
	temp.fill(0);
	for( unsigned iy = 0; iy < img->get_height(); iy++ ){
		QRgb* row = (QRgb*)temp.scanLine( iy );
		color* img_row = img->scan_line( iy );
		for( unsigned ix = 0; ix < img->get_width(); ix++, img_row++, row++ ){
			color c = *img_row;
		//	c.sRgb();
		//	c = c.rec709_to_rgb();
			
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
			*row = qRgba( rounded.r, rounded.g, rounded.b, rounded.a );
		}
	}
	delete line;
	delete img; */
	
	qDebug( "image load took: %d", t.elapsed() );
	
	return img;
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




