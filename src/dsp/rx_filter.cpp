/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2011 Alexandru Csete OZ9AEC.
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
#include <cmath>
#include <gnuradio/io_signature.h>
#include <gnuradio/filter/firdes.h>
#include <volk/volk.h>
#include <iostream>
#include <QDebug>
#include "dsp/rx_filter.h"

static const int MIN_IN = 1;  /* Minimum number of input streams. */
static const int MAX_IN = 1;  /* Maximum number of input streams. */
static const int MIN_OUT = 1; /* Minimum number of output streams. */
static const int MAX_OUT = 1; /* Maximum number of output streams. */


/*
 * Create a new instance of rx_filter and return
 * a shared_ptr. This is effectively the public constructor.
 */
rx_filter_sptr make_rx_filter(double sample_rate, double low, double high, double trans_width)
{
    return gnuradio::get_initial_sptr(new rx_filter(sample_rate, low, high, trans_width));
}

rx_filter::rx_filter(double sample_rate, double low, double high, double trans_width)
    :sync_decimator("rx_filter",
                     gr::io_signature::make(1, 1, sizeof(gr_complex)),
                     gr::io_signature::make(1, 1, sizeof(gr_complex)),
                     1),
      d_fir(
#if GNURADIO_VERSION < 0x030900
        1,
#endif
        std::vector<gr_complex>({gr_complex(1.0)})),
      d_sample_rate(sample_rate),
      d_low(low),
      d_high(high),
      d_trans_width(trans_width),
      d_cw_offset(0)
{
    if (low < -0.95*sample_rate/2.0)
        d_low = -0.95*sample_rate/2.0;
    if (high > 0.95*sample_rate/2.0)
        d_high = 0.95*sample_rate/2.0;

    /* generate taps */
    d_taps = gr::filter::firdes::complex_band_pass(1.0, d_sample_rate, d_low, d_high, d_trans_width);

    /* create band pass filter */
    d_fir.set_taps(d_taps);

    set_history(d_max_taps);

    const int alignment_multiple = volk_get_alignment() / sizeof(float);
    set_alignment(std::max(1, alignment_multiple));
}

rx_filter::~rx_filter ()
{

}

void rx_filter::set_param(double low, double high, double trans_width)
{
    d_trans_width = trans_width;
    d_low         = low;
    d_high        = high;

    if (d_low < -0.95*d_sample_rate/2.0)
        d_low = -0.95*d_sample_rate/2.0;
    if (d_high > 0.95*d_sample_rate/2.0)
        d_high = 0.95*d_sample_rate/2.0;

    /* generate new taps */
    d_taps = gr::filter::firdes::complex_band_pass(1.0, d_sample_rate,
                                                   d_low + d_cw_offset,
                                                   d_high + d_cw_offset,
                                                   d_trans_width);

    qDebug() << "Generating taps for new filter   LO:" << d_low
             << "  HI:" << d_high << "  TW:" << d_trans_width
             << "  Taps:" << d_taps.size();
    if(d_taps.size()>d_max_taps)
        throw std::runtime_error("Failed to configure rx_filter low="+std::to_string(d_low)+
            " high="+std::to_string(d_high)+" tw="+std::to_string(d_trans_width)+" with "
            +std::to_string(d_taps.size())+" taps > max taps="+std::to_string(d_max_taps));

    {
        gr::thread::scoped_lock l(d_setlock);
        d_fir.set_taps(d_taps);
    }

}

void rx_filter::set_cw_offset(double offset)
{
    if (offset != d_cw_offset)
    {
        d_cw_offset = offset;
        set_param(d_low, d_high, d_trans_width);
    }
}

int rx_filter::work(int noutput_items,
    gr_vector_const_void_star& input_items,
    gr_vector_void_star& output_items)
{
    gr::thread::scoped_lock l(this->d_setlock);

    const gr_complex* in = (const gr_complex*)input_items[0];
    gr_complex* out = (gr_complex*)output_items[0];

    if (this->decimation() == 1) {
        d_fir.filterN(out, &in[d_max_taps - d_taps.size()], noutput_items);
    } else {
        d_fir.filterNdec(out, &in[d_max_taps - d_taps.size()], noutput_items, this->decimation());
    }

    return noutput_items;
}

/** Frequency translating filter **/

/*
 * Create a new instance of rx_xlating_filter and return
 * a shared_ptr. This is effectively the public constructor.
 */
rx_xlating_filter_sptr make_rx_xlating_filter(double sample_rate, double center, double low, double high, double trans_width)
{
    return gnuradio::get_initial_sptr(new rx_xlating_filter(sample_rate, center, low, high, trans_width));
}

rx_xlating_filter::rx_xlating_filter(double sample_rate, double center, double low, double high, double trans_width)
    : gr::hier_block2 ("rx_xlating_filter",
                      gr::io_signature::make (MIN_IN, MAX_IN, sizeof (gr_complex)),
                      gr::io_signature::make (MIN_OUT, MAX_OUT, sizeof (gr_complex))),
      d_sample_rate(sample_rate),
      d_center(center),
      d_low(low),
      d_high(high),
      d_trans_width(trans_width)
{
    /* generate taps */
    d_taps = gr::filter::firdes::complex_band_pass(1.0, d_sample_rate, -d_high, -d_low, d_trans_width);

    /* create band pass filter */
    d_bpf = gr::filter::freq_xlating_fir_filter_ccc::make(1, d_taps, d_center, d_sample_rate);

    /* connect filter */
    connect(self(), 0, d_bpf, 0);
    connect(d_bpf, 0, self(), 0);
}


rx_xlating_filter::~rx_xlating_filter()
{

}


void rx_xlating_filter::set_offset(double center)
{
    /* we have to change sign because the set_center_freq() actually
       shifts the passband with the specified amount, which has
       opposite sign of selecting a center frequency.
    */
    d_center = -center;
    d_bpf->set_center_freq(d_center);
}


void rx_xlating_filter::set_param(double low, double high, double trans_width)
{
    d_trans_width = trans_width;
    d_low         = low;
    d_high        = high;

    /* generate new taps */
    d_taps = gr::filter::firdes::complex_band_pass(1.0, d_sample_rate, -d_high, -d_low, d_trans_width);

    d_bpf->set_taps(d_taps);
}


void rx_xlating_filter::set_param(double center, double low, double high, double trans_width)
{
    set_offset(center);
    set_param(low, high, trans_width);
}
