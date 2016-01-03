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

#include "aligners/AverageAligner.hpp"
#include "planes/ImageEx.hpp"

#include <QString>

using namespace std;
using namespace Overmix;

static void convert( QString str, AlignMethod& func ){
	func = getEnum<AlignMethod>( str,
		{	{ "both", AlignMethod::ALIGN_BOTH }
		,	{ "ver",  AlignMethod::ALIGN_VER  }
		,	{ "hor",  AlignMethod::ALIGN_HOR  }
		} );
}


void Overmix::alignerParser( QString parameters, AContainer& container ){
	unique_ptr<AAligner> aligner;
	
	Splitter split( parameters, ':' );
	if( split.left == "average" ){
		AlignMethod method;
		double movement;
		double scale;
		convert( split.right, method, movement, scale );
		//TODO: parse parameters
		aligner = make_unique<AverageAligner>( AlignSettings{method, movement}, scale );
	}
	else
		throw std::invalid_argument( fromQString( "No aligner found with the name: '" + split.left + "'" ) );
	
	aligner->align( container );
}
