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

#ifndef IMAGES_MODEL_HPP
#define IMAGES_MODEL_HPP

#include "../containers/ImageContainer.hpp"

#include <QAbstractItemModel>
#include <QImage>


namespace Overmix{

class ImagesModel : public QAbstractItemModel{
	Q_OBJECT
	
	private:
		ImageContainer& images;
		
	public:
		ImagesModel( ImageContainer& images ) : images(images) { }
		
		//QAbstractItemModel implementation
		virtual QModelIndex index( int row, int column, const QModelIndex& parent=QModelIndex() ) const override;
		virtual QModelIndex parent( const QModelIndex& index ) const override;
		virtual int rowCount( const QModelIndex& parent=QModelIndex() ) const override;
		virtual int columnCount( const QModelIndex& parent=QModelIndex() ) const override;
		virtual QVariant data( const QModelIndex& index, int role=Qt::DisplayRole ) const override;
		virtual QVariant headerData( int section, Qt::Orientation orien, int role=Qt::DisplayRole ) const override;
		
		virtual Qt::ItemFlags flags( const QModelIndex& index ) const override;
		virtual bool setData( const QModelIndex& index, const QVariant& value, int role=Qt::EditRole ) override;
		virtual bool removeRows( int row, int count, const QModelIndex& parent=QModelIndex() ) override;
		
		void addGroup( QString name, const QModelIndex& index_from, const QModelIndex& index_to );
		
		QImage getImage( const QModelIndex& index ) const;
};

}

#endif

