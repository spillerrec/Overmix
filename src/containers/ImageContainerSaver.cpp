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

#include "pugixml/pugixml.hpp"
using namespace pugi;

const auto NODE_ROOT       = "alignment";
const auto NODE_GROUP      = "group";
const auto ATTR_GROUP_NAME = "name";
const auto NODE_ITEM       = "item";
const auto NODE_ITEM_PATH  = "filepath";

bool ImageContainerSaver::load( ImageContainer& container, QString file ){
	return false;
}

bool ImageContainerSaver::save( const ImageContainer& container, QString filename ){
	xml_document doc;
	
	auto param = doc.append_child( node_declaration );
	param.append_attribute( "version"  ) = 1.0;
	param.append_attribute( "encoding" ) = "UTF-8";
	
	auto root = doc.append_child( NODE_ROOT );
	
	for( auto& group : container ){
		auto group_node = root.append_child( NODE_GROUP );
		group_node.append_attribute( ATTR_GROUP_NAME ) = group.name.toUtf8().constData();
		
		for( auto& item : group ){
			auto item_node = group_node.append_child( NODE_ITEM );
			auto file_node = item_node.append_child( NODE_ITEM_PATH );
			file_node.append_child( node_pcdata ).set_value( item.filename.toUtf8().constData() );
			//TODO: Make item.filename relative to filename
			//TODO: fail if path is empty (detelecine)
		}
	}
	
	return doc.save_file( filename.toLocal8Bit().constData() );
}