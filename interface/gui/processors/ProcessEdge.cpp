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

ProcessEdge::ProcessEdge( QWidget* parent ) : AProcessor( parent ){
	method = newItem<QComboBox>( "Method" );
	
	method->addItem( "None"          ); //TODO: Remove
	method->addItem( "Robert"        );
	method->addItem( "Sobel"         );
	method->addItem( "Prewitt"       );
	method->addItem( "Laplacian (3)" );
	method->addItem( "Laplacian (5)" );
}

QString ProcessEdge::name() const{ return "Edge detection"; }

bool ProcessEdge::modifiesImage() const{
	return method->currentIndex() != 0; //TODO: Remove when "None" is removed
}

ImageEx ProcessEdge::process( const ImageEx& input ) const{
	//TODO: Use array when "None" is removed
	ImageEx output( input );
	output.to_grayscale();
	switch( method->currentIndex() ){
		case 1: output.apply( &Plane::edge_robert          ); break;
		case 2: output.apply( &Plane::edge_sobel           ); break;
		case 3: output.apply( &Plane::edge_prewitt         ); break;
		case 4: output.apply( &Plane::edge_laplacian       ); break;
		case 5: output.apply( &Plane::edge_laplacian_large ); break;
		default: break;
	};
	return output;
}