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

#ifndef RECURSIVE_ALIGNER_HPP
#define RECURSIVE_ALIGNER_HPP

#include "AImageAligner.hpp"
#include <utility>

class RecursiveAligner : public AImageAligner{
	protected:
		QPointF min_point() const;
		bool use_edges{ false };
		
		std::pair<Plane*,QPointF> combine( const Plane& first, const Plane& second ) const;
		Plane* align( AProcessWatcher* watcher, unsigned begin, unsigned end );
	public:
		RecursiveAligner( AlignMethod method, double scale=1.0 ) : AImageAligner( method, scale ){ }
		virtual void align( AProcessWatcher* watcher=nullptr ) override;
		
		virtual Plane prepare_plane( const Plane& p ) override;
		void set_edges( bool enabled ){ use_edges = enabled; }
};

#endif