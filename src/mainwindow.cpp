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

#include "viewer/imageCache.h"

#include "color.hpp"
#include "renders/AverageRender.hpp"
#include "renders/SimpleRender.hpp"
#include "renders/DiffRender.hpp"
#include "renders/FloatRender.hpp"
#include "renders/DifferenceRender.hpp"
#include "aligners/AverageAligner.hpp"
#include "aligners/RecursiveAligner.hpp"
#include "aligners/AnimatedAligner.hpp"
#include "aligners/LayeredAligner.hpp"
#include "aligners/FakeAligner.hpp"
#include "Deteleciner.hpp"
#include "Preprocessor.hpp"
#include "containers/ImageContainer.hpp"

#include "debug.hpp"

#include <vector>
#include <utility>

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
#include <QSettings>

class DialogWatcher : public AProcessWatcher{
	private:
		QProgressDialog& dialog;
	public:
		DialogWatcher( QProgressDialog& dialog ) : dialog(dialog) {
			dialog.setMinimum( 0 );
			dialog.setValue( 0 );
		}
		virtual void setTotal( int total ) override{
			dialog.setMaximum( total );
		}
		virtual void setCurrent( int current ) override{
			dialog.setValue( current );
		}
};

main_widget::main_widget( Preprocessor& preprocessor, ImageContainer& images )
	:	QMainWindow()
	,	ui(new Ui_main_widget)
#ifdef PORTABLE //Portable settings
	,	settings( QCoreApplication::applicationDirPath() + "/settings.ini", QSettings::IniFormat )
#else
	,	settings( "spillerrec", "overmix" )
#endif
	,	viewer( settings, (QWidget*)this )
	,	preprocessor( preprocessor )
	,	images( images )
{
	ui->setupUi(this);
	
	//Buttons
	connect( ui->btn_clear, SIGNAL( clicked() ), this, SLOT( clear_image() ) );
	connect( ui->btn_refresh, SIGNAL( clicked() ), this, SLOT( refresh_image() ) );
	connect( ui->btn_save, SIGNAL( clicked() ), this, SLOT( save_image() ) );
	connect( ui->btn_subpixel, SIGNAL( clicked() ), this, SLOT( subpixel_align_image() ) );
	connect( ui->pre_alpha_mask, SIGNAL( clicked() ), this, SLOT( set_alpha_mask() ) );
	connect( ui->pre_clear_mask, SIGNAL( clicked() ), this, SLOT( clear_mask() ) );
	update_draw();
	
	//Checkboxes
	connect( ui->cbx_interlaced, SIGNAL( toggled(bool) ), this, SLOT( change_interlace() ) );
	change_interlace();
	
	//Groupboxes
	connect( ui->preprocess_group, SIGNAL(clicked(bool)), this, SLOT(resize_preprocess()) );
	connect( ui->merge_group, SIGNAL(clicked(bool)), this, SLOT(resize_merge()) );
	connect( ui->render_group, SIGNAL(clicked(bool)), this, SLOT(resize_render()) );
	connect( ui->postprocess_group, SIGNAL(clicked(bool)), this, SLOT(resize_postprogress()) );
	connect( ui->color_group, SIGNAL(clicked(bool)), this, SLOT(resize_color()) );
	resize_preprocess();
	resize_merge();
	resize_render();
	resize_postprogress();
	resize_color();
	
	//Merge method
	connect( ui->cbx_merge_h, SIGNAL( toggled(bool) ), this, SLOT( toggled_hor() ) );
	connect( ui->cbx_merge_v, SIGNAL( toggled(bool) ), this, SLOT( toggled_ver() ) );
	
	//Reset aligner cache
	connect( ui->rbtn_avg,          SIGNAL( toggled(bool) ), this, SLOT( resetImage() ) );
	connect( ui->rbtn_dehumidifier, SIGNAL( toggled(bool) ), this, SLOT( resetImage() ) );
	connect( ui->rbtn_diff,         SIGNAL( toggled(bool) ), this, SLOT( resetImage() ) );
	connect( ui->rbtn_static_diff,  SIGNAL( toggled(bool) ), this, SLOT( resetImage() ) );
	connect( ui->rbtn_subpixel,     SIGNAL( toggled(bool) ), this, SLOT( resetImage() ) );
	connect( ui->cbx_chroma,        SIGNAL( toggled(bool) ), this, SLOT( resetImage() ) );
	
	//Add images
	qRegisterMetaType<QList<QUrl> >( "QList<QUrl>" );
	connect( this, SIGNAL( urls_retrived(QList<QUrl>) ), this, SLOT( process_urls(QList<QUrl>) ), Qt::QueuedConnection );

	//Refresh info labels
	refresh_text();
	
	setAcceptDrops( true );
	ui->main_layout->addWidget( &viewer );
	viewer.setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );
	
	save_dir = settings.value( "save_directory", "." ).toString();
	if( settings.value( "remember_position", true ).toBool() )
		restoreGeometry( settings.value( "window_position" ).toByteArray() );
}
void main_widget::resize_preprocess(){   resize_groupbox( ui->preprocess_group ); }
void main_widget::resize_merge(){        resize_groupbox( ui->merge_group ); }
void main_widget::resize_render(){       resize_groupbox( ui->render_group ); }
void main_widget::resize_postprogress(){ resize_groupbox( ui->postprocess_group ); }
void main_widget::resize_color(){        resize_groupbox( ui->color_group ); }

main_widget::~main_widget(){
	clear_image();
	delete detelecine;
}


void main_widget::resetAligner(){
	delete aligner;
	aligner = nullptr;
	ui->btn_save->setEnabled( false );
	resetImage();
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
void main_widget::closeEvent( QCloseEvent *event ){
	settings.setValue( "window_position", saveGeometry() );
	QWidget::closeEvent( event );
}

void main_widget::resize_groupbox( QGroupBox* box ){
	if( box->isChecked() )
		box->setMaximumSize( QWIDGETSIZE_MAX, QWIDGETSIZE_MAX );
	else
		box->setMaximumHeight( 20 );
}

//Load an image for mapped, doesn't work with lambdas appearently...
static ImageEx load( QUrl url ){
	ImageEx img;
	img.read_file( url.toLocalFile() );
	return img;
}

void main_widget::process_urls( QList<QUrl> urls ){
	QProgressDialog progress( tr("Loading images"), tr("Stop"), 0, urls.count(), this );
	progress.setWindowModality( Qt::WindowModal );
	
	QTime t;
	t.start();
	int loading_delay = 0;
	
	QFuture<ImageEx> img_loader = QtConcurrent::run( load, urls[0] );
	
	for( int i=0; i<urls.count(); i++ ){
		progress.setValue( i );
		
		QTime delay;
		delay.start();
		//Get and start loading next image
		ImageEx img = img_loader.result();
		//TODO: QtConcurrent is probably not optimized for move semantics
		if( i+1 < urls.count() )
			img_loader = QtConcurrent::run( load, urls[i+1] );
		loading_delay += delay.elapsed();
		
		//De-telecine
		if( detelecine )
			img = detelecine->process( img );
		if( !img.is_valid() )
			continue;
		
		//Crop
		preprocessor.crop_left   = ui->crop_left  ->value();
		preprocessor.crop_right  = ui->crop_right ->value();
		preprocessor.crop_top    = ui->crop_top   ->value();
		preprocessor.crop_bottom = ui->crop_bottom->value();
		
		//Deconvolve
		preprocessor.deviation = ui->pre_deconvolve_deviation->value();
		preprocessor.dev_iterations = ui->pre_deconvolve_iterations->value();
		
		//Scale
		//TODO: method
		preprocessor.scale_x = ui->pre_scale_width->value();
		preprocessor.scale_y = ui->pre_scale_height->value();
		
		preprocessor.processFile( img );
		images.addImage( std::move( img ), alpha_mask ); //TODO: get filename
		
		if( progress.wasCanceled() )
			break;
	}
	qDebug( "Adding images took: %d", t.elapsed() );
	qDebug( "Loading blocked for: %d ms", loading_delay );
	
	clear_cache();
	refresh_text();
	update_draw();
	update();
}


void main_widget::refresh_text(){
	QRect s = (aligner) ? aligner->size() : QRect();
	ui->lbl_info->setText(
			tr( "Size: " )
		+	QString::number(s.width()) + "x"
		+	QString::number(s.height()) + " ("
		+	QString::number( images.count() ) + ")"
	);
}

static color_type color_from_spinbox( QSpinBox* spinbox ){
	return color::fromDouble( spinbox->value() / (double)spinbox->maximum() );
}

#include <QMessageBox>
void main_widget::refresh_image(){
	if( !aligner ) //TODO: we need new way of deciding when to align
		subpixel_align_image();
	
	bool new_image = !temp_ex.is_valid();
	
	if( new_image ){
		//Select filter
		bool chroma_upscale = ui->cbx_chroma->isChecked();
		
		QProgressDialog progress( this );
		progress.setLabelText( "Rendering" );
		progress.setWindowModality( Qt::WindowModal );
		DialogWatcher watcher( progress );
		
		if( ui->rbtn_diff->isChecked() )
			temp_ex = DifferenceRender().render( images, INT_MAX, &watcher );
		else if( ui->rbtn_static_diff->isChecked() )
			temp_ex = DiffRender().render( images, INT_MAX, &watcher );
		else if( ui->rbtn_dehumidifier->isChecked() )
			temp_ex = SimpleRender( SimpleRender::DARK_SELECT, true ).render( images, INT_MAX, &watcher );
		else if( ui->rbtn_subpixel->isChecked() )
			temp_ex = FloatRender().render( images, INT_MAX, &watcher );
		else
		//	temp_ex = SimpleRender( SimpleRender::AVERAGE, chroma_upscale ).render( images, INT_MAX, &watcher );
			temp_ex = AverageRender( chroma_upscale ).render( images, INT_MAX, &watcher );
	}
	
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
	
	//Render image
	if( temp_ex.is_valid() ){	
		QTime t;
		t.start();
		
		pipe_scaling.setWidth( ui->dsbx_scale_width->value() );
		pipe_scaling.setHeight( ui->dsbx_scale_height->value() );
		
		pipe_deconvolve.setDeviation( ui->dsbx_deviation->value() );
		pipe_deconvolve.setIterations( ui->sbx_iterations->value() );
		
		pipe_blurring.setMethod( ui->cbx_blur->currentIndex() );
		pipe_blurring.setWidth( ui->spbx_blur_x->value() );
		pipe_blurring.setHeight( ui->spbx_blur_y->value() );
		
		pipe_edge.setMethod( ui->cbx_edge_filter->currentIndex() );
		
		pipe_level.setLimitMin( color_from_spinbox( ui->sbx_limit_min ) );
		pipe_level.setLimitMax( color_from_spinbox( ui->sbx_limit_max ) );
		pipe_level.setOutMin( color_from_spinbox( ui->sbx_out_min ) );
		pipe_level.setOutMax( color_from_spinbox( ui->sbx_out_max ) );
		pipe_level.setGamma( ui->dsbx_gamma->value() );
		
		//TODO: Sharpen
	//	if( ui->cbx_sharpen->isChecked() )
	//		edge->substract( *(*temp_ex)[0] );
		
		pipe_threshold.setThreshold( color_from_spinbox( ui->threshold_threshold ) );
		pipe_threshold.setSize( ui->threshold_size->value() );
		pipe_threshold.setMethod( ui->threshold_method->currentIndex() );
		
		ImageEx img_temp( pipe_threshold.get( pipe_level.get( pipe_edge.get( pipe_blurring.get( pipe_deconvolve.get( pipe_scaling.get( { temp_ex, new_image } ) ) ) ) ) ).first );
		
		//TODO: why no const on to_qimage?
		temp = QImage( img_temp.to_qimage( system, setting ) );
		
		qDebug( "to_qimage() took: %d", t.elapsed() );
	}
	else
		temp = QImage();
	
	viewer.change_image( new imageCache( temp ), true );
	refresh_text();
	ui->btn_save->setEnabled( true );
}

void main_widget::save_image(){
	/*
	if( temp ){
		debug::make_low_res( *temp, "lr", 4 );
		//debug::make_slide( *temp, "createdslide/upscale", 1.0/0.8 );
	}
	/*/
	QString filename = QFileDialog::getSaveFileName( this, tr("Save image"), save_dir, tr("PNG files (*.png)") );
	if( !filename.isEmpty() ){
		if( QFileInfo( filename ).suffix() == "dump" ){
			if( temp_ex.is_valid() )
				temp_ex.saveDump( filename );
		}
		else if( !temp.isNull() )
			temp.save( filename );
		
		//Remember the folder we saved in
		save_dir = QFileInfo( filename ).dir().absolutePath();
		settings.setValue( "save_directory", save_dir );
	}
	//*/
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

void main_widget::clear_cache(){
	resetAligner();
	temp = QImage();
	viewer.change_image( NULL, true );
}

void main_widget::clear_image(){
	clear_cache();
	clear_mask();
	
	if( detelecine )
		detelecine->clear();
	
	pipe_scaling.invalidate();
	pipe_deconvolve.invalidate();
	pipe_blurring.invalidate();
	pipe_edge.invalidate();
	pipe_level.invalidate();
	pipe_threshold.invalidate();
	
	images.clear();
	
	refresh_text();
	update_draw();
}

void main_widget::subpixel_align_image(){
	QProgressDialog progress( this );
	progress.setLabelText( "Aligning" );
	progress.setWindowModality( Qt::WindowModal );
	DialogWatcher watcher( progress );
	
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
	
	int scale = ui->merge_scale->value();
	
	if( aligner )
		resetAligner();
	
	int merge_index = ui->merge_method->currentIndex();
	switch( merge_index ){
		case 0: //Fake
			aligner = new FakeAligner( images ); break;
		case 1: //Ordered
			aligner = new AverageAligner( images, method, scale ); break;
		case 2: //Recursive
			aligner = new RecursiveAligner( images, method, scale ); break;
		case 3: //Layered
			aligner = new LayeredAligner( images, method, scale ); break;
		case 4: //Animated
			aligner = new AnimatedAligner( images, method, scale ); break;
	}
	
	double movement = ui->merge_movement->value() / 100.0;
	aligner->set_movement( movement );
	
	aligner->set_edges( ui->cbx_edges->isChecked() );
	
	aligner->addImages();
	aligner->align( &watcher );
	
	refresh_text();
	update_draw();
}

void main_widget::change_interlace(){
	bool value = ui->cbx_interlaced->isChecked();
	if( value ){
		if( !detelecine )
			detelecine = new Deteleciner();
		ui->cbx_interlaced->setChecked( true );
	}
	else{
		if( detelecine ){
			if( !detelecine->empty() ){
				ui->cbx_interlaced->setChecked( true );
				return; //We can't change, detelecining still in progress
			}
				
			delete detelecine;
			detelecine = nullptr;
		}
		
		ui->cbx_interlaced->setChecked( false );
	}
}

void main_widget::set_alpha_mask(){
	QString filename = QFileDialog::getOpenFileName( this, tr("Open alpha mask"), "", tr("PNG files (*.png)") );
	
	if( !filename.isEmpty() ){
		ImageEx img;
		img.read_file( filename );
		img.to_grayscale();
		
		alpha_mask = images.addMask( std::move( img[0] ) );
		ui->pre_clear_mask->setEnabled( true );
	}
}

void main_widget::clear_mask(){
	alpha_mask = -1;
	ui->pre_clear_mask->setEnabled( false );
}

void main_widget::update_draw(){
	if( !aligner )
		ui->btn_refresh->setText( tr( "Align&&Draw" ) );
	else
		ui->btn_refresh->setText( tr( "Draw" ) );
	
	ui->btn_refresh->setEnabled( images.count() > 0 );
}
