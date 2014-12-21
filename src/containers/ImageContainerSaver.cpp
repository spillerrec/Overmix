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
#include "../utils/ImageLoader.hpp"

#include <cassert>
#include <memory>


#include <QDir>
#include <QFileInfo>
#include <QtConcurrent>
#include <QStringList>

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
const auto NODE_ITEM_CROP      = "crop";
const auto ATTR_ITEM_CROP_L    = "left";
const auto ATTR_ITEM_CROP_T    = "top";
const auto ATTR_ITEM_CROP_B    = "bottom";
const auto ATTR_ITEM_CROP_R    = "right";

static QString getStringAttr( const xml_node& node, const char* name )
	{ return QString::fromUtf8( node.attribute( name ).value() ); }

static QString getString( const xml_node& node, const char* name )
	{ return QString::fromUtf8( node.child( name ).text().get() ); }

static QString baseDir( QDir dir, QString file )
	{ return QDir::isRelativePath( file ) ? dir.absolutePath() + "/" + file : file; }

std::unique_ptr<wchar_t[]> getUnicodeFilepath( QString filename ){
	std::unique_ptr<wchar_t[]> wpath( new wchar_t[filename.size()+1] );
	auto end = filename.toWCharArray( wpath.get() );
	wpath.get()[end] = 0;
	return wpath;
}

unsigned countChildren( xml_node t, const char* name ){
	unsigned count=0;
	for( auto item : t.children( name ) ) //TODO: find a better way of doing this?
		count++;
	return count;
}

QString ImageContainerSaver::load( ImageContainer& container, QString filename ){
	//TODO: progress monitoring
	xml_document doc;
	auto folder = QFileInfo( filename ).dir();
	
	if( !doc.load_file( getUnicodeFilepath(filename).get() ) )
		return QObject::tr( "The file did not contain valid XML" );
	auto root = doc.child( NODE_ROOT );
	
	//Load masks
	QStringList mask_paths;
	for( auto mask : root.child( NODE_MASKS ).children( NODE_MASK ) )
		mask_paths << baseDir( folder, QString::fromUtf8( mask.text().get() ) );
		
	std::vector<int> mask_ids;
	auto mask_future = QtConcurrent::mapped( mask_paths, ImageEx::fromFile );
	for( int i=0; i<mask_paths.size(); i++ ){
		auto img = mask_future.resultAt( i );
		if( img.is_valid() )
			mask_ids.emplace_back( container.addMask( std::move( img[0] ) ) );
		else{
			mask_future.cancel();
			return QObject::tr( "The following mask failed loading: " ) + mask_paths[i];
		}
	}
	
	for( auto group : root.child( NODE_GROUPS ).children( NODE_GROUP ) ){
		container.addGroup( getStringAttr( group, ATTR_GROUP_NAME ) );
		
		//Figure out how many images there is in this group
		//NOTE: needed, as otherwise the references will be broken
		auto amount = countChildren( group, NODE_ITEM );
		ImageLoader images( amount );
		container.prepareAdds( amount );
		
		for( auto item : group.children( NODE_ITEM ) ){
			//Translate mask id
			auto mask  = item.child( NODE_ITEM_MASK  ).text().as_int( -1 );
			if( mask >= 0 ){
				if( unsigned(mask) >= mask_ids.size() )
					return QObject::tr( "The mask id was invalid, got: " ) + QString::number( mask );
				mask = mask_ids[mask];
			}
			
			//Add an image for loading later in parallel
			auto file = baseDir( folder, getString( item, NODE_ITEM_PATH ) );
			auto& img_item = container.addImage( ImageEx(), mask, -1, file );
			images.add( file, img_item.imageRef() );
			
			img_item.frame = item.child( NODE_ITEM_FRAME ).text().as_int( -1 );
			
			auto offset_node = item.child( NODE_ITEM_OFFSET );
			img_item.offset.x = offset_node.attribute( ATTR_ITEM_OFFSET_X ).as_double( 0.0 );
			img_item.offset.y = offset_node.attribute( ATTR_ITEM_OFFSET_Y ).as_double( 0.0 );
		}
		
		auto& items = images.loadAll();
		
		//Can't do cropping until the file is loaded
		int files_added = 0;
		for( auto item : group.children( NODE_ITEM ) ){
			assert( baseDir( folder, getString( item, NODE_ITEM_PATH ) ) == items[files_added].first );
			
			auto crop_node = item.child( NODE_ITEM_CROP );
			items[files_added].second.crop(
					crop_node.attribute( ATTR_ITEM_CROP_L ).as_int( 0 )
				,	crop_node.attribute( ATTR_ITEM_CROP_T ).as_int( 0 )
				,	crop_node.attribute( ATTR_ITEM_CROP_R ).as_int( 0 )
				,	crop_node.attribute( ATTR_ITEM_CROP_B ).as_int( 0 )
				);
			
			files_added++;
		}
		
		//TODO: clean up images which failed loading!
	}
	
	container.setAligned();
	return "";
}

//Convenience functions for adding xml elements containing only of a text node
static void addXmlItem( xml_node& node, const char* name, const char* value )
	{ node.append_child( name ).append_child( node_pcdata ).set_value( value ); }
static void addXmlItem( xml_node& node, const char* name, QString value )
	{ addXmlItem( node, name, value.toUtf8().constData() ); }

template<typename T>
void addXmlItem( xml_node& node, const char* name, T value )
	{ addXmlItem( node, name, std::to_string( value ).c_str() ); }

QString ImageContainerSaver::save( const ImageContainer& container, QString filename ){
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
				return QObject::tr( "Does not support saving generated images" );
			
			auto item_node = group_node.append_child( NODE_ITEM );
			addXmlItem( item_node, NODE_ITEM_PATH , folder.relativeFilePath( item.filename ) );
			addXmlItem( item_node, NODE_ITEM_MASK , item.maskId() );
			addXmlItem( item_node, NODE_ITEM_FRAME, item.frame );
			
			auto offset_node = item_node.append_child( NODE_ITEM_OFFSET );
			offset_node.append_attribute( ATTR_ITEM_OFFSET_X ) = item.offset.x;
			offset_node.append_attribute( ATTR_ITEM_OFFSET_Y ) = item.offset.y;
			
			auto crop_node = item_node.append_child( NODE_ITEM_CROP );
			auto crop = item.image().getCrop();
			crop_node.append_attribute( ATTR_ITEM_CROP_L ) = crop.pos.x;
			crop_node.append_attribute( ATTR_ITEM_CROP_T ) = crop.pos.y;
			crop_node.append_attribute( ATTR_ITEM_CROP_R ) = crop.size.width();
			crop_node.append_attribute( ATTR_ITEM_CROP_B ) = crop.size.height();
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
	
	return doc.save_file( getUnicodeFilepath(filename).get() ) ? "" : QObject::tr("Could not save XML");
}