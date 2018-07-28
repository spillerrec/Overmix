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


#include "ProcessEdge.hpp"

#include <QComboBox>
#include "planes/ImageEx.hpp"

using namespace Overmix;

struct EdgeFunc{
	Plane (Plane::*func)() const;
	const char* const name;
};
static const EdgeFunc edge_mapping[] = {
		{ Plane::edge_robert         , "Robert"        }
	,	{ Plane::edge_sobel          , "Sobel"         }
	,	{ Plane::edge_prewitt        , "Prewitt"       }
	,	{ Plane::edge_laplacian      , "Laplacian (3)" }
	,	{ Plane::edge_laplacian_large, "Laplacian (5)" }
};


ProcessEdge::ProcessEdge( QWidget* parent ) : AProcessor( parent ){
	method = newItem<QComboBox>( "Method" );
	
	for( auto edge : edge_mapping )
		method->addItem( edge.name );
}

QString ProcessEdge::name() const{ return "Edge detection"; }

ImageEx ProcessEdge::process( const ImageEx& input ) const{
	//TODO: assert size
	ImageEx output( input );
	output.apply( edge_mapping[method->currentIndex()].func );
	return output;
}