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

#include "FullscreenViewer.hpp"

#include "../color.hpp"
#include "../renders/AverageRender.hpp"
#include "../renders/SimpleRender.hpp"
#include "../renders/DiffRender.hpp"
#include "../renders/FloatRender.hpp"
#include "../renders/DifferenceRender.hpp"
#include "../aligners/AverageAligner.hpp"
#include "../aligners/RecursiveAligner.hpp"
#include "../aligners/AnimatedAligner.hpp"
#include "../aligners/LayeredAligner.hpp"
#include "../aligners/FakeAligner.hpp"
#include "../Deteleciner.hpp"
#include "../Preprocessor.hpp"
#include "../containers/FrameContainer.hpp"
#include "../containers/ImageContainer.hpp"
#include "../containers/ImageContainerSaver.hpp"

#include "savers/DumpSaver.hpp"

#include "../debug.hpp"

#include <vector>
#include <utility>

#include <QDesktopServices>
#include <QFileInfo>
#include <QFile>
#include <QMessageBox>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QInputDialog>
#include <QImage>
#include <QImageReader>
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

void foldableGroupBox( QWidget* widget, bool enabled, QGroupBox* box ){
	auto update = [=]( bool checked )
		{ box->setMaximumHeight( checked ? QWIDGETSIZE_MAX : 20 ); };
	//TODO: find proper size
	
	widget->connect( box, &QGroupBox::clicked, update );
	box->setCheckable( true );
	box->setChecked( enabled );
	update( enabled );
}

main_widget::main_widget( Preprocessor& preprocessor, ImageContainer& images )
	:	QMainWindow()
	,	ui(new Ui_main_widget)
#ifdef PORTABLE //Portable settings
	,	settings( QCoreApplication::applicationDirPath() + "/settings.ini", QSettings::IniFormat )
#else
	,	settings( "spillerrec", "overmix" )
#endif
	,	viewer( settings, (QWidget*)this )
	,	browser( settings, (QWidget*)this )
	,	preprocessor( preprocessor )
	,	images( images )
	,	img_model( images )
{
	ui->setupUi(this);
	
	//Buttons
	connect( ui->btn_clear,      SIGNAL( clicked() ), this, SLOT( clear_image()          ) );
	connect( ui->btn_refresh,    SIGNAL( clicked() ), this, SLOT( refresh_image()        ) );
	connect( ui->btn_save,       SIGNAL( clicked() ), this, SLOT( save_image()           ) );
	connect( ui->btn_save_files, SIGNAL( clicked() ), this, SLOT( save_files()           ) );
	connect( ui->btn_subpixel,   SIGNAL( clicked() ), this, SLOT( subpixel_align_image() ) );
	connect( ui->pre_alpha_mask, SIGNAL( clicked() ), this, SLOT( set_alpha_mask()       ) );
	connect( ui->pre_clear_mask, SIGNAL( clicked() ), this, SLOT( clear_mask()           ) );
	connect( ui->btn_as_mask,    SIGNAL( clicked() ), this, SLOT( use_current_as_mask()  ) );
	connect( ui->btn_apply_mods, SIGNAL( clicked() ), this, SLOT( applyModifications()  ) );
	update_draw();
	
	//Checkboxes
	connect( ui->cbx_interlaced, SIGNAL( toggled(bool) ), this, SLOT( change_interlace() ) );
	change_interlace();
	
	//Groupboxes
	foldableGroupBox( this, false, ui->preprocess_group  );
	foldableGroupBox( this, true,  ui->merge_group       );
	foldableGroupBox( this, false, ui->render_group      );
	foldableGroupBox( this, false, ui->postprocess_group );
	foldableGroupBox( this, false, ui->color_group       );
	//foldableGroupBox( this, true,  ui->images_group      );
	foldableGroupBox( this, false, ui->masks_group       );
	foldableGroupBox( this, false, ui->selection_group   );
	
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
	connect( &img_model, SIGNAL( dataChanged(const QModelIndex&, const QModelIndex&) ), this, SLOT( resetImage() ) );
	
	//Menubar
	connect( ui->action_add_files,    SIGNAL( triggered() ), this, SLOT( open_image()     ) );
	connect( ui->action_save,         SIGNAL( triggered() ), this, SLOT( save_image()     ) );
	connect( ui->action_exit,         SIGNAL( triggered() ), this, SLOT( close()          ) );
	connect( ui->action_show_menubar, SIGNAL( triggered() ), this, SLOT( toggleMenubar()  ) );
	connect( ui->action_fullscreen,   SIGNAL( triggered() ), this, SLOT( showFullscreen() ) );
	connect( ui->action_online_wiki,  SIGNAL( triggered() ), this, SLOT( openOnlineHelp() ) );
	ui->action_show_menubar->setChecked( settings.value( "show_menubar", true ).toBool() );
	toggleMenubar();
	
	
	//Add images
	qRegisterMetaType<QList<QUrl> >( "QList<QUrl>" );
	connect( this, SIGNAL( urls_retrived(QList<QUrl>) ), this, SLOT( process_urls(QList<QUrl>) ), Qt::QueuedConnection );

	//Refresh info labels
	refresh_text();
	
	//Init files model
	ui->files_view->setModel( &img_model );
	ui->files_view->setColumnWidth( 0, 120 );
	connect( ui->files_view->selectionModel(), &QItemSelectionModel::selectionChanged
		,	this, &main_widget::browserChangeImage );
	connect( ui->btn_add_group,    SIGNAL(clicked()), this, SLOT(addGroup()) );
	connect( ui->btn_delete_files, SIGNAL(clicked()), this, SLOT(removeFiles()) );
	
	setAcceptDrops( true );
	ui->preview_layout->addWidget( &viewer );
	ui->files_layout  ->addWidget( &browser );
	viewer .setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );
	browser.setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );
	connect( &viewer, SIGNAL(double_clicked()), this, SLOT(showFullscreen()) );
	
	save_dir = settings.value( "save_directory", "." ).toString();
	if( settings.value( "remember_position", true ).toBool() )
		restoreGeometry( settings.value( "window_position" ).toByteArray() );
}

main_widget::~main_widget(){
	clear_image();
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


//Load an image for mapped, doesn't work with lambdas appearently...
static ImageEx load( QUrl url ){
	ImageEx img;
	img.read_file( url.toLocalFile() );
	return img;
}

void main_widget::process_urls( QList<QUrl> urls ){
	QProgressDialog progress( tr("Loading images"), tr("Stop"), 0, urls.count(), this );
	progress.setWindowModality( Qt::WindowModal );
		
//Update preprocessor settings, TODO: move to separate function?
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
	
	QTime t;
	t.start();
	int loading_delay = 0;
	
	QFuture<ImageEx> img_loader = QtConcurrent::mapped( urls, load );
	for( int i=0; i<urls.count(); i++ ){
		auto file = urls[i].toLocalFile();
		progress.setValue( i );
		
		
		if( QFileInfo( file ).completeSuffix() == "overmix.xml" ){
			//Handle aligner xml
			auto error = ImageContainerSaver::load( images, file );
			if( !error.isEmpty() )
				QMessageBox::warning( this, tr("Could not load alignment"), error );
		}
		else{
			QTime delay;
			delay.start();
			//Get and start loading next image
			ImageEx img( img_loader.resultAt( i ) );
			loading_delay += delay.elapsed();
			
			//De-telecine
			if( detelecine.isActive() ){
				img = detelecine.process( img );
				file = ""; //The result might be a combination of several files
			}
			if( !img.is_valid() )
				continue;
			
			preprocessor.processFile( img );
			images.addImage( std::move( img ), alpha_mask, -1, file );
		}
		
		if( progress.wasCanceled() )
			break;
	}
	qDebug( "Adding images took: %d", t.elapsed() );
	qDebug( "Loading blocked for: %d ms", loading_delay );
	
	clear_cache();
	refresh_text();
	update_draw();
	update();
	ui->files_view->reset();
}


void main_widget::refresh_text(){
	QRect s = images.size();
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

const ImageEx& main_widget::postProcess( const ImageEx& input, bool new_image ){
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
	
	return pipe_threshold.get( 
			pipe_level.get( 
			pipe_edge.get( 
			pipe_blurring.get( 
			pipe_deconvolve.get( 
			pipe_scaling.get( 
				{ input, new_image } ) ) ) ) ) ).first
		;
}


ImageEx main_widget::renderImage( const AContainer& container ){
		//Select filter
		bool chroma_upscale = ui->cbx_chroma->isChecked();
		
		QProgressDialog progress( this );
		progress.setLabelText( "Rendering" );
		progress.setWindowModality( Qt::WindowModal );
		DialogWatcher watcher( progress );
		
		if( ui->rbtn_diff->isChecked() )
			return DifferenceRender().render( container, INT_MAX, &watcher );
		else if( ui->rbtn_static_diff->isChecked() )
			return DiffRender().render( container, INT_MAX, &watcher );
		else if( ui->rbtn_dehumidifier->isChecked() )
			return SimpleRender( SimpleRender::DARK_SELECT, true ).render( container, INT_MAX, &watcher );
		else if( ui->rbtn_subpixel->isChecked() )
			return FloatRender().render( container, INT_MAX, &watcher );
		else
			return AverageRender( chroma_upscale ).render( container, INT_MAX, &watcher );
}

QImage main_widget::qrenderImage( const ImageEx& img ){
	if( !img.is_valid() )
		return QImage();
	
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
	//TODO: fix const on to_qimage
	//TODO: fix postProcess
	return ImageEx( postProcess( img, true ) ).to_qimage( system, setting );
}

imageCache* main_widget::createViewerCache() const{
	auto full_size = images.size();
	
	//TODO: proper frame timings
	auto cache = new imageCache();
	cache->set_info( renders.size(), renders.size() > 1, -1 );
	for( auto& render : renders ){
		QImage current( full_size.width(), full_size.height(), QImage::Format_ARGB32 );
		current.fill( qRgba( 0,0,0,0 ) );
		
		QPainter painter( &current );
		painter.drawImage( render.offset.x, render.offset.y, render.qimg );
		
		cache->add_frame( current, 1000*3/25 ); //3 frame animation delay, with 25 frames a second
	}
	cache->set_fully_loaded();
	return cache;
}


void main_widget::refresh_image(){
	if( !images.isAligned() )
		subpixel_align_image();
	
	auto start = images.minPoint();
	if( renders.size() == 0 ){
		auto frames = getAlignedImages().getFrames();
		renders.reserve( frames.size() );
		
		for( auto& frame : frames ){
			FrameContainer current( getAlignedImages(), frame );
			renders.emplace_back( renderImage( current ), current.minPoint()-start );
		}
	}
	
	for( auto& render : renders )
		render.qimg = qrenderImage( render.raw );
	
	ui->btn_as_mask->setEnabled( renders.size() == 1 && renders[0].raw.get_system() == ImageEx::GRAY );
	viewer.change_image( createViewerCache(), true );
	refresh_text();
	ui->btn_save->setEnabled( true );
}

QString main_widget::getSavePath( QString title, QString file_types ){
	auto filename = QFileDialog::getSaveFileName( this, title, save_dir, file_types );
	
	//Remember the folder we saved in
	if( !filename.isEmpty() ){
		save_dir = QFileInfo( filename ).dir().absolutePath();
		settings.setValue( "save_directory", save_dir );
	}
	
	return filename;
}

void main_widget::save_image(){
	//TODO: assert for equal size
	
	for( auto& render : renders ){
		QString filename = getSavePath( tr("Save image"), tr("PNG files (*.png)") );
		if( !filename.isEmpty() ){
			if( QFileInfo( filename ).suffix() == "dump" )
				DumpSaver( postProcess( render.raw, true ), filename ).exec(); //TODO: fix postProcess
			else
				render.qimg.save( filename );
		}
	}
}

void main_widget::save_files(){
	auto filename = getSavePath( tr("Save alignment"), tr("XML (*.overmix.xml)") );
	if( !filename.isEmpty() ){
		auto error = ImageContainerSaver::save( images, filename );
		if( !error.isEmpty() )
			QMessageBox::warning( this, tr("Could not save alignment"), error );
	}
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
	//TODO: a lot of this should probably be in resetImage!
	ui->btn_save->setEnabled( false );
	resetImage();
	for( auto& render : renders )
		render.qimg = QImage();
	viewer.change_image( nullptr );
}

void main_widget::clear_image(){
	clear_cache();
	clear_mask();
	detelecine.clear();
	
	pipe_scaling.invalidate();
	pipe_deconvolve.invalidate();
	pipe_blurring.invalidate();
	pipe_edge.invalidate();
	pipe_level.invalidate();
	pipe_threshold.invalidate();
	
	images.clear();
	browser.change_image( nullptr );
	ui->files_view->reset();
	
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
	AImageAligner* aligner = nullptr;
	
	int scale = ui->merge_scale->value();
	
	int merge_index = ui->merge_method->currentIndex();
	switch( merge_index ){
		case 0: //Fake
			aligner = new FakeAligner( getAlignedImages() ); break;
		case 1: //Ordered
			aligner = new AverageAligner( getAlignedImages(), method, scale ); break;
		case 2: //Recursive
			aligner = new RecursiveAligner( getAlignedImages(), method, scale ); break;
		case 3: //Layered
			aligner = new LayeredAligner( getAlignedImages(), method, scale ); break;
		case 4: //Animated
			aligner = new AnimatedAligner( getAlignedImages(), method, scale ); break;
	}
	
	double movement = ui->merge_movement->value() / 100.0;
	aligner->set_movement( movement );
	
	aligner->set_edges( ui->cbx_edges->isChecked() );
	
	aligner->addImages();
	aligner->align( &watcher );
	
	delete aligner; //TODO: fix this
	
	clear_cache();
	refresh_text();
	update_draw();
}

void main_widget::change_interlace(){
	detelecine.setEnabled( ui->cbx_interlaced->isChecked() );
}

void main_widget::set_alpha_mask(){
	QString filename = QFileDialog::getOpenFileName( this, tr("Open alpha mask"), save_dir, tr("PNG files (*.png)") );
	
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

void main_widget::use_current_as_mask(){
	if( renders.size() == 1 ){
		//TODO: postProcess cache no longer valid as we have several frames
		alpha_mask = images.addMask( Plane( postProcess(renders[0].raw, false)[0] ) );
		images.onAllItems( [=]( ImageItem& item ){ item.setSharedMask( alpha_mask ); } );
	}
}

void main_widget::update_draw(){
	if( !images.isAligned() )
		ui->btn_refresh->setText( tr( "Align&&Draw" ) );
	else
		ui->btn_refresh->setText( tr( "Draw" ) );
	
	ui->btn_refresh->setEnabled( images.count() > 0 );
}


void main_widget::addGroup(){
	auto name = QInputDialog::getText( this, tr("New group"), tr("Enter group name") );
	if( !name.isEmpty() ){
		images.addGroup( name );
		ui->files_view->reset();
	}
}

static QImage fromSelection( const ImagesModel& model, const QModelIndexList& indexes ){
	if( indexes.size() > 0 ){
		auto index = indexes.front();
		return model.getImage( index );
	}
	return QImage();
}

void main_widget::browserChangeImage( const QItemSelection& selected, const QItemSelection& ){
	auto img = fromSelection( img_model, selected.indexes() );
	if( !img.isNull() )
		browser.change_image( new imageCache( img ), true );
}

void main_widget::removeFiles(){
	auto indexes = ui->files_view->selectionModel()->selectedRows();
	if( indexes.size() > 0 )
		img_model.removeRows( indexes.front().row(), indexes.size(), img_model.parent(indexes.front()) );
	refresh_text();
	resetImage();
}

void main_widget::showFullscreen(){
	if( ui->tab_pages->currentIndex() != 0 ){
		auto img = fromSelection( img_model, ui->files_view->selectionModel()->selectedIndexes() );
		if( !img.isNull() )
			FullscreenViewer::show( settings, img );
	}
	else if( renders.size() > 0 )
		FullscreenViewer::show( settings, createViewerCache() );
}


void main_widget::open_image(){
	auto formats = QImageReader::supportedImageFormats();
	auto filter = QString( "*.dump" );
	for( auto f : formats )
		filter += " *." + f;
	
	auto files = QFileDialog::getOpenFileNames( this, "Select images", "", "Images (" + filter + ")" );
	QList<QUrl> urls;
	for( auto file : files )
		urls.push_back( QUrl::fromLocalFile( file ) );
	process_urls( urls );
}

void main_widget::toggleMenubar(){
	if( ui->action_show_menubar->isChecked() )
		ui->menuBar->show();
	else
		ui->menuBar->hide();
}

void main_widget::openOnlineHelp(){
	QDesktopServices::openUrl( QUrl( "https://github.com/spillerrec/Overmix/wiki" ) );
}

AContainer& main_widget::getAlignedImages(){
	return images;
}

void main_widget::applyModifications(){
	auto left   = ui->crop_left  ->value();
	auto top    = ui->crop_top   ->value();
	auto right  = ui->crop_right ->value();
	auto bottom = ui->crop_bottom->value();
	
	auto& container = getAlignedImages();
	for( unsigned i=0; i<container.count(); ++i )
		container.cropImage( i, left, top, right, bottom );
	
	clear_cache();
}
