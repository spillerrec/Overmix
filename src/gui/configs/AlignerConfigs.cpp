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


#include "AlignerConfigs.hpp"

#include "../../aligners/AverageAligner.hpp"

AlignerConfigChooser::AlignerConfigChooser( QObject* parent )
	: ConfigChooser<AAlignerConfig>( parent ){
	addConfig( std::move( std::make_unique<AverageAlignerConfig>(parent) ) );
}


AImageAligner::AlignMethod AAlignerConfig::getMethod() const{
	return AImageAligner::ALIGN_VER; //TODO:
}

double AAlignerConfig::getScale() const{
	return 1.0; //TODO:
}


std::unique_ptr<AImageAligner> AverageAlignerConfig::getAligner( AContainer& container ) const{
	return std::make_unique<AverageAligner>( container, getMethod(), getScale() );
}

QObject* AverageAlignerConfig::subEditor( QObject* parent ){
	return nullptr;
}

