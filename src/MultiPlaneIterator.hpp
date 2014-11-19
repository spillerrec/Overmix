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

#ifndef MULTI_PLANE_ITERATOR_H
#define MULTI_PLANE_ITERATOR_H

#include <vector>
#include <utility>
#include <QImage>
#include <QtConcurrent>

#include "planes/Plane.hpp"
#include "color.hpp"
#include "renders/ARender.hpp"

struct PlaneItInfo{
	Plane& p;
	int x, y;
	color_type* row_start{ nullptr };
	color_type* row{ nullptr };
	
	PlaneItInfo( Plane& p, Point<> pos={0,0} ) : p(p), x(pos.x), y(pos.y) { }
	
	bool check_x( int x )
		{ return x >= 0 && (unsigned)x < p.get_width(); }
	bool check_y( int y )
		{ return y >= 0 && (unsigned)y < p.get_height(); }
	
	int right()  const{ return x + p.get_width()  - 1; }
	int bottom() const{ return y + p.get_height() - 1; }
};

class PlaneLine{
	private:
		color_type* start;
		color_type* end;
		color_type* current;
		
	public:
		PlaneLine( color_type* start, color_type* end, color_type* current )
			:	start( start ), end( end ), current( current ) { }
		
		bool valid() const{ return current >= start && current < end; }
		void next(){ current++; }
		color_type& value() const{ return *current; }
		operator color_type&(){ return value(); }
};

class MultiPlaneLineIterator{
	public:
		int x;
		const int right;
		std::vector<PlaneLine> lines;
		void *data;
		
	public:
		MultiPlaneLineIterator(
				int y, int left, int right, const std::vector<PlaneItInfo> &infos, void *data
			);
		
		unsigned left() const{ return right - x; }
		
		void next(){
			for( PlaneLine& l : lines )
				l.next();
		}
		
		void next_2(){
			lines[0].next();
			lines[1].next();
		}
		
		void next_3(){
			lines[0].next();
			lines[1].next();
			lines[2].next();
		}
		
		void next_4(){
			lines[0].next();
			lines[1].next();
			lines[2].next();
			lines[3].next();
		}
		
		bool valid(){ return ++x <= right; }
		
		color_type& operator[]( unsigned index ) const{
			return lines[index].value();
		}
		bool valid( unsigned index ) const{
			return lines[index].valid();
		}
		unsigned size() const{ return lines.size(); }
		
		void for_all( void func( MultiPlaneLineIterator &it ) ){
			func( *this );
			while( valid() ){
				next();
				func( *this );
			}
		}
		
		template <class T>
		T for_all_combine(
				T func( MultiPlaneLineIterator &it ) 
			,	T comb( T t1, T t2 )
			){
			T t = func( *this );
			while( valid() ){
				next();
				t = comb( t, func( *this ) );
			}
			return t;
		}
		
		
		color_type diff( unsigned i1, unsigned i2 ){
			color_type c1 = (*this)[i1], c2 = (*this)[i2];
			return c2 > c1 ? c2-c1 : c1-c2;
		}
};

class MultiPlaneIterator{
	private:
		std::vector<PlaneItInfo> infos;
		int x;
		int y;
		int right;
		int bottom;
		int left;
		int top;
		
	private:
		void new_y( int y );
		void new_x( int x );
	
	public:
		MultiPlaneIterator( std::vector<PlaneItInfo> info ) :	infos( info ){
			x = y = left = top = 0;
			right = bottom = -1;
		}
		
		void *data;
		
		bool valid() const{ return y <= bottom && x <= right; }
		unsigned width(){ return right - left + 1; }
		unsigned height(){ return bottom - top + 1; }
		
		void iterate( int x, int y, int right, int bottom );
		bool iterate_all();
		void iterate_shared();
		
		void next_y(){ new_y( y + 1 ); }
		void next_x(){ new_x( x + 1 ); }
		void next_line(){
			next_y();
			new_x( left );
		}
		void next(){
			if( x < right )
				next_x();
			else
				next_line();
		}
		
		static std::vector<int> range( int from, int to ){
			std::vector<int> its;
			its.reserve( to - from );
			for( int i=from; i<=to; i++ )
				its.push_back( i );
			return its;
		}
		
		void for_all_lines( void func( MultiPlaneLineIterator &it ) ){
			auto its = range( top, bottom );
			QtConcurrent::blockingMap( its, [func,this](int iy){
					MultiPlaneLineIterator it( iy, this->left, this->right, this->infos, data );
					func( it );
				} );
		}
		
		void for_all_pixels( void func( MultiPlaneLineIterator &it ) ){
			auto its = range( top, bottom );
			QtConcurrent::blockingMap( its, [func,this](int iy){
					MultiPlaneLineIterator it( iy, this->left, this->right, this->infos, data );
					it.for_all( func );
				} );
		}
		
		void for_all_pixels( void func( MultiPlaneLineIterator &it ), AProcessWatcher* watcher, unsigned offset=0, unsigned count=1000 ){
			auto its = range( top, bottom );
			QFuture<void> future = QtConcurrent::map( its, [func,this](int iy){
					MultiPlaneLineIterator it( iy, this->left, this->right, this->infos, data );
					it.for_all( func );
				} );
			
			
			if( watcher )
				while( future.isRunning() ){
					watcher->setCurrent( offset + (future.progressValue() * count / future.progressMaximum()) );
					//TODO: wait
				}
			else
				future.waitForFinished();
		}
		
		template <class T>
		T for_all_pixels_combine( 
				T func( MultiPlaneLineIterator &it )
			,	T init
			,	T combine( T val1, T val2 )
			){
			typedef std::pair<int,T> Working;
			std::vector<Working> its;
			its.reserve( bottom - top );
			
			for( int iy=top; iy<=bottom; iy++ )
				its.push_back( Working( iy, init ) );
			
			QtConcurrent::blockingMap( its, [func,combine,this](Working &val){
					MultiPlaneLineIterator it( val.first, this->left, this->right, this->infos, data );
					val.second = it.for_all_combine( func, combine );
				} );
			
			for( Working w : its )
				init = combine( init, w.second );
			
			return init;
		}
		
		template <class T>
		T for_all_lines_combine( 
				T func( MultiPlaneLineIterator &it )
			,	T init
			,	T combine( T val1, T val2 )
			){
			typedef std::pair<int,T> Working;
			std::vector<Working> its;
			its.reserve( bottom - top );
			
			for( int iy=top; iy<=bottom; iy++ )
				its.push_back( Working( iy, init ) );
			
			QtConcurrent::blockingMap( its, [func,combine,this](Working &val){
					MultiPlaneLineIterator it( val.first, this->left, this->right, this->infos, data );
					val.second = func( it );
				} );
			
			for( Working w : its )
				init = combine( init, w.second );
			
			return init;
		}
		
	public:
		color_type& operator[]( const int index ) const{
			return *infos[index].row;
		}
		
		color gray() const{
			color_type g = (*this)[0];
			return color( g, g, g );
		}
		color gray_alpha() const{
			color_type g = (*this)[0];
			return color( g, g, g, (*this)[1] );
		}
		
		color pixel() const{
			return color( (*this)[0], (*this)[1], (*this)[2] );
		}
		
		color pixel_alpha() const{
			return color( (*this)[0], (*this)[1], (*this)[2], (*this)[3] );
		}
};

#endif