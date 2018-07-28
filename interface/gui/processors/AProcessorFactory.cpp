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


#include "AProcessorFactory.hpp"

#include "AProcessor.hpp"
#include "ProcessBinarize.hpp"
#include "ProcessBlur.hpp"
#include "ProcessColor.hpp"
#include "ProcessDither.hpp"
#include "ProcessDilate.hpp"
#include "ProcessDeconvolve.hpp"
#include "ProcessEdge.hpp"
#include "ProcessLevels.hpp"
#include "ProcessScale.hpp"


using namespace Overmix;

AProcessorFactory::AProcessorFactory(){
	addProcessor<ProcessBinarize  >( "Binarize"         );
	addProcessor<ProcessBlur      >( "Bluring"          );
	addProcessor<ProcessColor     >( "Color space"      );
	addProcessor<ProcessDither    >( "Dither"           );
	addProcessor<ProcessDilate    >( "Dilate"           );
	addProcessor<ProcessDeconvolve>( "Deconvolve"       );
	addProcessor<ProcessEdge      >( "Edge detection"   );
	addProcessor<ProcessLevels    >( "Level adjustment" );
	addProcessor<ProcessScale     >( "Scaling"          );
}


const char* const AProcessorFactory::name( int index ) const{
	return factory.at(index).name;
}

std::unique_ptr<AProcessor> AProcessorFactory::create( int index, QWidget* parent ) const{
	return factory.at(index).create( parent );
}