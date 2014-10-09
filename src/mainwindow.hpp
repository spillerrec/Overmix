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
#include <QSettings>
#include <QUrl>

#include "ImageEx.hpp"
#include "viewer/imageViewer.h"

#include "RenderOperations.hpp"
#include "containers/ImageContainer.hpp"

class AImageAligner;
class Deteleciner;
class Plane;
class QGroupBox;

class main_widget: public QMainWindow{
	Q_OBJECT
	
	private:
		class Ui_main_widget *ui;
		
		QSettings settings;
		QString save_dir;
		
		imageViewer viewer;
		ImageContainer images;
		QImage *temp{ nullptr };
		
		AImageAligner *aligner{ nullptr };
		ImageEx temp_ex;
		Deteleciner *detelecine{ nullptr };
		
		Plane alpha_mask;
		
		void clear_cache();
		
		
	private:
		RenderPipeScaling pipe_scaling;
		RenderPipeDeconvolve pipe_deconvolve;
		RenderPipeBlurring pipe_blurring;
		RenderPipeEdgeDetection pipe_edge;
		RenderPipeLevel pipe_level;
		RenderPipeThreshold pipe_threshold;
	
	public:
		explicit main_widget();
		~main_widget();
	
	protected:
		void dragEnterEvent( QDragEnterEvent *event );
		void dropEvent( QDropEvent *event );
		void closeEvent( QCloseEvent *event );
		
		void resize_groupbox( QGroupBox* box );
		void make_slide();
	
	signals:
		void urls_retrived( QList<QUrl> urls );
		
	private slots:
		void resize_preprocess();
		void resize_merge();
		void resize_render();
		void resize_postprogress();
		void resize_color();
		
		void resetAligner();
		void resetImage(){ temp_ex = ImageEx(); }
		void update_draw();
	
	private slots:
		void process_urls( QList<QUrl> urls );
		void refresh_text();
		void refresh_image();
		void save_image();
		void clear_image();
		void subpixel_align_image();
		void toggled_hor();
		void toggled_ver();
		void change_interlace();
		
		void set_alpha_mask();
		void clear_mask();
};

#endif