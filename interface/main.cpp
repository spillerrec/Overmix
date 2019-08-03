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
#include "gui/mainwindow.hpp"

#include <QApplication>
#include <QMessageBox>
#include <QStringList>

#include <iostream>

#ifdef _WIN32
#include "Windows.h"
#endif

int main( int argc, char *argv[] ){
#ifdef _WIN32
	//Enable console output on Windows if console is active. Does not work with pipe redirection
	//From: https://stackoverflow.com/a/41701133/2248153
	if( AttachConsole(ATTACH_PARENT_PROCESS) ){
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
	}
#endif
	
	QApplication a( argc, argv );
	Overmix::ImageContainer images;
	Overmix::CommandParser parser( images );
	
	try{
		//Parse command-line arguments
		auto args = a.arguments();
		args.removeFirst();
		parser.parse( args );
		
		//Do not run GUI if the user is not interested in doing so
		if( !parser.useGui() )
			return 0;
		
		//Start GUI
		Overmix::main_widget w( images );
		//w.show();
		return a.exec();
	}
	catch( std::exception& e ){
		if( parser.useGui() ){
			QMessageBox::critical( nullptr, "An uncaught error occurred", e.what() );
			return -1;
		}
		else{
			std::cout << "Some error occurred:" << std::endl;
			std::cout << e.what();
			return -1;
		}
	}
}
