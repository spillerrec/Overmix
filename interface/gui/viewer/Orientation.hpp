/*
	This file is part of imgviewer.

	imgviewer is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	imgviewer is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with imgviewer.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ORIENTATION_HPP
#define ORIENTATION_HPP

#include <QSize>

struct Orientation{
	int8_t rotation{ 0 };
	bool flip_ver{ false };
	bool flip_hor{ false };
	
	Orientation() {}
	Orientation( int8_t rotation, bool flip_ver, bool flip_hor )
		:	rotation(rotation), flip_ver(flip_ver), flip_hor(flip_hor) {}
	
	/** @return The rotation in the range 0-3 */
	int8_t normalizedRotation( int8_t rotation ) const{
		auto normalized = rotation % 4;
		if( normalized < 0 )
			normalized += 4;
		return normalized;
	}
	
	Orientation rotate180() const
		{ return { rotation, !flip_ver, !flip_hor }; }
	Orientation rotateRight() const
		{ return { int8_t(rotation+1), flip_hor, flip_ver }; }
	
	Orientation rotate( int8_t amount ) const{
		switch( normalizedRotation( amount ) ){
			case 1: return rotateRight();
			case 2: return rotate180();
			case 3: return rotate180().rotateRight();
			default: return *this;
		}
	}
	
	QSize finalSize( QSize before ) const{
		if( rotation%2 == 0 )
			return before;
		else
			return QSize( before.height(), before.width() );
	}
	
	Orientation mirror( bool hor, bool ver )
		{ return { rotation, ver?!flip_ver:flip_ver, hor?!flip_hor:flip_hor };	}
	
	Orientation normalized() const{
		auto rot = normalizedRotation( rotation );
		if( rot > 1 ) //Rotate 180
			return { int8_t(rot-2), !flip_ver, !flip_hor };
		else
			return { rot, flip_ver, flip_hor };
	}
	
	Orientation add( Orientation other ) const{
		return { int8_t(rotation + other.rotation)
			,	other.flip_ver ? !flip_ver : flip_ver
			,	other.flip_hor ? !flip_hor : flip_hor
			};
	}
	
	Orientation difference( Orientation other ) const{
		auto a = this->normalized();
		auto b = other.normalized();
		
		return { int8_t(b.rotation-a.rotation)
			,	a.flip_ver != b.flip_ver
			,	a.flip_hor != b.flip_hor
			};
	}
};

#endif

