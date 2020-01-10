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

#include "ExceptionCatcher.hpp"
#include "ProgressWatcher.hpp"
#include "viewer/imageCache.h"

#include "processors/ProcessorList.hpp"
#include "FullscreenViewer.hpp"

#include "color.hpp"
#include "debug.hpp"
#include "renders/AnimRender.hpp"
#include "renders/FastRender.hpp"
#include "Deteleciner.hpp"
#include "containers/DelegatedContainer.hpp"
#include "containers/FrameContainer.hpp"
#include "containers/ImageContainer.hpp"
#include "containers/ImageContainerSaver.hpp"
#include "utils/Animator.hpp"
#include "utils/AProcessWatcher.hpp"
#include "utils/ImageLoader.hpp"
#include "utils/SRSampleCreator.hpp"

#include "savers/DumpSaver.hpp"
#include "importers/VideoImporter.hpp"
#include "visualisations/AnimatorUI.hpp"
#include "visualisations/MovementGraph.hpp"
#include "visualisations/SkipRenderPreview.hpp"
#include "Spinbox2D.hpp"

#include <vector>
#include <utility>

#include <QCoreApplication>
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
#include <QTime>
#include <QtConcurrent>
#include <QSettings>

using namespace Overmix;


void foldableGroupBox( QWidget* widget, bool enabled, QGroupBox* box ){
	//Does not work properly, see issue #118
	auto update = [=]( bool checked )
		{ box->setMaximumHeight( checked ? QWIDGETSIZE_MAX : std::max(box->layout()->geometry().top(), 20) ); };
	
	widget->connect( box, &QGroupBox::clicked, update );
	box->setCheckable( true );
	box->setChecked( enabled );
	QCoreApplication::processEvents(); //Update layout
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
	,	   aligner_config( this, true )
	,	comparator_config( this, true )
	,	    render_config( this, true )
	,	img_model( images )
	,	mask_model( images )
	,	   processor_list( new ProcessorList( this ) )
	,	preprocessor_list( new ProcessorList( this ) )
{
	ui->setupUi(this);
	   aligner_config.initialize();
	comparator_config.initialize();
	    render_config.initialize();
	ui->align_layout     ->insertWidget( 0, &   aligner_config );
	ui->comparing_layout ->insertWidget( 0, &comparator_config );
	ui->render_layout    ->insertWidget( 0, &    render_config );
	ui->postprocess_layout->addWidget(    processor_list );
	ui->preprocess_layout ->addWidget( preprocessor_list );
	
	//Buttons
	connect( ui->btn_clear,       SIGNAL( clicked() ), this, SLOT( clear_image()         ) );
	connect( ui->btn_refresh,     SIGNAL( clicked() ), this, SLOT( refresh_image()       ) );
	connect( ui->btn_save,        SIGNAL( clicked() ), this, SLOT( save_image()          ) );
	connect( ui->btn_save_files,  SIGNAL( clicked() ), this, SLOT( save_files()          ) );
	connect( ui->btn_subpixel,    SIGNAL( clicked() ), this, SLOT( alignImage()          ) );
	connect( ui->pre_alpha_mask,  SIGNAL( clicked() ), this, SLOT( set_alpha_mask()      ) );
	connect( ui->pre_clear_mask,  SIGNAL( clicked() ), this, SLOT( clear_mask()          ) );
	connect( ui->pre_remove_mask, SIGNAL( clicked() ), this, SLOT( remove_mask()         ) );
	connect( ui->btn_as_mask,     SIGNAL( clicked() ), this, SLOT( use_current_as_mask() ) );
	connect( ui->btn_apply_mods,  SIGNAL( clicked() ), this, SLOT( applyModifications()  ) );
	update_draw();
	
	//Checkboxes
	connect( ui->cbx_interlaced, SIGNAL( toggled(bool) ), this, SLOT( change_interlace() ) );
	change_interlace();
	
	//Comparing changes
	connect( &comparator_config, SIGNAL( changed() ), this, SLOT( updateComparator() ) );
	updateComparator();
	
	//Reset aligner cache
	connect( &render_config, SIGNAL( changed() ), this, SLOT( updateRender() ) );
	connect( &img_model, SIGNAL( dataChanged(const QModelIndex&, const QModelIndex&) ), this, SLOT( clearCache() ) );
	connect( &mask_model, SIGNAL( dataChanged(const QModelIndex&, const QModelIndex&) ), this, SLOT( clearCache() ) );
	
	//Menubar
	connect( ui->action_add_files,    SIGNAL( triggered() ), this, SLOT( open_image()     ) );
	connect( ui->action_save,         SIGNAL( triggered() ), this, SLOT( save_image()     ) );
	connect( ui->action_exit,         SIGNAL( triggered() ), this, SLOT( close()          ) );
	connect( ui->action_show_menubar, SIGNAL( triggered() ), this, SLOT( toggleMenubar()  ) );
	connect( ui->action_fullscreen,   SIGNAL( triggered() ), this, SLOT( showFullscreen() ) );
	connect( ui->action_make_black,   SIGNAL( triggered() ), this, SLOT( makeViewerBlack() ) );
	connect( ui->action_online_wiki,  SIGNAL( triggered() ), this, SLOT( openOnlineHelp() ) );
	connect( ui->action_crop_all,     SIGNAL( triggered() ), this, SLOT( crop_all() ) );
	connect( ui->action_create_slide,     SIGNAL( triggered() ), this, SLOT( create_slide() ) );
	connect( ui->action_show_skip_preview,     SIGNAL( triggered() ), this, SLOT( show_skip_render_preview() ) );
	ui->action_show_menubar->setChecked( settings.value( "show_menubar", true ).toBool() );
	toggleMenubar();
	
	connect( ui->action_movement_graph, &QAction::triggered, [&](){ new MovementGraph( images ); } );
	
	connect( ui->action_create_sr, &QAction::triggered, [&](){ create_sr_sample(); } );
	
	
	//Add images
	//qRegisterMetaType<QList<QUrl> >( "QStringList" );
	connect( this, SIGNAL( urls_retrived(QStringList) ), this, SLOT( process_urls(QStringList) ), Qt::QueuedConnection );
	
	//Init files model
	ui->files_view->setModel( &img_model );
	ui->files_view->setColumnWidth( 0, 120 );
	connect( ui->files_view->selectionModel(), &QItemSelectionModel::currentChanged
		,	this, &main_widget::browserChangeImage );
	connect( ui->btn_add_group,    SIGNAL(clicked()), this, SLOT(addGroup()) );
	connect( ui->btn_delete_files, SIGNAL(clicked()), this, SLOT(removeFiles()) );
	
	ui->mask_view->setModel( &mask_model );
	connect( ui->mask_view->selectionModel(), &QItemSelectionModel::currentChanged
		,	this, &main_widget::browserChangeMask );
	connect( ui->files_view, &QAbstractItemView::clicked, this, &main_widget::browserClickImage );
	connect( ui->mask_view,  &QAbstractItemView::clicked, this, &main_widget::browserClickMask  );
	
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
	
	//Groupboxes
    show(); //It needs to be showed so we know how large the Text is for resizing it
	foldableGroupBox( this, false, ui->preprocess_group  );
	foldableGroupBox( this, false, ui->comparing_group   );
	foldableGroupBox( this, true,  ui->merge_group       );
	foldableGroupBox( this, false, ui->render_group      );
	foldableGroupBox( this, false, ui->postprocess_group );
	foldableGroupBox( this, false, ui->color_group       );
	//foldableGroupBox( this, true,  ui->images_group      );
	foldableGroupBox( this, false, ui->masks_group       );
	foldableGroupBox( this, false, ui->selection_group   );

	//Refresh info labels
	clearCache();
}

main_widget::~main_widget(){
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
	ExceptionCatcher::Guard( this, [&](){
		if( files.count() == 1 && VideoImporter::supportedFile( files[0] ) ){
			VideoImporter importer( files[0], this );
			auto result = importer.exec();
			if( result == QDialog::Accepted ){
				ProgressWatcher watcher( this, tr("Loading images") );
				importer.import( images, &watcher );
			}
		}
		else{
			ProgressWatcher watcher( this, tr("Loading images") );
			ImageLoader::loadImages( files, images, detelecine, alpha_mask, &watcher );
		}
	} );
	
	clearCache();
	update_draw();
	update();
	ui->files_view->reset();
	updateMasks();
}


void main_widget::refresh_text(){
	ExceptionCatcher::Guard( this, [&](){
		auto& aligner = getAlignedImages();
		auto size       = aligner.size().size;
		auto imageCount = aligner.count();
		
		if( imageCount == 0 )
			ui->lbl_info->setText( tr( "No images" ) );
		else
			ui->lbl_info->setText(
					tr( "Size: " )
				+	QString::number( size.width() ) + "x"
				+	QString::number( size.height()) + " ("
				+	tr( "%n image(s) in ", nullptr, imageCount )
				+	tr( "%n frame(s))",    nullptr, aligner.getFrames().size() )
			);
	} );
}

const ImageEx& main_widget::postProcess( const ImageEx& input, bool new_image ){
	ExceptionCatcher::Guard( this, [&](){
		processor_cache = processor_list->process( input );
	} );
	return processor_cache;
}


//NOTE: This method is not exception protected!
std::unique_ptr<ARender> main_widget::getRender() const{
	return render_config.getRender();
}

ImageEx main_widget::renderImage( const AContainer& container ){
	return ExceptionCatcher::Guard( this, [&](){
		ProgressWatcher watcher( this, "Rendering" );
		
		return getRender()->render( container, &watcher );
	} );
}

QImage main_widget::qrenderImage( const ImageEx& img ){
	if( !img.is_valid() )
		return QImage();
	
	return ExceptionCatcher::Guard( this, [&](){
		auto image = postProcess( img, true );
		if( ui->cbx_nocolor->isChecked() )
			image = image.flatten();
		
		//Render image
		//TODO: fix postProcess
		return image.to_qimage( ui->cbx_dither->isChecked() );
	} );
}

void main_widget::refreshQImageCache(){
	if( renders.size() == 0 )
		return;
	
	//Render images and calculate offsets first
	//TODO: this is because the offset requires pipe_scaling to be updated, which is done by qrenderImage(). Improve?
	for( auto& render : renders ){
		render.qimg = qrenderImage( render.raw );
		render.qoffset = render.offset * Point<double>( 1.0, 1.0 ); //TODO: pipe_scaling.getSize();
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

std::shared_ptr<imageCache> main_widget::createViewerCache() const{
	//TODO: proper frame timings
	auto cache = std::make_shared<imageCache>();
	cache->set_info( renders.size(), renders.size() > 1, -1 );
	for( auto& render : renders )
		cache->add_frame( render.qimg, 1000*3/25 ); //3 frame animation delay, with 25 frames a second
	
	cache->set_fully_loaded();
	return cache;
}


void main_widget::refresh_image(){
	if( !images.isAligned() )
		alignImage();
	
	ExceptionCatcher::Guard( this, [&](){
		Timer t( "refresh_image" );
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
				for( int frame=0; frame<anim.count(); frame++ ){
					FrameContainer current( getAlignedImages(), frame ); //TODO: remove requirement of this!
					auto img = anim.render( frame );
					if( img.is_valid() )
						renders.emplace_back( std::move(img), current.minPoint()-start );
				}
			}
		}
	} );
	
	refreshQImageCache();
	
	ui->btn_as_mask->setEnabled( renders.size() == 1 && renders[0].raw.getColorSpace().isGray() );
	viewer.change_image( createViewerCache() );
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
	ExceptionCatcher::Guard( this, [&](){
		for( auto& render : renders ){
			QString filename = getSavePath( tr("Save image"), tr("PNG files (*.png);; PNG 16-bit (*.16bit.png);; dump files (*.dump)") );
			if( !filename.isEmpty() ){
				auto suffix = QFileInfo( filename ).completeSuffix();
				if( suffix == "" )
					filename += ".png";
				
				if( suffix == "16bit.png" )
				{
					QFile out( filename );
					if( out.open(QIODevice::WriteOnly) )
						postProcess( render.raw, true ).to_png( out ); //TODO: fix postProcess
				}
				else if( suffix == "dump" )
					DumpSaver( postProcess( render.raw, true ), filename ).exec(); //TODO: fix postProcess
				else
					render.qimg.save( filename );
			}
		}
	} );
}

void main_widget::save_files(){
	ExceptionCatcher::Guard( this, [&](){
		auto filename = getSavePath( tr("Save alignment"), tr("XML (*.xml.overmix)") );
		if( !filename.isEmpty() ){
			auto error = ImageContainerSaver::save( images, filename );
			if( !error.isEmpty() )
				QMessageBox::warning( this, tr("Could not save alignment"), error );
		}
	} );
}

void main_widget::clearCache(){
	refresh_text();
	ui->btn_save->setEnabled( false );
	renders.clear();
	
	QImage preview = FastRender().render(getAlignedImages(),nullptr).to_qimage(false);
	viewer.change_image( std::make_shared<imageCache>(preview) );
}

void main_widget::clear_image(){
	images.clear();
	clearCache();
	clear_mask();
	detelecine.clear();
	
	browser.change_image( nullptr );
	ui->files_view->reset();
	
	selection = nullptr;
	ui->selection_selector->setCurrentIndex( 0 );
	
	update_draw();
}

void main_widget::alignImage(){
	ExceptionCatcher::Guard( this, [&](){
		Timer t( "alignImage" );
		clearCache(); //Prevent any animation from running
		ProgressWatcher watcher( this, "Aligning" );
		
		auto aligner = aligner_config.getAligner();
		if( ui->cbx_each_frame->isChecked() ){
			//TODO: support displaying several frames in watcher
			for( auto frame : getAlignedImages().getFrames() ){
				FrameContainer container( getAlignedImages(), frame );
				aligner->align( container, &watcher );
			}
		}
		else
			aligner->align( getAlignedImages(), &watcher );
	} );
	
	clearCache();
	update_draw();
}

void main_widget::change_interlace(){
	detelecine.setEnabled( ui->cbx_interlaced->isChecked() );
}

void main_widget::set_alpha_mask(){
	QString filename = QFileDialog::getOpenFileName( this, tr("Open alpha mask"), save_dir, tr("PNG files (*.png)") );
	
	if( !filename.isEmpty() ){
		ExceptionCatcher::Guard( this, [&](){
			alpha_mask = images.addMask( std::move( ImageEx::fromFile( filename )[0] ) );
		} );
		updateMasks();
	}
}

void main_widget::clear_mask(){
	alpha_mask = -1;
	images.clearMasks();
	updateMasks();
}

void main_widget::use_current_as_mask(){
	ExceptionCatcher::Guard( this, [&](){
		if( renders.size() == 1 ){
			//TODO: postProcess cache no longer valid as we have several frames
			alpha_mask = images.addMask( Plane( postProcess(renders[0].raw, false)[0] ) );
			auto& aligner = getAlignedImages();
			for( unsigned i=0; i<aligner.count(); i++ )
				aligner.setMask( i, alpha_mask );
			updateMasks();
		}
	} );
}

void main_widget::update_draw(){
	ui->btn_refresh->setText( images.isAligned() ? tr( "Draw" ) : tr( "Align&&Draw" ) );
	ui->btn_refresh->setEnabled( images.count() > 0 );
}


void main_widget::addGroup(){
	auto name = QInputDialog::getText( this, tr("New group"), tr("Enter group name") );
	if( !name.isEmpty() ){
		ExceptionCatcher::Guard( this, [&](){
			const auto& indexes = ui->files_view->selectionModel()->selectedIndexes();
			if( indexes.size() > 0 )
				img_model.addGroup( name, indexes.front(), indexes.back() );
			else
				images.addGroup( name );
		} );
		ui->files_view->reset();
	}
}

void main_widget::browserClickImage( const QModelIndex &index ){
	auto img = img_model.getImage( index );
	if( !img.isNull() )
		browser.change_image( std::make_shared<imageCache>( img ) );
}

void main_widget::browserClickMask( const QModelIndex &index ){
	auto img = mask_model.getImage( index );
	if( !img.isNull() )
		browser.change_image( std::make_shared<imageCache>( img ) );
}

void main_widget::browserChangeImage( const QModelIndex& current, const QModelIndex& /*previous*/ ){
	browserClickImage( current );
}

void main_widget::browserChangeMask( const QModelIndex& current, const QModelIndex& /*previous*/ ){
	browserClickMask( current );
}

void main_widget::removeFiles(){
	ExceptionCatcher::Guard( this, [&](){
		auto indexes = ui->files_view->selectionModel()->selectedRows();
		if( indexes.size() > 0 )
			img_model.removeRows( indexes.front().row(), indexes.size(), img_model.parent(indexes.front()) );
	} );
	clearCache();
}

void main_widget::remove_mask(){
	ExceptionCatcher::Guard( this, [&](){
		auto indexes = ui->mask_view->selectionModel()->selectedRows();
		if( indexes.size() > 0 )
			mask_model.removeRows( indexes.front().row(), indexes.size(), img_model.parent(indexes.front()) );
	} );
	clearCache();
}

void main_widget::showFullscreen(){
	ExceptionCatcher::Guard( this, [&](){
		if( ui->tab_pages->currentIndex() != 0 ){
			//Show selected file
			QItemSelection empty;
			auto img = img_model.getImage( ui->files_view->selectionModel()->currentIndex() );
			if( !img.isNull() )
				FullscreenViewer::show( settings, img, this );
		}
		else if( renders.size() > 0 ) //Show current render
			FullscreenViewer::show( settings, createViewerCache(), this );
	} );
}

void main_widget::makeViewerBlack(){
	auto pal = viewer.palette();
	auto color = ui->action_make_black->isChecked() ? Qt::black : Qt::transparent;
	pal.setColor( QPalette::Background, color );
	viewer.setAutoFillBackground(true);
	viewer.setPalette( pal );
	viewer.update();
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
	ExceptionCatcher::Guard( this, [&](){
		switch( selection_type ){
			case 1: selection = std::make_unique<DelegatedContainer>( images.getGroup( selection_value ) ); break;
			case 2: selection = std::make_unique<FrameContainer>( images, images.getFrames().at(selection_value) ); break;
		}
	} );
	return selection ? *selection : images;
}

void main_widget::applyModifications(){
	ExceptionCatcher::Guard( this, [&](){
		auto& container = getAlignedImages();
		ProgressWatcher( this, "Applying modifications" ).loopAll( container.count(), [&]( int i ){
				container.setPos( i, preprocessor_list->modifyOffset( container.pos(i) ) );
				container.imageRef(i) = preprocessor_list->process( container.imageRef(i) );
			} );
	} );
	clearCache();
}

void main_widget::updateSelection(){
	auto getIndex = [&]( QString title, QString label, int max_value ){
			bool ok = false;
			auto result = QInputDialog::getInt( this, title, label, 1, 1, max_value, 1, &ok );
			if( ok )
				selection_value = result - 1;
		};
	
	ExceptionCatcher::Guard( this, [&](){
		selection_type = ui->selection_selector->currentIndex();
		switch( selection_type ){
			case 1:
					getIndex( tr( "Select group" ), tr( "Select the group number" ), images.groupAmount() );
				break;
			case 2: { //Select frame
					auto frames = images.getFrames();
					getIndex( tr( "Select frame" ), tr( "Select the frame number" ), frames.size() );
				} break;
				
			case 3: QMessageBox::warning( this, tr("Not implemented"), tr("Custom selection not yet implemented") );
				//TODO: implement this obviously
			case 0:
			default:
					selection_type = 0;
					selection = nullptr;
				break;
		}
	} );
	
	clearCache();
}

void main_widget::updateComparator(){
	images.setComparator( comparator_config.getComparator() );
}
void main_widget::updateRender(){
	clearCache();
	if( ui->render_redraw->isChecked() )
		refresh_image();
}
void main_widget::updateMasks(){
	ui->mask_view->reset();
	ui->pre_clear_mask->setEnabled( images.maskCount() > 0 );
}

void main_widget::crop_all(){
	ExceptionCatcher::Guard( this, [&](){
		debug::output_rectable( images,
			{{  QInputDialog::getInt( this, tr("Pick area"), tr("x") )
			 ,  QInputDialog::getInt( this, tr("Pick area"), tr("y") )
			 }
			,{  QInputDialog::getInt( this, tr("Pick area"), tr("width") )
			 ,  QInputDialog::getInt( this, tr("Pick area"), tr("height") )
			 }
		});
	} );
	
	clearCache();
}

void main_widget::create_slide(){
	ExceptionCatcher::Guard( this, [&](){
		QString image_path = QFileDialog::getOpenFileName( this, tr("Open image for slide"), save_dir, tr("PNG files (*.png)") );
		ImageEx img;
		if( !img.read_file(image_path) )
		{
			QMessageBox::warning( this, tr("Load error"), tr("Could not open file as an image") );
			return;
		}
		
		AnimatorUI animator(settings, img, this);
		if( animator.exec() == QDialog::Accepted )
		{
			ProgressWatcher watcher( this, "Creating images" );
			animator.render(images, &watcher);
			clearCache();
			update_draw();
			
			if( animator.isPixilated() )
				render_config.setSkipRenderConfig( animator.getSkip(), animator.getOffset() );
		}
	} );
}

void main_widget::create_sr_sample(){
	ExceptionCatcher::Guard( this, [&](){
		QString image_path = QFileDialog::getOpenFileName( this, tr("Open image for SR sample creation"), save_dir, tr("PNG files (*.png)") );
		ImageEx img;
		if( !img.read_file(image_path) )
		{
			QMessageBox::warning( this, tr("Load error"), tr("Could not open file as an image") );
			return;
		}
		
		SRSampleCreator creator;
		bool accepted = false;
		int scale = QInputDialog::getInt( this, tr("Scaling"), tr("Scaling"), 2, 2,8, 1, &accepted );
		if( accepted )
		{
			creator.scale = scale;
			creator.sample_count = scale * scale;
			creator.render(images, img);
		}
	} );
}

void main_widget::show_skip_render_preview(){
	SkipRenderPreview dialog( settings, images, this );
	dialog.setConfig( render_config.getSkipRenderSkip(), render_config.getSkipRenderOffset() );
	
	if( dialog.exec() == QDialog::Accepted )
		render_config.setSkipRenderConfig( dialog.getSkip(), dialog.getOffset() );
}

