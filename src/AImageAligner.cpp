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

AImageAligner::~AImageAligner(){
	//Remove scaled images
	for( unsigned i=0; i<images.size(); ++i )
		if( (*(images[i].original))[0] != images[i].image )
			delete images[i].image;
}

double AImageAligner::x_scale() const{
	if( method == ALIGN_BOTH || method == ALIGN_HOR )
		return scale;
	else
		return 1.0;
}
double AImageAligner::y_scale() const{
	if( method == ALIGN_BOTH || method == ALIGN_VER )
		return scale;
	else
		return 1.0;
}

void AImageAligner::add_image( /*const*/ ImageEx* const img ){
	/*const*/ Plane* const use = (*img)[0];
	
	/*const*/ Plane* prepared = NULL;
	
	if( scale > 1.0 ){
		Plane *temp = use->scale_cubic(
				use->get_width() * x_scale() + 0.5
			,	use->get_height() * y_scale() + 0.5
			);
		
		prepared = prepare_plane( temp );
		
		if( temp != prepared )
			delete temp;
	}
	else
		prepared = prepare_plane( use );
	
	images.push_back( ImagePosition( img, prepared ) );
	
	on_add( images[ images.size()-1 ] );
}

QRect AImageAligner::size() const{
	QRectF total;
	
	for( unsigned i=0; i<count(); i++ )
		total = total.united( QRectF( pos(i), QSizeF( plane(i,0)->get_width(), plane(i,0)->get_height() ) ) );
	
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
	int level = 6;
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
	}while( result.second > 24*256 && level++ < 6 );
	
	ImageOffset offset;
	offset.distance_x = result.first.x();
	offset.distance_y = result.first.y();
	offset.error = result.second;
	offset.overlap = calculate_overlap( result.first, img1, img2 );
	
	return offset;
}

/*const*/ Plane* AImageAligner::plane( unsigned img_index, unsigned p_index ) const{
	if( raw )
		return images[img_index].image;
	else
		return (*(images[img_index].original))[p_index];
}

QPointF AImageAligner::pos( unsigned index ) const{
	if( raw )
		return images[index].pos;
	else
		return QPointF( images[index].pos.x() / x_scale(), images[index].pos.y() / y_scale() );
}

