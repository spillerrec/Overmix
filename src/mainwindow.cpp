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

#include "SimpleRender.hpp"
#include "FloatRender.hpp"
#include "AverageAligner.hpp"

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
	
	//Checkboxes
	change_use_average();
	connect( ui->cbx_average, SIGNAL( toggled(bool) ), this, SLOT( change_use_average() ) );
	connect( ui->cbx_interlaced, SIGNAL( toggled(bool) ), this, SLOT( change_interlace() ) );
	
	//Sliders
	change_threshould();
	change_movement();
	connect( ui->sld_threshould, SIGNAL( valueChanged(int) ), this, SLOT( change_threshould() ) );
	connect( ui->sld_movement, SIGNAL( valueChanged(int) ), this, SLOT( change_movement() ) );
	
	//Merge method
	connect( ui->cbx_merge_h, SIGNAL( toggled(bool) ), this, SLOT( toggled_hor() ) );
	connect( ui->cbx_merge_v, SIGNAL( toggled(bool) ), this, SLOT( toggled_ver() ) );
	
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
		
		//TODO: de-interlace
		
		//TODO: crop
		
		//Deconvolve
		double deviation = ui->pre_deconvolve_deviation->value();
		unsigned iterations = ui->pre_deconvolve_iterations->value();
		if( deviation > 0.0009 && iterations > 0 )
			img->apply_operation( &Plane::deconvolve_rl, deviation, iterations );
		
		//Scale
		double scale_width = ui->pre_scale_width->value();
		double scale_height = ui->pre_scale_height->value();
		if( scale_width <= 0.9999 || scale_width >= 1.0001
			|| scale_height <= 0.9999 || scale_height >= 1.0001 )
			img->scale( img->get_width() * scale_width + 0.5, img->get_height() * scale_height + 0.5 );
		
		images.push_back( img );
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
	QRect s = (aligner) ? aligner->size() : QRect();
	ui->lbl_info->setText(
			tr( "Size: " )
		+	QString::number(s.width()) + "x"
		+	QString::number(s.height()) + " ("
		+	QString::number( images.size() ) + ")"
	);
}

void main_widget::refresh_image(){
	if( !aligner )
		return;
	
	//Select filter
	ImageEx *img_org{ nullptr };
	bool chroma_upscale = ui->cbx_chroma->isChecked();
	
	#undef DIFFERENCE //TODO: where the heck did this macro come from? And why does it prevent my code from compiling?
	if( ui->rbtn_diff->isChecked() )	//TODO: missing renderer
		img_org = SimpleRender( SimpleRender::DIFFERENCE, chroma_upscale ).render( *aligner );
	else if( ui->rbtn_windowed->isChecked() )
		img_org = SimpleRender( SimpleRender::SIMPLE_SLIDE, chroma_upscale ).render( *aligner );
	else if( ui->rbtn_subpixel->isChecked() )
		img_org = FloatRender().render( *aligner );
	else
		img_org = SimpleRender( SimpleRender::AVERAGE, chroma_upscale ).render( *aligner );
	
	
	
	//Set color system
	ImageEx::YuvSystem system = ImageEx::SYSTEM_KEEP;
	if( ui->rbtn_rec709->isChecked() )
		system = ImageEx::SYSTEM_REC709;
	if( ui->rbtn_rec601->isChecked() )
		system = ImageEx::SYSTEM_REC601;
	
	//Set settings
	unsigned setting = ImageEx::SETTING_NONE;
	if( ui->cbx_dither->isChecked() )
		setting = setting | ImageEx::SETTING_DITHER;
	if( ui->cbx_gamma->isChecked() )
		setting = setting | ImageEx::SETTING_GAMMA;
	
	//Aspect
	double scale_width = ui->dsbx_scale_width->value();
	double scale_height = ui->dsbx_scale_height->value();
	
	//Render image
//	ImageEx *img_org = image.render_image( type, ui->cbx_chroma->isChecked() );
	if( img_org ){	
		QTime t;
		t.start();
		
		
		//Fix aspect ratio
		if( scale_width <= 0.9999 || scale_width >= 1.0001
			|| scale_height <= 0.9999 || scale_height >= 1.0001 )
			img_org->scale( img_org->get_width() * scale_width + 0.5, img_org->get_height() * scale_height + 0.5 );
		
		Plane *org = (*img_org)[0];
		ImageEx img_temp( *img_org );
		
		//Deconvolve
		double deviation = ui->dsbx_deviation->value();
		unsigned iterations = ui->sbx_iterations->value();
		if( deviation > 0.0009 && iterations > 0 )
			img_temp.apply_operation( &Plane::deconvolve_rl, deviation, iterations );
		
		//Blurring
		unsigned blur_x = ui->spbx_blur_x->value();
		unsigned blur_y = ui->spbx_blur_y->value();
		switch( ui->cbx_blur->currentIndex() ){
			case 1: img_temp.apply_operation( &Plane::blur_box, blur_x, blur_y ); break;
			case 2: img_temp.apply_operation( &Plane::blur_gaussian, blur_x, blur_y ); break;
			default: break;
		}
		
		//Edge detection
		switch( ui->cbx_edge_filter->currentIndex() ){
			case 1: img_temp.apply_operation( &Plane::edge_robert ); break;
			case 2: img_temp.apply_operation( &Plane::edge_sobel ); break;
			case 3: img_temp.apply_operation( &Plane::edge_prewitt ); break;
			case 4: img_temp.apply_operation( &Plane::edge_laplacian ); break;
			case 5: img_temp.apply_operation( &Plane::edge_laplacian_large ); break;
			default: break;
		};
		
		//Level
		double scale = (256*256-1) / 255.0;
		color_type limit_min = round( ui->sbx_limit_min->value() * scale );
		color_type limit_max = round( ui->sbx_limit_max->value() * scale );
		color_type out_min = round( ui->sbx_out_min->value() * scale );
		color_type out_max = round( ui->sbx_out_max->value() * scale );
		double gamma = ui->dsbx_gamma->value();
		img_temp.apply_operation( &Plane::level, limit_min, limit_max, out_min, out_max, gamma );
		
		
		//Sharpen
	//	if( ui->cbx_sharpen->isChecked() )
	//		edge->substract( *(*img_org)[0] );
		delete img_org;
		
		temp = new QImage( img_temp.to_qimage( system, setting ) );
		
		qDebug( "to_qimage() took: %d", t.elapsed() );
	}
	else
		temp = new QImage();
	
	viewer.change_image( temp, true );
	refresh_text();
}

void main_widget::save_image(){
	QString filename = QFileDialog::getSaveFileName( this, tr("Save image"), "", tr("PNG files (*.png)") );
	if( !filename.isEmpty() && temp )
		temp->save( filename );
}

void main_widget::change_use_average(){
//	image.set_use_average( ui->cbx_average->isChecked() );
}

void main_widget::change_movement(){
//	image.set_movement( (double)ui->sld_movement->value()/(double)ui->sld_movement->maximum() );
}

void main_widget::change_threshould(){
//	image.set_threshould( ui->sld_threshould->value() );
}

void main_widget::toggled_hor(){
	//Always have one checked
	if( !(ui->cbx_merge_h->isChecked()) )
		ui->cbx_merge_v->setChecked( true );
}
void main_widget::toggled_ver(){
	//Always have one checked
	if( !(ui->cbx_merge_v->isChecked()) )
		ui->cbx_merge_h->setChecked( true );
}

void main_widget::clear_image(){
	for( auto img : images )
		delete img;
	images.clear();
	
	delete aligner;
	aligner = nullptr;
	
	temp = NULL; //TODO: huh?
	viewer.change_image( NULL, true );
	refresh_text();
}


void main_widget::subpixel_align_image(){
	//Select movement type
	AImageAligner::AlignMethod method{ AImageAligner::ALIGN_BOTH };
	if( ui->cbx_merge_v->isChecked() )
		method = AImageAligner::ALIGN_HOR;
	if( ui->cbx_merge_h->isChecked() ){
		if( ui->cbx_merge_v->isChecked() )
			method = AImageAligner::ALIGN_BOTH;
		else
			method = AImageAligner::ALIGN_VER;
	}
	
	//TODO: show progress
	if( aligner )
		delete aligner;
	aligner = new AverageAligner( method, 1.0 ); //TODO: some way of setting size
	
	for( auto img : images )
		aligner->add_image( img );
	aligner->align();
	
	refresh_text();
}

void main_widget::change_interlace(){
	bool value = ui->cbx_interlaced->isChecked();
//	if( image.set_interlaceing( value ) != value )
//		ui->cbx_interlaced->setChecked( !value );
//TODO:
}

