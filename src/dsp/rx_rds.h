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

#include "applications/gqrx/compat.h"
#include <mutex>
#include <gnuradio/hier_block2.h>

//for conditional includes see "applications/gqrx/compat.h"

#include <gnuradio/analog/agc_cc.h>
#include <gnuradio/digital/constellation_receiver_cb.h>
#include <gnuradio/blocks/keep_one_in_n.h>
#include <gnuradio/digital/diff_decoder_bb.h>
#include <gnuradio/blocks/file_sink.h>
#include <gnuradio/blocks/message_debug.h>
#include <queue>
#include "dsp/rds/decoder.h"
#include "dsp/rds/parser.h"

class rx_rds;
class rx_rds_store;

typedef compat_shared_ptr<rx_rds> rx_rds_sptr;
typedef compat_shared_ptr<rx_rds_store> rx_rds_store_sptr;


rx_rds_sptr make_rx_rds(double sample_rate);

rx_rds_store_sptr make_rx_rds_store();

class rx_rds_store : public gr::block
{
public:
    rx_rds_store();
    ~rx_rds_store();

    void get_message(std::string &out, int &type);

private:
    void store(pmt::pmt_t msg);

    std::mutex d_mutex;
    std::queue<pmt::pmt_t> d_messages;

};

class rx_rds : public gr::hier_block2
{

public:
    rx_rds(double sample_rate=240000.0);
    ~rx_rds();

    void set_param(double low, double high, double trans_width);

private:
    std::vector<float> d_fxff_tap;
    std::vector<float> d_rsmp_tap;
    gr::filter::fir_filter_ccf::sptr d_bpf;
    gr::filter::freq_xlating_fir_filter_fcf::sptr d_fxff;

    gr::filter::rational_resampler_ccf::sptr d_rsmp;

    std::vector<float> d_rrcf;
    std::vector<float> d_rrcf_manchester;
    gr::analog::agc_cc::sptr d_agc;
#if GNURADIO_VERSION < 0x030800
    gr::digital::clock_recovery_mm_cc::sptr d_sync;
    gr::blocks::keep_one_in_n::sptr d_koin;
#else
    gr::digital::symbol_sync_cc::sptr d_sync;
#endif
    gr::digital::constellation_receiver_cb::sptr d_mpsk;
    gr::digital::diff_decoder_bb::sptr d_ddbb;

    double d_sample_rate;
};


#endif // RX_RDS_H
