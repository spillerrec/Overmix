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

#include <QFileInfo>


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

int ImagesModel::columnCount( const QModelIndex& parent ) const{
	return parent.isValid() ? 5 : 5; //TODO:
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

QImage ImagesModel::getImage( const QModelIndex& model_index ) const{
	ImagesIndex index( model_index, images );
	
	if( index.isValid() && !index.isGroup() )
		return ImageEx( index.getItem().image() ).to_qimage( ImageEx::SYSTEM_REC709 );
	return QImage();
}

Qt::ItemFlags ImagesModel::flags( const QModelIndex& ) const{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
	//TODO: Use Qt::ItemNeverHasChildren ?
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
	
	if( index.isGroup() )
		index.getGroup(images).name = value.toString();
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

