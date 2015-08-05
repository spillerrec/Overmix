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
#include "../renders/AnimRender.hpp"
#include "../renders/AverageRender.hpp"
#include "../renders/DiffRender.hpp"
#include "../renders/FloatRender.hpp"
#include "../renders/StatisticsRender.hpp"
#include "../renders/PixelatorRender.hpp"
#include "../renders/RobustSrRender.hpp"
#include "../renders/EstimatorRender.hpp"
#include "../aligners/AnimationSeparator.hpp"
#include "../aligners/AverageAligner.hpp"
#include "../aligners/FakeAligner.hpp"
#include "../aligners/FrameAligner.hpp"
#include "../aligners/LayeredAligner.hpp"
#include "../aligners/RecursiveAligner.hpp"
#include "../aligners/LinearAligner.hpp"
#include "../aligners/SuperResAligner.hpp"
#include "../Deteleciner.hpp"
#include "../containers/DelegatedContainer.hpp"
#include "../containers/FrameContainer.hpp"
#include "../containers/ImageContainer.hpp"
#include "../containers/ImageContainerSaver.hpp"
#include "../utils/ImageLoader.hpp"

#include "savers/DumpSaver.hpp"
#include "visualisations/MovementGraph.hpp"

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
		QProgressDialog dialog;
	public:
		DialogWatcher( QWidget* parent, QString label ) : dialog( parent ) {
			dialog.setLabelText( label );
			dialog.setWindowModality( Qt::WindowModal );
			dialog.setMinimum( 0 );
			dialog.setValue( 0 );
		}
		virtual void setTotal( int total ) override{
			dialog.setMaximum( total );
		}
		virtual void setCurrent( int current ) override{
			dialog.setValue( current );
		}
		virtual int getCurrent() const override{ return dialog.value(); }
		
		virtual bool shouldCancel() const override{ return dialog.wasCanceled(); }
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

main_widget::main_widget( ImageContainer& images )
	:	QMainWindow()
	,	ui(new Ui_main_widget)
#ifdef PORTABLE //Portable settings
	,	settings( QCoreApplication::applicationDirPath() + "/settings.ini", QSettings::IniFormat )
#else
	,	settings( "spillerrec", "overmix" )
#endif
	,	viewer( settings, (QWidget*)this )
	,	browser( settings, (QWidget*)this )
	,	images( images )
	,	img_model( images )
{
	ui->setupUi(this);
	
	//Buttons
	connect( ui->btn_clear,      SIGNAL( clicked() ), this, SLOT( clear_image()          ) );
	connect( ui->btn_refresh,    SIGNAL( clicked() ), this, SLOT( refresh_image()        ) );
	connect( ui->btn_save,       SIGNAL( clicked() ), this, SLOT( save_image()           ) );
	connect( ui->btn_save_files, SIGNAL( clicked() ), this, SLOT( save_files()           ) );
	connect( ui->btn_subpixel,   SIGNAL( clicked() ), this, SLOT( alignImage() ) );
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
	connect( ui->rbtn_median,       SIGNAL( toggled(bool) ), this, SLOT( resetImage() ) );
	connect( ui->rbtn_pixelator,    SIGNAL( toggled(bool) ), this, SLOT( resetImage() ) );
	connect( ui->cbx_chroma,        SIGNAL( toggled(bool) ), this, SLOT( resetImage() ) );
	connect( &img_model, SIGNAL( dataChanged(const QModelIndex&, const QModelIndex&) ), this, SLOT( resetImage() ) );
	
	//Menubar
	connect( ui->action_add_files,    SIGNAL( triggered() ), this, SLOT( open_image()     ) );
	connect( ui->action_save,         SIGNAL( triggered() ), this, SLOT( save_image()     ) );
	connect( ui->action_exit,         SIGNAL( triggered() ), this, SLOT( close()          ) );
	connect( ui->action_show_menubar, SIGNAL( triggered() ), this, SLOT( toggleMenubar()  ) );
	connect( ui->action_fullscreen,   SIGNAL( triggered() ), this, SLOT( showFullscreen() ) );
	connect( ui->action_online_wiki,  SIGNAL( triggered() ), this, SLOT( openOnlineHelp() ) );
	connect( ui->action_crop_all,     SIGNAL( triggered() ), this, SLOT( crop_all() ) );
	ui->action_show_menubar->setChecked( settings.value( "show_menubar", true ).toBool() );
	toggleMenubar();
	
	connect( ui->action_movement_graph, &QAction::triggered, [&](){ new MovementGraph( images ); } );
	
	
	//Add images
	//qRegisterMetaType<QList<QUrl> >( "QStringList" );
	connect( this, SIGNAL( urls_retrived(QStringList) ), this, SLOT( process_urls(QStringList) ), Qt::QueuedConnection );

	//Refresh info labels
	refresh_text();
	
	//Init files model
	ui->files_view->setModel( &img_model );
	ui->files_view->setColumnWidth( 0, 120 );
	connect( ui->files_view->selectionModel(), &QItemSelectionModel::selectionChanged
		,	this, &main_widget::browserChangeImage );
	connect( ui->btn_add_group,    SIGNAL(clicked()), this, SLOT(addGroup()) );
	connect( ui->btn_delete_files, SIGNAL(clicked()), this, SLOT(removeFiles()) );
	
	connect( ui->selection_selector, SIGNAL(activated(int)), this, SLOT(updateSelection()) );
	
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
	//TODO: fix imageViewer, so it cleans itself up!
	browser.change_image( nullptr );
	viewer.change_image( nullptr );
	delete ui;
}


void main_widget::dragEnterEvent( QDragEnterEvent *event ){
	if( event->mimeData()->hasUrls() )
		event->acceptProposedAction();
}
void main_widget::dropEvent( QDropEvent *event ){
	if( event->mimeData()->hasUrls() ){
		event->setDropAction( Qt::CopyAction );
		event->accept();
		
		QStringList files;
		for( auto url : event->mimeData()->urls() )
			files << url.toLocalFile();
		emit urls_retrived( files );
	}
}
void main_widget::closeEvent( QCloseEvent *event ){
	settings.setValue( "window_position", saveGeometry() );
	QWidget::closeEvent( event );
}


void main_widget::process_urls( QStringList files ){
	//QProgressDialog progress( tr("Loading images"), tr("Stop"), 0, files.count(), this );
	//progress.setWindowModality( Qt::WindowModal );
	//TODO: 
	ImageLoader::loadImages( files, images, detelecine, alpha_mask );
	
	clear_cache();
	refresh_text();
	update_draw();
	update();
	ui->files_view->reset();
}


void main_widget::refresh_text(){
	auto s = getAlignedImages().size().size;
	ui->lbl_info->setText(
			tr( "Size: " )
		+	QString::number(s.width()) + "x"
		+	QString::number(s.height()) + " ("
		+	QString::number( getAlignedImages().count() ) + ")"
	);
}

static color_type color_from_spinbox( QSpinBox* spinbox ){
	return color::fromDouble( spinbox->value() / (double)spinbox->maximum() );
}

static ScalingFunction translateScaling( int id ){
	switch( id ){
		case 0:  return ScalingFunction::SCALE_NEAREST;
		case 1:  return ScalingFunction::SCALE_LINEAR;
		case 2:  return ScalingFunction::SCALE_MITCHELL;
		case 3:  return ScalingFunction::SCALE_LANCZOS;
		default: return ScalingFunction::SCALE_NEAREST;
	}
}

const ImageEx& main_widget::postProcess( const ImageEx& input, bool new_image ){
	pipe_scaling.setWidth( ui->dsbx_scale_width->value() );
	pipe_scaling.setHeight( ui->dsbx_scale_height->value() );
	pipe_scaling.setScaling( translateScaling( ui->post_scaling->currentIndex() ) );
	
	
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


std::unique_ptr<ARender> main_widget::getRender() const{
	using namespace std;
	//Select filter
	bool chroma_upscale = ui->cbx_chroma->isChecked();
	
	if( ui->rbtn_static_diff->isChecked() )
		return make_unique<DiffRender>();
	else if( ui->rbtn_subpixel->isChecked() )
		return make_unique<EstimatorRender>( ui->merge_scale->value() );
	else if( ui->rbtn_diff->isChecked() )
		return make_unique<StatisticsRender>( Statistics::DIFFERENCE );
	else if( ui->rbtn_dehumidifier->isChecked() )
		return make_unique<StatisticsRender>( Statistics::MIN );
	else if( ui->rbtn_median->isChecked() )
		return make_unique<StatisticsRender>( Statistics::MEDIAN );
	else if( ui->rbtn_pixelator->isChecked() )
		return make_unique<PixelatorRender>();
	else
		return make_unique<AverageRender>( chroma_upscale );
}

ImageEx main_widget::renderImage( const AContainer& container ){
	DialogWatcher watcher( this, "Rendering" );
	
	return getRender()->render( container, &watcher );
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

void main_widget::refreshQImageCache(){
	if( renders.size() == 0 )
		return;
	
	//Render images and calculate offsets first
	//TODO: this is because the offset requires pipe_scaling to be updated, which is done by qrenderImage(). Improve?
	for( auto& render : renders ){
		render.qimg = qrenderImage( render.raw );
		render.qoffset = render.offset * pipe_scaling.getSize();
	}
	
	//Calculate full size, based on the qimage
	auto min_point = renders[0].qoffset;
	auto max_point = min_point;
	for( auto& render : renders ){
		min_point = min_point.min( render.qoffset );
		max_point = max_point.max( render.qoffset + Point<int>( render.qimg.size() ) );
	}
	auto full_size = max_point - min_point;
	
	//Expand the qimage with transparent areas, in order to have the same size, keeping them aligned with the offset
	for( auto& render : renders ){
		QImage current( full_size.width(), full_size.height(), QImage::Format_ARGB32 );
		current.fill( qRgba( 0,0,0,0 ) );
		
		QPainter painter( &current );
		painter.drawImage( render.qoffset.x, render.qoffset.y, render.qimg );
		
		render.qimg = current;
	}
}

imageCache* main_widget::createViewerCache() const{
	//TODO: proper frame timings
	auto cache = new imageCache();
	cache->set_info( renders.size(), renders.size() > 1, -1 );
	for( auto& render : renders )
		cache->add_frame( render.qimg, 1000*3/25 ); //3 frame animation delay, with 25 frames a second
	
	cache->set_fully_loaded();
	return cache;
}


void main_widget::refresh_image(){
	if( !images.isAligned() )
		alignImage();
	
	auto start = getAlignedImages().minPoint();
	if( renders.size() == 0 ){
		auto frames = getAlignedImages().getFrames();
		renders.reserve( frames.size() );
		
		auto render = getRender();
		
		if( frames.size() == 1 )
			renders.emplace_back( renderImage( getAlignedImages() ), Point<double>(0.0,0.0) );
		else{
			//TODO: watcher
			AnimRender anim( getAlignedImages(), *render );
			for( auto& frame : frames ){
				FrameContainer current( getAlignedImages(), frame ); //TODO: remove requirement of this!
				auto img = anim.render( frame );
				if( img.is_valid() )
					renders.emplace_back( std::move(img), current.minPoint()-start );
			}
		}
	}
	
	refreshQImageCache();
	
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
	for( auto& render : renders ){
		QString filename = getSavePath( tr("Save image"), tr("PNG files (*.png);; dump files (*.dump)") );
		if( !filename.isEmpty() ){
			if( QFileInfo( filename ).suffix() == "dump" )
				DumpSaver( postProcess( render.raw, true ), filename ).exec(); //TODO: fix postProcess
			else
				render.qimg.save( filename );
		}
	}
}

void main_widget::save_files(){
	auto filename = getSavePath( tr("Save alignment"), tr("XML (*.xml.overmix)") );
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
	
	selection = nullptr;
	ui->selection_selector->setCurrentIndex( 0 );
	
	refresh_text();
	update_draw();
}

static void alignContainer( AContainer& container, int merge_index, AImageAligner::AlignMethod method, int scale, double movement, bool edges, DialogWatcher& watcher ){
	AImageAligner* aligner = nullptr;
	
	switch( merge_index ){
		case 0: //Fake
			aligner = new FakeAligner( container ); break;
		case 1: //Ordered
			aligner = new AverageAligner( container, method, scale ); break;
		case 2: //Recursive
			aligner = new RecursiveAligner( container, method, scale ); break;
		case 3: //Layered
			aligner = new LayeredAligner( container, method, scale ); break;
		case 4: //Separate Frames
			aligner = new AnimationSeparator( container, method, scale ); break;
		case 5: //Align Frames
			aligner = new FrameAligner( container, method, scale ); break;
		case 6: //Linear Curve Fitting
			aligner = new LinearAligner( container, method ); break;
		case 7: //SuperResolution alignment
			aligner = new SuperResAligner( container, method, scale ); break;
		default: return;
	}
	
	aligner->set_movement( movement );
	
	aligner->set_edges( edges );
	
	aligner->addImages();
	aligner->align( &watcher );
	
	delete aligner; //TODO: fix this

}

void main_widget::alignImage(){
	clear_cache(); //Prevent any animation from running
	DialogWatcher watcher( this, "Aligning" );
	
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
	
	auto merge_index = ui->merge_method->currentIndex();
	auto scale = ui->merge_scale->value();
	auto movement = ui->merge_movement->value() / 100.0;
	auto edges = ui->cbx_edges->isChecked();
	
	if( ui->cbx_each_frame->isChecked() ){
		//TODO: support displaying several frames in watcher
		for( auto frame : getAlignedImages().getFrames() ){
			FrameContainer container( getAlignedImages(), frame );
			alignContainer( container, merge_index, method, scale, movement, edges, watcher );
		}
	}
	else
		alignContainer( getAlignedImages(), merge_index, method, scale, movement, edges, watcher );
	
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
		alpha_mask = images.addMask( std::move( ImageEx::fromFile( filename )[0] ) );
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
		auto& aligner = getAlignedImages();
		for( unsigned i=0; i<aligner.count(); i++ )
			aligner.setMask( i, alpha_mask );
	}
}

void main_widget::update_draw(){
	ui->btn_refresh->setText( images.isAligned() ? tr( "Draw" ) : tr( "Align&&Draw" ) );
	ui->btn_refresh->setEnabled( images.count() > 0 );
}


void main_widget::addGroup(){
	auto name = QInputDialog::getText( this, tr("New group"), tr("Enter group name") );
	if( !name.isEmpty() ){
		const auto& indexes = ui->files_view->selectionModel()->selectedIndexes();
		if( indexes.size() > 0 )
			img_model.addGroup( name, indexes.front(), indexes.back() );
		else
			images.addGroup( name );
		ui->files_view->reset();
	}
}

static QImage fromSelection( const ImagesModel& model, const QModelIndexList& indexes ){
	if( indexes.size() > 0 )
		return model.getImage( indexes.front() );
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
	
	process_urls( QFileDialog::getOpenFileNames( this, "Select images", "", "Images (" + filter + ")" ) );
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
	return selection ? *selection : images;
}

void main_widget::applyModifications(){
	//Crop
	auto left   = ui->crop_left  ->value();
	auto top    = ui->crop_top   ->value();
	auto right  = ui->crop_right ->value();
	auto bottom = ui->crop_bottom->value();
	
	//Deconvolve
	double   deviation      = ui->pre_deconvolve_deviation ->value();
	unsigned dev_iterations = ui->pre_deconvolve_iterations->value();
	
	//Scale
	auto scale_method = translateScaling( ui->pre_scale_method->currentIndex() );
	Point<double> scale( ui->pre_scale_width->value(), ui->pre_scale_height->value() );
	
	auto& container = getAlignedImages();
	DialogWatcher( this, "Applying modifications" ).loopAll( container.count(), [&]( int i ){
			if( deviation > 0.0009 && dev_iterations > 0 )
				container.imageRef( i ).apply( &Plane::deconvolve_rl, deviation, dev_iterations );
			
			container.cropImage( i, left, top, right, bottom );
			container.scaleImage( i, scale, scale_method );
			
			if( ui->convert_rgb->isChecked() )
				container.imageRef(i) = container.imageRef(i).toRgb();
			else if( ui->convert_devlc->isChecked() )
				container.imageRef(i) = deVlcImage( container.imageRef(i) );
		} );
	
	clear_cache();
}

void main_widget::updateSelection(){
	//TODO: use make_unique
	switch( ui->selection_selector->currentIndex() ){
		case 1: {
				auto group_count = images.groupAmount();
				bool ok;
				auto group = QInputDialog::getInt( this, tr( "Select group" ), tr( "Select the group number" )
					,	0, 0, group_count-1, 1, &ok );
				if( ok )
					selection = std::unique_ptr<AContainer>( new DelegatedContainer( images.getGroup( group ) ) );
			} break;
		case 2: { //Select frame
				auto frames = images.getFrames();
				auto frame_count = frames.size();
				bool ok;
				auto frame = QInputDialog::getInt( this, tr( "Select frame" ), tr( "Select the frame number" )
					,	0, 0, frame_count-1, 1, &ok );
				if( ok )
					selection = std::unique_ptr<AContainer>( new FrameContainer( images, frames[frame] ) );
			} break;
			
		case 3: QMessageBox::warning( this, tr("Not implemented"), tr("Custom selection not yet implemented") );
			//TODO: implement this obviously
		case 0:
		default: selection = nullptr; break;
	}
	
	clear_cache();
}

void main_widget::crop_all(){
	debug::output_rectable( images,
		{{  QInputDialog::getInt( this, tr("Pick area"), tr("x") )
		 ,  QInputDialog::getInt( this, tr("Pick area"), tr("y") )
		 }
		,{  QInputDialog::getInt( this, tr("Pick area"), tr("width") )
		 ,  QInputDialog::getInt( this, tr("Pick area"), tr("height") )
		 }
	});
	
	clear_cache();
}
