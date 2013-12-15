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


#include "TestPlane.hpp"

#include "Plane.hpp"
#include "color.hpp"

#include <QTest>
#include <climits>

void TestPlane::testInitialization_data(){
	QTest::addColumn<unsigned>( "width" );
	QTest::addColumn<unsigned>( "height" );
	QTest::addColumn<bool>( "invalid" );
	
	QTest::newRow( "normal1" ) << 1u << 1u << false;
	QTest::newRow( "normal2" ) << 24u << 54u << false;
	
	QTest::newRow( "zero" ) << 0u << 0u << true;
	QTest::newRow( "too big" ) << UINT_MAX << UINT_MAX << true;
}

void TestPlane::testInitialization(){
	QFETCH(unsigned, width);
	QFETCH(unsigned, height);
	QFETCH(bool, invalid);
	
	Plane p( width, height );
	QCOMPARE( p.is_invalid(), invalid );
	QCOMPARE( p.get_width(), width );
	QCOMPARE( p.get_height(), height );
}


void TestPlane::testPixel_data(){
	QTest::addColumn<unsigned>( "width" );
	QTest::addColumn<unsigned>( "height" );
	
	QTest::newRow( "1x1" ) << 1u << 1u;
	QTest::newRow( "67x18" ) << 67u << 18u;
	QTest::newRow( "40x68" ) << 40u << 68u;
	QTest::newRow( "91x15" ) << 91u << 15u;
	QTest::newRow( "99x12" ) << 99u << 12u;
}
void TestPlane::testPixel(){
	QFETCH(unsigned, width);
	QFETCH(unsigned, height);
	
	Plane p( width, height );
	
	//Set all pixels
	for( unsigned iy=0; iy<height; iy++ )
		for( unsigned ix=0; ix<width; ix++ )
			p.pixel(ix,iy) = color::from_8bit( iy+ix % 256 );
	
	//Read all pixels using Plane::pixel
	for( unsigned iy=0; iy<height; iy++ )
		for( unsigned ix=0; ix<width; ix++ )
			QCOMPARE( p.pixel(ix,iy), color::from_8bit( iy+ix % 256 ) );
	
	//Read all pixels using Plane::scan_line
	for( unsigned iy=0; iy<height; iy++ ){
		color_type *row = p.scan_line( iy );
		for( unsigned ix=0; ix<width; ix++ )
			QCOMPARE( row[ix], color::from_8bit( iy+ix % 256 ) );
	}
}

void TestPlane::testFill_data(){
	QTest::addColumn<unsigned>( "width" );
	QTest::addColumn<unsigned>( "height" );
	QTest::addColumn<color_type>( "value" );
	
	QTest::newRow( "1x1 white" ) << 1u << 1u << (color_type)color::WHITE;
	QTest::newRow( "1x1 black" ) << 1u << 1u << (color_type)color::BLACK;
	
	QTest::newRow( "67x18 white" ) << 67u << 18u << (color_type)color::WHITE;
	QTest::newRow( "67x18 black" ) << 67u << 18u << (color_type)color::BLACK;
	
	QTest::newRow( "40x68 white*0.50" ) << 40u << 68u << static_cast<color_type>(color::WHITE*0.50);
	QTest::newRow( "40x68 black*0.50" ) << 40u << 68u << static_cast<color_type>(color::BLACK*0.50);
	
	QTest::newRow( "91x15 white*0.25" ) << 91u << 15u << static_cast<color_type>(color::WHITE*0.25);
	QTest::newRow( "91x15 black*0.25" ) << 91u << 15u << static_cast<color_type>(color::BLACK*0.25);
	
	QTest::newRow( "99x12 white*0.75" ) << 99u << 12u << static_cast<color_type>(color::WHITE*0.75);
	QTest::newRow( "99x12 black*0.75" ) << 99u << 12u << static_cast<color_type>(color::BLACK*0.75);
}
void TestPlane::testFill(){
	QFETCH(unsigned, width);
	QFETCH(unsigned, height);
	QFETCH(color_type, value);
	
	Plane p( width, height );
	p.fill( value );
	
	for( unsigned iy=0; iy<height; iy++ )
		for( unsigned ix=0; ix<width; ix++ )
			QCOMPARE( p.pixel(ix,iy), value );
}

