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

class imageCache;

class QStaticText;

class imageViewer: public QWidget{
	Q_OBJECT
	
	private:
		imageCache *image_cache;
		int frame_amount;
		int current_frame;
		int loop_counter;
		bool continue_animating;
		int waiting_on_frame;
		QImage get_frame() const;
	public:
		int get_frame_amount() const{ return frame_amount; }
		int get_current_frame() const{ return current_frame; }
		bool can_animate() const;
		bool is_animating() const{ return continue_animating; }
	
	//Color managed cache
	private:
		QImage converted;
		int converted_monitor;
		void clear_converted(){
			converted = QImage();
			converted_monitor = -1;
		}
	
	//How the image is to be viewed
	private:
		QPoint shown_pos;
		QSize shown_size;
		double shown_zoom_level;
		bool moveable() const;
		
	//Settings to autoscale
	private:
		bool auto_scale_on;
		bool auto_aspect_ratio;
		bool auto_downscale_only;
		bool auto_upscale_only;
		bool restrict_viewpoint;
		bool initial_resize;
		bool keep_resize;
		
		void restrict_view( bool force=false );
		void change_zoom( double new_level, QPoint keep_on );
		void auto_zoom();
		
	private:
		QTimer *time;
		const QSettings& settings;
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
	
	protected:
		void draw_message( QStaticText* text );
		void paintEvent( QPaintEvent* );
		void resizeEvent( QResizeEvent* ){
			if( auto_scale_on )
				auto_zoom();
			else
				restrict_view();
		}
	
	//Controlling mouse actions
	protected:
		Qt::MouseButtons mouse_active;
		bool multi_button;
		bool is_zooming;
		double start_zoom;
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
		explicit imageViewer( const QSettings& settings, QWidget* parent = 0 );
		
		void change_image( imageCache *new_image, bool delete_old = true );
		
		Qt::MouseButton get_context_button() const{ return button_context; }
		void create_context_event( const QMouseEvent& event );
		
		bool auto_zoom_active() const{ return auto_scale_on; }
		QSize sizeHint() const;
	
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