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


QModelIndex ImagesModel::index( int row, int column, const QModelIndex& parent ) const{
	//Root/Group level
	if( !parent.isValid() )
		return createIndex( row, column, 0ull );
	
	//Item level
	if( parent.internalId() == 0 ){
		auto pos = parent.row();
		if( (unsigned)pos < images.groupAmount() )
			return createIndex( row, column, pos + 1 );
		return QModelIndex();
	}
	
	//level is too high!
	return QModelIndex();
}

QModelIndex ImagesModel::parent( const QModelIndex& child ) const{
	auto id = child.internalId();
	if( !child.isValid() ||id == 0 )
		return QModelIndex();
	return index( id-1, 0 );
}

int ImagesModel::rowCount( const QModelIndex &parent ) const{
	if( !parent.isValid() )
		return images.groupAmount();
	
	auto pos = parent.row();
	if( (unsigned)pos < images.groupAmount() && parent.internalId() == 0 )
		return images.getConstGroup( pos ).count();
	else
		return 0;
}

int ImagesModel::columnCount( const QModelIndex& parent ) const{
	return parent.isValid() ? 5 : 5; //TODO:
}

QVariant ImagesModel::data( const QModelIndex& index, int role ) const{
	auto id = index.internalId();
	if( !index.isValid() || role != Qt::DisplayRole )
		return QVariant();
	
	//Root/Group level
	if( id == 0 ){
		auto row = index.row();
		if( (unsigned)row >= images.groupAmount() || index.column() != 0 )
			return QVariant(); //Out-of-bounds, invalid 
		return images.getConstGroup( row ).name;
	}
	
	if( id-1 >= images.groupAmount() )
		return QVariant();
	
	auto& group = images.getConstGroup( id-1 );
	if( (unsigned)index.row() >= group.count() )
		return QVariant();
	auto& item = group.items[index.row()];
	
	switch( index.column() ){
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

QImage ImagesModel::getImage( const QModelIndex& index ) const{
	auto id = index.internalId();
	if( index.isValid() && id > 0 )
		if( unsigned(id-1) < images.groupAmount() ){
			auto& group = images.getConstGroup( id-1 );
			if( index.row() >= 0 && (unsigned)index.row() < group.items.size() )
				return ImageEx( group.image(index.row()) ).to_qimage( ImageEx::SYSTEM_REC709 );
		}
	
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

bool ImagesModel::setData( const QModelIndex& index, const QVariant& value, int role ){
	if( !index.isValid() )
		return false;
	//TODO: we really need some way of abstracting from the how we use QModelIndex...
	
	if( index.internalId() == 0 ){
		if( index.column() != 0 || index.row() >= images.groupAmount() )
			return false;
		images.getGroup( index.row() ).name = value.toString();
	}
	else{
		unsigned id = index.internalId() - 1;
		if( id > images.groupAmount() || index.column() >= 5 )
			return false;
		auto& item = images.getGroup( id ).items[index.row()];
		switch( index.column() ){
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
	emit dataChanged( index, index );
	return true;
}

