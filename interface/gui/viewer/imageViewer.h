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

#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QWidget>
#include <QColor>
#include <QSettings>
#include <QContextMenuEvent>

#include <memory>

#include "Orientation.hpp"
#include "ZoomBox.hpp"

class imageCache;

class QStaticText;

class imageViewer: public QWidget{
	Q_OBJECT
	
	private:
		std::shared_ptr<imageCache> image_cache;
		int frame_amount{ 0 };
		int current_frame{ 0 };
		int loop_counter{ 0 };
		bool continue_animating{ false };
		int waiting_on_frame{ -1 };
	public:
		int get_frame_amount() const{ return frame_amount; }
		int get_current_frame() const{ return current_frame; }
		bool can_animate() const;
		bool is_animating() const{ return continue_animating; }
	
	//Color managed cache
	private:
		QImage converted;
		int converted_monitor{ -1 };
		void clear_converted(){
			converted = QImage();
			converted_monitor = -1;
		}
	
	//How the image is to be viewed
	private:
		ZoomBox zoom;
		Orientation orientation;
		
	//Settings to autoscale
	private:
		bool auto_scale_on{ true };
		bool initial_resize;
		
		void restrict_view( bool force=false );
		void change_zoom( double new_level, QPoint keep_on );
		void auto_zoom();
	public slots:
		void updateView();
		
	private:
		QTimer *time;
		QSettings& settings;
		void init_size();
		
	private slots:
		void read_info();
		void check_frame( unsigned int idx );
	private slots:
		void change_frame( int frame );
		void next_frame(){ change_frame( current_frame + 1 ); }
	public slots:
		void goto_frame( int idx );
		void goto_next_frame(){ goto_frame( current_frame + 1); }
		void goto_prev_frame(){ goto_frame( current_frame - 1); }
		void restart_animation();
		bool toogle_animation();
		void rotate( int8_t amount );
		void rotateLeft (){ rotate( -1 ); };
		void rotateRight(){ rotate( +1 ); };
		void mirror( bool hor, bool ver );
		void mirrorHor(){ mirror( true, false ); }
		void mirrorVer(){ mirror( false, true ); }
	
	protected:
		void updateOrientation( Orientation wanted, Orientation current );
		void draw_message( QStaticText* text );
		void paintEvent( QPaintEvent* );
		void resizeEvent( QResizeEvent* ){ updateView(); }
	
	//Controlling mouse actions
	protected:
		Qt::MouseButtons mouse_active{ Qt::NoButton };
		bool multi_button{ false };
		bool is_zooming{ false };
		double start_zoom{ 1.0 };
		QPoint mouse_last_pos;
		void update_cursor();
		
		Qt::MouseButton button_rleft;
		Qt::MouseButton button_rright;
		Qt::MouseButton button_drag;
		Qt::MouseButton button_double;
		Qt::MouseButton button_scaling;
		Qt::MouseButton button_context;
		
		Qt::MouseButton translate_button( const char* name, char fallback ) const;
		
		void mousePressEvent( QMouseEvent *event );
		void mouseMoveEvent( QMouseEvent *event );
		void mouseDoubleClickEvent( QMouseEvent *event );
		void mouseReleaseEvent( QMouseEvent *event );
		void wheelEvent( QWheelEvent *event );
	
	public:
		explicit imageViewer( QSettings& settings, QWidget* parent = 0 );
		
		void change_image( std::shared_ptr<imageCache> new_image );
		
		Qt::MouseButton get_context_button() const{ return button_context; }
		void create_context_event( const QMouseEvent& event );
		
		bool auto_zoom_active() const{ return auto_scale_on; }
		QSize sizeHint() const;
		QImage get_frame();
		QSize frameSize( unsigned index ) const;
		QSize frameSize() const{ return frameSize( current_frame ); }
	
	signals:
		void image_info_read();
		void resize_wanted();
		void image_changed();
		void double_clicked();
		void rocker_left();
		void rocker_right();
		void clicked();
		void context_menu( QContextMenuEvent event );
};


#endif
