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
#pragma once

#include <gnuradio/analog/quadrature_demod_cf.h>
#include <gnuradio/analog/pll_freqdet_cf.h>
#include <gnuradio/hier_block2.h>
#include <gnuradio/filter/iir_filter_ffd.h>
#if GNURADIO_VERSION < 0x030800
#include <gnuradio/filter/fir_filter_fff.h>
#else
#include <gnuradio/filter/fir_filter_blk.h>
#endif
#include <vector>
#include "dsp/fm_deemph.h"

class rx_demod_fm;
class rx_demod_fmpll;
#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<rx_demod_fm> rx_demod_fm_sptr;
typedef boost::shared_ptr<rx_demod_fmpll> rx_demod_fmpll_sptr;
#else
typedef std::shared_ptr<rx_demod_fm> rx_demod_fm_sptr;
typedef std::shared_ptr<rx_demod_fmpll> rx_demod_fmpll_sptr;
#endif

/*! \brief Return a shared_ptr to a new instance of rx_demod_fm.
 *  \param quad_rate The input sample rate.
 *  \param max_dev Maximum deviation in Hz
 *  \param tau De-emphasis time constant in seconds (75us in US, 50us in EUR, 0.0 disables).
 *
 * This is effectively the public constructor. To avoid accidental use
 * of raw pointers, rx_demod_fm's constructor is private.
 * make_rx_dmod_fm is the public interface for creating new instances.
 */
rx_demod_fm_sptr make_rx_demod_fm(float quad_rate, float max_dev=5000.0, double tau=50.0e-6);

/*! \brief FM demodulator.
 *  \ingroup DSP
 *
 * This class implements the FM demodulator using the gr_quadrature_demod block.
 * It also provides de-emphasis with variable time constant (use 0.0 to disable).
 *
 */
class rx_demod_fm : public gr::hier_block2
{

public:
    rx_demod_fm(float quad_rate, float max_dev, double tau); // FIXME: should be private
    ~rx_demod_fm();

    void set_max_dev(float max_dev);
    void set_tau(double tau);
    void set_subtone_filter(bool state);

private:
    /* GR blocks */
    gr::analog::quadrature_demod_cf::sptr   d_quad;      /*! The quadrature demodulator block. */
    fm_deemph_sptr                          d_deemph;    /*! De-emphasis IIR filter. */
    gr::filter::fir_filter_fff::sptr        d_hpf;

    /* other parameters */
    float       d_quad_rate;     /*! Quadrature rate. */
    float       d_max_dev;       /*! Max deviation. */
    bool        d_subtone_filter;
};


/*! \brief Return a shared_ptr to a new instance of rx_demod_fm.
 *  \param quad_rate The input sample rate.
 *  \param max_dev Maximum deviation in Hz
 *  \param tau De-emphasis time constant in seconds (75us in US, 50us in EUR, 0.0 disables).
 *
 * This is effectively the public constructor. To avoid accidental use
 * of raw pointers, rx_demod_fm's constructor is private.
 * make_rx_dmod_fm is the public interface for creating new instances.
 */
rx_demod_fmpll_sptr make_rx_demod_fmpll(float quad_rate, float max_dev=5000.0, double pllbw=0.1);

/*! \brief FM demodulator.
 *  \ingroup DSP
 *
 * This class implements the FM demodulator using the gr_quadrature_demod block.
 * It also provides de-emphasis with variable time constant (use 0.0 to disable).
 *
 */
class rx_demod_fmpll : public gr::hier_block2
{

public:
    rx_demod_fmpll(float quad_rate, float max_dev, double pllbw); // FIXME: should be private
    ~rx_demod_fmpll();

    void set_max_dev(float max_dev);
    void set_damping_factor(double df);
    void set_pll_bw(float bw);
    void set_subtone_filter(bool state);

private:
    /* GR blocks */
    gr::analog::pll_freqdet_cf::sptr    d_pll_demod;
    gr::filter::fir_filter_fff::sptr    d_hpf;

    /* other parameters */
    float       d_quad_rate;     /*! Quadrature rate. */
    float       d_max_dev;       /*! Max deviation. */
    float       d_pll_bw;
    bool        d_subtone_filter;
};
