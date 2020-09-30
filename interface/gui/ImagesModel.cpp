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


#include "ImagesModel.hpp"

#include "utils/utils.hpp"

#include <QFileInfo>
#include <QPainter>

using namespace Overmix;


class ImagesIndex{
	private:
		const QModelIndex& index;
		const ImageContainer& images;
		
		//These are valid only as long as index is valid, otherwise they contain garbage!
	public:
		unsigned group{ 0 };
		unsigned item{ 0 };
		unsigned column{ 0 };
		
	public:
		ImagesIndex( const QModelIndex& index, const ImageContainer& images )
			: index(index), images(images) {
			
			if( isGroup() )
				group = index.row();
			else{
				group = index.internalId() - 1;
				item = index.row();
			}
			column = index.column();
		}
		
		bool isGroup() const{ return index.internalId() == 0; }
		
		bool isValid() const{
			if( !index.isValid() )
				return false;
			
			if( group >= images.groupAmount() )
				return false;
			
			if( column >= (isGroup() ? 1 : 5) )
				return false;
			
			if( !isGroup() && item >= images.getConstGroup(group).items.size() )
				return false;
			
			return true;
		}
		
		const ImageGroup& getGroup() const{ return images.getConstGroup( group ); }
		const ImageItem& getItem() const{ return getGroup().items[item];}
		
		//non-const versions
		ImageGroup& getGroup( ImageContainer& images ){ return images.getGroup( group ); }
		ImageItem&  getItem ( ImageContainer& images ){ return getGroup(images).items[item];}
};


QModelIndex ImagesModel::index( int row, int column, const QModelIndex& model_parent ) const{
	ImagesIndex parent( model_parent, images );
	
	if( !parent.isValid() )
		return createIndex( row, column, 0ull );
	
	if( parent.isGroup() )
		return createIndex( row, column, parent.group + 1 );
	else
		return QModelIndex();
}

QModelIndex ImagesModel::parent( const QModelIndex& child ) const{
	auto id = child.internalId();
	if( !child.isValid() || id == 0 )
		return QModelIndex();
	return index( id-1, 0 );
}

int ImagesModel::rowCount( const QModelIndex &parent ) const{
	ImagesIndex index( parent, images );
	if( !index.isValid() )
		return images.groupAmount();
	
	return index.isGroup() ? index.getGroup().count() : 0;
}

int ImagesModel::columnCount( const QModelIndex& ) const{
	return 5;
}

QVariant ImagesModel::data( const QModelIndex& model_index, int role ) const{
	ImagesIndex index( model_index, images );
	if( !index.isValid() || role != Qt::DisplayRole )
		return QVariant();
	
	//Root/Group level
	if( index.isGroup() )
		return index.getGroup().name;
	
	auto& item = index.getItem();
	switch( index.column ){
		case 0: return QFileInfo( item.filename ).fileName();
		case 1: return item.offset.x;
		case 2: return item.offset.y;
		case 3: return item.maskId();
		case 4: return item.frame;
		default: return QVariant();
	}
}

QVariant ImagesModel::headerData( int section, Qt::Orientation orien, int role ) const{
	if( orien!=Qt::Horizontal || role != Qt::DisplayRole || section >= 5 )
		return QVariant();
	
	switch( section ){
		case 0: return "File";
		case 1: return "x";
		case 2: return "y";
		case 3: return "Mask";
		case 4: return "Frame";
		default: return QVariant();
	}
}

void ImagesModel::addGroup( QString name, const QModelIndex& index_from, const QModelIndex& index_to ){
	ImagesIndex from( index_from, images );
	ImagesIndex to  ( index_to  , images );
	
	if( from.isValid() && to.isValid() && from.group == to.group )
		if( to.item - from.item > 0 ){
			images.addGroup( name, from.group, from.item, to.item+1 );
			return;
		}
	
	images.addGroup( name );
}

QImage ImagesModel::getImage( const QModelIndex& model_index ) const{
	ImagesIndex index( model_index, images );
	
	if( index.isValid() && !index.isGroup() ){
		auto& item = index.getItem();
		auto img = item.image().to_qimage();
		img = setQImageAlpha( img, item.alpha( images.getMasks() ) );
		
		//Expand with transparent to fill entire output size
		auto area = images.size();
		auto offset = item.offset - area.pos;
		QImage full_img(area.size.width(), area.size.height(), QImage::Format_ARGB32);
		QPainter painter(&full_img);
		painter.drawImage(QPoint(offset.x, offset.y), img);
		
		return full_img;
		
	}
	return QImage();
}

Qt::ItemFlags ImagesModel::flags( const QModelIndex& model_index ) const{
	ImagesIndex index( model_index, images );
	
	if( index.isValid() ){
		Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
		if( !index.isGroup() ){
			if( index.column == 0 ) //We can't change filename!
				flags &= ~Qt::ItemIsEditable;
			flags |= Qt::ItemNeverHasChildren;
		}
		return flags;
	}
	
	return Qt::NoItemFlags;
}

//TODO: make a generic variant of this stuff
static bool checkingToInt( const QVariant& value, int& output ){
	bool ok;
	int val = value.toInt( &ok );
	if( ok )
		output = val;
	return ok;
}

static bool checkingToDouble( const QVariant& value, double& output ){
	bool ok;
	double val = value.toDouble( &ok );
	if( ok )
		output = val;
	return ok;
}

bool ImagesModel::setData( const QModelIndex& model_index, const QVariant& value, int role ){
	ImagesIndex index( model_index, images );
	if( !index.isValid() || role != Qt::EditRole )
		return false;
	
	if( index.isGroup() ){
		auto new_name = value.toString();
		if( !new_name.isEmpty() )
			index.getGroup(images).name = new_name;
	}
	else{
		auto& item = index.getItem(images);
		switch( index.column ){
			case 1: if( !checkingToDouble( value, item.offset.x ) ) return false; break;
			case 2: if( !checkingToDouble( value, item.offset.y ) ) return false; break;
			case 4: if( !checkingToInt   ( value, item.frame    ) ) return false; break;
			case 3: {
					bool ok;
					int out = value.toInt( &ok );
					if( !ok || out >= (int)images.maskCount() )
						return false;
					item.setSharedMask( out );
				} break;
			default: return false;
		}
	}
	
	//Everything went fine
	emit dataChanged( model_index, model_index );
	return true;
}

bool ImagesModel::removeRows( int row, int count, const QModelIndex& model_parent ){
	ImagesIndex parent( model_parent, images );
	if( !parent.isValid() )
		return false;
	
	emit beginRemoveRows( model_parent, row, row+count );
	//TODO: this is likely incorrect, we probably can't call it before we will know it will succeed
	if( parent.isGroup() ){
		if( !util::removeItems( parent.getGroup( images ).items, row, count ) )
			return false;
		images.rebuildIndexes(); //TODO: this is kinda hackish
	}
	else
		if( !images.removeGroups( row, count ) )
			return false;
	emit endRemoveRows();
	
	return true;
}

