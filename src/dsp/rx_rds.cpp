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
#include <QMessageBox>
#include <QFileDialog>
#include <cmath>
#include <gnuradio/io_signature.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/digital/constellation_receiver_cb.h>
#include <iostream>
#include <stdio.h>
#include <stdarg.h>
#include "dsp/rx_rds.h"

#if GNURADIO_VERSION >= 0x030800
#include <gnuradio/digital/timing_error_detector_type.h>
#endif


class iir_corr : public gr::sync_block
{
protected:
    iir_corr(int period, double alfa):
        gr::sync_block ("iir_corr",
        gr::io_signature::make(1, 1, sizeof(gr_complex)),
        gr::io_signature::make(1, 1, sizeof(gr_complex))),
        d_alfa(alfa),
        d_ialfa(1.0-alfa),
        d_period(period),
        d_p(0),
        d_b(d_period)
    {
        for(int k=0;k<d_period;k++)
            d_b[k]=0.0;
    }

public:
#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<iir_corr> sptr;
#else
typedef std::shared_ptr<iir_corr> sptr;
#endif
    ~iir_corr()
    {
    }

    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items) override
    {
        const gr_complex *in = (const gr_complex *) input_items[0];
        gr_complex *out = (gr_complex *) output_items[0];
        for(int k=0;k<noutput_items;k++)
        {
            out[k]=d_b[d_p]=d_b[d_p]*d_ialfa+in[k]*d_alfa;
            d_p++;
            if(d_p>=d_period)
                d_p=0;
        }
        return noutput_items;
    }

    static sptr make(int period, double alfa)
    {
        return gnuradio::get_initial_sptr(new iir_corr(period,alfa));
    }

private:
    gr_complex      d_alfa;
    gr_complex      d_ialfa;
    int             d_period;
    int d_p;
    std::vector<gr_complex> d_b;
};




static const int MIN_IN = 1;  /* Minimum number of input streams. */
static const int MAX_IN = 1;  /* Maximum number of input streams. */
static const int MIN_OUT = 1; /* Minimum number of output streams. */
static const int MAX_OUT = 1; /* Maximum number of output streams. */

/*
 * Create a new instance of rx_rds and return
 * a shared_ptr. This is effectively the public constructor.
 */
rx_rds_sptr make_rx_rds(double sample_rate)
{
    return gnuradio::get_initial_sptr(new rx_rds(sample_rate));
}

rx_rds::rx_rds(double sample_rate)
    : gr::hier_block2 ("rx_rds",
                      gr::io_signature::make (MIN_IN, MAX_IN, sizeof (float)),
                      gr::io_signature::make (MIN_OUT, MAX_OUT, sizeof (char))),
      d_sample_rate(sample_rate)
{
    if (sample_rate != 240000.0) {
        throw std::invalid_argument("RDS sample rate not supported");
    }

    d_fxff_tap = gr::filter::firdes::low_pass(1, d_sample_rate, 7500, 5000);
    d_fxff = gr::filter::freq_xlating_fir_filter_fcf::make(10, d_fxff_tap, 57000, d_sample_rate);

    int interpolation = 19;
    int decimation = 24*4;
#if GNURADIO_VERSION < 0x030900
    double rate = (double) interpolation / (double) decimation;
    d_rsmp_tap = gr::filter::firdes::low_pass(interpolation, interpolation, rate * 0.45, rate * 0.1);
    d_rsmp = gr::filter::rational_resampler_base_ccf::make(interpolation, decimation, d_rsmp_tap);
#else
    d_rsmp = gr::filter::rational_resampler_ccf::make(interpolation, decimation);
#endif

    int n_taps = 151*3;
    d_rrcf = gr::filter::firdes::root_raised_cosine(1, (d_sample_rate*interpolation)/decimation/10, 2375, 1, n_taps);

    gr::digital::constellation_sptr p_c = gr::digital::constellation_bpsk::make()->base();
    auto corr=iir_corr::make((d_sample_rate*interpolation)/decimation/11875*104,0.01);

#if GNURADIO_VERSION < 0x030800
    d_bpf = gr::filter::fir_filter_ccf::make(1, d_rrcf);

    d_agc = gr::analog::agc_cc::make(2e-3, 0.585 * 1.25, 53 * 1.25);

    d_sync = gr::digital::clock_recovery_mm_cc::make((d_sample_rate*interpolation)/decimation/23750, 0.25 * 0.175 * 0.175, 0.5, 0.175, 0.005);

    d_koin = gr::blocks::keep_one_in_n::make(sizeof(unsigned char), 2);
#else
    d_rrcf_manchester = std::vector<float>(n_taps-8);
    for (int n = 0; n < n_taps-8; n++) {
        d_rrcf_manchester[n] = d_rrcf[n] - d_rrcf[n+8];
    }
    d_bpf = gr::filter::fir_filter_ccf::make(1, d_rrcf_manchester);

    d_agc = gr::analog::agc_cc::make(2e-3, 0.585, 53);

    d_sync = gr::digital::symbol_sync_cc::make(gr::digital::TED_ZERO_CROSSING, 16, 0.01, 1, 1, 0.1, 1, p_c);
#endif

    d_mpsk = gr::digital::constellation_receiver_cb::make(p_c, 2*M_PI/200.0, -0.002, 0.002);

    d_ddbb = gr::digital::diff_decoder_bb::make(2);

    /* connect filter */
    connect(self(), 0, d_fxff, 0);
    connect(d_fxff, 0, d_rsmp, 0);
    connect(d_rsmp, 0, d_bpf, 0);
    #if 0
    connect(d_bpf, 0, corr, 0);
    connect(corr, 0, d_agc, 0);
    #else
    connect(d_bpf, 0, d_agc, 0);
    #endif
    connect(d_agc, 0, d_sync, 0);
    connect(d_sync, 0, d_mpsk, 0);

#if GNURADIO_VERSION < 0x030800
    connect(d_mpsk, 0, d_koin, 0);
    connect(d_koin, 0, d_ddbb, 0);
#else
    connect(d_mpsk, 0, d_ddbb, 0);
#endif

    connect(d_ddbb, 0, self(), 0);
}

rx_rds::~rx_rds ()
{

}
