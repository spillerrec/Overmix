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

#ifndef AVERAGE_ALIGNER_HPP
#define AVERAGE_ALIGNER_HPP

#include "AImageAligner.hpp"

namespace Overmix{

class AverageAlignerImpl : public AImageAligner{
	public:
		AverageAlignerImpl( AContainer& container, AlignMethod method, double scale=1.0 )
			:	AImageAligner( container, method, scale ){ }
		virtual void align( AProcessWatcher* watcher=nullptr ) override;
};

class AverageAligner : public WrapperImageAligner{
	virtual std::unique_ptr<AImageAligner> makeAligner( AContainer& container ) override
		{ return std::make_unique<AverageAlignerImpl>( container, method, scale ); }
};

}

#endif