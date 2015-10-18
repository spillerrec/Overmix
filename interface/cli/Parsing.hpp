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

#ifndef PARSING_HPP
#define PARSING_HPP

#include <QString>

namespace Overmix{


struct Splitter{
	QString left;
	QString right;
        
        template<typename T>
	Splitter( QString str, T split ){
		auto pos = str.indexOf( split );
		left  = str.left( pos );
		right = ( pos >= 0 ) ? str.right( str.length() - (pos+1) ) : "";
	}
};

inline int asInt( QString encoded ){
	bool result;
	auto integer = encoded.toInt( &result );
	//if( !result )
		//TODO:
	return integer;
}

inline double asDouble( QString encoded ){
	bool result;
	auto integer = encoded.toDouble( &result );
	//if( !result )
		//TODO:
	return integer;
}

}

#endif