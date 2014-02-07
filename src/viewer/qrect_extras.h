/*
	This file is part of imgviewer.

	imgviewer is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	imgviewer is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with imgviewer.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef QRECT_EXTRAS_H
#define QRECT_EXTRAS_H

#include <QRect>

QRect constrain( QRect outer, QRect inner, bool keep_aspect = false );
QPoint contrain_point( QRect outer, QPoint inner );
inline int qsize_area( QSize a ){ return a.height() * a.width(); }


inline QPoint toQPoint( const QSize t1 ){
	return QPoint( t1.width(), t1.height() );
}
inline QPointF toQPointF( const QSize t1 ){
	return QPointF( toQPoint( t1 ) );
}

inline QPoint operator*( const QPoint& t1, const QPoint& t2 ){
	return QPoint( t1.x() * t2.x(), t1.y() * t2.y() );
}
inline QPoint operator/( const QPoint& t1, const QPoint& t2 ){
	return QPoint( t1.x() / t2.x(), t1.y() / t2.y() );
}
inline QPointF operator*( const QPointF& t1, const QPointF& t2 ){
	return QPointF( t1.x() * t2.x(), t1.y() * t2.y() );
}
inline QPointF operator/( const QPointF& t1, const QPointF& t2 ){
	return QPointF( t1.x() / t2.x(), t1.y() / t2.y() );
}
inline QPointF operator*( const QPointF& t1, const QPoint& t2 ){
	return QPointF( t1.x() * t2.x(), t1.y() * t2.y() );
}
inline QPointF operator/( const QPointF& t1, const QPoint& t2 ){
	return QPointF( t1.x() / t2.x(), t1.y() / t2.y() );
}
inline QPointF operator*( const QPointF& t1, const QSize& t2 ){
	return QPointF( t1.x() * t2.width(), t1.y() * t2.height() );
}
inline QPointF operator/( const QPointF& t1, const QSize& t2 ){
	return QPointF( t1.x() / t2.width(), t1.y() / t2.height() );
}

#endif