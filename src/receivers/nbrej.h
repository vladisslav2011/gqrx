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
#ifndef NBREJ_H
#define NBREJ_H

#include <gnuradio/basic_block.h>
#include "receivers/receiver_base.h"

/*! \brief Narrow band rejector
 *  \ingroup RX
 *
 * This block provides conrol interface to single-tone "birdies" rejector.
 */
class nbrej : public receiver_base_cf
{
public:
#if GNURADIO_VERSION < 0x030900
    typedef boost::shared_ptr<nbrej> sptr;
#else
    typedef std::shared_ptr<nbrej> sptr;
#endif
/*! \brief Public constructor of nbrx_sptr. */
    static sptr make(double quad_rate, float audio_rate, std::vector<receiver_base_cf_sptr> & rxes);
    nbrej(double quad_rate, float audio_rate, std::vector<receiver_base_cf_sptr> & rxes);
    virtual ~nbrej();

    bool start() override { return true; }
    bool stop() override { return true; }
    bool set_filter_shape(const c_def::v_union &) override;
    bool         set_demod(const c_def::v_union &v) override { return vfo_s::set_demod(Modulations::MODE_NB_REJECTOR);}
    void set_quad_rate(double quad_rate) override {}
    void set_audio_rate(int audio_rate) override {}
    void set_filter(int low, int high, Modulations::filter_shape shape) override;
    bool set_offset(int offset, bool locked) override;

#if 0
    bool set_tracking_pll_bw(const c_def::v_union &) override;
    bool set_filter_alfa(const c_def::v_union &) override;
#endif
private:
};

#endif // NBREJ_H
