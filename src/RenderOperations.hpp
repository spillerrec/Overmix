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

#ifndef RENDER_OPERATIONS_HPP
#define RENDER_OPERATIONS_HPP

#include "ARenderPipe.hpp"
#include "color.hpp"
#include "ImageEx.hpp"

class RenderPipeScaling : public ARenderPipe{
	private:
		double width{ 1.0 }, height{ 1.0 };
		
	protected:
		virtual bool renderNeeded() const override{
			return width <= 0.9999 || width >= 1.0001 || height <= 0.9999 || height >= 1.0001;
		}
		virtual ImageEx render( const ImageEx& img ) const override{
			//TODO: support multiple scaling methods?
			ImageEx temp( img );
			temp.scale( temp.get_width() * width + 0.5, temp.get_height() * height + 0.5 );
			return temp;
		}
		
	public:
		void setWidth( double width ){ set( this->width, width ); }
		void setHeight( double height ){ set( this->height, height ); }
};

class RenderPipeDeconvolve : public ARenderPipe{
	private:
		double deviation{ 1.0 };
		unsigned iterations{ 0 };
		
	protected:
		virtual bool renderNeeded() const override{ return deviation > 0.0009 && iterations > 0; }
		virtual ImageEx render( const ImageEx& img ) const override{
			return img.copyApply( &Plane::deconvolve_rl, deviation, iterations );
		}
		
	public:
		void setDeviation( double deviation ){ set( this->deviation, deviation ); }
		void setIterations( unsigned iterations ){ set( this->iterations, iterations ); }
};

class RenderPipeBlurring : public ARenderPipe{
	private:
		unsigned width{ 0 }, height{ 0 };
		unsigned method{ 0 };
		
	protected:
		virtual bool renderNeeded() const override{ return method != 0; }
		virtual ImageEx render( const ImageEx& img ) const override{
			switch( method ){
				case 1: return img.copyApplyAll( true, &Plane::blur_box, width, height ); break;
				case 2: return img.copyApplyAll( true, &Plane::blur_gaussian, width, height ); break;
				default: return img;
			}
		}
		
	public:
		void setWidth( unsigned width ){ set( this->width, width ); }
		void setHeight( unsigned height ){ set( this->height, height ); }
		void setMethod( unsigned method ){ set( this->method, method ); }
};

class RenderPipeEdgeDetection : public ARenderPipe{
	private:
		unsigned method{ 0 };
		
	protected:
		virtual bool renderNeeded() const override{ return method != 0; }
		virtual ImageEx render( const ImageEx& img ) const override{
			ImageEx temp( img );
			temp.to_grayscale();
			switch( method ){
				case 1: temp.apply( &Plane::edge_robert ); break;
				case 2: temp.apply( &Plane::edge_sobel ); break;
				case 3: temp.apply( &Plane::edge_prewitt ); break;
				case 4: temp.apply( &Plane::edge_laplacian ); break;
				case 5: temp.apply( &Plane::edge_laplacian_large ); break;
				default: break;
			};
			return temp;
		}
		
	public:
		void setMethod( unsigned method ){ set( this->method, method ); }
};

class RenderPipeLevel : public ARenderPipe{
	private:
		color_type limit_min{ color::BLACK };
		color_type limit_max{ color::WHITE };
		color_type out_min{ color::BLACK };
		color_type out_max{ color::WHITE };
		double gamma{ 1.0 };
		
	protected:
		virtual bool renderNeeded() const override{
			return limit_min != color::BLACK
				||	limit_max != color::WHITE
				||	out_min != color::BLACK
				||	out_max != color::WHITE
				||	gamma != 1.0
				;
			}
		virtual ImageEx render( const ImageEx& img ) const override{
			return img.copyApply( &Plane::level, limit_min, limit_max, out_min, out_max, gamma );
		}
		
	public:
		void setLimitMin( color_type limit_min ){ set( this->limit_min, limit_min ); }
		void setLimitMax( color_type limit_max ){ set( this->limit_max, limit_max ); }
		void setOutMin( color_type out_min ){ set( this->out_min, out_min ); }
		void setOutMax( color_type out_max ){ set( this->out_max, out_max ); }
		void setGamma( double gamma ){ set( this->gamma, gamma ); }
};

//TODO: sharpen

class RenderPipeThreshold : public ARenderPipe{
	private:
		unsigned method;
		color_type threshold{ color::BLACK };
		int size{ 20 };
		
	protected:
		virtual bool renderNeeded() const override{ return method != 0; }
		virtual ImageEx render( const ImageEx& img ) const override{
			ImageEx temp( img );
			temp.to_grayscale();
			switch( method ){
				case 1:
						temp[0].binarize_threshold( threshold );
						if( size > 0 )
							temp.apply( &Plane::dilate, size );
					break;
					
				case 2: temp[0].binarize_adaptive( size, threshold >> 6 ); break;
				case 3: temp[0].binarize_dither(); break;
				default: break;
			}
			return temp;
		}
		
	public:
		void setMethod( unsigned method ){ set( this->method, method ); }
		void setThreshold( color_type threshold ){ set( this->threshold, threshold ); }
		void setSize( int size ){ set( this->size, size ); }
};

#endif