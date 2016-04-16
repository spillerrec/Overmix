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


double AImageAligner::calculate_overlap( Point<> offset, const Plane& img1, const Plane& img2 ){
	QRect first( 0,0, img1.get_width(), img1.get_height() );
	QRect second( { offset.x, offset.y }, QSize(img2.get_width(), img2.get_height()) );
	QRect common = first.intersected( second );
	
	double area = first.width() * first.height();
	return (double)common.width() * common.height() / area;
}

AImageAligner::ImageOffset AImageAligner::findOffset( Point<double> movement, const Plane& img1, const Plane& img2, const Plane& a1, const Plane& a2 ){
	//Keep repeating with higher levels until it drops
	//below threshold
	//TODO: magic settings which should be configurable
	int level = 1;
	int max_level = 6;
	bool fast_diffing = true;
	auto max_difference = 0.10*color::WHITE; //Difference must not be above this to match
	
	std::pair<Point<>,double> result;
	DiffCache cache;
	
	do{
		result = img1.best_round_sub( img2
			,	a1, a2, level
			,	((int)1 - (int)img2.get_width()) * movement.x, ((int)img1.get_width() - 1) * movement.x
			,	((int)1 - (int)img2.get_height()) * movement.y, ((int)img1.get_height() - 1) * movement.y
			,	&cache, fast_diffing
			);
	}while( result.second > max_difference && level++ < max_level );
	
	return { result.first, result.second, calculate_overlap( result.first, img1, img2 ) };
}

Point<double> AlignerProcessor::scale() const{
	switch( settings.method ){
		case AlignMethod::VER: return { 1.0, scale_amount };
		case AlignMethod::HOR: return { scale_amount, 1.0 };
		default: return { scale_amount, scale_amount };
	}
}

Point<double> AlignerProcessor::filter( Point<double> value ) const{
	switch( settings.method ){
		case AlignMethod::VER: return { 0.0, value.x };
		case AlignMethod::HOR: return { value.y, 0.0 };
		default: return value;
	}
}

ModifiedPlane AlignerProcessor::operator()( const Plane& p ) const{
	auto output = scalePlane( p );
	if( edges )
		output.modify( [&](const Plane& p){ return p.edge_sobel(); } );
	return output;
}

template<typename Func>
static ImageEx applyFunc( ImageEx img, Func f ){
	img[0] = f( img[0] );
	img.alpha_plane() = f( img.alpha_plane() );
	return img;
}

static Modified<ImageEx> getScaled2( const ImageEx& img, Size<unsigned> size ){
	if( img && img.getSize() != size )
		return { applyFunc( img, [=]( auto& p ){ return p.scale_cubic( size ); } ) };
	else
		return { img };
}

Modified<ImageEx> AlignerProcessor::image( const ImageEx& img ) const{
	auto output = getScaled2( img, img.getSize()*scale() );
	if( edges )
		output.modify( [=]( auto& img ){ return applyFunc( img, []( auto& p ){ return p.edge_sobel(); } ); } );
	return output;
}

