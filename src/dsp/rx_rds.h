/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2015 Alexandru Csete OZ9AEC.
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
#ifndef RX_RDS_H
#define RX_RDS_H

#include <mutex>
#include <gnuradio/hier_block2.h>

#if GNURADIO_VERSION < 0x030800
#include <gnuradio/filter/fir_filter_ccf.h>
#include <gnuradio/filter/freq_xlating_fir_filter_fcf.h>
#include <gnuradio/digital/clock_recovery_mm_cc.h>
#else
#include <gnuradio/filter/fir_filter_blk.h>
#include <gnuradio/filter/freq_xlating_fir_filter.h>
#include <gnuradio/digital/symbol_sync_cc.h>
#endif

#if GNURADIO_VERSION < 0x030800
#include <gnuradio/filter/rational_resampler_base_ccf.h>
#elif GNURADIO_VERSION < 0x030900
#include <gnuradio/filter/rational_resampler_base.h>
#else
#include <gnuradio/filter/rational_resampler.h>
#endif

#include <gnuradio/analog/agc_cc.h>
#include <gnuradio/digital/constellation_receiver_cb.h>
#include <gnuradio/blocks/keep_one_in_n.h>
#include <gnuradio/digital/diff_decoder_bb.h>
#include <gnuradio/blocks/file_sink.h>
#include <gnuradio/blocks/message_debug.h>
#include "dsp/rds/decoder.h"
#include "dsp/rds/parser.h"
#include "dsp/clock_recovery.h"
#include "applications/gqrx/dcontrols.h"

class rx_rds;

#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<rx_rds> rx_rds_sptr;
#else
typedef std::shared_ptr<rx_rds> rx_rds_sptr;
#endif


rx_rds_sptr make_rx_rds(double sample_ratee=240000.0, bool encorr=false);

class rx_rds : public gr::hier_block2, public conf_notifier
{

public:
    rx_rds(double sample_rat, bool encorr);
    ~rx_rds();

    void set_index(int v) {d_index=v;}
    void set_agc_rate(float v) {d_agc->set_rate(v);}
    void set_gain_mu(float v) {d_sync->set_gain_mu(d_gain_mu=v);}
    void set_gain_omega(float v) {d_sync->set_gain_omega(d_gain_omega=v);}
    void trig();
    void set_fxff_bw(float bw) {d_fxff_bw=bw; update_fxff_taps();}
    void set_fxff_tw(float tw) {d_fxff_tw=tw; update_fxff_taps();}
    void set_omega_lim(float v);

private:
    void update_fxff_taps();
    std::vector<float> d_fxff_tap;
    std::vector<float> d_rsmp_tap;
    gr::filter::fir_filter_ccf::sptr d_bpf;
    gr::filter::freq_xlating_fir_filter_fcf::sptr d_fxff;

#if GNURADIO_VERSION < 0x030900
    gr::filter::rational_resampler_base_ccf::sptr d_rsmp;
#else
    gr::filter::rational_resampler_ccf::sptr d_rsmp;
#endif

    std::vector<float> d_rrcf;
    std::vector<float> d_rrcf_manchester;
    gr::analog::agc_cc::sptr d_agc;
#if GNURADIO_VERSION < 0x030800
    clock_recovery_el_cc::sptr d_sync;
    gr::blocks::keep_one_in_n::sptr d_koin;
#else
    gr::digital::symbol_sync_cc::sptr d_sync;
#endif
    gr::digital::constellation_receiver_cb::sptr d_mpsk;
    gr::digital::diff_decoder_bb::sptr d_ddbb;

    double d_sample_rate;
    int d_index;
    constexpr static int d_interpolation = 19;
    constexpr static int d_decimation = 24;
    float d_fxff_tw{500.0f};
    float d_fxff_bw{1000.0f};
    float d_gain_mu{0.175*0.2};
    float d_gain_omega{0.25 * 0.175 * 0.000175};
    float d_omega_lim{0.00025};
};


#endif // RX_RDS_H
