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

#include "cli/CommandParser.hpp"
#include "containers/ImageContainer.hpp"

#include <QCoreApplication>
#include <QStringList>
#include <QLoggingCategory>

int main( int argc, char *argv[] ){
	QCoreApplication a( argc, argv );
	Overmix::ImageContainer images;
	Overmix::CommandParser parser( images );
	
	//Parse command-line arguments
	auto args = a.arguments();
	args.removeFirst();
	parser.parse( args );
	
	return 0;
}
