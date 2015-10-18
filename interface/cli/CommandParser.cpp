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
#include "Processor.hpp"

#include "containers/ImageContainer.hpp"
#include "containers/ImageContainerSaver.hpp"

#include <QFileInfo>
#include <QImage>
#include <QStringList>

#include <vector>

using namespace Overmix;


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

#include "renders/AverageRender.hpp"
static ImageEx renderParser( QString parameters, const AContainer& container ){
	//TODO: parse parameters
	return AverageRender().render( container );
}

#include "aligners/AverageAligner.hpp"
static void alignerParser( QString parameters, AContainer& container ){
	//TODO: parse parameters
	AverageAligner aligner( container, AImageAligner::ALIGN_BOTH );
	aligner.addImages();
	aligner.align();
}

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
			//TODO:
			qWarning( "processing not yet implemented" );
		}
		else if( cmd.is( "post-process" ) ){
			auto processor = processingParser( cmd.arguments() );
			if( processor )
				for( auto& img : renders )
					processor->process( img );
			qWarning( "processing not yet implemented" );
		}
		else if( cmd.is( "align" ) ){
			alignerParser( cmd.arguments(), images );
		}
		else if( cmd.is( "render" ) ){
			renders.push_back( renderParser( cmd.arguments(), images ) );
		}
		else if( cmd.is( "save" ) ){
			Splitter args( cmd.arguments(), ':' );
			auto id = asInt( args.left ); //TODO: bounds-check
			renders[id].to_qimage().save( args.right );
		}
	}
	
}
