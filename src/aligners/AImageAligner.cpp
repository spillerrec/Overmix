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

#include "AImageAligner.hpp"
#include "../color.hpp"

#include <fstream>

AImageAligner::~AImageAligner(){
	
}

QPointF AImageAligner::min_point() const{
	if( count() == 0 )
		return QPointF(0,0);
	
	QPointF min = pos( 0 );
	for( unsigned i=0; i<count(); i++ ){
		if( pos(i).x() < min.x() )
			min.setX( pos(i).x() );
		if( pos(i).y() < min.y() )
			min.setY( pos(i).y() );
	}
	
	return min;
}

double AImageAligner::x_scale() const{
	return (method != ALIGN_VER) ? scale : 1.0;
}
double AImageAligner::y_scale() const{
	return (method != ALIGN_HOR) ? scale : 1.0;
}

static Plane scalePlane( const Plane& p, double x_scale, double y_scale ){
	return p.scale_cubic(
			p.get_width() * x_scale + 0.5
		,	p.get_height() * y_scale + 0.5
		);
}

Plane AImageAligner::prepare_plane( const Plane& p ){
	if( use_edges ){
		Plane edges = p.edge_sobel();
		return ( scale != 1.0 ) ? scalePlane( edges, x_scale(), y_scale() ) : edges;
	}
	else if( scale != 1.0 )
			return scalePlane( p, x_scale(), y_scale() );
	else
		return Plane();
}

void AImageAligner::add_image( const ImageEx& img ){
	images.emplace_back( img, prepare_plane( img[0] ) );
	on_add();
}

QRect AImageAligner::size() const{
	QRectF total;
	
	for( unsigned i=0; i<count(); i++ )
		total = total.united( QRectF( pos(i), QSizeF( plane(i).get_width(), plane(i).get_height() ) ) );
	
	//Round so that we only increase size
	total.setLeft( floor( total.left() ) );
	total.setTop( floor( total.top() ) );
	total.setRight( ceil( total.right() ) );
	total.setBottom( ceil( total.bottom() ) );
	//TODO: just return a QRectF and let the caller deal with the rounding
	
	return total.toRect();
}

double AImageAligner::calculate_overlap( QPoint offset, const Plane& img1, const Plane& img2 ){
	QRect first( 0,0, img1.get_width(), img1.get_height() );
	QRect second( offset, QSize(img2.get_width(), img2.get_height()) );
	QRect common = first.intersected( second );
	
	double area = first.width() * first.height();
	return (double)common.width() * common.height() / area;
}

AImageAligner::ImageOffset AImageAligner::find_offset( const Plane& img1, const Plane& img2 ) const{
	//Keep repeating with higher levels until it drops
	//below threshold
	int level = 1; //TODO: magic number
	std::pair<QPoint,double> result;
	DiffCache cache;
	
	//Restrict movement
	double movement_x = movement, movement_y = movement;
	switch( method ){
		case ALIGN_HOR:	movement_y = 0; break;
		case ALIGN_VER:	movement_x = 0; break;
		default: break;
	}
	do{
		result = img1.best_round_sub( img2
			,	level
			,	((int)1 - (int)img2.get_width()) * movement_x, ((int)img1.get_width() - 1) * movement_x
			,	((int)1 - (int)img2.get_height()) * movement_y, ((int)img1.get_height() - 1) * movement_y
			,	&cache
			);
	}while( result.second > 0.10*color::WHITE && level++ < 6 ); //TODO: magic number!
	
	ImageOffset offset;
	offset.distance_x = result.first.x();
	offset.distance_y = result.first.y();
	offset.error = result.second;
	offset.overlap = calculate_overlap( result.first, img1, img2 );
	
	return offset;
}

const Plane& AImageAligner::plane( unsigned img_index, unsigned p_index ) const{
	if( raw && images[img_index].image )
		return images[img_index].image;
	else
		return images[img_index].original[p_index];
}

QPointF AImageAligner::pos( unsigned index ) const{
	if( raw )
		return images[index].pos;
	else
		return QPointF( images[index].pos.x() / x_scale(), images[index].pos.y() / y_scale() );
}

void AImageAligner::debug( QString csv_file ) const{
	//Open output file
	const char* path = csv_file.toLocal8Bit().constData();
	std::fstream rel( path, std::fstream::out );
	if( !rel.is_open() ){
		qWarning( "AImageAligner::debug( \"%s\" ), couldn't open file", path );
		return;
	}
	
	//Write header
	rel << "\"Scale: " << scale << "\", ";
	rel << "\"x\", ";
	rel << "\"y\", ";
	rel << "\"dx\", ";
	rel << "\"dy\"\n";
	
	//Write alignment info
	for( unsigned i=0; i<count(); i++ ){
		rel << i+1 << ", ";
		rel << pos(i).x() << ", ";
		rel << pos(i).y() << ", ";
		
		if( i > 0 ){
			QPointF diff = pos(i) - pos(i-1);
			rel << diff.x() << ", ";
			rel << diff.y();
		}
		rel << "\n";
	}
	
	rel.close();
}

