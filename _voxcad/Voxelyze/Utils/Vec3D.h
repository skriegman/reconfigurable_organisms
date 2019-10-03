/*******************************************************************************
Copyright (c) 2012, Jonathan Hiller

This file is part of the AMF Tools suite. http://amf.wikispaces.com/
AMF Tools is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
AMF Tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
See <http://www.opensource.org/licenses/lgpl-3.0.html> for license details.
*******************************************************************************/

#ifndef _VEC3D_H
#define _VEC3D_H

//Possible Linux portability issues: min, max

#include <math.h>
#include <float.h>

#define PI 3.14159265358979
#define VEC3D_HYSTERESIS_FACTOR 1.1 /*how much a valuemust go past 1.0 to switch states*/

#ifdef PREC_LOW //Max allowable error: 0.1%, 0.0548 Rad = 3.14° Small angle, 0.0001 Rad = 0.00573° round to zero
	#define vfloat float
	static const vfloat MAX_ERROR_PERCENT = 0.001;
	static const vfloat DISCARD_ANGLE_RAD = 0.0001; //Anything less than this angle can be considered 0
	static const vfloat SMALL_ANGLE_RAD = 0.0548; //Angles less than this get small angle approximations. To get: Root solve atan(t)/t-1+MAX_ERROR_PERCENT. From: MAX_ERROR_PERCENT = (t-atan(t))/t 
	static const vfloat W_THRESH_ACOS2SQRT = 0.9880; //Threshhold of w above which we can approximate acos(w) with sqrt(2-2w). To get: Root solve 1-sqrt(2-2wt)/acos(wt) - MAX_ERROR_PERCENT. From MAX_ERROR_PERCENT = (acos(wt)-sqrt(2-2wt))/acos(wt)
	//Set compiler to /fp:fast
#elif defined PREC_HIGH //Max allowable error: 0.0001%, 0.00173205 Rad = 0.1° Small angle, 1e-7 Rad = 5.73e-6° round to zero
	#define vfloat double
	static const vfloat MAX_ERROR_PERCENT = 1e-6;
	static const vfloat DISCARD_ANGLE_RAD = 1e-7; //Anything less than this angle can be considered 0
	static const vfloat SMALL_ANGLE_RAD = 1.732e-3; //Angles less than this get small angle approximations. To get: Root solve atan(t)/t-1+MAX_ERROR_PERCENT. From: MAX_ERROR_PERCENT = (t-atan(t))/t 
	static const vfloat W_THRESH_ACOS2SQRT = 0.999987737; //Threshhold of w above which we can approximate acos(w) with sqrt(2-2w). To get: Root solve 1-sqrt(2-2wt)/acos(wt) - MAX_ERROR_PERCENT. From MAX_ERROR_PERCENT = (acos(wt)-sqrt(2-2wt))/acos(wt)
	//Set compiler to /fp:precise
//#elif defined PREC_MAX //Max allowable error: 0.00000001%, 1.732e-5 Rad = 0.001° Small angle, 1e-12 Rad = 5.73e-11° round to zero
//	#define vfloat double
//	static const vfloat MAX_ERROR_PERCENT = 1e-10;
//	static const vfloat DISCARD_ANGLE_RAD = 1e-12; //Anything less than this angle can be considered 0
//	static const vfloat SMALL_ANGLE_RAD = 1.732e-5; //Angles less than this get small angle approximations. To get: Root solve atan(t)/t-1+MAX_ERROR_PERCENT. From: MAX_ERROR_PERCENT = (t-atan(t))/t 
//	static const vfloat W_THRESH_ACOS2SQRT = 0.9999997996; //Threshhold of w above which we can approximate acos(w) with sqrt(2-2w). To get: Root solve 1-sqrt(2-2wt)/acos(wt) - MAX_ERROR_PERCENT. From MAX_ERROR_PERCENT = (acos(wt)-sqrt(2-2wt))/acos(wt)
	//Set compiler to /fp:precise
#else //defined PREC_MED //Max allowable error: 0.01%, 0.0173 Rad = 1° Small angle, 0.00001 Rad = 0.000573° round to zero
	#define vfloat double //temporary 6-10
//	#define vfloat float
	static const vfloat MAX_ERROR_PERCENT = 1e-4;
	static const vfloat DISCARD_ANGLE_RAD = 1e-7; //Anything less than this angle can be considered 0
	static const vfloat SMALL_ANGLE_RAD = 1.732e-2; //Angles less than this get small angle approximations. To get: Root solve atan(t)/t-1+MAX_ERROR_PERCENT. From: MAX_ERROR_PERCENT = (t-atan(t))/t 
	static const vfloat W_THRESH_ACOS2SQRT = 0.9988; //Threshhold of w above which we can approximate acos(w) with sqrt(2-2w). To get: Root solve 1-sqrt(2-2wt)/acos(wt) - MAX_ERROR_PERCENT. From MAX_ERROR_PERCENT = (acos(wt)-sqrt(2-2wt))/acos(wt)
	//Set compiler to /fp:fast

#endif

//calculated constants
static const vfloat SMALL_ANGLE_W = cos(SMALL_ANGLE_RAD*0.5); //quaternion W value corresponding to a SMALL_ANGLE_RAD
static const vfloat SMALLISH_ANGLE_W = cos(VEC3D_HYSTERESIS_FACTOR*SMALL_ANGLE_RAD*0.5); //30% greater than Small Angle. (can add hysteresis in switching small angle modes in certain uses.
static const vfloat SLTHRESH_DISCARD_ANGLE = (vfloat)(1.0-cos((double)DISCARD_ANGLE_RAD)*cos((double)DISCARD_ANGLE_RAD)); //SquareLength (x^2+y^2+z^2 comparison) threshhold for what constitutes a discard-small angle. From DISCARD_ANGLE_RAD ~= acos(w) and SquareLength alo = 1-w*w. Must upcast to double or else truncated floats.
static const vfloat SLTHRESH_ACOS2SQRT = 1.0-W_THRESH_ACOS2SQRT*W_THRESH_ACOS2SQRT; //SquareLength threshhold for when we can use square root optimization for acos. From SquareLength = 1-w*w.
	
template <typename T = vfloat> class CQuat;

template <typename T = vfloat> //<typename T=vfloat> ?
class Vec3D {
public:
	//Constructors
	Vec3D() {x = 0; y = 0; z = 0;}
	Vec3D(const Vec3D& s) {x = s.x; y = s.y; z = s.z;} //copy constructor
	Vec3D(const T dx, const T dy, const T dz) {x = dx; y = dy; z = dz;}

	//Stuff to make code with mixed template parameters work...
	template <typename U> Vec3D<T>(const Vec3D<U>& s) {x = s.x; y = s.y; z = s.z;} //copy constructor
	template <typename U> operator Vec3D<U>() const {return Vec3D<U>(x, y, z);} //overload conversion operator for different template types?
	template <typename U> Vec3D<T> operator=(const Vec3D<U>& s) {x = s.x; y = s.y; z = s.z; return *this; }; //overload equals
	template <typename U> const Vec3D<T> operator+(const Vec3D<U>& s){return Vec3D<T>(x+s.x, y+s.y, z+s.z);}
	template <typename U> const Vec3D<T> operator-(const Vec3D<U>& s){return Vec3D<T>(x-s.x, y-s.y, z-s.z);}
//	template <typename U> friend Vec3D<T> operator+(Vec3D<T> const& lhs, Vec3D<U> const& rhs){return Vec3D<T>(lhs.x+rhs.x, lhs.y+rhs.y, lhs.z+rhs.z);}
	template <typename U> const Vec3D<T> operator*(const U& f) const {return Vec3D<T>(f*x, f*y, f*z);}
	template <typename U> const friend Vec3D<T> operator*(const U f, const Vec3D<T>& v) {return v*f;} //must be friend because float operator is first...
	template <typename U> const Vec3D<T>& operator+=(const Vec3D<U>& s) {x += s.x; y += s.y; z += s.z; return *this;} //add and set
	template <typename U> const Vec3D<T>& operator-=(const Vec3D<U>& s) {x -= s.x; y -= s.y; z -= s.z; return *this;} //subract and set

	//Operators
	inline Vec3D& operator=(const Vec3D& s) {x = s.x; y = s.y; z = s.z; return *this; }; //overload equals
	const Vec3D operator+(const Vec3D &v) const {return Vec3D(x+v.x, y+v.y, z+v.z);}
	const Vec3D operator-(const Vec3D &v) const {return Vec3D(x-v.x, y-v.y, z-v.z);}
	const Vec3D operator-() const {return Vec3D(-x, -y, -z);} //Negation (unary)
	const Vec3D operator*(const T &f) const {return Vec3D(f*x, f*y, f*z);}
	const friend Vec3D operator*(const T f, const Vec3D& v) {return v*f;} //must be friend because float operator is first...
	const Vec3D operator/(const T &f) const {vfloat Inv = 1.0/f; return Vec3D(Inv*x, Inv*y, Inv*z);}
	bool operator==(const Vec3D& v) {return (x==v.x && y==v.y && z==v.z);} //Is equal
	bool operator!=(const Vec3D& v) {return !(*this==v);} //Is not equal
	const Vec3D& operator+=(const Vec3D& s) {x += s.x; y += s.y; z += s.z; return *this;} //add and set
	const Vec3D& operator-=(const Vec3D& s) {x -= s.x; y -= s.y; z -= s.z; return *this;} //subract and set
	const Vec3D& operator*=(const T f) {x *= f; y *= f; z *= f; return *this;} //multiply and set
	const Vec3D& operator/=(const T f) {vfloat Inv = 1.0/f; x *= Inv; y *= Inv; z *= Inv; return *this;} //subtract and set

#ifdef WIN32
	bool IsValid() const {return !_isnan((double)x) && _finite((double)x) && !_isnan((double)y) && _finite((double)y) && !_isnan((double)z) && _finite((double)z);} //is a valid vector? (funky windows _ versions...)
#else
	bool IsValid() const {return !__isnand(x) && finite(x) && !__isnand(y) && finite(y) && !__isnand(z) && finite(z);} //is a valid vector?
#endif

	bool IsNear(const Vec3D& s, const vfloat thresh) {return Dist2(s)<thresh*thresh;}

	//Attributes
	T x, y, z;
	T getX(void) const {return x;};
	T getY(void) const {return y;};
	T getZ(void) const {return z;};
	void setX(const T XIn) {x = XIn;};
	void setY(const T YIn) {y = YIn;};
	void setZ(const T ZIn) {z = ZIn;};

	//Vector operations (change this object)
	vfloat Normalize() {T l = sqrt(x*x+y*y+z*z); if (l > 0) {x /= l;y /= l;z /= l;} return l;} //normalizes this vector
	void NormalizeFast() {T l = sqrt(x*x+y*y+z*z); if (l>0) {T li = 1.0/l;	x*=li; y*=li; z*=li;}} //Make it slightly quicker without return value... 
	Vec3D Rot(const Vec3D u, const T a) {T ca = cos(a); T sa = sin(a); T t = 1-ca; return Vec3D((u.x*u.x*t + ca) * x + (u.x*u.y*t - u.z*sa) * y + (u.z*u.x*t + u.y*sa) * z, (u.x*u.y*t + u.z*sa) * x + (u.y*u.y*t+ca) * y + (u.y*u.z*t - u.x*sa) * z, (u.z*u.x*t - u.y*sa) * x + (u.y*u.z*t + u.x*sa) * y + (u.z*u.z*t + ca) * z);} //rotates by arbitrary vector arbitrary amount (http://www.cprogramming.com/tutorial/3d/rotation.html (Errors! LH one is actually RH one))
	const inline Vec3D<T> Rot(const CQuat<T>& Q) const; //below CQuat for linking sake...

	void RotZ(const T a) {T xt =  x*cos(a) - y*sin(a); T yt = x*sin(a) + y*cos(a); x = xt; y = yt;} //rotates about Z axis "a" radians
	void RotY(const T a) {T xt =  x*cos(a) + z*sin(a); T zt = -x*sin(a) + z*cos(a); x = xt; z = zt;} //rotates about Y axis "a" radians
	void RotX(const T a) {T yt =  y*cos(a) + z*sin(a); T zt = -y*sin(a) + z*cos(a); y = yt; z = zt;} //rotates about X axis "a" radians

	//Vector operations (don't change this object!)
	Vec3D Cross(const Vec3D& v) const {return Vec3D(y*v.z-z*v.y,z*v.x-x*v.z,x*v.y-y*v.x);} //Cross product
	vfloat Dot(const Vec3D& v) const {return (x * v.x + y * v.y + z * v.z);} //Dot product
	Vec3D Abs() const {return Vec3D(x>=0?x:-x, y>=0?y:-y, z>=0?z:-z);} //Absolute value
	Vec3D Normalized() const {T l = sqrt(x*x+y*y+z*z); return (l>0?(*this)/l:(*this));} //returns normalized vec
	Vec3D ProjXY() const {	return Vec3D(x,y,0);} //projection into the xy plane
	T Length() const {return sqrt(x*x+y*y+z*z);} //length of the vector
	T Length2() const {return (x*x+y*y+z*z);} //length squared of the vector
	Vec3D Min(const Vec3D& s) const {return Vec3D(x<s.x ? x:s.x, y<s.y ? y:s.y, z<s.z ? z:s.z);} //min vector of the two
	Vec3D Max(const Vec3D& s) const {return Vec3D(x>s.x ? x:s.x, y>s.y ? y:s.y, z>s.z ? z:s.z);} //max vector of the two
	T Min() const {T Min1 = (x<y ? x:y); return (z<Min1 ? z:Min1);} //minimum element of this vector
	T Max() const {T Max1 = (x>y ? x:y); return (z>Max1 ? z:Max1);} //max element of this vector
	Vec3D Scale(const Vec3D& v) const {return Vec3D(x*v.x, y*v.y, z*v.z);} //scales by another vector
	Vec3D ScaleInv(const Vec3D& v) const {return Vec3D(x/v.x, y/v.y, z/v.z);} //scales by inverse of another vector
	T Dist(const Vec3D& v) const {return sqrt(Dist2(v));} //distance from another vector
	T Dist2(const Vec3D& v) const {return (v.x-x)*(v.x-x)+(v.y-y)*(v.y-y)+(v.z-z)*(v.z-z);} //distance from another vector
	T AlignWith(const Vec3D target, Vec3D& rotax) const {Vec3D thisvec = Normalized(); Vec3D targvec = target.Normalized(); Vec3D rotaxis = thisvec.Cross(targvec); if (rotaxis.Length2() == 0) {rotaxis=target.ArbitraryNormal();} rotax = rotaxis.Normalized(); return acos(thisvec.Dot(targvec));} //returns vector (rotax) and amount (return val) to align this vector with target vector
	Vec3D ArbitraryNormal() const {Vec3D n = Normalized(); if (fabs(n.x) <= fabs(n.y) && fabs(n.x) <= fabs(n.z)){n.x = 1;} else if (fabs(n.y) <= fabs(n.x) && fabs(n.y) <= fabs(n.z)){n.y = 1;}	else {n.z = 1;}	return Cross(n).Normalized();} //generate arbitrary normal

	Vec3D ProjectOnTo(const Vec3D& v) const {return this->Dot(v)*v.Normalized();} // nac: water
};

//quaternion properties (for my reference)
//1) To rotate a vector V, form a quaternion with w = 0; To rotate by Quaternion Q, do Q*V*Q.Conjugate() and trim off the w component.
//2) To do multiple rotations: To Rotate by Q1 THEN Q2, Q2*Q1*V*Q1.Conjugate*Q2.Conjugate(), or make a Qtot = Q2*Q1 and do Qtot*V*Qtot.Conjucate()
//3) Q1*Q1.Conjugate - Identity
//4) To do a reverse rotation Q1, just do Q1.conjugate*V*Q1
//http://www.cprogramming.com/tutorial/3d/quaternions.html

template <typename T>
class CQuat // : public Vec3D //extending Vec3D saves us reimplementing some stuff, I think? this is not a comprehensive quaternion class at this point...
{
public:
	CQuat(void) {Clear();}
	~CQuat(void){};
	
	CQuat(const T dw, const T dx, const T dy, const T dz) {w=dw; x=dx; y=dy; z=dz;} //constructor
	CQuat(const CQuat& QuatIn) {w = QuatIn.w; x = QuatIn.x; y = QuatIn.y; z = QuatIn.z;} //copy constructor
//	CQuat(const CQuat& Quat) {*this = Quat;} //copy constructor
	CQuat(const Vec3D<T>& VecIn) {w = 0; x = VecIn.x; y = VecIn.y; z = VecIn.z;}
	CQuat(const T angle, const Vec3D<T> &axis){Clear(); const T a = angle * (T)0.5; const T s = sin(a); const T c = cos(a); w = c; x = axis.x * s; y = axis.y * s; z = axis.z * s;};
	CQuat(const Vec3D<T> &RotateFrom, const Vec3D<T> &RotateTo){ //probably quicker if we roll in CQuat(angle/axis)
		T theta = acos(RotateFrom.Dot(RotateTo)/sqrt(RotateFrom.Length2()*RotateTo.Length2())); //angle between vectors. from A.B=|A||B|cos(theta)
//		if (theta < DISCARD_ANGLE_RAD) {*this = CQuat(1,0,0,0); return;} //very small angle, return no rotation
		if (theta <= 0) {*this = CQuat(1,0,0,0); return;} //very small angle, return no rotation
		Vec3D<T> Axis = RotateFrom.Cross(RotateTo); //Axis of rotation
		Axis.NormalizeFast();
		if (theta > PI-DISCARD_ANGLE_RAD) {*this = CQuat(Axis); return;} //180 degree rotation (180 degree rot about axis ax, ay, az is Quat(0,ax,ay,az))
		*this = CQuat(theta, Axis); //otherwaise for the quaternion from angle-axis. 
	};

	void Clear() {w=1; x=0; y=0; z=0;} // for (int i=0; i<3; i++){for (int j=0; j<3; j++){M[i][j] = 0;iM[i][j] = 0;}}};

	//Stuff to make code with mixed template parameters work...
	template <typename U> CQuat<T>(const CQuat<U>& QuatIn) {w = QuatIn.w; x = QuatIn.x; y = QuatIn.y; z = QuatIn.z;} //copy constructor
	template <typename U> CQuat<T>(const Vec3D<U>& VecIn) {w = 0; x = VecIn.x; y = VecIn.y; z = VecIn.z;}
	template <typename U> operator CQuat<U>() const {return CQuat<U>(w, x, y, z);} //overload conversion operator for different template types?
	template <typename U> CQuat<T> operator=(const CQuat<U>& s) {w=s.w; x=s.x; y=s.y; z=s.z; return *this; }; //overload equals
	template <typename U> const CQuat<T> operator+(const CQuat<U>& s){return CQuat<T>(w+s.w, x+s.x, y+s.y, z+s.z);}
//	template <typename U> friend Vec3D<T> operator+(Vec3D<T> const& lhs, Vec3D<U> const& rhs){return Vec3D<T>(lhs.x+rhs.x, lhs.y+rhs.y, lhs.z+rhs.z);}
	template <typename U> const CQuat<T> operator*(const U& f) const {return CQuat<T>(f*w, f*x, f*y, f*z);}
	template <typename U> const CQuat<T> operator*(const CQuat<U>& f) const {return CQuat(w*f.w - x*f.x - y*f.y - z*f.z, w*f.x + x*f.w + y*f.z - z*f.y, w*f.y - x*f.z + y*f.w + z*f.x, w*f.z + x*f.y - y*f.x + z*f.w);} //overload Quaternion multiplication!


	CQuat& operator=(const CQuat& s) {w = s.w; x = s.x; y = s.y; z = s.z; return *this; }; //overload equals
	const CQuat operator+(const CQuat& s) const {return CQuat(w+s.w, x+s.x, y+s.y, z+s.z);} //Plus
	const CQuat operator-(const CQuat& s) const {return CQuat(w-s.w, x-s.x, y-s.y, z-s.z);} //Minus
	const CQuat operator*(const vfloat f) const {return CQuat(w*f, x*f, y*f, z*f);} //scalar multiplication
	const CQuat friend operator*(const T f, const CQuat v) {return CQuat(v.w*f, v.x*f, v.y*f, v.z*f);}
	const CQuat operator*(const CQuat& f) const {return CQuat(w*f.w - x*f.x - y*f.y - z*f.z, w*f.x + x*f.w + y*f.z - z*f.y, w*f.y - x*f.z + y*f.w + z*f.x, w*f.z + x*f.y - y*f.x + z*f.w);} //overload Quaternion multiplication!
	bool operator==(const CQuat& s) const {return (w==s.w && x==s.x && y==s.y && z==s.z);} //Is equal
	bool operator!=(const CQuat& s) const {return (w!=s.w || x!=s.x || y!=s.y || z!=s.z);} //Is equal
	const CQuat& operator+=(const CQuat& s) {w += s.w; x += s.x; y += s.y; z += s.z; return *this;} //add and set
	const CQuat& operator-=(const CQuat& s) {w -= s.w; x -= s.x; y -= s.y; z -= s.z; return *this;} //subract and set
	const Vec3D<T> ToVec(void) const {return Vec3D<T>(x, y, z);} //shouldnt be necessary... should be able to just set equal...

	operator Vec3D<T>() const {return Vec3D<T>(x, y, z);};

	
	void FromRotationVector(const Vec3D<T>& RotVector){
		Vec3D<T> Axis = RotVector;
		T Angle = Axis.Normalize(); //Axis.Length();
		*this = CQuat(Angle, Axis);
	}
	void FromAngleToPosX(const Vec3D<T>& RotateFrom){ //highly optimized at the expense of readability
		if (Vec3D<T>(0,0,0) == RotateFrom) return; //leave off if it slows down too much!!

		//Catch and handle small angle:
		T YoverX = RotateFrom.y/RotateFrom.x;
		T ZoverX = RotateFrom.z/RotateFrom.x;
		if (YoverX<SMALL_ANGLE_RAD && YoverX>-SMALL_ANGLE_RAD && ZoverX<SMALL_ANGLE_RAD && ZoverX>-SMALL_ANGLE_RAD){ //Intercept small angle and zero angle
			x=0; y=0.5*ZoverX; z=-0.5*YoverX;
			w = 1+0.5*(-y*y-z*z); //w=sqrt(1-x*x-y*y), small angle sqrt(1+x) ~= 1+x/2 at x near zero.
			return;
		}

		//more accurate non-small angle:
		Vec3D<> RotFromNorm = RotateFrom;
		RotFromNorm.NormalizeFast(); //Normalize the input...

		T theta = acos(RotFromNorm.x); // = acos(RotateFrom.Dot(Vec3d(1,0,0))) because RotFromNorm is normalized, 1,0,0 is normalized, and A.B = |A||B|cos(theta) = cos(theta)
//		vfloat theta = acos(RotateFrom.Dot(Vec3D(1,0,0))); //because RotFromNorm is normalized, 1,0,0 is normalized, and A.B = |A||B|cos(theta) = cos(theta)
		if (theta > PI-DISCARD_ANGLE_RAD) {w=0; x=0; y=1; z=0; return;} //180 degree rotation (about y axis, since the vector must be pointing in -x direction

		const T AxisMagInv = 1.0/sqrt(RotFromNorm.z*RotFromNorm.z+RotFromNorm.y*RotFromNorm.y);
		//Here theta is the angle, axis is RotFromNorm.Cross(Vec3D(1,0,0)) = Vec3D(0, RotFromNorm.z/AxisMagInv, -RotFromNorm.y/AxisMagInv), which is still normalized. (super rolled together)
		//Vec3D Axis = Vec3D(0, RotFromNorm.z*AxisMagInv, -RotFromNorm.y*AxisMagInv); //RotateFrom.Cross(Vec3D(1,0,0)); //is still normalized

		//angle axis function, reduced
		const T a = 0.5*theta;
		const T s = sin(a);
		w=cos(a); x=0; y=RotFromNorm.z*AxisMagInv*s; z = -RotFromNorm.y*AxisMagInv*s;

	} //returns quat to rotate from RotateFrom to positive X direction


	const T Length() const {return sqrt(Length2());} //length of the vector
	const T Length2() const {return (w*w+x*x+y*y+z*z);} //length squared of the vector
	const T Normalize(void) {T l = Length(); if (l == 0){w = 1; x = 0; y = 0; z = 0;} else if (l > 0) {T li = 1.0/l; w*=li; x*=li; y*=li; z*=li;} return l;};
	void NormalizeFast() {
		T l = sqrt(x*x+y*y+z*z+w*w);
		if (l!=0) {T li = 1.0/l;	w*=li; x*=li; y*=li; z*=li;}
		if (w>=1.0){w=1.0; x=0; y=0; z=0;}} //Make it slightly quicker without return value... 
	const CQuat Inverse(void) const {T n = w*w + x*x + y*y + z*z; return CQuat(w/n, -x/n, -y/n, -z/n); };
	const CQuat Conjugate(void) const {return CQuat(w, -x, -y, -z);};

	//Attributes
	T w, x, y, z;

#ifdef MATRIX_OUTPUT_ENABLED
//	vfloat M [3][3];
//	vfloat iM [3][3];
//	void CalcMatrix(void) {M[0][0] = 1.0f-2.0f*(y*y+z*z); M[0][1] = 2.0f*(y*x-z*w); M[0][2] = 2.0f*(z*x+y*w); M[1][1] = 1.0f-2.0f*(x*x+z*z); M[1][2] = 2.0f*(z*y-x*w); M[2][2] = 1.0f-2.0f*(x*x+y*y); M[1][0] = M[0][1]; M[2][1] = M[1][2]; M[2][0] = M[0][2];};
//	void CalciMatrix(void) {vfloat determinant = -M[0][2]*M[1][1]*M[2][0] + M[0][1]*M[1][2]*M[2][0] + M[0][2]*M[1][0]*M[2][1] - M[0][0]*M[1][2]*M[2][1] - M[0][1]*M[1][0]*M[2][2] + M[0][0]*M[1][1]*M[2][2]; vfloat k = 1.0 / determinant; iM[0][0] = (M[1][1]*M[2][2] - M[2][1]*M[1][2])*k; iM[0][1] = (M[2][1]*M[0][2] - M[0][1]*M[2][2])*k; iM[0][2] = (M[0][1]*M[1][2] - M[1][1]*M[0][2])*k; iM[1][0] = (M[1][2]*M[2][0] - M[2][2]*M[1][0])*k; iM[1][1] = (M[2][2]*M[0][0] - M[0][2]*M[2][0])*k; iM[1][2] = (M[0][2]*M[1][0] - M[1][2]*M[0][0])*k; iM[2][0] = (M[1][0]*M[2][1] - M[2][0]*M[1][1])*k; iM[2][1] = (M[2][0]*M[0][1] - M[0][0]*M[2][1])*k; iM[2][2] = (M[0][0]*M[1][1] - M[1][0]*M[0][1])*k;};
#endif

	void AngleAxis(T &angle, Vec3D<T> &axis) const
	{
		T squareLength = 1.0-w*w; //because x*x + y*y + z*z + w*w = 1.0, but more susceptible to w noise (when 
//		if (squareLength < SLTHRESH_DISCARD_ANGLE){angle=0; axis=Vec3D<T>(1,0,0);} //Not worth doing small angle because requires two sqrts
		if (squareLength <= 0){angle=0; axis=Vec3D<T>(1,0,0);} //Not worth doing small angle because requires two sqrts
		else {angle = 2.0*acos(w>1?1:w); axis = Vec3D<T>(x, y, z)/sqrt(squareLength);}
	};

	void AngleAxis(T *pAngle, T* pAxisX, T* pAxisY, T* pAxisZ) const {T Ang = *pAngle; Vec3D<T> Ax = Vec3D<T>(*pAxisX, *pAxisY, *pAxisZ); AngleAxis(Ang, Ax); *pAngle=Ang; *pAxisX = Ax.x; *pAxisY = Ax.y; *pAxisZ = Ax.z; }

	const Vec3D<T> ToRotationVector() const {

//		vfloat squareLength = x*x + y*y + z*z;
		T squareLength = 1.0-w*w; //because x*x + y*y + z*z + w*w = 1.0, but more susceptible to w noise (when 

		//5-21
//		if (squareLength < SLTHRESH_DISCARD_ANGLE) return Vec3D<T>(0,0,0); //solution
		if (squareLength <= 0) return Vec3D<T>(0,0,0); //solution
		else if (squareLength < SLTHRESH_ACOS2SQRT) return Vec3D<T>(x, y, z)*2.0*sqrt((2-2*(w>1?1:w))/squareLength); //acos(w) = sqrt(2*(1-x)) for w close to 1. for w=0.001, error is 1.317e-6
		else return Vec3D<T>(x, y, z)*2.0*acos(w>1?1:w)/sqrt(squareLength);

//		if (squareLength <=0) return Vec3D(0,0,0);
//		else return Vec3D(x, y, z)*2.0*acos(w>1?1:w)/sqrt(squareLength);

		//http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToAngle/
	};


	const T Angle() const {return 2.0*acos(w>1?1:w);}
	bool IsNegligibleAngle() const {return 2.0*acos(w) < DISCARD_ANGLE_RAD;}
	bool IsSmallAngle() const {return w>SMALL_ANGLE_W;} 
	bool IsSmallishAngle() const {return w>SMALLISH_ANGLE_W;}

	const Vec3D<T> RotateVec3D(const Vec3D<T>& f) const { //rotate a vector in the direction of this quaternion
		T tw = f.x*x + f.y*y + f.z*z;
		T tx = f.x*w - f.y*z + f.z*y;
		T ty = f.x*z + f.y*w - f.z*x;
		T tz = -f.x*y + f.y*x + f.z*w;
		return Vec3D<T>(w*tx + x*tw + y*tz - z*ty, w*ty - x*tz + y*tw + z*tx, w*tz + x*ty - y*tx + z*tw);
	}
	const Vec3D<T> RotateVec3DInv(const Vec3D<T>& f) const { //rotate a vector in the opposite(inverse) direction of this quaternion
		T tw = x*f.x + y*f.y + z*f.z;
		T tx = w*f.x - y*f.z + z*f.y;
		T ty = w*f.y + x*f.z - z*f.x;
		T tz = w*f.z - x*f.y + y*f.x;

#ifdef _DEBUG
		Vec3D<T> RetVec = Vec3D<T>(tw*x + tx*w + ty*z - tz*y, tw*y - tx*z + ty*w + tz*x, tw*z + tx*y - ty*x + tz*w);
		if (Vec3D<T>(0,0,0) != f && CQuat<T>(1,0,0,0) != *this && RetVec == f)
			int a = 0;
		return RetVec;
#else
		return Vec3D<T>(tw*x + tx*w + ty*z - tz*y, tw*y - tx*z + ty*w + tz*x, tw*z + tx*y - ty*x + tz*w);	
#endif
	}

	
template <typename U> const Vec3D<U> RotateVec3DInv(const Vec3D<U>& f) const { //rotate a vector in the opposite(inverse) direction of this quaternion
		T tw = x*f.x + y*f.y + z*f.z;
		T tx = w*f.x - y*f.z + z*f.y;
		T ty = w*f.y + x*f.z - z*f.x;
		T tz = w*f.z - x*f.y + y*f.x;

#ifdef _DEBUG
		Vec3D<U> RetVec = Vec3D<U>(tw*x + tx*w + ty*z - tz*y, tw*y - tx*z + ty*w + tz*x, tw*z + tx*y - ty*x + tz*w);
		if (Vec3D<U>(0,0,0) != f && CQuat<T>(1,0,0,0) != *this && RetVec == f)
			int a = 0;
		return RetVec;
#else
		return Vec3D<U>(tw*x + tx*w + ty*z - tz*y, tw*y - tx*z + ty*w + tz*x, tw*z + tx*y - ty*x + tz*w);	
#endif
	}

};

template <typename T> const inline Vec3D<T> Vec3D<T>::Rot(const CQuat<T>& Q) const {return (Q*CQuat<T>(*this)*Q.Conjugate()).ToVec();}

#endif //_VEC3D_H
