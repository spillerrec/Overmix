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

#ifndef VIDEO_PREVIEW_MODEL_HPP
#define VIDEO_PREVIEW_MODEL_HPP

#include <QAbstractTableModel>
#include <QImage>
#include <memory>
#include <vector>


#include "video/VideoStream.hpp"

namespace Overmix{
	
class VideoStream;

class PreviewItem{
	public:
		QImage img;
		//TODO:
};

class VideoPreviewModel : public QAbstractTableModel{
	Q_OBJECT
	
	private:
		constexpr static int SKIP_AMOUNT = 5;
		std::unique_ptr<VideoStream> stream;
		std::vector<PreviewItem> previews;
		int offset { 0 };
		int amount { 0 };
		
	public:
		VideoPreviewModel( QObject* parent ) : QAbstractTableModel( parent ) { }
		
		void setVideo( QString path, int seek_offset, int amount );
		
	protected:
		int rowCount( const QModelIndex& parent ) const override;
		int columnCount( const QModelIndex& parent ) const override;
		QVariant data( const QModelIndex& index, int role ) const override;
};

}

#endif

