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

#include "comparators/BruteForceComparator.hpp"
#include "comparators/GradientComparator.hpp"
#include "comparators/LogPolarComparator.hpp"
#include "containers/AContainer.hpp"
#include "containers/ImageContainer.hpp"

#include <QString>
#include <iostream>
#include <QTextStream>

using namespace Overmix;


static void convert( QString str, Difference::SimpleSettings& func ){
	func = convertConstruct<Difference::SimpleSettings,unsigned,bool,color_type>( str, '/' );
}

static void convert( QString str, AlignMethod& func ){
	func = getEnum<AlignMethod>( "direction", str,
		{	{ "both", AlignMethod::BOTH }
		,	{ "ver",  AlignMethod::VER  }
		,	{ "hor",  AlignMethod::HOR  }
		} );
}

static std::unique_ptr<AComparator> makeComparator( QString name, QString parameters ){
	if( name == "Gradient" )
		return convertUnique<GradientComparator,Difference::SimpleSettings,AlignMethod,double,int,int,color_type>( parameters );
	if( name == "BruteForce" )
		return convertUnique<BruteForceComparator,Difference::SimpleSettings,AlignMethod,double>( parameters );
	if( name == "LogPolar" )
		return std::make_unique<LogPolarComparator>();
	else
		throw std::invalid_argument( fromQString( "No comparator found with the name: '" + name + "'" ) );
}

void Overmix::comparatorParser( QString parameters, ImageContainer& container ){
	Splitter split( parameters, ':' );
	container.setComparator( makeComparator( split.left, split.right ) );
}

void Overmix::comparatorHelpText( QTextStream& std ){
	std << "Specifies how images should be compared for aligning:\n";
	std << "\n";
	std << "Algorithms:\n";
	std << "\t" << "Gradient:<stride>/<use L2>/<min difference>:<AlignMethod>:<movement>:<start level>:<max level>:<max difference>\n";
	std << "\t" << "BruteForce:<stride>/<use L2>/<min difference>:<AlignMethod>:<movement>\n";
	std << "\t" << "LogPolar\n";
	std << "\n";
	std << "AlignMethod: both, ver, hor\n";
	std << "\n";
	std << "See the wiki for details on the algorithms\n";
}

