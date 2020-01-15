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
#ifndef _INCLUDED_EDGEDETECT_SINGLEPORT_H_
#define _INCLUDED_EDGEDETECT_SINGLEPORT_H_

// Revision History
//    Rev 1 - Coding of edge detection algorithm in C++
//    Rev 2 - Converted to using bit-accurate data types
//            Calculated bit growth for internal variables
//            Quantized angle values for 5 fractional bits -pi to pi
//    Rev 3 - Switch to using HLSLIBS ac_math library for high performance
//            math functions.
//            Add support for verification using SCVerify
//    Rev 4 - Refinining memory architecture for 1PPC
//    Rev 5 - Modularized into hierarchy for performance
//    Rev 6 - Recode to use single-port memories
//    Rev 7 - Recode to make the design programmable for image size

#include <ac_fixed.h>
// This is Catapult's math library implementation, see docs for details
#include <ac_math/ac_sqrt_pwl.h>
#include <ac_math/ac_atan2_cordic.h>

// Class for fifo-style hierarchical interconnect objects
#include <ac_channel.h>

// Include constant kernel definition
#include "edge_defs.h"
#include <mc_scverify.h>

template <int imageWidth, int imageHeight>
class EdgeDetect_SinglePort
{
  // Define some bit-accurate types to use in this model
  typedef uint8                  pixelType;    // input pixel is 0-255
  typedef uint16                 pixelType2x;  // two pixels packed
  typedef int9                   gradType;     // Derivative is max range -255 to 255
  typedef uint18                 sqType;       // Result of 9-bit x 9-bit
  typedef ac_fixed<19,19,false>  sumType;      // Result of 18-bit + 18-bit fixed pt integer for squareroot
  typedef uint9                  magType;      // 9-bit unsigned magnitute result
  typedef ac_fixed<8,3,true>     angType;      // 3 integer bit, 5 fractional bits for quantized angle -pi to pi

  // Static interconnect channels (FIFOs) between blocks
  ac_channel<gradType>       dy;
  ac_channel<gradType>       dx;
  ac_channel<pixelType>      dat; // channel for passing input pixels to horizontalDerivative

public:
  //Compute number of bits for max image size count, used internally and in testbench
  typedef ac_int<ac::nbits<imageWidth+1>::val,false> maxW;
  typedef ac_int<ac::nbits<imageHeight+1>::val,false> maxH;
  EdgeDetect_SinglePort() {}

  //--------------------------------------------------------------------------
  // Function: run
  //   Top interface for data in/out of class. Combines vertical and
  //   horizontal derivative and magnitude/angle computation.
#pragma hls_design interface
  void CCS_BLOCK(run)(ac_channel<pixelType> &dat_in,
                      maxW                  &widthIn,
                      maxH                  &heightIn,
                      ac_channel<magType>   &magn,
                      ac_channel<angType>   &angle) 
  {
    verticalDerivative(dat_in, widthIn, heightIn, dat, dy);
    horizontalDerivative(dat, widthIn, heightIn, dx);
    magnitudeAngle(dx, dy, widthIn, heightIn, magn, angle);
  }

private:
  //--------------------------------------------------------------------------
  // Function: verticalDerivative
  //   Compute the vertical derivative on the input data
#pragma hls_design
  void verticalDerivative(ac_channel<pixelType> &dat_in,
                          maxW                  &widthIn,
                          maxH                  &heightIn,
                          ac_channel<pixelType> &dat_out,
                          ac_channel<gradType>  &dy) 
  {
    // Line buffers store pixel line history - Mapped to RAM
    pixelType2x line_buf0[imageWidth/2];
    pixelType2x line_buf1[imageWidth/2];
    pixelType2x rdbuf0_pix, rdbuf1_pix;
    pixelType2x wrbuf0_pix, wrbuf1_pix;
    pixelType pix0, pix1, pix2;
    gradType pix;

    // Remove loop upperbounds for RTL code coverage
    // Use bit accurate data types on loop iterator
    VROW: for (maxH y = 0;; y++) { // One extra iteration to ramp-up window
      VCOL: for (maxW x = 0;; x++) {
        if (y <= imageHeight-1) {
          pix0 = dat_in.read(); // Read streaming interface
        }
        // Write data cache, write lower 8 on even iterations of COL loop, upper 8 on odd
        if ( (x&1) == 0 ) {
          wrbuf0_pix.set_slc(0,pix0);
        } else {
          wrbuf0_pix.set_slc(8,pix0);
        }
        // Read line buffers into read buffer caches on even iterations of COL loop
        if ( (x&1) == 0 ) {
          // vertical window of pixels
          rdbuf1_pix = line_buf1[x/2];
          rdbuf0_pix = line_buf0[x/2];
        } else { // Write line buffer caches on odd iterations of COL loop
          line_buf1[x/2] = rdbuf0_pix; // copy previous line
          line_buf0[x/2] = wrbuf0_pix; // store current line
        }
        // Get 8-bit data from read buffer caches, lower 8 on even iterations of COL loop
        pix2 = ((x&1)==0) ? rdbuf1_pix.slc<8>(0) : rdbuf1_pix.slc<8>(8);
        pix1 = ((x&1)==0) ? rdbuf0_pix.slc<8>(0) : rdbuf0_pix.slc<8>(8);

        // Boundary condition processing
        if (y == 1) {
          pix2 = pix1; // top boundary (replicate pix1 up to pix2)
        }
        if (y == imageHeight) {
          pix0 = pix1; // bottom boundary (replicate pix1 down to pix0)
        }

        // Calculate derivative
        pix = pix2*kernel[0] + pix1*kernel[1] + pix0*kernel[2];

        if (y != 0) { // Write streaming interfaces
          dat_out.write(pix1); // Pass thru original data
          dy.write(pix); // derivative output
        }
        // programmable width exit condition
        if (x == maxW(widthIn-1)) // cast to maxW for RTL code coverage
          break; 
      }
      //programmable height exit condition
      if (y == heightIn)
        break;
    }
  }

  //--------------------------------------------------------------------------
  // Function: horizontalDerivative
  //   Compute the horizontal derivative on the input data
#pragma hls_design
  void horizontalDerivative(ac_channel<pixelType> &dat_in,
                            maxW                  &widthIn,
                            maxH                  &heightIn,
                            ac_channel<gradType>  &dx) 
  {
    // pixel buffers store pixel history
    pixelType pix_buf0;
    pixelType pix_buf1;

    pixelType pix0 = 0;
    pixelType pix1 = 0;
    pixelType pix2 = 0;

    gradType  pix;

    HROW: for (maxH y = 0; ; y++) {
      HCOL: for (maxW x = 0; ; x++) { // One extra iteration to ramp-up window
        pix2 = pix_buf1;
        pix1 = pix_buf0;
        if (x <= imageWidth-1) {
          pix0 = dat_in.read(); // Read streaming interface
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
        pix = pix2*kernel[0] + pix1*kernel[1] + pix0*kernel[2];

        if (x != 0) { // Write streaming interface
          dx.write(pix); // derivative out
        }
        // programmable width exit condition
        if ( x == widthIn)
          break;
      }
      // programmable height exit condition
      if (y == maxH(heightIn-1)) // cast to maxH for RTL code coverage
        break; 
    }
  }

  //--------------------------------------------------------------------------
  // Function: magnitudeAngle
  //   Compute the magnitute and angle based on the horizontal and vertical
  //   derivative results
#pragma hls_design
  void magnitudeAngle(ac_channel<gradType> &dx_in,
                      ac_channel<gradType> &dy_in,
                      maxW                 &widthIn,
                      maxH                 &heightIn,
                      ac_channel<magType>  &magn,
                      ac_channel<angType>  &angle) 
  {
    gradType dx, dy;
    sqType dx_sq;
    sqType dy_sq;
    sumType sum; // fixed point integer for sqrt
    angType at;
    ac_fixed<16,9,false> sq_rt; // square-root return type

    MROW: for (maxH y = 0; ; y++) {
      MCOL: for (maxW x = 0; ; x++) {
        dx = dx_in.read();
        dy = dy_in.read();
        dx_sq = dx * dx;
        dy_sq = dy * dy;
        sum = dx_sq + dy_sq;
        // Catapult's math library piecewise linear implementation of sqrt and atan2
        ac_math::ac_sqrt_pwl(sum,sq_rt);
        magn.write(sq_rt.to_uint());
        ac_math::ac_atan2_cordic((ac_fixed<9,9>)dy, (ac_fixed<9,9>) dx, at);
        angle.write(at);
        // programmable width exit condition
        if (x == maxW(widthIn-1)) // cast to maxW for RTL code coverage
          break;
      }
      // programmable height exit condition
      if (y == maxH(heightIn-1)) // cast to maxH for RTL code coverage
        break;
    }
  }

};

#endif

