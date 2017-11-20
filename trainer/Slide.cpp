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

#include "Slide.hpp"
#include "ConfusionMatrix.hpp"

#include <memory>
#include <QObject>
#include <QImage>

#include <comparators/GradientComparator.hpp>
#include <planes/ImageEx.hpp>
#include <planes/DistanceMatrix.hpp>
#include <utils/ImageLoader.hpp>
#include <debug.hpp>

#include <pugixml.hpp>
using namespace pugi;
using namespace Overmix;


ConfusionMatrix ImageInfo::interlazeTest( QString prev ) {
	auto img      = ImageEx::fromFile( filename );
	auto img_prev = ImageEx::fromFile( prev );
	if( !img.is_valid() )
		return {};
	interlaze_predicted = img.is_interlaced( img_prev );
	auto bool_str = [](bool val){ return val ? "true" : "false"; };
	qDebug( "made prediction %s, should be %s", bool_str(interlaze_predicted), bool_str(interlazed) );
	return { interlazed, interlaze_predicted };
}


const auto NODE_ROOT           = "slide";
const auto NODE_IMAGE          = "image";
const auto NODE_FILENAME       = "path";
const auto NODE_INTERLAZED     = "interlazed";

static QString getString( const xml_node& node, const char* name )
	{ return QString::fromUtf8( node.child( name ).text().get() ); }

static std::unique_ptr<wchar_t[]> getUnicodeFilepath( QString filename ){
	auto wpath = std::make_unique<wchar_t[]>( filename.size()+1 );
	auto end = filename.toWCharArray( wpath.get() );
	wpath.get()[end] = 0;
	return wpath;
}
static void addXmlItem( xml_node& node, const char* name, const char* value )
	{ node.append_child( name ).append_child( node_pcdata ).set_value( value ); }
static void addXmlItem( xml_node& node, const char* name, QString value )
	{ addXmlItem( node, name, value.toUtf8().constData() ); }

QString Slide::saveXml( QString filename ){
	xml_document doc;
	
	auto param = doc.append_child( node_declaration );
	param.append_attribute( "version"  ) = 1.0;
	param.append_attribute( "encoding" ) = "UTF-8";
	
	auto root = doc.append_child( NODE_ROOT );
	
	for( auto img : images ){
		auto node = root.append_child( NODE_IMAGE );
		addXmlItem( node, NODE_FILENAME, img.filename );
		addXmlItem( node, NODE_INTERLAZED, img.interlazed ? QString("t") : QString("f") );
	}
	
	return doc.save_file( getUnicodeFilepath(filename).get() ) ? QString() : QObject::tr("Could not save XML");
}

QString Slide::loadXml( QString filename ){
	xml_document doc;
	
	if( !doc.load_file( getUnicodeFilepath(filename).get() ) )
		return QObject::tr( "The file did not contain valid XML" );
	auto root = doc.child( NODE_ROOT );
	
	
	for( auto image : root.children( NODE_IMAGE ) ){
		auto filename  = getString( image, NODE_FILENAME   );
		auto interlazed = getString( image, NODE_INTERLAZED );
		add( filename, (interlazed == "t") );
	}
	
	return {};
}

ConfusionMatrix Slide::evaluateInterlaze(){
	ConfusionMatrix matrix;
	
	for( unsigned i=0; i<images.size(); i++ ){
		auto prev_file = (i > 0) ? images[i-1].filename : QString();
		matrix += images[i].interlazeTest( prev_file );
	}
	
	return matrix;
}

void Slide::createErrorMatrix( QString filepath ) const{
	QStringList names;
	for( auto& image : images )
		names << image.filename;
	auto images = ImageLoader::loadImages( names );
	
	DistanceMatrix matrix( images.size() );
	debug::CsvFile output( (filepath + ".csv").toLocal8Bit().constData() );
	
	for( unsigned i=0; i<images.size(); i++ ){
		auto row = matrix.matrix.scan_line( i );
		for( unsigned j=0; j<images.size(); j++ ){
			GradientComparator gradient; //TODO: configure
			row[j] = gradient.findOffset( images[i][0], images[j][0], images[i].alpha_plane(), images[j].alpha_plane() );
			output.add( row[j].error );
		}
		output.stop();
	}
	
	matrix.toQImage().save( filepath + ".png" );
}

