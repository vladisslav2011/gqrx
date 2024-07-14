/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2024 Vladislav P.
 *
 * Gqrx is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * Gqrx is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Gqrx; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */
#include <math.h>
#include <gnuradio/io_signature.h>
#include "dsp/rx_rejector.h"

static const int MIN_IN = 1;  /* Minimum number of input streams. */
static const int MAX_IN = 1;  /* Maximum number of input streams. */
static const int MIN_OUT = 1; /* Minimum number of output streams. */
static const int MAX_OUT = 1; /* Maximum number of output streams. */


 /** Rejector **/

/*
 * Create a new instance of rx_rejector_cc and return
 * a shared_ptr. This is effectively the public constructor.
 */
rx_rejector_cc::sptr rx_rejector_cc::make(double sample_rate, double offset, double bw, double alfa)
{
    return gnuradio::get_initial_sptr(new rx_rejector_cc(sample_rate, offset, bw, alfa));
}

rx_rejector_cc::rx_rejector_cc(double sample_rate, double offset, double bw, double alfa)
    : gr::hier_block2 ("rx_rejector",
                      gr::io_signature::make (MIN_IN, MAX_IN, sizeof (gr_complex)),
                      gr::io_signature::make (MIN_OUT, MAX_OUT, sizeof (gr_complex))),
    d_sample_rate(sample_rate),
    d_offset(offset),
    d_bw(bw)
{
    d_pll = gr::analog::pll_refout_cc::make(0.0001,
        2. * M_PI * (offset + bw) / d_sample_rate,
        2. * M_PI * (offset - bw) / d_sample_rate);
    d_fftaps.resize(2);
    d_fbtaps.resize(2);
    d_fftaps[0] = 1.0;
    d_fftaps[1] = -1.0;
    d_fbtaps[0] = 0.0;
    d_fbtaps[1] = 1.-alfa;
    d_dcr = gr::filter::iir_filter_ccd::make(d_fftaps, d_fbtaps);
    d_fwd = gr::blocks::multiply_conjugate_cc::make();
    d_bwd = gr::blocks::multiply_cc::make();

    connect(self(), 0, d_pll, 0);
    connect(self(), 0, d_fwd, 0);
    connect(d_pll, 0, d_fwd, 1);
    connect(d_fwd, 0, d_dcr, 0);
    connect(d_dcr, 0, d_bwd, 0);
    connect(d_pll, 0, d_bwd, 1);
    connect(d_bwd, 0, self(), 0);
}


rx_rejector_cc::~rx_rejector_cc()
{

}


void rx_rejector_cc::set_sample_rate(double rate)
{
    if(d_sample_rate == rate)
        return;
    d_sample_rate = rate;
    d_pll->set_min_freq(2. * M_PI * (d_offset - d_bw) / d_sample_rate);
    d_pll->set_max_freq(2. * M_PI * (d_offset + d_bw) / d_sample_rate);
}

void rx_rejector_cc::set_offset(double offset)
{
    if(d_offset == offset)
        return;
    d_offset = offset;
    d_pll->set_min_freq(2. * M_PI * (d_offset - d_bw) / d_sample_rate);
    d_pll->set_max_freq(2. * M_PI * (d_offset + d_bw) / d_sample_rate);
}


void rx_rejector_cc::set_bw(double bw)
{
    if(d_bw == bw)
        return;
    d_bw = bw;
    d_pll->set_min_freq(2. * M_PI * (d_offset - d_bw) / d_sample_rate);
    d_pll->set_max_freq(2. * M_PI * (d_offset + d_bw) / d_sample_rate);
}


void rx_rejector_cc::set_alfa(double alfa)
{
    d_fbtaps[1] = 1.-alfa;
    d_dcr->set_taps(d_fftaps, d_fbtaps);
}
