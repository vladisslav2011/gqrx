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
#include <gnuradio/blocks/wavfile_sink.h>
#include <gnuradio/blocks/complex_to_imag.h>
#include <gnuradio/blocks/complex_to_real.h>
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
rx_rds_sptr make_rx_rds(double sample_rate, bool encorr)
{
    return gnuradio::get_initial_sptr(new rx_rds(sample_rate, encorr));
}

rx_rds::rx_rds(double sample_rate, bool encorr)
    : gr::hier_block2 ("rx_rds",
                      gr::io_signature::make (MIN_IN, MAX_IN, sizeof (float)),
                      gr::io_signature::make (MIN_OUT, MAX_OUT, sizeof (char))),
      d_sample_rate(sample_rate)
{
    if (sample_rate != 240000.0) {
        throw std::invalid_argument("RDS sample rate not supported");
    }

//    d_fxff_tap = gr::filter::firdes::low_pass(1, d_sample_rate, 5500, 3500);
    d_fxff_tap = gr::filter::firdes::band_pass(1, d_sample_rate,
        2375.0/2.0-d_fxff_bw, 2375.0/2.0+d_fxff_bw, d_fxff_tw/*,gr::filter::firdes::WIN_BLACKMAN_HARRIS*/);
    std::cerr<<"--------------------- rx_rds::rx_rds d_fxff_tap.size()="<<d_fxff_tap.size()<<"\n";
    d_fxff = gr::filter::freq_xlating_fir_filter_fcf::make(10, d_fxff_tap, 57000, d_sample_rate);

    int interpolation = 19;
    int decimation = 24;
#if GNURADIO_VERSION < 0x030900
    float rate = (float) interpolation / (float) decimation;
    d_rsmp_tap = gr::filter::firdes::low_pass(interpolation, interpolation, rate * 0.45, rate * 0.1);
    d_rsmp = gr::filter::rational_resampler_base_ccf::make(interpolation, decimation, d_rsmp_tap);
#else
    d_rsmp = gr::filter::rational_resampler_ccf::make(interpolation, decimation);
#endif

    int n_taps = 151*5;
    d_rrcf = gr::filter::firdes::root_raised_cosine(1, (d_sample_rate*interpolation)/(decimation*10), 2375, 1, n_taps);
    d_rrcf_manchester = std::vector<float>(n_taps-8);
    for (int n = 0; n < n_taps-8; n++) {
        d_rrcf_manchester[n] = d_rrcf[n] - d_rrcf[n+8];
    }

    gr::digital::constellation_sptr p_c = gr::digital::constellation_bpsk::make()->base();
    auto corr=iir_corr::make((d_sample_rate*interpolation*104.0*2.0)/(decimation*23750.0),0.1);

#if GNURADIO_VERSION < 0x030800
    d_bpf = gr::filter::fir_filter_ccf::make(1, d_rrcf);

    d_agc = gr::analog::agc_cc::make(2e-3, 0.585 * 1.25, 53 * 1.25);

    d_sync = gr::digital::clock_recovery_mm_cc::make((d_sample_rate*interpolation)/(decimation*23750.f), 0.25 * 0.175 * 0.000175, 0.5, 0.175*0.2, 0.0002);

    d_koin = gr::blocks::keep_one_in_n::make(sizeof(unsigned char), 2);
#else
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
    if(encorr)
    {
        connect(d_bpf, 0, corr, 0);
        connect(corr, 0, d_agc, 0);
    }else
        connect(d_bpf, 0, d_agc, 0);
    connect(d_agc, 0, d_sync, 0);
    connect(d_sync, 0, d_mpsk, 0);

#if GNURADIO_VERSION < 0x030800
    connect(d_mpsk, 0, d_koin, 0);
    connect(d_koin, 0, d_ddbb, 0);
#else
    connect(d_mpsk, 0, d_ddbb, 0);
#endif

    connect(d_ddbb, 0, self(), 0);
    #if 0
    auto w1=gr::blocks::wavfile_sink::make("/home/vlad/rrcf.wav",2,19000);
    auto im1=gr::blocks::complex_to_imag::make();
    auto re1=gr::blocks::complex_to_real::make();
    auto w2=gr::blocks::wavfile_sink::make("/home/vlad/rrcf_manchester.wav",2,19000);
    auto im2=gr::blocks::complex_to_imag::make();
    auto re2=gr::blocks::complex_to_real::make();
    auto bpf_manc=gr::filter::fir_filter_ccf::make(1, d_rrcf_manchester);
    auto w3=gr::blocks::wavfile_sink::make("/home/vlad/raw.wav",2,19000);
    auto im3=gr::blocks::complex_to_imag::make();
    auto re3=gr::blocks::complex_to_real::make();
    connect(d_bpf,0,re1,0);
    connect(d_bpf,0,im1,0);
    connect(re1,0,w1,0);
    connect(im1,0,w1,1);
    connect(d_rsmp,0,bpf_manc,0);
    connect(bpf_manc,0,re2,0);
    connect(bpf_manc,0,im2,0);
    connect(re2,0,w2,0);
    connect(im2,0,w2,1);
    connect(d_rsmp,0,re3,0);
    connect(d_rsmp,0,im3,0);
    connect(re3,0,w3,0);
    connect(im3,0,w3,1);
    #endif
}

rx_rds::~rx_rds ()
{

}

void rx_rds::trig()
{
    changed_value(C_RDS_CR_OMEGA, d_index, d_sync->omega());
    changed_value(C_RDS_CR_MU, d_index, d_sync->mu());
}

void rx_rds::update_fxff_taps()
{
    //lock();
    d_fxff_tap = gr::filter::firdes::band_pass(1, d_sample_rate,
        2375.0/2.0-d_fxff_bw, 2375.0/2.0+d_fxff_bw, d_fxff_tw);
    std::cerr<<"--------------------- rx_rds::rx_rds d_fxff_tap.size()="<<d_fxff_tap.size()<<"\n";
    d_fxff->set_taps(d_fxff_tap);
    //unlock();
}
