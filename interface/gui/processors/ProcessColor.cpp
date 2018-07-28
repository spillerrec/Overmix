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


#include "ProcessColor.hpp"

#include <QComboBox>
#include "../Spinbox2D.hpp"
#include "planes/ImageEx.hpp"

using namespace Overmix;


static const std::pair<Transform, const char* const> transform_mapping[] = {
		{ Transform::GRAY     , "Grayscale"  }
	,	{ Transform::RGB      , "RGB"        }
	,	{ Transform::YCbCr_601, "Rec. 601"   }
	,	{ Transform::YCbCr_709, "Rec. 709"   }
	,	{ Transform::JPEG     , "JPEG YCbCr" }
};

static const std::pair<Transfer, const char* const> transfer_mapping[] = {
		{ Transfer::LINEAR, "Linear"       }
	,	{ Transfer::SRGB  , "sRGB"         }
	,	{ Transfer::REC709, "Rec. 601/709" }
};

ProcessColor::ProcessColor( QWidget* parent ) : AProcessor( parent ){
	transform = newItem<QComboBox>( "Tranform" );
	for( auto mapping : transform_mapping )
		transform->addItem( mapping.second );
	
	transfer  = newItem<QComboBox>( "Transfer function" );
	for( auto mapping : transfer_mapping )
		transfer->addItem( mapping.second );
	transfer->setCurrentIndex( 1 );
}

QString ProcessColor::name() const{ return "Color space"; }

ImageEx ProcessColor::process( const ImageEx& input ) const{
	return input.toColorSpace(
		{	transform_mapping[ transform->currentIndex() ].first
		,	transfer_mapping[  transfer ->currentIndex() ].first
		} );
}