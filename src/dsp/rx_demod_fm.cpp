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
#include <gnuradio/filter/firdes.h>
#include <gnuradio/io_signature.h>
#include <iostream>
#include <math.h>
#include <QDebug>

#include "dsp/rx_demod_fm.h"

/* Create a new instance of rx_demod_fm and return a shared_ptr. */
rx_demod_fm_sptr make_rx_demod_fm(float quad_rate, float max_dev, double tau)
{
    return gnuradio::get_initial_sptr(new rx_demod_fm(quad_rate, max_dev, tau));
}

static const int MIN_IN = 1;  /* Minimum number of input streams. */
static const int MAX_IN = 1;  /* Maximum number of input streams. */
static const int MIN_OUT = 1; /* Minimum number of output streams. */
static const int MAX_OUT = 1; /* Maximum number of output streams. */

rx_demod_fm::rx_demod_fm(float quad_rate, float max_dev, double tau)
    : gr::hier_block2 ("rx_demod_fm",
                      gr::io_signature::make (MIN_IN, MAX_IN, sizeof (gr_complex)),
                      gr::io_signature::make (MIN_OUT, MAX_OUT, sizeof (float))),
    d_quad_rate(quad_rate),
    d_max_dev(max_dev),
    d_subtone_filter(false)
{
    float gain;

    /* demodulator gain */
    gain = d_quad_rate / (2 * (float)M_PI * d_max_dev);

    qDebug() << "FM demod gain:" << gain;

    /* demodulator */
    d_quad = gr::analog::quadrature_demod_cf::make(gain);

    /* de-emphasis */
    d_deemph = make_fm_deemph(d_quad_rate, tau);
    d_hpf = gr::filter::fir_filter_fff::make(1, gr::filter::firdes::high_pass_2(1.0, (double)d_quad_rate, 300.0, 50.0, 15.0));

    /* connect block */
    connect(self(), 0, d_quad, 0);
    connect(d_quad, 0, d_deemph, 0);
    connect(d_deemph, 0, self(), 0);
}

rx_demod_fm::~rx_demod_fm ()
{
}

/*! \brief Set maximum FM deviation.
 *  \param max_dev The new mximum deviation in Hz
 *
 * The maximum deviation is related to the gain of the
 * quadrature demodulator by:
 *
 *   gain = quad_rate / (2 * PI * max_dev)
 */
void rx_demod_fm::set_max_dev(float max_dev)
{
    float gain;

    if ((max_dev < 500.0f) || (max_dev > d_quad_rate/2.0f))
    {
        return;
    }

    d_max_dev = max_dev;

    gain = d_quad_rate / (2.f * (float)M_PI * max_dev);
    d_quad->set_gain(gain);
}

/*! \brief Set FM de-emphasis time constant.
 *  \param tau The new time constant.
 */
void rx_demod_fm::set_tau(double tau)
{
    d_deemph->set_tau(tau);
}

/*! \brief Enable/disable subtone filter.
 *  \param state New filter state.
 */
void rx_demod_fm::set_subtone_filter(bool state)
{
    if(state == d_subtone_filter)
        return;
    d_subtone_filter = state;
    if(state)
    {
        disconnect(d_deemph, 0, self(), 0);
        connect(d_deemph, 0, d_hpf, 0);
        connect(d_hpf, 0, self(), 0);
    }
    else
    {
        disconnect(d_deemph, 0, d_hpf, 0);
        disconnect(d_hpf, 0, self(), 0);
        connect(d_deemph, 0, self(), 0);
    }
}

/* Create a new instance of rx_demod_fm and return a shared_ptr. */
rx_demod_fmpll_sptr make_rx_demod_fmpll(float quad_rate, float max_dev, double pllbw)
{
    return gnuradio::get_initial_sptr(new rx_demod_fmpll(quad_rate, max_dev, pllbw));
}

rx_demod_fmpll::rx_demod_fmpll(float quad_rate, float max_dev, double pllbw)
    : gr::hier_block2 ("rx_demod_fmpll",
                      gr::io_signature::make (MIN_IN, MAX_IN, sizeof (gr_complex)),
                      gr::io_signature::make (MIN_OUT, MAX_OUT, sizeof (float))),
    d_quad_rate(quad_rate),
    d_max_dev(max_dev),
    d_pll_bw(pllbw),
    d_subtone_filter(false)
{
    float gain;

    /* demodulator gain */
    gain = d_quad_rate / (2.0f * (float)M_PI * d_max_dev);

    qDebug() << "FM demod gain:" << gain;

    /* demodulator */
    d_pll_demod = gr::analog::pll_freqdet_cf::make(d_pll_bw,
                                                   (2.f*(float)M_PI*max_dev/quad_rate),
                                                   (2.f*(float)M_PI*(-max_dev)/quad_rate));

    d_hpf = gr::filter::fir_filter_fff::make(1, gr::filter::firdes::high_pass_2(1.0, (double)d_quad_rate, 300.0, 50.0, 15.0));

    /* connect block */
    connect(self(), 0, d_pll_demod, 0);
    connect(d_pll_demod, 0, self(), 0);
}

rx_demod_fmpll::~rx_demod_fmpll ()
{
}

/*! \brief Set maximum FM deviation.
 *  \param max_dev The new mximum deviation in Hz
 *
 * The maximum deviation is related to the gain of the
 * quadrature demodulator by:
 *
 *   gain = quad_rate / (2 * PI * max_dev)
 */
void rx_demod_fmpll::set_max_dev(float max_dev)
{
    if ((max_dev < 500.f) || (max_dev > d_quad_rate/2.f))
    {
        return;
    }

    d_max_dev = max_dev;

    d_pll_demod->set_max_freq(2.f*(float)M_PI*d_max_dev/d_quad_rate);
    d_pll_demod->set_min_freq(-2.f*(float)M_PI*d_max_dev/d_quad_rate);
}

/*! \brief Set FM de-emphasis time constant.
 *  \param tau The new time constant.
 */
void rx_demod_fmpll::set_damping_factor(double df)
{
    // df 0.0 ... 1.0
    d_pll_demod->set_damping_factor(df);
}

void rx_demod_fmpll::set_pll_bw(float bw)
{
    // bw = 0.001 ... 0.5
    d_pll_demod->set_loop_bandwidth(bw);
}

/*! \brief Enable/disable subtone filter.
 *  \param state New filter state.
 */
void rx_demod_fmpll::set_subtone_filter(bool state)
{
    if(state == d_subtone_filter)
        return;
    d_subtone_filter = state;
    if(state)
    {
        disconnect(d_pll_demod, 0, self(), 0);
        connect(d_pll_demod, 0, d_hpf, 0);
        connect(d_hpf, 0, self(), 0);
    }
    else
    {
        disconnect(d_pll_demod, 0, d_hpf, 0);
        disconnect(d_hpf, 0, self(), 0);
        connect(d_pll_demod, 0, self(), 0);
    }
}
