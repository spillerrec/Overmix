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


#include "CommandParser.hpp"
#include "Parsing.hpp"
#include "Parsers.hpp"
#include "Processor.hpp"

#include "containers/ImageContainer.hpp"
#include "containers/ImageContainerSaver.hpp"

#include <QFileInfo>
#include <QImage>
#include <QStringList>
#include <QDebug>

#include <vector>

using namespace Overmix;

static void printHelp(){
	QTextStream std( stdout );
	
	std << "OvermixCli [files]... [--command=arguments]...\n\n";
	
	std << "Available commands:\n";
	std << "\t" << "--pre-process\n";
	std << "\t" << "--align\n";
	std << "\t" << "--render\n";
	std << "\t" << "--post-process\n";
	std << "\t" << "--save\n";
	std << "\t" << "--no-gui\n";
	std << "\t" << "--help\n";
}


struct Command{
	bool is_file;
	Splitter parts;
	
	Command( QString cmd )
		:	is_file( !cmd.startsWith( "--" ) )
		,	parts( cmd.right( is_file ? cmd.length() : cmd.length()-2 ), '=' )
		{ }
	
	template<typename T>
	bool is( T name ) const { return parts.left == name; }
	QString filename() const { return parts.left; }
	QString arguments() const { return parts.right; }
};


void CommandParser::parse( QStringList commands ){
	std::vector<ImageEx> renders;
	
	for( Command cmd : commands ){
		if( cmd.is_file ){ //Load a file
			if( QFileInfo( cmd.filename() ).completeSuffix() == "xml.overmix" )
				ImageContainerSaver::load( images, cmd.filename() );
			else
				images.addImage( ImageEx::fromFile( cmd.filename() ), -1, -1, cmd.filename() );
		}
		else if( cmd.is( "no-gui" ) ) //Prevent GUI from starting
			use_gui = false;
		else if( cmd.is( "pre-process" ) ){
			auto processor = processingParser( cmd.arguments() );
			for( auto& group : images )
				for( auto& item : group )
					processor->process( item.imageRef() );
		}
		else if( cmd.is( "post-process" ) ){
			int id;
			QString arguments;
			convert( cmd.arguments(), id, arguments );
			
			processingParser( arguments )->process( renders[requireBound( id, 0, renders.size() )] );
		}
		else if( cmd.is( "align" ) ){
			alignerParser( cmd.arguments(), images );
		}
		else if( cmd.is( "render" ) ){
			renders.push_back( renderParser( cmd.arguments(), images ) );
		}
		else if( cmd.is( "save" ) ){
			Splitter args( cmd.arguments(), ':' );
			auto id = requireBound( asInt( args.left ), 0, renders.size() );
			renders[id].to_qimage().save( args.right );
		}
		else if( cmd.is( "help" ) )
			printHelp();
		else{
			qWarning() << "Unknown command:" << cmd.parts.left;
		}
	}
	
}
