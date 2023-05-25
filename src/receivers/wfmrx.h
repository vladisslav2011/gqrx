/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2012 Alexandru Csete OZ9AEC.
 * FM stereo implementation by Alex Grinkov a.grinkov(at)gmail.com.
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
#ifndef WFMRX_H
#define WFMRX_H

#include "receivers/receiver_base.h"
#include "dsp/rx_noise_blanker_cc.h"
#include "dsp/rx_filter.h"
#include "dsp/rx_demod_fm.h"
#include "dsp/stereo_demod.h"
#include "dsp/rx_rds.h"
#include "dsp/rds/decoder.h"
#include "dsp/rds/parser.h"
#include <gnuradio/analog/quadrature_demod_cf.h>

class wfmrx;

#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<wfmrx> wfmrx_sptr;
#else
typedef std::shared_ptr<wfmrx> wfmrx_sptr;
#endif

/*! \brief Public constructor of wfm_rx. */
wfmrx_sptr make_wfmrx(double quad_rate, float audio_rate);

/*! \brief Wide band FM receiver.
 *  \ingroup RX
 *
 * This block provides receiver for broadcast FM transmissions.
 */
class wfmrx : public receiver_base_cf
{

public:
    wfmrx(double quad_rate, float audio_rate);
    ~wfmrx();

    bool start() override;
    bool stop() override;

    void set_audio_rate(int audio_rate) override;

    void set_filter(int low, int high, int tw) override;

    /* Noise blanker */
    bool has_nb() override { return false; }

    void set_demod(Modulations::idx demod) override;
    void set_index(int index) override;

    bool set_wfm_deemph(const c_def::v_union & v) override;

    bool set_rds_on(const c_def::v_union &) override;
    bool get_rds_pi(c_def::v_union &) const override;
    bool get_rds_ps(c_def::v_union &) const override;
    bool get_rds_pty(c_def::v_union &) const override;
    bool get_rds_flagstring(c_def::v_union &to) const override;
    bool get_rds_rt(c_def::v_union &) const override;
    bool get_rds_clock(c_def::v_union &) const override;
    bool get_rds_af(c_def::v_union &) const override;

private:
    void start_rds_decoder();
    void stop_rds_decoder();
    bool   d_running;          /*!< Whether receiver is running or not. */
    using vfo_s::d_rds_on;

    rx_filter_sptr            filter;    /*!< Non-translating bandpass filter.*/

    gr::analog::quadrature_demod_cf::sptr demod_fm;  /*!< FM demodulator. */
    stereo_demod_sptr         stereo;    /*!< FM stereo demodulator. */
    stereo_demod_sptr         stereo_oirt;    /*!< FM stereo oirt demodulator. */
    stereo_demod_sptr         mono;      /*!< FM stereo demodulator OFF. */

    rx_rds_sptr               rds;       /*!< RDS decoder */
    gr::rds::decoder::sptr    rds_decoder;
    gr::rds::parser::sptr     rds_parser;
};

#endif // WFMRX_H
