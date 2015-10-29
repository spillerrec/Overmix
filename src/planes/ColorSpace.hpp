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

#ifndef COLOR_SPACE_HPP
#define COLOR_SPACE_HPP

namespace Overmix{

enum class Transform{
		GRAY
	,	RGB
	,	YCbCr_601 //As specified in Rec. 601
	,	YCbCr_709 //As specified in Rec. 709
	,	JPEG      //As YCbCr_601, but without studio-swing
	,	UNKNOWN
};
enum class Transfer{ //i.e. gamma function
		LINEAR
	,	SRGB    //As specified in the sRGB standard
	,	REC709  //As specified in Rec. 601 and 709
	,	UNKNOWN
};
class color;

class ColorSpace{
	private:
		Transform _transform;
		//TODO: primaries
		Transfer _transfer;
		
	public:
		ColorSpace( Transform transform, Transfer transfer )
			:	_transform(transform), _transfer(transfer) { }
		
		Transform transform() const{ return _transform; }
		Transfer transfer() const{ return _transfer; }
		
		ColorSpace changed( Transform t ) const
			{ return { t, _transfer }; }
		ColorSpace changed( Transfer t ) const
			{ return { _transform, t }; }
		
		///Returns true if transform is YCbCr
		bool isYCbCr() const{
			switch( _transform ){
				case Transform::YCbCr_601:
				case Transform::YCbCr_709:
				case Transform::JPEG:
					return true;
				default: return false;
			}
		}
		
		///Returns true if transform is RGB
		bool isRgb() const{ return _transform == Transform::RGB; }
		
		///Returns true if transform is GRAY
		bool isGray() const{ return _transform == Transform::GRAY; }
		
		int components() const{
			switch( _transform ){
				case Transform::GRAY:
					return 1;
				case Transform::RGB:
				case Transform::YCbCr_601:
				case Transform::YCbCr_709:
				case Transform::JPEG:
						return 3;
				default: return 0;
			}
		}
		
		color convert( color from, ColorSpace to ) const;
};

}

#endif