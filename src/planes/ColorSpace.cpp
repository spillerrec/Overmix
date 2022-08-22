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

#include "ColorSpace.hpp"

#include "../color.hpp"

#include <stdexcept>
#include <QDebug>

using namespace Overmix;

color ColorSpace::convert( color from, ColorSpace to ) const{
	if( _transform == to._transform && _transfer == to._transfer )
		return from;
	
	if( _transform != to._transform ){
		auto gammaFrom = _transfer != Transfer::SRGB; //TODO: handle this properly
		switch( _transform ){
			case Transform::GRAY: break;
			case Transform::RGB: break;
			case Transform::YCbCr_601: from = from.rec601ToRgb( gammaFrom ); break;
			case Transform::YCbCr_709: from = from.rec709ToRgb( gammaFrom ); break;
			case Transform::JPEG:      from = from.jpegToRgb(); break;
			default:
				qWarning() << "Unsupported transform: " << (int)_transform;
				throw std::runtime_error( "ColorSpace::convert(): unsupported transform!" );
		}
		
		if( _transfer != to._transfer ){
			//TODO: implement
		}
		
		auto gammaTo = to._transfer == Transfer::REC709; //TODO: handle this properly
		switch( to._transform ){
			case Transform::GRAY: break;
			case Transform::RGB:
				if( _transfer == Transfer::LINEAR && to._transfer == Transfer::SRGB )
				{
					auto convert = [](color_type x){ return color::fromDouble( color::linear2sRgb( color::asDouble(x) ) ); };
					from.r = convert(from.r);
					from.g = convert(from.g);
					from.b = convert(from.b);
					from.a = convert(from.a);
				}
				break;
			case Transform::YCbCr_601: from = from.rgbToYcbcr( 0.299,  0.587,  0.114,  gammaTo, true  ); break;
			case Transform::YCbCr_709: from = from.rgbToYcbcr( 0.2126, 0.7152, 0.0722, gammaTo, true  ); break;
			case Transform::JPEG:      from = from.rgbToYcbcr( 0.299,  0.587,  0.114,  gammaTo, false ); break;
			default:
				qWarning() << "Unsupported transform: " << (int)to._transform;
				throw std::runtime_error( "ColorSpace::convert(): unsupported transform!" );
		}
	}
	else
	{
		double r = color::asDouble(from.r);
		double g = color::asDouble(from.g);
		double b = color::asDouble(from.b);
		double a = color::asDouble(from.a);
		
		auto apply = [&](auto func){
				r = func( r );
				g = func( g );
				b = func( b );
				a = func( a );
		};
		switch( _transfer ){
			case Transfer::LINEAR: break;
			case Transfer::SRGB:   apply(color::sRgb2linear); break;
			case Transfer::REC709: apply(color::ycbcr2linear); break;
			default:
				qWarning() << "Unsupported transfer: " << (int)to._transfer;
				throw std::runtime_error( "ColorSpace::convert(): unsupported transfer!" );
		}
		switch( to._transfer ){
			case Transfer::LINEAR: break;
			case Transfer::SRGB:   apply(color::linear2sRgb); break;
			//case Transfer::REC709: apply(color::linear2ycbcr); break; //TODO: Implement
			default:
				qWarning() << "Unsupported transfer: " << (int)to._transfer;
				throw std::runtime_error( "ColorSpace::convert(): unsupported transfer!" );
		}
		
		from.r = color::fromDouble( r );
		from.g = color::fromDouble( g );
		from.b = color::fromDouble( b );
		from.a = color::fromDouble( a );
	}
	
	return from;
}
