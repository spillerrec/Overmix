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

