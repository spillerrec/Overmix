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

//This code is based on a opencv implementation by fukushima and still bears some similarity to it
// http://opencv.jp/opencv2-x-samples/usage_of_sparsemat_2_superresolution

#include "RobustSrRender.hpp"
#include "../planes/Plane.hpp"
#include "../color.hpp"
#include "AverageRender.hpp"
#include "../planes/ImageEx.hpp"
#include "../containers/AContainer.hpp"
#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <QDebug>

using namespace Eigen;
using namespace std;

MatrixXf imageToMatrix( const Plane& p ){
	MatrixXf out( 1, p.get_width() * p.get_height() );

	for( unsigned iy=0; iy<p.get_height(); iy++ )
		for( unsigned ix=0; ix<p.get_width(); ix++ )
			out( 0, ix  + iy * p.get_width()) = color::asDouble( p.pixel( {ix,iy} ) );

	return out;
}

float limit( float in ){ return (in>1.0f) ? 1.0f : ((in<0.0f) ? 0.0f : in); }
Plane matrixToImage( const MatrixXf& mat, unsigned width ){
	Plane out( width, mat.size()/width );

	for( unsigned iy=0; iy < out.get_height(); iy++ )
		for( unsigned ix=0; ix<out.get_width(); ix++ )
			out.setPixel( {ix,iy}, color::fromDouble(limit(mat(0, iy*width+ix))) );

	return out;
}


struct MatrixImg{
	SparseMatrix<float> dhf;
	MatrixXf img;

	MatrixImg( const Plane& p, Point<double> offset, Size<unsigned> full, int upscale_factor );
};

MatrixImg::MatrixImg( const Plane& p, Point<double> offset, Size<unsigned> size, int upscale_factor )
	:	dhf( size.width()*size.height(), size.width()*size.height()/(upscale_factor*upscale_factor) )
	,	img( imageToMatrix(p) )
	{
	qDebug() << "dhf: " << dhf.rows() << "x" << dhf.cols();
	
	offset *= upscale_factor;

	float div = 1.0f/((float)(upscale_factor*upscale_factor));
	int x1 = (int)(offset.x+1);
	int x0 = (int)(offset.x);
	float a1 = (float)(offset.x-x0);
	float a0 = (float)(1.0-a1);

	int y1 = (int)(offset.y+1);
	int y0 = (int)(offset.y);
	float b1 = (float)(offset.y-y0);
	float b0 = (float)(1.0-b1);

	int bsx =x1;
	int bsy =y1;

	vector<Triplet<float>> triplets;
	auto add = [&]( int x, int y, float val ){ triplets.emplace_back( x, y, val ); };

	const auto step = size.width() / upscale_factor;
	for( unsigned j=0; j<size.height(); j+=upscale_factor )
		for( unsigned i=0; i<size.width(); i+=upscale_factor ){
			int y = size.width() * j + i;
			int s = step * j / upscale_factor + i / upscale_factor;

			if( i>=bsx &&i<size.width()-bsx-upscale_factor &&j>=bsy &&j<size.height()-bsy-upscale_factor ){
				for( int l=0; l<upscale_factor; l++ )
					for( int k=0; k<upscale_factor; k++ ){
						add( y+size.width()*(y0+l)+x0+k,s, a0*b0*div );
						add( y+size.width()*(y0+l)+x1+k,s, a1*b0*div );
						add( y+size.width()*(y1+l)+x0+k,s, a0*b1*div );
						add( y+size.width()*(y1+l)+x1+k,s, a1*b1*div );
					}
			}
		}
	
	dhf.setFromTriplets( triplets.begin(), triplets.end() );
}

static float signFloat( float a, float b ){ return (a>b) ? 1.0f : (a<b) ? -1.0f : 0.0f; }

MatrixXf sign( const MatrixXf& mat1, const MatrixXf& mat2 ){
//	return mat1 - mat2; //L2 ??
	MatrixXf out( mat1.rows(), mat1.cols() );
	for( unsigned i=0; i<mat1.size(); i++ )
		out(i) = signFloat( limit(mat1(i)), limit(mat2(i)) );
	return out;
}

//void btRegularization( const MatrixXf& mat, Size<> kernel, float alpha, MatrixXf& destVec, Size<> size ){
//	
//}

ImageEx RobustSrRender::render(const AContainer &group, AProcessWatcher *watcher) const {
	auto planes_amount = group.image(0).size();
	ImageEx img( planes_amount!=1 ? group.image(0).get_system() : ImageEx::GRAY );
	auto min_point = group.minPoint();
	
	auto est = AverageRender().render(group); //Starting estimate
	for( unsigned c=0; c<planes_amount; ++c ){
		auto output = imageToMatrix( est[c].scale_cubic( group.image(0).getSize()*upscale_factor ) ); //TODO: support real upscale
		qDebug() << "Output size: " << output.size();
		vector<MatrixImg> lowres;
		for( unsigned i=0; i<group.count(); i++ )
			lowres.emplace_back( group.image(i)[c], group.pos(i)-min_point, group.image(0).getSize()*upscale_factor, upscale_factor );

		for( int i=0; i<iterations; i++ ){
			qDebug() << "Starting iteration " << i;
			auto output_copy = output;
			for( const auto& lr : lowres )
				output -= (sign( output_copy * lr.dhf, lr.img ) * lr.dhf.transpose()) * beta;
		}
		
		img.addPlane( matrixToImage( output, group.image(0).getSize().width() * upscale_factor ) );
	}

	return img;
}