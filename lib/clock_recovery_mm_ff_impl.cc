/* -*- c++ -*- */
/*
 * Copyright 2004,2010-2012,2014 Free Software Foundation, Inc.
 * Copyright (C) 2016  Andy Walls <awalls.cx18@gmail.com>
 *
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "clock_recovery_mm_ff_impl.h"
#include <gnuradio/io_signature.h>
#include <gnuradio/math.h>
#include <stdexcept>

namespace gr {
  namespace nwr {

    clock_recovery_mm_ff::sptr
    clock_recovery_mm_ff::make(float omega, float gain_omega,
                               float mu, float gain_mu,
                               float omega_relative_limit)
    {
      return gnuradio::get_initial_sptr
        (new clock_recovery_mm_ff_impl(omega, gain_omega,
                                       mu, gain_mu,
                                       omega_relative_limit));
    }

    clock_recovery_mm_ff_impl::clock_recovery_mm_ff_impl(
                                                  float omega, float gain_omega,
                                                  float mu, float gain_mu,
                                                  float omega_relative_limit)
      : block("clock_recovery_mm_ff",
              io_signature::make(1, 1, sizeof(float)),
              io_signature::make(1, 1, sizeof(float))),
        d_mu(mu), d_gain_mu(gain_mu), d_gain_omega(gain_omega),
        d_omega_relative_limit(omega_relative_limit),
        d_prev_y(0.0f),
        d_interp(new filter::mmse_fir_interpolator_ff()),
        d_prev_mu(mu),
        d_prev2_y(0.0f)
    {
      if(omega <  1)
        throw std::out_of_range("clock rate must be > 0");
      if(gain_mu <  0  || gain_omega < 0)
        throw std::out_of_range("Gains must be non-negative");

      set_omega(omega); // also sets min and max omega
      d_prev_decision = slice(d_prev_y);
      d_prev2_decision = slice(d_prev2_y);
      set_relative_rate (1.0 / omega);
      enable_update_rate(true);  // fixes tag propagation through variable rate block
    }

    clock_recovery_mm_ff_impl::~clock_recovery_mm_ff_impl()
    {
      delete d_interp;
    }

    void
    clock_recovery_mm_ff_impl::forecast(int noutput_items,
                                        gr_vector_int &ninput_items_required)
    {
      unsigned ninputs = ninput_items_required.size();
      // The '+ 2' in the expression below is an effort to always have at least
      // one output sample, even if the main loop decides it has to revert
      // one computed sample and wait for the next call to general_work().
      // The d_omega_mid + d_omega_lim is also an effort to do the same,
      // in case we have the worst case allowable clock timing deviation on
      // input.
      for(unsigned i=0; i < ninputs; i++)
        ninput_items_required[i] = static_cast<int>(
                     ceil((noutput_items + 2) * (d_omega_mid + d_omega_lim)
                     + d_interp->ntaps()));
    }

    float
    clock_recovery_mm_ff_impl::slice(float x)
    {
      return x < 0 ? -1.0F : 1.0F;
    }

    void
    clock_recovery_mm_ff_impl::set_omega (float omega)
    {
      d_prev_omega = omega;
      d_omega = omega;
      d_omega_mid = omega;
      d_omega_lim = d_omega_mid * d_omega_relative_limit;
    }

    float
    clock_recovery_mm_ff_impl::timing_error_detector(float curr_y)
    {
        float error;
        float curr_decision = slice(curr_y);

        error = d_prev_decision * curr_y - curr_decision * d_prev_y;

        d_prev2_y = d_prev_y;
        d_prev2_decision = d_prev_decision;

        d_prev_y = curr_y;
        d_prev_decision = curr_decision;

        return error;
    }

    void
    clock_recovery_mm_ff_impl::revert_timing_error_detector_state()
    {
        d_prev_y = d_prev2_y;
        d_prev_decision = d_prev2_decision;
    }

    void
    clock_recovery_mm_ff_impl::symbol_period_limit()
    {
        d_omega = d_omega_mid
                  + gr::branchless_clip(d_omega-d_omega_mid, d_omega_lim);
    }

    void
    clock_recovery_mm_ff_impl::advance_loop(float error)
    {
        d_prev_omega = d_omega;
        d_prev_mu = d_mu;

        d_omega = d_omega + d_gain_omega * error;
        d_mu = d_mu + d_omega + d_gain_mu * error;
    }

    void
    clock_recovery_mm_ff_impl::revert_loop_state()
    {
        d_omega = d_prev_omega;
        d_mu = d_prev_mu;
    }

    int
    clock_recovery_mm_ff_impl::clock_sample_phase_wrap()
    {
        float whole_samples_until_clock = floorf(d_mu);
        d_mu = d_mu - whole_samples_until_clock;
        return static_cast<int>(whole_samples_until_clock);
    }

    int
    clock_recovery_mm_ff_impl::general_work(
                                         int noutput_items,
                                         gr_vector_int &ninput_items,
                                         gr_vector_const_void_star &input_items,
                                         gr_vector_void_star &output_items)
    {
      const float *in = (const float *)input_items[0];
      float *out = (float *)output_items[0];

      int ni = ninput_items[0] - d_interp->ntaps(); // max input to consume
      if (ni <= 0)
          return 0;

      int ii = 0; // input index
      int oo = 0; // output index
      float error;
      int n;

      while (oo < noutput_items) {
        // produce output sample
        out[oo] = d_interp->interpolate(&in[ii], d_mu);

        error = timing_error_detector(out[oo]);

        advance_loop(error);
        n = clock_sample_phase_wrap();
        symbol_period_limit();

        ii += n;
        oo++;
        if (ii >= ni) {
            // This check and revert is needed when the samples per
            // symbol is greater than d_interp->ntaps() (normally 8);
            // otherwise we would consume() more input than we were
            // given.
            revert_loop_state();
            revert_timing_error_detector_state();
            ii -= n;
            oo--;
            break;
        }
      }

      consume_each(ii);
      return oo;
    }

  } /* namespace nwr */
} /* namespace gr */
