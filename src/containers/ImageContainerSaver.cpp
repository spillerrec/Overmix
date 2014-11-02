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


#include "ImageContainerSaver.hpp"
#include "ImageContainer.hpp"

#include <QDir>
#include <QFileInfo>

#include "pugixml/pugixml.hpp"
using namespace pugi;

const auto NODE_ROOT           = "alignment";
const auto NODE_MASKS          = "masks";
const auto NODE_MASK           = "mask";
const auto NODE_GROUPS         = "groups";
const auto NODE_GROUP          = "group";
const auto ATTR_GROUP_NAME     = "name";
const auto NODE_ITEM           = "item";
const auto NODE_ITEM_PATH      = "filepath";
const auto NODE_ITEM_MASK      = "mask";
const auto NODE_ITEM_FRAME     = "frame";
const auto NODE_ITEM_OFFSET    = "offset";
const auto ATTR_ITEM_OFFSET_X  = "x";
const auto ATTR_ITEM_OFFSET_Y  = "y";

static QString getStringAttr( const xml_node& node, const char* name )
	{ return QString::fromUtf8( node.attribute( name ).value() ); }

static QString getString( const xml_node& node, const char* name )
	{ return QString::fromUtf8( node.child( name ).text().get() ); }

bool ImageContainerSaver::load( ImageContainer& container, QString filename ){
	//TODO: progress monitoring
	xml_document doc;
	auto folder = QFileInfo( filename ).dir();
	
	if( !doc.load_file( filename.toLocal8Bit().constData() ) ) //TODO: unicode
		return false;
	auto root = doc.child( NODE_ROOT );
	
	//Load masks
	std::vector<int> mask_ids;
	for( auto mask : root.child( NODE_MASKS ).children( NODE_MASK ) ){
		auto filename = folder.absolutePath() + "/" + QString::fromUtf8( mask.text().get() );
		ImageEx img;
		if( !img.read_file( filename ) )
			return false;
		
		mask_ids.emplace_back( container.addMask( std::move( img[0] ) ) );
	}
	
	for( auto group : root.child( NODE_GROUPS ).children( NODE_GROUP ) ){
		container.addGroup( getStringAttr( group, ATTR_GROUP_NAME ) );
		
		for( auto item : group.children( NODE_ITEM ) ){
			auto file = folder.absolutePath() + "/" + getString( item, NODE_ITEM_PATH );
			auto mask  = item.child( NODE_ITEM_MASK  ).text().as_int( -1 );
			auto frame = item.child( NODE_ITEM_FRAME ).text().as_int( -1 );
			
			//Translate mask id
			if( mask >= 0 ){
				if( unsigned(mask) >= mask_ids.size() )
					return false;
				mask = mask_ids[mask];
			}
			
			ImageEx img;
			if( !img.read_file( file ) )
				return false;
			auto& img_item = container.addImage( std::move(img), mask, -1, file );
			
			img_item.frame = frame;
			
			auto offset_node = item.child( NODE_ITEM_OFFSET );
			img_item.offset.x = offset_node.attribute( ATTR_ITEM_OFFSET_X ).as_double( 0.0 );
			img_item.offset.y = offset_node.attribute( ATTR_ITEM_OFFSET_Y ).as_double( 0.0 );
		}
	}
	
	return true;
}

//Convenience functions for adding xml elements containing only of a text node
static void addXmlItem( xml_node& node, const char* name, const char* value )
	{ node.append_child( name ).append_child( node_pcdata ).set_value( value ); }
static void addXmlItem( xml_node& node, const char* name, QString value )
	{ addXmlItem( node, name, value.toUtf8().constData() ); }

template<typename T>
void addXmlItem( xml_node& node, const char* name, T value )
	{ addXmlItem( node, name, std::to_string( value ).c_str() ); }

bool ImageContainerSaver::save( const ImageContainer& container, QString filename ){
	//NOTE: we do not support alpha planes stored directly in the ImageEx, it will be ignored!
	xml_document doc;
	auto folder = QFileInfo( filename ).dir();
	
	auto param = doc.append_child( node_declaration );
	param.append_attribute( "version"  ) = 1.0;
	param.append_attribute( "encoding" ) = "UTF-8";
	
	auto root = doc.append_child( NODE_ROOT );
	
	auto groups = root.append_child( NODE_GROUPS );
	for( auto& group : container ){
		auto group_node = groups.append_child( NODE_GROUP );
		group_node.append_attribute( ATTR_GROUP_NAME ) = group.name.toUtf8().constData();
		
		for( auto& item : group ){
			if( item.filename.isEmpty() ) //Abort if no filename
				return false;
			
			auto item_node = group_node.append_child( NODE_ITEM );
			addXmlItem( item_node, NODE_ITEM_PATH , folder.relativeFilePath( item.filename ) );
			addXmlItem( item_node, NODE_ITEM_MASK , item.maskId() );
			addXmlItem( item_node, NODE_ITEM_FRAME, item.frame );
			
			auto offset_node = item_node.append_child( NODE_ITEM_OFFSET );
			offset_node.append_attribute( ATTR_ITEM_OFFSET_X ) = item.offset.x;
			offset_node.append_attribute( ATTR_ITEM_OFFSET_Y ) = item.offset.y;
		}
	}
	
	if( container.getMasks().size() > 0 ){
		//Save masks
		auto masks = root.append_child( NODE_MASKS );
		auto mask_folder = QFileInfo( filename ).baseName() + "_masks";
		folder.mkdir( mask_folder );
		int id = 0;
		for( auto& mask : container.getMasks() ){
			auto mask_file = mask_folder + "/" + QString::number( id++ ) + ".dump";
			ImageEx( mask ).saveDump( folder.absolutePath() + "/" + mask_file );
			addXmlItem( masks, NODE_MASK, mask_file );
		}
	}
	
	//TODO: support unicode
	return doc.save_file( filename.toLocal8Bit().constData() );
}