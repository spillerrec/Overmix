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


#include "AnimatorUI.hpp"
#include "ui_AnimatorUI.h"

#include "../viewer/imageViewer.h"
#include "../viewer/imageCache.h"

#include "../Spinbox2D.hpp"
#include <containers/ImageContainer.hpp>
#include <renders/AverageRender.hpp>
#include <utils/AProcessWatcher.hpp>
#include <color.hpp>

#include <QSettings>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSpinBox>
#include <QPainter>
#include <QFileDialog>
#include <QMessageBox>

using namespace Overmix;

//From Animator.cpp
void pixelate( ImageEx& img, Point<double> offset, Point<double> pos, Size<double> view_size, Size<int> pixel_size );

AnimatorUI::~AnimatorUI(){ delete ui; }

AnimatorUI::AnimatorUI( QSettings& settings, ImageEx img, QWidget* parent )
	:	QDialog( parent ), ui(new Ui::AnimatorUI), viewer(new imageViewer(settings, this)), img(img)
{
    ui->setupUi(this);
	
	auto setRangeToImageSize = [&](Spinbox2D* box){
		//TODO: set each box limit individually
		int max_size = std::max(img.get_width(), img.get_height());
		box->call(&QSpinBox::setRange, 0, (int)max_size );
	};
	
	movement = new DoubleSpinbox2D( this );
	size = new Spinbox2D( this );
	movement->call( &QDoubleSpinBox::setRange, -9999.0, 9999.0 );
	setRangeToImageSize(size);
	ui->movement_layout->addRow(tr("Movement"), movement);
	ui->movement_layout->addRow(tr("Size"), size);
	
	censor_pos = new Spinbox2D( this );
	censor_size = new Spinbox2D( this );
	censor_pixel_size = new Spinbox2D( this );
	censor_pixel_size->call(&QSpinBox::setRange, 0, 200 );
	setRangeToImageSize(censor_pos);
	setRangeToImageSize(censor_size);
	ui->censor_layout->addRow(tr("Position"), censor_pos);
	ui->censor_layout->addRow(tr("Area size"), censor_size);
	ui->censor_layout->addRow(tr("Pixel size"), censor_pixel_size);
	
	viewer->setSizePolicy( QSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding ) );
	viewer->setMinimumSize( {480, 360} );
	ui->mainLayout->addWidget(viewer);
	
	connect( ui->buttons, SIGNAL(accepted()), this, SLOT(accept()) );
	connect( ui->buttons, SIGNAL(rejected()), this, SLOT(reject()) );
	connect( ui->enable_censor, SIGNAL(clicked(bool)), this, SLOT(update_preview()) );
	movement->connectToChanges( this, SLOT( update_preview() ) );
	size    ->connectToChanges( this, SLOT( update_preview() ) );
	censor_pos       ->connectToChanges( this, SLOT( update_preview() ) );
	censor_size      ->connectToChanges( this, SLOT( update_preview() ) );
	censor_pixel_size->connectToChanges( this, SLOT( update_preview() ) );
	update_preview();
	
	
	connect( ui->add_overlay_btn, &QPushButton::clicked, [&](bool){
		QString image_path = QFileDialog::getOpenFileName( this, tr("Open overlay image for slide"), "", tr("PNG files (*.png)") );
		if( !overlay.read_file(image_path) )
		{
			QMessageBox::warning( this, tr("Load error"), tr("Could not open file as an image") );
			return;
		}
	});
}

bool AnimatorUI::isPixilated() const{ return ui->enable_censor->isChecked(); }
Point<double> AnimatorUI::getSkip() const { return censor_pixel_size->getValue() - 1.0; }
Point<double> AnimatorUI::getOffset() const { return getSkip() / 2; }

std::vector<Rectangle<double>> AnimatorUI::getCrops(){
	auto s = size->getValue();
	auto move = movement->getValue();
	if (s.width() <= 0 || s.height() <= 0)
		return {}; //Must have size
	if (move.x == 0.0 && move.y == 0.0)
		return {}; //Must have a direction
	
	Point<double> pos {0.0, 0.0};
	//Start at the oposite corner from movement
	if( move.x < 0 )
		pos.x = img.get_width() - s.width();
	if( move.y < 0 )
		pos.y = img.get_height() - s.height();
	
	Rectangle<double> rect( pos, s );
	Rectangle<double> area( {0.0, 0.0}, img.getSize().to<double>() );
	
	std::vector<Rectangle<double>> crops;
	while( area.contains(rect) ){
		crops.push_back( rect );
		rect.pos += move;
	}
	
	return crops;
}

std::vector<Rectangle<double>> AnimatorUI::getCropsOverlay(){
	auto main_crops = getCrops();
	if( main_crops.size() == 0 || !overlay ){
		qDebug("No overlay image!");
		return {};
	}
	
	auto size = main_crops.at(0).size;
	auto needed_movement = (overlay.getSize() - size).max(Point<int>(0, 0));
	qDebug("needed_movement: %d x %d", needed_movement.x, needed_movement.y);
	auto movement_per_frame = needed_movement.to<double>() / main_crops.size();
	qDebug("movement_per_frame: %f x %f", movement_per_frame.x, movement_per_frame.y);
	std::vector<Rectangle<double>> crops;
	
	auto crop_size = overlay.getSize().min( size );
	for( size_t i=0; i<main_crops.size(); i++ ){
		auto pos = (movement_per_frame * (double)i).floor();
		
		crops.push_back( {pos, crop_size.to<double>()} );
	}
	
	return crops;
}


void AnimatorUI::update_preview(){
	QImage preview;
	if( ui->enable_censor->isChecked() )
	{
		auto copy = img;
		pixelate(copy, {0,0}, censor_pos->getValue(), censor_size->getValue().to<double>(), censor_pixel_size->getValue());
		preview = copy.to_qimage();
	}
	else
		preview = img.to_qimage();
	
	//Draw rects based on crops
	QPainter painter(&preview);
	painter.setPen(Qt::red);
	getCrops();
	for(auto crop : getCrops())
		painter.drawRect(crop.pos.x, crop.pos.y, crop.size.width(), crop.size.height());
	
	viewer->change_image( std::make_shared<imageCache>( preview ) );
}

void AnimatorUI::render(ImageContainer& container, AProcessWatcher* watcher){
	auto crops = getCrops();
	auto cropsOverlay = getCropsOverlay();
	Progress progress( "Animator render", crops.size(), watcher );
	for(size_t i=0; i<crops.size(); i++){
		auto crop = crops[i];
		QString filename = QString::number(i);//TODO: filename
		ImageEx copy = img;
		copy.crop(crop.pos, crop.size); //TODO: Support floating point crop
		if( ui->enable_censor->isChecked() )
			pixelate(copy, crop.pos, censor_pos->getValue(), censor_size->getValue().to<double>(), censor_pixel_size->getValue());
		
		if( overlay ){
			ImageEx copyOverlay = overlay;
			auto overlay_crop = cropsOverlay.at(i);
			qDebug("Overlay at %fx%f, %fx%f", overlay_crop.pos.x, overlay_crop.pos.y, overlay_crop.size.x, overlay_crop.size.y);
			copyOverlay.crop(overlay_crop.pos, overlay_crop.size);
			
			//TODO: alpha?
			for( size_t c=0; c<copy.size(); c++ )
				copy[c] = copy[c].overlay( copyOverlay[c], copyOverlay.alpha_plane() );
		}
		
		container.addImage(std::move(copy), -1, -1, filename);
		container.setPos(i, crop.pos);
		progress.add();
	}
	container.setAligned();
}

