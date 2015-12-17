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

#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <QWidget>
#include <QSettings>
#include <QTreeView>

#include "Slide.hpp"
#include "SlideModel.hpp"

class imageViewer;

namespace Overmix{


class MainWindow: public QWidget{
	Q_OBJECT
	
	private:
		Slide s;
		SlideModel s_model;
		
		QTreeView view;
		imageViewer* viewer;
		QSettings settings;
		
	public:
		explicit MainWindow();
		
		
	protected:
		
		void dragEnterEvent( QDragEnterEvent* ) override;
		void dropEvent( QDropEvent* ) override;
		void keyPressEvent( QKeyEvent* ) override;
		
		
	private:
		void toogleInterlaze();
		void loadSlide();
		void saveSlide();
		void evaluateInterlaze();
		void newSlide();
};

}

#endif