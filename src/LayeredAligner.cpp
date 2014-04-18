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


#include "LayeredAligner.hpp"
#include "AverageAligner.hpp"
#include "SimpleRender.hpp"
#include "DifferenceRender.hpp"

class InternAligner : public AverageAligner{
	protected:
		AImageAligner& normal;
		ImageEx* diff;
	public:
		InternAligner( AImageAligner& normal, AlignMethod method, double scale=1.0 )
			:	AverageAligner( method, scale ), normal(normal){
			diff = DifferenceRender().render( normal );
		}
		virtual Plane prepare_plane( const Plane& p ) override{
			//TODO: randomize repeating errors
			return AImageAligner::prepare_plane( p );
		}
};

void LayeredAligner::align( AProcessWatcher* watcher ){
	if( count() == 0 )
		return;
	
	AverageAligner normal( method, scale );
	for( unsigned i=0; i<count(); i++ )
		normal.add_image( image( i ) );
	normal.align();
	
	InternAligner aligner( normal, method, scale );
	for( unsigned i=0; i<count(); i++ )
		aligner.add_image( image( i ) );
	aligner.align();
	
	for( unsigned i=0; i<count(); i++ )
		setPos( i, aligner.pos(i) );
}

