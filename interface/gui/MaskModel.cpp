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


#include "MaskModel.hpp"

#include "utils/utils.hpp"

using namespace Overmix;



QModelIndex MaskModel::index( int row, int column, const QModelIndex& model_parent ) const{
	if( row < rowCount() )
		return createIndex( row, column );
	
	return QModelIndex();
}

QModelIndex MaskModel::parent( const QModelIndex& child ) const{
	return QModelIndex();
}

int MaskModel::rowCount( const QModelIndex &parent ) const{
	return images.maskCount();
}

int MaskModel::columnCount( const QModelIndex& ) const{
	return 3;
}

QVariant MaskModel::data( const QModelIndex& model_index, int role ) const{
	if( !model_index.isValid() || role != Qt::DisplayRole )
		return QVariant();
	
	switch( model_index.column() ){
		case 0: return model_index.row();
		case 1: return images.mask( model_index.row() ).get_width();
		case 2: return images.mask( model_index.row() ).get_height();
		//TODO: Amount of users?
		default: return QVariant();
	}
}

QVariant MaskModel::headerData( int section, Qt::Orientation orien, int role ) const{
	if( orien!=Qt::Horizontal || role != Qt::DisplayRole )
		return QVariant();
	
	switch( section ){
		case 0: return "ID";
		case 1: return "Width";
		case 2: return "Height";
		default: return QVariant();
	}
}

QImage MaskModel::getImage( const QModelIndex& model_index ) const{
	if( model_index.isValid() )
		return ImageEx( images.mask( model_index.row() ) ).to_qimage();
	return QImage();
}

Qt::ItemFlags MaskModel::flags( const QModelIndex& model_index ) const{
	if( model_index.isValid() ){
		Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemNeverHasChildren;
		//if( index.getItem().column == 0 )
			flags &= ~Qt::ItemIsEditable;
		return flags;
	}
	
	return Qt::NoItemFlags;
}

bool MaskModel::setData( const QModelIndex& model_index, const QVariant& value, int role ){
	return false;
}

bool MaskModel::removeRows( int row, int count, const QModelIndex& model_parent ){
	emit beginRemoveRows( model_parent, row, row+count );
	//TODO: Implement in ImageContainer
	return false;
	emit endRemoveRows();
	
	return true;
}

