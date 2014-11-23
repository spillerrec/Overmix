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


#include "TestGeometry.hpp"

#include "Geometry.hpp"

#include <QTest>
#include <climits>

void TestGeometry::fillTestData(){
	QTest::addColumn<unsigned>( "width" );
	QTest::addColumn<unsigned>( "height" );
	
	QTest::newRow( "same" ) << 1u << 1u;
	QTest::newRow( "max" ) << UINT_MAX << UINT_MAX;
	QTest::newRow( "min" ) << 0u << 0u;
	QTest::newRow( "withMax" ) << 653u << UINT_MAX;
	QTest::newRow( "value" ) << 24u << 54u;
	QTest::newRow( "value" ) << 35u << 756u;
	QTest::newRow( "value" ) << 6458u << 2347u;
	QTest::newRow( "value" ) << 2764u << 4786u;
}

void TestGeometry::testSizeInit(){
	QFETCH(unsigned, width);
	QFETCH(unsigned, height);
	
	Size<unsigned>  p1( width, height );
	Point<unsigned> p2( width, height );
	QCOMPARE( p1.width(),  width );
	QCOMPARE( p2.width(),  width );
	QCOMPARE( p1.height(), height );
	QCOMPARE( p2.height(), height );
}


