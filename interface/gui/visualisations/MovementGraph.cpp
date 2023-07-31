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

#include "qcustomplot/qcustomplot.h"
#include <QHBoxLayout>

using namespace Overmix;

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

class Line{
	private:
		QVector<double> x, y;
		
	public:
		void add( double x_pos, double y_pos ){
			x << x_pos;
			y << y_pos;
		}
		void addToPlot( QCustomPlot& plot, QColor color ){
			auto curve = new QCPCurve( plot.xAxis, plot.yAxis );
			curve->setData( x, y );
			curve->setPen( { color } );
		}
};

static void addPoint( Line& line, FrameContainer& container, int index, Point<bool> moves ){
	auto pos = container.pos( index );
	auto real_index = container.realIndex( index );
	
	line.add(
			moves.x ? pos.x : real_index
		,	moves.y ? pos.y : real_index
		);
}

static void setLimit( QCustomPlot& plot, const AContainer& images, Point<bool> moves ){
	auto min_point = images.minPoint();
	auto max_point = images.maxPoint();
	
	plot.xAxis->setRange( moves.x ? min_point.x : 0, moves.x ? max_point.x : images.count() );
	plot.yAxis->setRange( moves.y ? min_point.y : 0, moves.y ? max_point.y : images.count() );
	
	plot.yAxis->setLabel( moves.y ? QObject::tr("Y") : QObject::tr("Id") );
	plot.xAxis->setLabel( moves.y ? QObject::tr("X") : QObject::tr("Id") );
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
	auto plot = new QCustomPlot( this );
	setLayout( new QHBoxLayout( this ) );
	layout()->addWidget( plot );
	layout()->setContentsMargins( 0, 0, 0, 0 );
	
	auto moves = imagesMoves( images );
	
	for( auto frame : images.getFrames() ){
		Line line;
		FrameContainer container( images, frame );
		for( unsigned i=0; i<container.count(); ++i )
			addPoint( line, container, i, moves );
		line.addToPlot( *plot, getColor( frame ) );
	}
	
	setLimit( *plot, images, moves );
	plot->setInteractions( QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables );
	
	resize( 640, 480 );
	show();
}
