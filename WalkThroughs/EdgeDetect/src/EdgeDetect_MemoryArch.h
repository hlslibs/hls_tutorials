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
#ifndef _INCLUDED_EDGEDETECT_MEMORYARCH_H_
#define _INCLUDED_EDGEDETECT_MEMORYARCH_H_

// Revision History
//    Rev 1 - Coding of edge detection algorithm in C++
//    Rev 2 - Converted to using bit-accurate data types
//            Calculated bit growth for internal variables
//            Quantized angle values for 5 fractional bits -pi to pi
//    Rev 3 - Switch to using HLSLIBS ac_math library for high performance
//            math functions.
//            Add support for verification using SCVerify
//    Rev 4 - Refinining memory architecture for 1PPC

#include <ac_fixed.h>
// This is Catapult's math library implementation, see docs for details
#include <ac_math/ac_sqrt_pwl.h>
#include <ac_math/ac_atan2_cordic.h>

// Include constant kernel definition
#include "edge_defs.h"
#include <mc_scverify.h>

class EdgeDetect_MemoryArch
{
  // Define some bit-accurate types to use in this model
  typedef uint8                  pixelType;    // input pixel is 0-255
  typedef int9                   gradType;     // Derivative is max range -255 to 255
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
  EdgeDetect_MemoryArch() {}

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
    horizontalDerivative(dat_in,dx);
    magnitudeAngle(dx, dy, magn, angle);
  }

  //--------------------------------------------------------------------------
  // Function: verticalDerivative
  //   Compute the vertical derivative on the input data
  void verticalDerivative(pixelType dat_in[imageHeight][imageWidth],
                          gradType  dy[imageHeight][imageWidth])
  {
    // Line buffers store pixel line history - Mapped to RAM
    pixelType line_buf0[imageWidth];
    pixelType line_buf1[imageWidth];
    pixelType pix0,pix1, pix2;

    VROW: for (int y = 0; y < imageHeight+1; y++) { // One extra iteration to ramp-up window
      VCOL: for (int x = 0; x < imageWidth; x++) {
        // vertical window of pixels
        pix2 = line_buf1[x];
        pix1 = line_buf0[x];
        if (y <= imageHeight-1) {
          pix0 = dat_in[y][x];   // Read memory while in bounds
        }
        line_buf1[x] = pix1; // copy previous line
        line_buf0[x] = pix0; // store current line
        // Boundary condition processing
        if (y == 1) {
          pix2 = pix1; // top boundary (replicate pix1 up to pix2)
        }
        if (y == imageHeight) {
          pix0 = pix1; // bottom boundary (replicate pix1 down to pix0)
        }

        // Calculate derivative
        if (y > 0) {
          // wait till window ramp-up and adjust index i
          dy[y-1][x] = pix2*kernel[0] + pix1*kernel[1] + pix0*kernel[2];
        }
      }
    }
  }

  //--------------------------------------------------------------------------
  // Function: horizontalDerivative
  //   Compute the horizontal derivative on the input data
  void horizontalDerivative(pixelType dat_in[imageHeight][imageWidth],
                            gradType  dx[imageHeight][imageWidth]) 
  {
    // pixel buffers store pixel history
    pixelType pix_buf0;
    pixelType pix_buf1;

    pixelType pix0 = 0;
    pixelType pix1 = 0;
    pixelType pix2 = 0;

    HROW: for (int y = 0; y < imageHeight; y++) {
      HCOL: for (int x = 0; x < imageWidth+1; x++) {
        pix2 = pix_buf1;
        pix1 = pix_buf0;
        if (x <= imageWidth-1) {
          pix0 = dat_in[y][x]; // Read memory while in bounds
        }
        if (x == 1) {
          pix2 = pix1; // left boundary condition (replicate pix1 left to pix2)
        }
        if (x == imageWidth) {
          pix0 = pix1; // right boundary condition (replicate pix1 right to pix0)
        }

        pix_buf1 = pix_buf0;
        pix_buf0 = pix0;
        // Calculate derivative
        if (x > 0) {
          // wait till window ramp-up and adjust index j
          dx[y][x-1] = pix2*kernel[0] + pix1*kernel[1] + pix0*kernel[2];
        }
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

};

#endif

