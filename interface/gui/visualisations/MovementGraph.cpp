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


#include "MovementGraph.hpp"

#include "containers/FrameContainer.hpp"
#include "containers/ImageContainer.hpp"

#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QHBoxLayout>

using namespace Overmix;
using namespace QtCharts;

static Point<bool> imagesMoves( const AContainer& container ){
	if( container.count() == 0 )
		return { false, false };
	
	Point<bool> moves{ false, false };
	auto base = container.pos( 0 );
	for( unsigned i=1; i<container.count(); ++i ){
		auto current = container.pos( i );
		moves.x = moves.x || (base.x != current.x);
		moves.y = moves.y || (base.y != current.y);
	}
	
	return moves;
}

static void addPoint( QLineSeries& line, FrameContainer& container, int index, Point<bool> moves ){
	auto pos = container.pos( index );
	auto real_index = container.realIndex( index );
	
	line.append(
			moves.x ? pos.x : real_index
		,	moves.y ? pos.y : real_index
		);
}

static QColor getColor( int frame ){
	switch( frame % 6 ){
		case 0: return Qt::red;
		case 1: return Qt::green;
		case 2: return Qt::blue;
		case 3: return Qt::cyan;
		case 4: return Qt::magenta;
		case 5: return Qt::yellow;
		default: return Qt::black;
	}
}

MovementGraph::MovementGraph( ImageContainer& images ) : QWidget(nullptr), images(images) {
	//Create and add chart to widget
	auto plot = new QChart();
	auto view = new QChartView( plot );
	view->setRenderHint( QPainter::Antialiasing );
	setLayout( new QHBoxLayout( this ) );
	layout()->addWidget( view );
	layout()->setContentsMargins( 0, 0, 0, 0 );
	
	//Start adding lines
	auto moves = imagesMoves( images );
	for( auto frame : images.getFrames() ){
		auto line = new QLineSeries(); //TODO: is needed to be on the heap?
		
		FrameContainer container( images, frame );
		for( unsigned i=0; i<container.count(); ++i )
			addPoint( *line, container, i, moves );
		
		line->setColor( getColor( frame ) );
		plot->addSeries( line );
	}
	
	//Custimize chart visuals
	plot->setTitle( "Position of images" );
	plot->legend()->hide();
	plot->createDefaultAxes();
	//plot->setInteractions( QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables );
	//TOOD: replace this functionality?
	
	resize( 640, 480 );
	show();
}
