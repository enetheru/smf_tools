// float3.cpp: implementation of the float3 class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

float float3::maxxpos=2048;
float float3::maxzpos=2048;

bool float3::CheckInBounds()
{
	bool in=true;
	if(x<0){
		x=0;
		in=false;
	}
	if(z<0){
		z=0;
		in=false;
	}
	if(x>maxxpos){
		x=maxxpos;
		in=false;
	}
	if(z>maxzpos){
		z=maxzpos;
		in=false;
	}

	return in;
}