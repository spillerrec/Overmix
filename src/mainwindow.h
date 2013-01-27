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

#ifndef MAIN_WIDGET_H
#define MAIN_WIDGET_H

#include <QMainWindow>
#include <QImage>

#include "MultiImage.h"
#include "imageViewer.h"

class main_widget: public QMainWindow{
	Q_OBJECT
	
	private:
		class Ui_main_widget *ui;
		imageViewer viewer;
		MultiImage image;
	
	public:
		explicit main_widget();
	
	protected:
		void dragEnterEvent( QDragEnterEvent *event );
		void dropEvent( QDropEvent *event );
	
	private:
	
	private slots:
		void refresh_text();
		void refresh_image();
		void save_image();
		void clear_image();
		void change_dither();
		void change_diff();
		void change_use_average();
		void change_threshould();
		void change_movement();
		void change_merge_method();
};

#endif