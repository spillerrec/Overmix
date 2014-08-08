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

#ifndef FAKE_ALIGNER_HPP
#define FAKE_ALIGNER_HPP

#include "AImageAligner.hpp"

class FakeAligner : public AImageAligner{
	public:
		FakeAligner( AContainer& container ) : AImageAligner( container, ALIGN_BOTH, 1.0 ){ }
		virtual void align( AProcessWatcher* ) override{ resetPosition(); }
};

#endif