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

#ifndef PROCESSOR_HPP
#define PROCESSOR_HPP

#include <memory>
class QString;
class QTextStream;

namespace Overmix{

class ImageEx;

class Processor{
	public:
		virtual void process( ImageEx& ) = 0;
		virtual ~Processor() { }
};

std::unique_ptr<Processor> processingParser( QString parameters );
void processingHelpText( QTextStream& std );

}

#endif