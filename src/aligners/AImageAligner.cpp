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
#include "../containers/ImageContainer.hpp"

#include <QRect>

#include <fstream>

using namespace Overmix;


AImageAligner::AImageAligner( AContainer& container, AlignMethod method, double scale )
	:	method(method), scale(scale), raw(false), container(container){
	
}

static Plane scalePlane( const Plane& p, Point<double> scale )
	{ return p.scale_cubic( (p.getSize() * scale).round() ); }

Plane AImageAligner::prepare_plane( const Plane& p ) const{
	if( use_edges ){
		Plane edges = p.edge_sobel();
		return ( scale != 1.0 ) ? scalePlane( edges, scales() ) : edges;
	}
	else if( scale != 1.0 )
			return scalePlane( p, scales() );
	else
		return Plane();
}

void AImageAligner::addImages(){
	//TODO: we can't do this in the constructor?
	images.clear();
	for( unsigned i=0; i<container.count(); i++ ){
		images.emplace_back( ImageEx( prepare_plane( image(i)[0] ) ) );
		on_add();
	}
}

double AImageAligner::calculate_overlap( Point<> offset, const Plane& img1, const Plane& img2 ){
	QRect first( 0,0, img1.get_width(), img1.get_height() );
	QRect second( { offset.x, offset.y }, QSize(img2.get_width(), img2.get_height()) );
	QRect common = first.intersected( second );
	
	double area = first.width() * first.height();
	return (double)common.width() * common.height() / area;
}

AImageAligner::ImageOffset AImageAligner::find_offset( const Plane& img1, const Plane& img2, const Plane& a1, const Plane& a2 ) const{
	//Keep repeating with higher levels until it drops
	//below threshold
	int level = 1; //TODO: magic number
	std::pair<Point<>,double> result;
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
			,	a1, a2, level
			,	((int)1 - (int)img2.get_width()) * movement_x, ((int)img1.get_width() - 1) * movement_x
			,	((int)1 - (int)img2.get_height()) * movement_y, ((int)img1.get_height() - 1) * movement_y
			,	&cache, fast_diffing
			);
	}while( result.second > 0.10*color::WHITE && level++ < 6 ); //TODO: magic number!
	
	return { result.first, result.second, calculate_overlap( result.first, img1, img2 ) };
}

const ImageEx& AImageAligner::image( unsigned index ) const{
	if( raw && images[index].is_valid() )
		return images[index];
	else
		return container.image( index );
}

ImageEx& AImageAligner::imageRef( unsigned index ){
	if( raw && images[index].is_valid() )
		return images[index];
	else
		return container.imageRef( index );
}

Point<double> AImageAligner::pos( unsigned index ) const{
	auto pos = container.pos( index );
	return raw ? pos * scales() : pos;
}
void AImageAligner::setPos( unsigned index, Point<double> newVal ){
	container.setPos( index, raw ? newVal / scales() : newVal );
}

