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


#include "Processor.hpp"
#include "Parsing.hpp"

#include "../planes/ImageEx.hpp"

#include <QString>
#include <QDebug>

#include <algorithm>
#include <stdexcept>
#include <vector>

using namespace Overmix;


Point<double> asPoint( Splitter split ){
	return { asDouble( split.left ), asDouble( split.right ) };
}

template<typename T>
T getEnum( QString str, std::vector<std::pair<const char*, T>> cases ){
	auto pos = std::find_if( cases.begin(), cases.end(), [&]( auto pair ){ return pair.first == str; } );
	if( pos != cases.end() )
		return pos->second;
	throw std::invalid_argument( "Unknown enum value" );
}

class ScaleProcessor : public Processor{
	private:
		ScalingFunction function{ ScalingFunction::SCALE_CATROM };
		Point<double> scale;
		
		auto getFunction( QString str ){
			return getEnum<ScalingFunction>( str, {
					{ "none",     ScalingFunction::SCALE_NEAREST   }
				,	{ "linear",   ScalingFunction::SCALE_LINEAR    }
				,	{ "mitchell", ScalingFunction::SCALE_MITCHELL  }
				,	{ "catrom",   ScalingFunction::SCALE_CATROM    }
				,	{ "spline",   ScalingFunction::SCALE_SPLINE    }
				,	{ "lanczos3", ScalingFunction::SCALE_LANCZOS_3 }
				,	{ "lanczos5", ScalingFunction::SCALE_LANCZOS_5 }
				,	{ "lanczos7", ScalingFunction::SCALE_LANCZOS_7 }
				} );
		}
		
	public:
		ScaleProcessor( QString str ){
			//method:XxY
			Splitter split( str, ':' );
			function = getFunction( split.left );
			scale = asPoint( { split.right, 'x' } );
		}
		
		ImageEx process( const ImageEx& img ) override{
			ImageEx copy( img );
			copy.scaleFactor( scale, function );
			return copy;
		}
};

std::unique_ptr<Processor> Overmix::processingParser( QString parameters ){
	Splitter split( parameters, '&' );
	if( split.left == "scale" )
		return std::make_unique<ScaleProcessor>( split.right );
	qDebug() << "No processor found!" << split.left;
	//TODO: Deconvolve
	//TODO: Blurring
	//TODO: edge detection
	//TODO: level
	//TODO: binarize
	
	return {};
}
