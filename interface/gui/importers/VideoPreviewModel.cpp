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


#include "VideoPreviewModel.hpp"

#include "utils/AProcessWatcher.hpp"
#include "video/VideoStream.hpp"
#include "video/VideoFrame.hpp"
#include "containers/ImageContainer.hpp"
#include <QFileInfo>
#include <QBrush>

using namespace Overmix;

PreviewItem::PreviewItem(VideoFrame& frame){
	img = frame.toPreview( 128 );
	keyframe = frame.is_keyframe();
}

QSize VideoPreviewModel::thumbnailSize() const{
	if( amount > 0 )
	{
		auto size = previews.at(0).img.size();
		return { size.width()+4, size.height()+4 };
	}
	return {0, 0};
}

void VideoPreviewModel::setVideo( QString path, int seek_offset, int amount ){
	amount = amount / SKIP_AMOUNT; //TODO: Add one if uneven
	
	stream = std::make_unique<VideoStream>( path );
	offset = seek_offset;
	this->amount = amount;
	
	//TODO: Seek and start populating
	emit beginResetModel();
	stream->seek( seek_offset );
	previews.clear();
	previews.reserve( amount );
	
	//TODO: Rest of it in another thread
	for( int i=0; i<amount; i++ ){
		auto f = stream->getFrame();
		previews.emplace_back( f );
		for( int j=1; j<SKIP_AMOUNT; j++ ){
			auto frame = stream->getFrame();
			if( frame.is_keyframe() )
				previews.emplace_back( frame );
		}
	}
	emit endResetModel();
}

int VideoPreviewModel::rowCount( const QModelIndex& parent ) const {
	auto columns = columnCount(parent);
	return amount / columns + ((amount % columns) != 0 ? 1 : 0); //TODO: Wrong if 0!
}
int VideoPreviewModel::columnCount( const QModelIndex& ) const
	{ return 4; }
	
QVariant VideoPreviewModel::data( const QModelIndex& index, int role ) const {
	int pos = index.row()*columnCount(index) + index.column();
	if( !index.isValid() || pos < 0 || (unsigned)pos >= previews.size() )
		return QVariant();
	
	bool keyframe = previews.at( pos ).keyframe;
	switch( role ){
		case Qt::DecorationRole: return previews.at( pos ).img;
		case Qt::BackgroundRole: return QBrush(QColor(255, (!keyframe) ? 255 : 0, (!keyframe) ? 255 : 0));
	}
	return {};
	
}
