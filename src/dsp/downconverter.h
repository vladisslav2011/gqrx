/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2020 Clayton Smith VE3IRR.
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
#pragma once

#if GNURADIO_VERSION < 0x030800
#include <gnuradio/filter/freq_xlating_fir_filter_ccf.h>
#else
#include <gnuradio/filter/freq_xlating_fir_filter.h>
#endif
#include <gnuradio/filter/firdes.h>

#include <gnuradio/blocks/rotator_cc.h>
#include <gnuradio/hier_block2.h>

class downconverter_cc;

#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<downconverter_cc> downconverter_cc_sptr;
typedef gr::filter::firdes::win_type wintype;
#else
typedef std::shared_ptr<downconverter_cc> downconverter_cc_sptr;
typedef gr::fft::window::win_type wintype;
#endif
downconverter_cc_sptr make_downconverter_cc(unsigned int decim, double center_freq, double samp_rate);

class downconverter_cc : public gr::hier_block2
{
    friend downconverter_cc_sptr make_downconverter_cc(unsigned int decim, double center_freq, double samp_rate);

public:
    downconverter_cc(unsigned int decim, double center_freq, double samp_rate);
    ~downconverter_cc();
    void set_decim_and_samp_rate(unsigned int decim, double samp_rate);
    void set_center_freq(double center_freq);
    void set_wintype(int win_type)
    {
        d_wintype=win_type;
        update_proto_taps();
    }
    void set_beta(float beta)
    {
        d_beta=beta;
        update_proto_taps();
    }
    void set_att(float att)
    {
        d_att=att;
        update_proto_taps();
    }
    void connect_fir_tap(gr::basic_block_sptr to);

private:
    unsigned int d_decim;
    double d_center_freq;
    double d_samp_rate;
    int d_wintype=0;
    float d_beta=6.7;
    float d_att=40.0;
    std::vector<float> d_proto_taps;

    void connect_all();
    void update_proto_taps();
    void update_phase_inc();

    gr::filter::freq_xlating_fir_filter_ccf::sptr filt;
    gr::filter::freq_xlating_fir_filter_ccf::sptr tap_filt;
    gr::blocks::rotator_cc::sptr rot;
    gr::basic_block_sptr tap_block;
};
