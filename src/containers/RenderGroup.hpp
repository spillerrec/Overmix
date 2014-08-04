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

#ifndef RENDER_GROUP_HPP
#define RENDER_GROUP_HPP

#include "ImageContainer.hpp"
#include "../aligners/AImageAligner.hpp"

#include <QRect>

class RenderGroup{
	private:
		const AImageAligner& aligner;
		
	public:
		//TODO: init from AlignGroup
		RenderGroup( const AImageAligner& aligner ) : aligner(aligner) { }//NOTE: Temporary
		
		unsigned count() const;
		const ImageEx& image( unsigned index ) const;
		const Plane& plane( unsigned img_index, unsigned p_index=0 ) const;
		QPointF pos( unsigned index ) const;
		
		QPointF minPoint() const;
		QRect size() const;
};

#endif