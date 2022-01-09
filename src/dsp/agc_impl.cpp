//////////////////////////////////////////////////////////////////////
// agc_impl.cpp: implementation of the CAgc class.
//
//  This class implements an automatic gain function.
//
// History:
//	2010-09-15  Initial creation MSW
//	2011-03-27  Initial release
//      2011-09-24  Adapted for gqrx
//////////////////////////////////////////////////////////////////////
//==========================================================================================
// + + +   This Software is released under the "Simplified BSD License"  + + +
//Copyright 2010 Moe Wheatley. All rights reserved.
//
//Redistribution and use in source and binary forms, with or without modification, are
//permitted provided that the following conditions are met:
//
//   1. Redistributions of source code must retain the above copyright notice, this list of
//	  conditions and the following disclaimer.
//
//   2. Redistributions in binary form must reproduce the above copyright notice, this list
//	  of conditions and the following disclaimer in the documentation and/or other materials
//	  provided with the distribution.
//
//THIS SOFTWARE IS PROVIDED BY Moe Wheatley ``AS IS'' AND ANY EXPRESS OR IMPLIED
//WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
//FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Moe Wheatley OR
//CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
//ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
//ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//The views and conclusions contained in the software and documentation are those of the
//authors and should not be interpreted as representing official policies, either expressed
//or implied, of Moe Wheatley.
//==========================================================================================

#include <dsp/agc_impl.h>
#include <math.h>
#include <string.h>

CAgc::CAgc()
{

}

CAgc::~CAgc()
{

}

void CAgc::SetParameters(double sample_rate, bool agc_on, int target_level, int manual_gain, int max_gain, int attack, int decay, int hang)
{

}

void CAgc::ProcessData(float * pOutData, const float * pInData, int Length)
{
    memcpy(pOutData, pInData, Length * sizeof(pOutData[0]));
}

void CAgc::ProcessData(TYPECPX * pOutData, const TYPECPX * pInData, int Length)
{
    memcpy(pOutData, pInData, Length * sizeof(pOutData[0]));
}
