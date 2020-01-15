/**************************************************************************
 *                                                                        *
 *  Edge Detect Design Walkthrough for HLS                                *
 *                                                                        *
 *  Software Version: 1.0                                                 *
 *                                                                        *
 *  Release Date    : Tue Jan 14 15:40:43 PST 2020                        *
 *  Release Type    : Production Release                                  *
 *  Release Build   : 1.0.0                                               *
 *                                                                        *
 *  Copyright 2020, Mentor Graphics Corporation,                          *
 *                                                                        *
 *  All Rights Reserved.                                                  *
 *  
 **************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      * 
 *  You may obtain a copy of the License at                               *
 *                                                                        *
 *      http://www.apache.org/licenses/LICENSE-2.0                        *
 *                                                                        *
 *  Unless required by applicable law or agreed to in writing, software   * 
 *  distributed under the License is distributed on an "AS IS" BASIS,     * 
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or       *
 *  implied.                                                              * 
 *  See the License for the specific language governing permissions and   * 
 *  limitations under the License.                                        *
 **************************************************************************
 *                                                                        *
 *  The most recent version of this package is available at github.       *
 *                                                                        *
 *************************************************************************/
#ifndef _INCLUDED_EDGEDETECT_SYNTHESIZABLE_H_
#define _INCLUDED_EDGEDETECT_SYNTHESIZABLE_H_

// Revision History
//    Rev 1 - Coding of edge detection algorithm in C++
//    Rev 2 - Converted to using bit-accurate data types
//            Calculated bit growth for internal variables
//            Quantized angle values for 5 fractional bits -pi to pi
//    Rev 3 - Switch to using HLSLIBS ac_math library for high performance
//            math functions.
//            Add support for verification using SCVerify

#include <ac_fixed.h>
// This is Catapult's math library implementation, see docs for details
#include <ac_math/ac_sqrt_pwl.h>
#include <ac_math/ac_atan2_cordic.h>

// Include constant kernel definition
#include "edge_defs.h"
#include <mc_scverify.h>

class EdgeDetect_Synthesizable
{
  // Define some bit-accurate types to use in this model
  typedef uint8                  pixelType;    // input pixel is 0-255
  typedef int9                   gradType;     // Gradient is max range -255 to 255
  typedef uint18                 sqType;       // Result of 9-bit x 9-bit
  typedef ac_fixed<19,19,false>  sumType;      // Result of 18-bit + 18-bit fixed pt integer for squareroot
  typedef uint9                  magType;      // 9-bit unsigned magnitute result
  typedef ac_fixed<8,3,true>     angType;      // 3 integer bit, 5 fractional bits for quantized angle -pi to pi

  // Define some "constants" for use in algorithm
  enum {
    imageWidth  = 1296,
    imageHeight =  864
  };

public:
  // Constructor
  EdgeDetect_Synthesizable() {}

  //--------------------------------------------------------------------------
  // Function: run
  //   Top interface for data in/out of class. Combines vertical and 
  //   horizontal derivative and magnitude/angle computation.
#pragma hls_design interface
  void CCS_BLOCK(run)(pixelType dat_in[imageHeight][imageWidth],
                      magType   magn[imageHeight][imageWidth],
                      angType   angle[imageHeight][imageWidth]) 
  {
    // allocate buffers for image data
    gradType dx[imageHeight][imageWidth];
    gradType dy[imageHeight][imageWidth];

    verticalDerivative(dat_in, dy);
    horizontalDerivative(dat_in, dx);
    magnitudeAngle(dx, dy, magn, angle);
  }

  //--------------------------------------------------------------------------
  // Function: verticalDerivative
  //   Compute the vertical derivative on the input data
  void verticalDerivative(pixelType dat_in[imageHeight][imageWidth],
                          gradType  dy[imageHeight][imageWidth]) 
  {
    VROW: for (int y = 0; y < imageHeight; y++) {
      VCOL: for (int x = 0; x < imageWidth; x++) {
        dy[y][x] =
          dat_in[clip(y - 1, imageHeight-1)][x] * kernel[0] +
          dat_in[y][x]                          * kernel[1] +
          dat_in[clip(y + 1, imageHeight-1)][x] * kernel[2];
      }
    }
  }

  //--------------------------------------------------------------------------
  // Function: horizontalDerivative
  //   Compute the horizontal derivative on the input data
  void horizontalDerivative(pixelType dat_in[imageHeight][imageWidth],
                            gradType  dx[imageHeight][imageWidth]) 
  {
    HROW: for (int y = 0; y < imageHeight; y++) {
      HCOL: for (int x = 0; x < imageWidth; x++) {
        dx[y][x] =
          dat_in[y][clip(x - 1, imageWidth-1)] * kernel[0] +
          dat_in[y][x]                         * kernel[1] +
          dat_in[y][clip(x + 1, imageWidth-1)] * kernel[2];
      }
    }
  }

  //--------------------------------------------------------------------------
  // Function: magnitudeAngle
  //   Compute the magnitute and angle based on the horizontal and vertical
  //   derivative results
  void magnitudeAngle(gradType dx[imageHeight][imageWidth],
                      gradType dy[imageHeight][imageWidth],
                      magType  magn[imageHeight][imageWidth],
                      angType  angle[imageHeight][imageWidth]) 
  {
    sqType dx_sq;
    sqType dy_sq;
    sumType sum; // fixed point integer for sqrt
    angType at;
    ac_fixed<16,9,false> sq_rt; // square-root return type

    MROW: for (int y = 0; y < imageHeight; y++) {
      MCOL: for (int x = 0; x < imageWidth; x++) {
        dx_sq = dx[y][x] * dx[y][x];
        dy_sq = dy[y][x] * dy[y][x];
        sum = dx_sq + dy_sq;
        // Catapult's math library piecewise linear implementation of sqrt and atan2
        ac_math::ac_sqrt_pwl(sum,sq_rt);
        magn[y][x] = sq_rt.to_uint();
        ac_math::ac_atan2_cordic((ac_fixed<9,9>) (dy[y][x]), (ac_fixed<9,9>) (dx[y][x]), at);
        angle[y][x] = at;
      }
    }
  }

private: // Helper functions

  //--------------------------------------------------------------------------
  // Function: clip
  //   Perform boundary processing by "adjusting" the index value to "clip"
  //   at either end
  int clip(int i, int bound) {
    if (i < 0) {
      return 0;               // clip to the top/left value
    } else if (i >= bound) {
      return bound;           // clip to the bottom/right value
    } else {
      return i;               // return all others untouched
    }
  }

  //--------------------------------------------------------------------------
  // Function: myPrint
  void myPrint(char a[]) {
#ifndef __SYNTHESIS__
    printf("%s\n",a);
#endif
  }

};

#endif

