/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2024 Vladislav P.
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
#ifndef RX_REJECTOR_H
#define RX_REJECTOR_H

#include <gnuradio/hier_block2.h>
#include <gnuradio/filter/iir_filter_ccd.h>
#include <gnuradio/analog/pll_refout_cc.h>
#include <gnuradio/blocks/multiply_conjugate_cc.h>
#if GNURADIO_VERSION < 0x030800
#include <gnuradio/blocks/multiply_cc.h>
#else
#include <gnuradio/blocks/multiply.h>
#endif
#include "receivers/defines.h"

/*! \brief Naroow-band PLL-aided interference rejector
 *  \ingroup DSP
 *
 */
class rx_rejector_cc : public gr::hier_block2
{
public:
#if GNURADIO_VERSION < 0x030900
    typedef boost::shared_ptr<rx_rejector_cc> sptr;
#else
    typedef std::shared_ptr<rx_rejector_cc> sptr;
#endif
/*! \brief Return a shared_ptr to a new instance of rx_rejector.
 *  \param sample_rate The sample rate.
 *  \param offset The filter offset.
 *  \param bw The PLL locking range.
 *  \param alfa The high-pass IIR alfa.
 *
 */
    static sptr make(double sample_rate,
                     double offset=0.0,
                     double bw=5.0,
                     double alfa=0.001);

    ~rx_rejector_cc();

    void set_sample_rate(double rate);
    void set_offset(double offset);
    void set_bw(double bw);
    void set_alfa(double alfa);

private:
    rx_rejector_cc(double sample_rate=96000.0, double offset=0.0, double bw=5.0, double alfa=0.001);
    gr::analog::pll_refout_cc::sptr         d_pll;
    gr::filter::iir_filter_ccd::sptr        d_dcr;
    gr::blocks::multiply_conjugate_cc::sptr d_fwd;
    gr::blocks::multiply_cc::sptr           d_bwd;

    std::vector< double > d_fftaps;
    std::vector< double > d_fbtaps;
    double                d_sample_rate;
    double                d_offset;
    double                d_bw;
};


#endif // RX_REJECTOR_H
