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

#include "Geometry.hpp"

#include <QString>

#include <stdexcept>

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

inline std::string fromQString( QString str )
	{ return std::string( str.toLocal8Bit().constData() ); }

inline int requireBound( int value, int from, int to ){
	if( value < from || value >= to )
		throw std::invalid_argument( "Value: " + std::to_string(value)
			+	" should be at least " + std::to_string(from)
			+	" and below " + std::to_string(to)
			);
	return value;
}

inline int asInt( QString encoded ){
	bool result;
	auto integer = encoded.toInt( &result );
	if( !result )
		throw std::invalid_argument( fromQString("Could not parse '" + encoded + "' as an integer") );
	return integer;
}

inline double asDouble( QString encoded ){
	bool result;
	auto integer = encoded.toDouble( &result );
	if( !result )
		throw std::invalid_argument( fromQString("Could not parse '" + encoded + "' as a real number") );
	return integer;
}

template<typename T>
T getEnum( QString str, std::vector<std::pair<const char*, T>> cases ){
	auto pos = std::find_if( cases.begin(), cases.end(), [&]( auto pair ){ return pair.first == str; } );
	if( pos != cases.end() )
		return pos->second;
	throw std::invalid_argument( fromQString("Unknown enum value: '" + str + "'") );
}

inline void convert( QString str, double& val ) { val = asDouble(str); }
inline void convert( QString str, int& val ) { val = asInt(str); }
inline void convert( QString str_in, QString& str_out ) { str_out = str_in; }

inline void convert( QString str, bool& value ){
	value = getEnum<bool>( str.toLower(),
		{	{ "0",     false }
		,	{ "f",     false }
		,	{ "false", false }
		,	{ "1",     true  }
		,	{ "t",     true  }
		,	{ "true",  true  }
		} );
}

template<typename Arg, typename Arg2, typename... Args>
void convert( QString str, Arg& val, Arg2& val2, Args&... args ){
	Splitter split( str, ':' );
	convert( split.left, val );
	convert( split.right, val2, args... );
}

template<typename T>
void convert( QString str, Point<T>& val ){
	Splitter split( str, 'x' );
	convert( split.left,  val.x );
	convert( split.right, val.y );
}

}

#endif