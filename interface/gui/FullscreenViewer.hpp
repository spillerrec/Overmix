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

#ifndef FULLSCREEN_VIEWER_HPP
#define FULLSCREEN_VIEWER_HPP

#include "viewer/imageViewer.h"

#include <QImage>
#include <QSettings>
#include <QWidget>


class FullscreenViewer : public imageViewer{
	Q_OBJECT
	
	private:
		FullscreenViewer( QSettings& settings, imageCache* cache, QWidget* parent );
		FullscreenViewer( QSettings& settings, QImage img, QWidget* parent );
		~FullscreenViewer() { change_image( nullptr ); }
		
	public:
		template<typename T>
		static void show( QSettings& settings, T img, QWidget* parent )
			{ new FullscreenViewer( settings, img, parent ); }
		
	protected:
		virtual void focusOutEvent( QFocusEvent* ) override{ close(); }
		virtual void keyPressEvent( QKeyEvent* event ) override;
};

#endif

