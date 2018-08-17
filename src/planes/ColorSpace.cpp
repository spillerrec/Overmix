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
		switch( _transform ){
			case Transform::GRAY: break;
			case Transform::RGB: break;
			case Transform::YCbCr_601: from = from.rec601ToRgb(); break;
			case Transform::YCbCr_709: from = from.rec709ToRgb(); break;
			case Transform::JPEG:      from = from.jpegToRgb(); break;
			default:
				qWarning() << "Unsupported transform: " << (int)_transform;
				throw std::runtime_error( "ColorSpace::convert(): unsupported transform!" );
		}
		
		if( _transfer != to._transfer ){
			//TODO: implement
		}
		
		auto gamma = true; //TODO: set to false
		switch( to._transform ){
			case Transform::GRAY: break;
			case Transform::RGB: break;
			case Transform::YCbCr_601: from = from.rgbToYcbcr( 0.299,  0.587,  0.114,  gamma, true  ); break;
			case Transform::YCbCr_709: from = from.rgbToYcbcr( 0.2126, 0.7152, 0.0722, gamma, true  ); break;
			case Transform::JPEG:      from = from.rgbToYcbcr( 0.299,  0.587,  0.114,  gamma, false ); break;
			default:
				qWarning() << "Unsupported transform: " << (int)to._transform;
				throw std::runtime_error( "ColorSpace::convert(): unsupported transform!" );
		}
	}
	
	return from;
}
