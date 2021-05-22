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
#include <QSpinBox>
#include <QDoubleSpinBox>
#include "planes/ImageEx.hpp"

using namespace Overmix;

struct EdgeFunc{
	Plane (Plane::*func)() const;
	const char* const name;
};
static const EdgeFunc edge_mapping[] = {
		{ &Plane::edge_robert         , "Robert"        }
	,	{ &Plane::edge_sobel          , "Sobel"         }
	,	{ &Plane::edge_prewitt        , "Prewitt"       }
	,	{ &Plane::edge_laplacian      , "Laplacian (3)" }
	,	{ &Plane::edge_laplacian_large, "Laplacian (5)" }
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


ProcessEdge2::ProcessEdge2( QWidget* parent ) : AProcessor( parent ){
	sigma  = newItem<QDoubleSpinBox>( "Sigma"  );
	amount = newItem<QDoubleSpinBox>( "Amount" );
	size   = newItem<QSpinBox      >( "Size"   );
	
	sigma->setValue(0.5);
	amount->setValue(1.0);
	size->setValue(2);
	
	sigma->setSingleStep(0.05);
	amount->setSingleStep(0.1);
}

QString ProcessEdge2::name() const{ return "Laplacian of Guassians"; }

ImageEx ProcessEdge2::process( const ImageEx& input ) const{
	//TODO: assert size
	ImageEx output( input );
	output.apply( &Plane::edge_laplacian_ex, sigma->value(), amount->value(), size->value() );
	return output;
}


ProcessEdge3::ProcessEdge3( QWidget* parent ) : AProcessor( parent ){
	sigma1  = newItem<QDoubleSpinBox>( "SigmaLow"  );
	sigma2  = newItem<QDoubleSpinBox>( "SigmaHigh"  );
	amount = newItem<QDoubleSpinBox>( "Amount" );
	size   = newItem<QSpinBox      >( "Size"   );
	
	sigma1->setValue(0.0);
	sigma2->setValue(0.5);
	amount->setValue(1.0);
	size->setValue(2);
	
	sigma1->setSingleStep(0.05);
	sigma2->setSingleStep(0.05);
	amount->setSingleStep(0.1);
}

QString ProcessEdge3::name() const{ return "Gaussian Edge"; }

ImageEx ProcessEdge3::process( const ImageEx& input ) const{
	//TODO: assert size
	ImageEx output( input );
	output.apply( &Plane::edge_guassian, sigma1->value(), sigma2->value(), amount->value() );
	return output;
}