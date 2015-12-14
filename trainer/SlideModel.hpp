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

#ifndef SLIDE_MODEL_HPP
#define SLIDE_MODEL_HPP

#include "Slide.hpp"

#include <QAbstractListModel>

namespace Overmix{

class SlideModel : public QAbstractListModel{
	private:
		Slide* slide{ nullptr };
		
	public:
		SlideModel( Slide* slide, QObject* parent=nullptr )
			:	QAbstractListModel(parent), slide(slide) { }
		
		int rowCount( const QModelIndex& ) const override{
			if( slide )
				return slide->images.size();
			else
				return 0;
		}
		
		QVariant data( const QModelIndex& index, int role ) const override{
			if( rowCount(index) < index.row() )
				return {};
			
			if( role == Qt::DisplayRole ){
				auto info = slide->images[index.row()];
				
				switch( index.column() ){
					case 0: return info.filename;
					case 1: return info.interlazed;
					default: return {};
				}
			}
			
			return {};
		}
		
		int columnCount( const QModelIndex& ) const override{ return 2; }
		
		QVariant headerData( int section, Qt::Orientation orientation, int role ) const override{
			if( role == Qt::DisplayRole ){
				switch( section ){
					case 0: return "Filename";
					case 1: return "Interlazed?";
					default: return {};
				}
			}
			
			return {};
		}
};

}

#endif