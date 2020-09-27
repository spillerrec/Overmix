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


#include "Processor.hpp"
#include "Parsing.hpp"

#include "planes/ImageEx.hpp"
#include "planes/PatternRemove.hpp"
#include "color.hpp"

#include <QString>
#include <QTextStream>

#include <algorithm>
#include <vector>

using namespace Overmix;


static void convert( QString str, ScalingFunction& func ){
	func = getEnum<ScalingFunction>( str,
		{	{ "none",     ScalingFunction::SCALE_NEAREST   }
		,	{ "linear",   ScalingFunction::SCALE_LINEAR    }
		,	{ "mitchell", ScalingFunction::SCALE_MITCHELL  }
		,	{ "catrom",   ScalingFunction::SCALE_CATROM    }
		,	{ "spline",   ScalingFunction::SCALE_SPLINE    }
		,	{ "lanczos3", ScalingFunction::SCALE_LANCZOS_3 }
		,	{ "lanczos5", ScalingFunction::SCALE_LANCZOS_5 }
		,	{ "lanczos7", ScalingFunction::SCALE_LANCZOS_7 }
		} );
}

using PlaneFunc = Plane (Plane::*)() const;
static void convert( QString str, PlaneFunc& func ){
	func = getEnum<PlaneFunc>( str,
		{	{ "robert",          &Plane::edge_robert          }
		,	{ "sobel",           &Plane::edge_sobel           }
		,	{ "prewitt",         &Plane::edge_prewitt         }
		,	{ "laplacian",       &Plane::edge_laplacian       }
		,	{ "laplacian-large", &Plane::edge_laplacian_large }
		} );
}

struct ScaleProcessor : public Processor{
	ScalingFunction function{ ScalingFunction::SCALE_CATROM };
	Point<double> scale;
	
	explicit ScaleProcessor( QString str ) { convert( str, function, scale ); }
	
	void process( ImageEx& img ) override
		{ img.scaleFactor( scale, function ); }
};

struct EdgeProcessor : public Processor {
	PlaneFunc function;
	
	explicit EdgeProcessor( QString str ) { convert( str, function ); }
	
	void process( ImageEx& img ) override
		{ img.apply( function ); }
};

struct DilateProcessor : public Processor {
	int size;
	
	explicit DilateProcessor( QString str ) { convert( str, size ); }
	
	void process( ImageEx& img ) override
		{ img.apply( &Plane::dilate, size ); }
};

struct BinarizeThresholdProcessor : public Processor {
	double threshold;
	
	explicit BinarizeThresholdProcessor( QString str ) { convert( str, threshold ); }
	
	void process( ImageEx& img ) override {
		for( unsigned i=0; i<img.size(); i++ )
			img[i].binarize_threshold( color::fromDouble( threshold ) );
	}
};

struct BinarizeAdaptiveProcessor : public Processor {
	int amount;
	double threshold;
	
	explicit BinarizeAdaptiveProcessor( QString str ) { convert( str, amount, threshold ); }
	
	void process( ImageEx& img ) override {
		for( unsigned i=0; i<img.size(); i++ )
			img[i].binarize_adaptive( amount, color::fromDouble( threshold ) );
	}
};

struct BinarizeDitherProcessor : public Processor {
	explicit BinarizeDitherProcessor( QString ) { }
	
	void process( ImageEx& img ) override {
		for( unsigned i=0; i<img.size(); i++ )
			img[i].binarize_dither();
	}
};

struct BlurProcessor : public Processor {
	Point<double> deviation;
	
	explicit BlurProcessor( QString str ) { convert( str, deviation ); }
	
	void process( ImageEx& img ) override {
		for( unsigned i=0; i<img.size(); i++ )
			img[i] = img[i].blur_gaussian( deviation.x, deviation.y );
	}
};

struct DeconvolveProcessor : public Processor {
	Point<double> deviation;
	int iterations;
	
	explicit DeconvolveProcessor( QString str ) { convert( str, deviation, iterations ); }
	
	void process( ImageEx& img ) override {
		for( unsigned i=0; i<img.size(); i++ )
			img[i] = img[i].deconvolve_rl( deviation, iterations );
	}
};

struct LevelProcessor : public Processor {
	double limit_min, limit_max, output_min, output_max, gamma;
	
	explicit LevelProcessor( QString str )
		{ convert( str, limit_min, limit_max, output_min, output_max, gamma ); }
	
	void process( ImageEx& img ) override {
		for( unsigned i=0; i<img.size(); i++ )
			img[i] = img[i].level( 
					color::fromDouble( limit_min )
				,	color::fromDouble( limit_max )
				,	color::fromDouble( output_min )
				,	color::fromDouble( output_max )
				,	gamma
				);
	}
};

struct PatternProcessor : public Processor {
	Point<double> size;
	
	explicit PatternProcessor( QString str )
		{ convert( str, size ); }
	
	void process( ImageEx& img ) override {
		img = patternRemove( img, size );
		
	//	for( unsigned i=0; i<img.size(); i++ )
	//		img[i] = patternRemove( img[i], size );
	}
};


std::unique_ptr<Processor> Overmix::processingParser( QString parameters ){
	Splitter split( parameters, ':' );
	if( split.left == "scale" )
		return std::make_unique<ScaleProcessor>( split.right );
	if( split.left == "edge" )
		return std::make_unique<EdgeProcessor>( split.right );
	if( split.left == "dilate" )
		return std::make_unique<DilateProcessor>( split.right );
	if( split.left == "binarize-threshold" )
		return std::make_unique<BinarizeThresholdProcessor>( split.right );
	if( split.left == "binarize-adaptive" )
		return std::make_unique<BinarizeAdaptiveProcessor>( split.right );
	if( split.left == "binarize-dither" )
		return std::make_unique<BinarizeDitherProcessor>( split.right );
	if( split.left == "blur" )
		return std::make_unique<BlurProcessor>( split.right );
	if( split.left == "deconvolve" )
		return std::make_unique<DeconvolveProcessor>( split.right );
	if( split.left == "level" )
		return std::make_unique<LevelProcessor>( split.right );
	if( split.left == "pattern" )
		return std::make_unique<PatternProcessor>( split.right );
	
	throw std::invalid_argument( fromQString( "No processor found with the name: '" + split.left + "'" ) );
}

void Overmix::processingHelpText( QTextStream& std ){
	std << "Available processors:\n";
	std << "\tscale:<method>:<width>x<height>\n";
	std << "\t\tScales the image by multiplying the size with the specified width and height\n";
	std << "\t\tMethods: none, linear, mitchell, catrom, spline, lanczos3, lanczos5, lanczos7\n";
	std << "\n";
	std << "\tedge:<method>\n";
	std << "\t\tApplies edge detection\n";
	std << "\t\tMethods: robert, sobel, prewitt, laplacian, laplacian-large\n";
	std << "\n";
	std << "\tdilate:<size>\n";
	std << "\t\tDilates the image with the specified size\n";
	std << "\n";
	std << "\tbinarize-threshold:<threshold>\n";
	std << "\t\tBinarizes the image using a threshold, with 0.0 being black, and 1.0 being white\n";
	std << "\n";
	std << "\tbinarize-adaptive:<amount>:<threshold>\n";
	std << "\t\tBinarizes the image using adaptive:\n";
	std << "\t\tamount: pixels width of considered area\n";
	std << "\t\tthreshold: -1.0 to 1.0\n";
	std << "\n";
	std << "\tbinarize-dither\n";
	std << "\t\tBinarizes the image using dithering\n";
	std << "\n";
	std << "\tblur:<width>x<height>\n";
	std << "\t\tBlurs the image using gaussian with deviation as specified\n";
	std << "\n";
	std << "\tdeconvolve:<width>x<height>:iterations\n";
	std << "\t\tSharpens the image using deconvolution, using a gaussian blur with deviation as 'width'x'height' and repeated for 'iterations'\n";
	std << "\n";
	std << "\tlevel:<limit_min>:<limit_max>:<output_min>:<output_max>:<gamma>\n";
	std << "\t\tChanges the color distribution, color values in 0.0 to 1.0:\n";
	std << "\t\tlimit_min: Truncates values bellow this point\n";
	std << "\t\tlimit_max: Truncates values above this point\n";
	std << "\t\toutput_min: output value of black\n";
	std << "\t\toutput_max: output value of white\n";
	std << "\t\tgamma: Applies value^<gamma>\n";
	std << "\n";
	std << "\tpattern:<width>x<height>\n";
	std << "\t\tDetects and removes patters which repeats every 'width'x'height'\n";
	std << "\t\tWIP\n";
	std << "\n";
}
