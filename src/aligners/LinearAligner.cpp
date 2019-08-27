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


#include "LinearAligner.hpp"
#include "../containers/AContainer.hpp"

using namespace Overmix;

struct LinearFunc{
	double x1 = 0.0, x2 = 0.0, xy = 0.0, y1 = 0.0;
	unsigned n = 0;
	
	void add( double x, double y ){
		x1 += x;
		x2 += x*x;
		xy += x*y;
		y1 += y;
		n++;
	}
	
	double dev() const{ return n * x2 - x1 * x1; }
	double a() const{ return 1.0 / dev() * (  n*xy - x1*y1 ); }
	double b() const{ return 1.0 / dev() * ( x2*y1 - x1*xy ); }
	
	double operator()( double x ) const{ return a() * x + b(); }
};

void LinearAligner::align( AContainer& container, AProcessWatcher* watcher ) const {
	/*if( frame_adjust ){
		auto frames = container.getFrames();
		std::vector<LinearFunc> hor( frames.size() ), ver( frames.size() );
		
		//TODO: Translate the number to an index into frames
		auto frame_lookup = [&]( int frame_id )
			{ return frame_id; };
		
		for( int i=0; i<frames.size(); i++ ){
			auto id = frame_lookup( container.frame(i) );
			hor[id].add( i, container.pos(i).x );
			ver[id].add( i, container.pos(i).y );
		}
		
		//Calculate the average of 'b' for all frame ids
		double avg_ver = 0.0, avg_hor = 0.0;
		for( auto& h : hor )
			avg_hor += h.b() / hor.size();
		for( auto& v : ver )
			avg_ver += v.b() / ver.size();
		//TODO: Consider accumulate version?
		
		//Offset the images
		for( unsigned i=0; i<container.count(); i++ ){
			//Calculate the offset
			auto pos = container.pos( i );
			auto frame_id = frame_lookup( container.frame( i ) );
			
			auto offset_h = avg_hor - hor[frame_id].b();
			auto offset_v = avg_ver - ver[frame_id].b();
			
			//Apply the offset
			switch( method ){
				case AlignMethod::BOTH: pos += { offset_h, offset_v }; break;
				case AlignMethod::VER:  pos += { 0       , offset_v }; break;
				case AlignMethod::HOR:  pos += { offset_h, 0        }; break;
			};
			container.setPos( i, pos );
		}
	}
	else*/{
		LinearFunc hor, ver;//, both;
		for( unsigned i=0; i<container.count(); i++ ){
			hor.add( i, container.pos(i).x );
			ver.add( i, container.pos(i).y );
			//both.add( pos(i).x, pos(i).y );
		}
		
		for( unsigned i=0; i<container.count(); i++ ){
			switch( method ){
				case AlignMethod::BOTH: container.setPos( i, { hor(i), ver(i) } ); break;
				case AlignMethod::VER:  container.setPos( i, { 0, ver(i) } ); break;
				case AlignMethod::HOR:  container.setPos( i, { hor(i), 0 } ); break;
			};
		}
	}
}

