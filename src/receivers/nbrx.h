/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2011-2016 Alexandru Csete OZ9AEC.
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
#ifndef NBRX_H
#define NBRX_H

#include <gnuradio/basic_block.h>
#include <gnuradio/blocks/complex_to_float.h>
#include <gnuradio/blocks/complex_to_real.h>
#include "receivers/receiver_base.h"
#include "dsp/rx_noise_blanker_cc.h"
#include "dsp/rx_filter.h"
#include "dsp/rx_demod_fm.h"
#include "dsp/rx_demod_am.h"

class nbrx;

#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<nbrx> nbrx_sptr;
#else
typedef std::shared_ptr<nbrx> nbrx_sptr;
#endif

/*! \brief Public constructor of nbrx_sptr. */
nbrx_sptr make_nbrx(double quad_rate, float audio_rate, std::vector<receiver_base_cf_sptr> & rxes);

/*! \brief Narrow band analog receiver
 *  \ingroup RX
 *
 * This block provides receiver for AM, narrow band FM and SSB modes.
 */
class nbrx : public receiver_base_cf
{
public:
    nbrx(double quad_rate, float audio_rate, std::vector<receiver_base_cf_sptr> & rxes);
    virtual ~nbrx() { };

    bool start() override;
    bool stop() override;

    void set_filter(int low, int high, Modulations::filter_shape shape) override;
    bool set_filter_shape(const c_def::v_union &) override;
    bool set_offset(int offset, bool locked) override;
    bool set_cw_offset(const c_def::v_union &) override;

    void set_audio_rate(int audio_rate) override;

    /* Noise blanker */
    bool has_nb() override { return true; }
    bool set_nb1_on(const c_def::v_union &) override;
    bool set_nb2_on(const c_def::v_union &) override;
    bool set_nb3_on(const c_def::v_union &) override;
    bool set_nb1_threshold(const c_def::v_union &) override;
    bool set_nb2_threshold(const c_def::v_union &) override;
    bool set_nb3_gain(const c_def::v_union &) override;


    bool set_demod(const c_def::v_union &) override;

    /* FM parameters */
    bool set_fm_maxdev(const c_def::v_union &) override;
    bool set_fm_deemph(const c_def::v_union &) override;
    bool set_fmpll_damping_factor(const c_def::v_union &) override;
    bool set_subtone_filter(const c_def::v_union &) override;

    /* AM parameters */
    bool set_am_dcr(const c_def::v_union &) override;

    /* AM-Sync parameters */
    bool set_amsync_dcr(const c_def::v_union &) override;
    bool set_amsync_pll_bw(const c_def::v_union &) override;
    bool set_pll_bw(const c_def::v_union &) override;

private:

    void update_filter();

    bool   d_running;          /*!< Whether receiver is running or not. */
    int d_fxff_decim{1};
    int old_filter_low{0};
    int old_filter_high{0};
    bool d_wb_rec{0};

    rx_filter_sptr            filter;  /*!< Non-translating bandpass filter.*/

    rx_nb_cc_sptr             nb;         /*!< Noise blanker. */
    gr::blocks::complex_to_float::sptr  demod_raw;  /*!< Raw I/Q passthrough. */
    gr::blocks::complex_to_real::sptr   demod_ssb;  /*!< SSB demodulator. */
    rx_demod_fm_sptr          demod_fm;   /*!< FM demodulator. */
    rx_demod_fmpll_sptr       demod_fmpll;   /*!< FM demodulator. */
    rx_demod_am_sptr          demod_am;   /*!< AM demodulator. */
    rx_demod_amsync_sptr      demod_amsync;   /*!< AM-Sync demodulator. */
    resampler_ff_sptr         audio_rr0;  /*!< Audio resampler. */
    resampler_ff_sptr         audio_rr1;  /*!< Audio resampler. */
    gr::filter::freq_xlating_fir_filter_ccf::sptr fxff;
    std::vector<float> d_fxff_taps;
    gr::blocks::complex_to_float::sptr  rec_raw;  /*!< Raw I/Q passthrough for recorder. */

    gr::basic_block_sptr      demod;    // dummy pointer used for simplifying reconf
};

#endif // NBRX_H
