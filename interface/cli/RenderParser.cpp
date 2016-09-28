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


#include "Parsers.hpp"
#include "Parsing.hpp"

#include "renders/AverageRender.hpp"
#include "renders/StatisticsRender.hpp"
#include "planes/ImageEx.hpp"

#include <QString>
#include <QTextStream>

using namespace Overmix;

static void convert( QString str, Statistics& func ){
	func = getEnum<Statistics>( str,
		{	{ "avg",        Statistics::AVG        }
		,	{ "min",        Statistics::MIN        }
		,	{ "max",        Statistics::MAX        }
		,	{ "median",     Statistics::MEDIAN     }
		,	{ "difference", Statistics::DIFFERENCE }
		} );
}

ImageEx Overmix::renderParser( QString parameters, const AContainer& container ){
	Splitter split( parameters, ':' );
	if( split.left == "average" )
		return convertUnique<AverageRender, bool, bool>( split.right )->render( container );
	else if( split.left == "statistics" )
		return convertUnique<StatisticsRender, Statistics>( split.right )->render( container );
	
	throw std::invalid_argument( fromQString( "No render found with the name: '" + split.left + "'" ) );
}

void Overmix::renderHelpText( QTextStream& std ){
	std << "Renders an image and puts it on the rendered images stack\n";
	std << "\n";
	std << "Available renders:\n";
	std << "\taverage:<upscale_chroma>:<for_merging>\n";
	std << "\t\tCombines each pixel using averaging, both parameters are boolean\n";
	std << "\n";
	std << "\tstatistics:<method>\n";
	std << "\t\tCombines each pixel using one of the following statistics functions:\n";
	std << "\t\tMethods: avg, min, max, median, difference\n";
	std << "\n";
}
