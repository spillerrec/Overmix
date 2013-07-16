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


#include "ui_mainwindow.h"
#include "mainwindow.hpp"

#include <vector>

#include <QFileInfo>
#include <QFile>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QImage>
#include <QPainter>
#include <QFileDialog>
#include <QProgressDialog>
#include <QTime>
#include <QtConcurrent>

main_widget::main_widget(): QMainWindow(), ui(new Ui_main_widget), viewer((QWidget*)this){
	ui->setupUi(this);
	temp = NULL;
	
	//Buttons
	connect( ui->btn_clear, SIGNAL( clicked() ), this, SLOT( clear_image() ) );
	connect( ui->btn_refresh, SIGNAL( clicked() ), this, SLOT( refresh_image() ) );
	connect( ui->btn_save, SIGNAL( clicked() ), this, SLOT( save_image() ) );
	connect( ui->btn_subpixel, SIGNAL( clicked() ), this, SLOT( subpixel_align_image() ) );
	//TODO: undo
	
	//Checkboxes
	change_use_average();
	connect( ui->cbx_average, SIGNAL( toggled(bool) ), this, SLOT( change_use_average() ) );
	
	//Sliders
	change_threshould();
	change_movement();
	connect( ui->sld_threshould, SIGNAL( valueChanged(int) ), this, SLOT( change_threshould() ) );
	connect( ui->sld_movement, SIGNAL( valueChanged(int) ), this, SLOT( change_movement() ) );
	
	//Merge method
	change_merge_method();
	connect( ui->merge_both, SIGNAL( clicked() ), this, SLOT( change_merge_method() ) );
	connect( ui->merge_h, SIGNAL( clicked() ), this, SLOT( change_merge_method() ) );
	connect( ui->merge_v, SIGNAL( clicked() ), this, SLOT( change_merge_method() ) );
	
	//Add images
	qRegisterMetaType<QList<QUrl> >( "QList<QUrl>" );
	connect( this, SIGNAL( urls_retrived(QList<QUrl>) ), this, SLOT( process_urls(QList<QUrl>) ), Qt::QueuedConnection );

	//Refresh info labels
	refresh_text();
	
	setAcceptDrops( true );
	ui->main_layout->addWidget( &viewer );
	viewer.setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );
}


void main_widget::dragEnterEvent( QDragEnterEvent *event ){
	if( event->mimeData()->hasUrls() )
		event->acceptProposedAction();
}
void main_widget::dropEvent( QDropEvent *event ){
	if( event->mimeData()->hasUrls() ){
		event->setDropAction( Qt::CopyAction );
		
		emit urls_retrived( event->mimeData()->urls() );
		
		event->accept();
	}
}

//Load an image for mapped, doesn't work with lambdas appearently...
static ImageEx* load( QUrl url ){
	ImageEx *img = new ImageEx();
	img->read_file( url.toLocalFile().toLocal8Bit().constData() );
	return img;
}

void main_widget::process_urls( QList<QUrl> urls ){
	QProgressDialog progress( "Mixing images", "Stop", 0, urls.count(), this );
	progress.setWindowModality( Qt::WindowModal );
	
	QTime t;
	t.start();
	int loading_delay = 0;
	
	QFuture<ImageEx*> img_loader = QtConcurrent::run( load, urls[0] );
	
	for( int i=0; i<urls.count(); i++ ){
		progress.setValue( i );
		
		QTime delay;
		delay.start();
		//Get and start loading next image
		ImageEx* img = img_loader.result();
		if( i+1 < urls.count() )
			img_loader = QtConcurrent::run( load, urls[i+1] );
		loading_delay += delay.elapsed();
		
		image.add_image( img );
		if( progress.wasCanceled() && i+1 < urls.count() ){
			delete img_loader.result();
			break;
		}
	}
	qDebug( "Adding images took: %d", t.elapsed() );
	qDebug( "Loading blocked for: %d ms", loading_delay );
	
	refresh_text();
	update();
}


void main_widget::refresh_text(){
	QRect s = image.get_size();
	ui->lbl_height->setText( tr( "Size: " ) + QString::number(s.width()) + "x" + QString::number(s.height()) );
	ui->lbl_amount->setText( tr( "Images: " ) + QString::number( image.get_count() ) );
}

void main_widget::refresh_image(){
	//Select filter
	MultiImage::filters type = MultiImage::FILTER_AVERAGE;
	if( ui->rbtn_simple->isChecked() )
		type = MultiImage::FILTER_SIMPLE;
	else if( ui->rbtn_diff->isChecked() )
		type = MultiImage::FILTER_DIFFERENCE;
	else if( ui->rbtn_slow_slide->isChecked() )
		type = MultiImage::FILTER_SIMPLE_SLIDE;
	else if( ui->rbtn_fast_slide->isChecked() )
		type = MultiImage::FILTER_SIMPLE_SLIDE;
	
	temp = new QImage( image.render( type, ui->cbx_dither->isChecked() ) );
	viewer.change_image( temp, true );
	refresh_text();
}

void main_widget::save_image(){
	QString filename = QFileDialog::getSaveFileName( this, tr("Save image"), "", tr("PNG files (*.png)") );
	if( !filename.isEmpty() && temp )
		temp->save( filename );
}

void main_widget::change_use_average(){
	image.set_use_average( ui->cbx_average->isChecked() );
}

void main_widget::change_movement(){
	image.set_movement( (double)ui->sld_movement->value()/(double)ui->sld_movement->maximum() );
}

void main_widget::change_threshould(){
	image.set_threshould( ui->sld_threshould->value() );
}

void main_widget::change_merge_method(){
	int selected = 0;
	if( ui->merge_both->isChecked() )
		selected = 0;
	else if( ui->merge_h->isChecked() )
		selected = 1;
	else if( ui->merge_v->isChecked() )
		selected = 2;
	image.set_merge_method( selected );
}

void main_widget::clear_image(){
	image.clear();
	temp = NULL;
	viewer.change_image( NULL, true );
	refresh_text();
}


void main_widget::subpixel_align_image(){
	image.subalign_images();
}

