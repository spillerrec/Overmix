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
}

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
	Progress progress( "Animator render", crops.size(), watcher );
	for(size_t i=0; i<crops.size(); i++){
		auto crop = crops[i];
		QString filename = QString::number(i);//TODO: filename
		ImageEx copy = img;
		copy.crop(crop.pos, crop.size); //TODO: Support floating point crop
		if( ui->enable_censor->isChecked() )
			pixelate(copy, crop.pos, censor_pos->getValue(), censor_size->getValue().to<double>(), censor_pixel_size->getValue());
		container.addImage(std::move(copy), -1, -1, filename);
		container.setPos(i, crop.pos);
		progress.add();
	}
	container.setAligned();
}

