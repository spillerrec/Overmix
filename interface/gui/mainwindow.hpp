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
#include <QItemSelection>
#include <QSettings>
#include <QUrl>

#include <memory>
#include <vector>

#include "planes/ImageEx.hpp"
#include "viewer/imageViewer.h"

#include "ImagesModel.hpp"
#include "MaskModel.hpp"

#include "Deteleciner.hpp"

#include "configs/AlignerConfigs.hpp"
#include "configs/ComparatorConfigs.hpp"
#include "configs/RenderConfigs.hpp"

class QGroupBox;
class imageCache;

namespace Overmix{

class ARender;
class AContainer;
class AImageAligner;
class Deteleciner;
class ImageContainer;

class main_widget: public QMainWindow{
	Q_OBJECT
	
	private:
		class Ui_main_widget *ui;
		
		QSettings settings;
		QString save_dir;
		
		imageViewer viewer;
		imageViewer browser;
		ImageContainer& images;
		std::unique_ptr<AContainer> selection;
		
		AlignerConfigChooser       aligner_config;
		ComparatorConfigChooser comparator_config;
		RenderConfigChooser         render_config;
		
		struct RenderCache{
			ImageEx raw;
			QImage qimg;
			Point<double> offset;
			Point<double> qoffset;
			
			RenderCache( ImageEx raw, Point<double> offset ) : raw(raw), offset(offset) { }
		};
		std::vector<RenderCache> renders;
		
		Deteleciner detelecine;
		
		int alpha_mask{ -1 };
		
		ImagesModel img_model;
		MaskModel mask_model;
		
		void clear_cache();
		
		
	private:
		ImageEx processor_cache;
		class ProcessorList* processor_list;
		
		std::unique_ptr<ARender> getRender() const;
		ImageEx renderImage( const AContainer& container );
		QImage qrenderImage( const ImageEx& img );
		
		AContainer& getAlignedImages();
		
		void refreshQImageCache();
		imageCache* createViewerCache() const;
	
	public:
		explicit main_widget( ImageContainer& images );
		~main_widget();
	
	protected:
		void dragEnterEvent( QDragEnterEvent *event );
		void dropEvent( QDropEvent *event );
		void closeEvent( QCloseEvent *event );
		
		void make_slide();
		
		const ImageEx& postProcess( const ImageEx& input, bool new_image=true );
		
		QString getSavePath( QString title, QString file_types );
	
	signals:
		void urls_retrived( QStringList files );
		
	private slots:
		void resetImage(){ renders.clear(); }
		void update_draw();
	
	public slots:
		void process_urls( QStringList files );
		void showFullscreen();
		void makeViewerBlack();
		void toggleMenubar();
		void openOnlineHelp();
		
		void applyModifications();
		
	private slots:
		void refresh_text();
		void refresh_image();
		void open_image();
		void save_image();
		void save_files();
		void clear_image();
		void alignImage();
		void change_interlace();
		
		void set_alpha_mask();
		void clear_mask();
		void use_current_as_mask();
		void updateSelection();
		
		void updateComparator();
		void updateRender();
		void updateMasks();
		
	//Related to the Files tab
		void addGroup();
		void removeFiles();
		void browserClickImage( const QModelIndex &index );
		void browserClickMask(   const QModelIndex &index );
		void browserChangeImage( const QItemSelection& selected, const QItemSelection& deselected );
		void browserChangeMask(  const QItemSelection& selected, const QItemSelection& deselected );
		
	//Related to the Tools tab
		void crop_all();
		void create_slide();
};

}

#endif
