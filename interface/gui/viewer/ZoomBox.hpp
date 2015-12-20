/*
	This file is part of imgviewer.

	imgviewer is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	imgviewer is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with imgviewer.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ZOOM_BOX_HPP
#define ZOOM_BOX_HPP

#include <QRect>
#include <cmath>

class ZoomBox{
	private:
		QSize  content;     //The size of the item in this box
		QPoint position;    //The offset of content in this zoom
		double zoom{ 0.0 }; //The level of zooming
		
	public:
		QPoint pos() const{ return position; }
		QSize size() const{ return content * scale(); }
		double level() const{ return zoom; }
		QRect area() const{ return { pos(), size() }; }
		
		double scale() const{ return std::pow( 2.0, zoom * 0.5 ); }
		double fromScale( double scale ){ return std::log2( scale ) * (2.0 / std::log2( 2 )); }
		
		bool change_scale( double new_level, QPoint keep_on );
		bool change_content( QSize content, bool keep_zoom );
		
		void restrict( QSize view );
		bool resize( QSize view, bool downscale_only, bool upscale_only, bool keep_aspect );
		
		bool moveable( QSize view ) const
			{ return ( size().width() > view.width() ) || ( size().height() > view.height() ); }
		
		bool move( QPoint offset );
};

#endif
