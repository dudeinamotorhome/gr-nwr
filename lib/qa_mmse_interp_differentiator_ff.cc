/* -*- c++ -*- */
/*
 * Copyright 2002,2007,2012 Free Software Foundation, Inc.
 * Copyright (C) 2017  Andy Walls <awalls.cx18@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this file; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cppunit/TestAssert.h>
#include <qa_mmse_interp_differentiator_ff.h>
#include <nwr/mmse_interp_differentiator_ff.h>
#include <gnuradio/fft/fft.h>
#include <volk/volk.h>
#include <cstdio>
#include <cmath>
#include <stdexcept>
#include <stdint.h>

namespace gr {
  namespace nwr {

    static const double k1 = 0.25 * 2 * M_PI;
    static const double k2 = 0.125 * M_PI;
    static const double k3 = 0.077 * 2 * M_PI;
    static const double k4 = 0.3 * M_PI;

    static float
    test_fcn(double index)
    {
      return (2 * sin (index * k1 + k2) + 3 * sin (index * k3 + k4));
    }

    static float
    test_fcn_d(double index)
    {
      return (k1 * 2 * cos (index * k1 + k2) + k3 * 3 * cos (index * k3 + k4));
    }

    void
    qa_mmse_interp_differentiator_ff::t1()
    {
      static const unsigned N = 100;
      float *input = (float*)volk_malloc((N + 10)*sizeof(float),
                                                   volk_get_alignment());

      for(unsigned i = 0; i < N+10; i++)
	input[i] = test_fcn((double) i);

      mmse_interp_differentiator_ff diffr;
      float inv_nsteps = 1.0 / diffr.nsteps();

      for(unsigned i = 0; i < N; i++) {
	for(unsigned imu = 0; imu <= diffr.nsteps (); imu += 1) {
	  float expected = test_fcn_d((i + 3) + imu * inv_nsteps);
	  float actual = diffr.differentiate(&input[i], imu * inv_nsteps);

	  CPPUNIT_ASSERT_DOUBLES_EQUAL(expected, actual, 0.004);
	  // printf ("%9.6f  %9.6f  %9.6f\n", expected, actual, expected - actual);
	}
      }
      volk_free(input);
    }

  } /* namespace nwr */
} /* namespace gr */
