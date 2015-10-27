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
	if( split.left == "average" ){
		bool upscale_chroma;
		bool for_merging;
		convert( split.right, upscale_chroma, for_merging );
		return AverageRender( upscale_chroma, for_merging ).render( container );
	}
	else if( split.left == "statistics" ){
		Statistics stats;
		convert( split.right, stats );
		return StatisticsRender(stats).render( container );
	}
	
	throw std::invalid_argument( fromQString( "No render found with the name: '" + split.left + "'" ) );
}
