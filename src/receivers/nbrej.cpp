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
#include "receivers/nbrej.h"


nbrej::sptr nbrej::make(double quad_rate, float audio_rate)
{
    return gnuradio::get_initial_sptr(new nbrej(quad_rate, audio_rate));
}

nbrej::nbrej(double quad_rate, float audio_rate)
    : receiver_base_cf("NBREJ", NB_PREF_QUAD_RATE, quad_rate, audio_rate)
{
}

void nbrej::set_filter(int low, int high, Modulations::filter_shape shape)
{
    receiver_base_cf::set_filter(low, high, shape);
    //TODO: set PLL locking range on all slaves here
}

void nbrej::set_offset(int offset)
{
    if(offset==get_offset())
        return;
    vfo_s::set_offset(offset);
    //TODO: update PLL locking range on all slaves here
}

#if 0
bool nbrej::set_tracking_pll_bw(const c_def::v_union & v)
{
    vfo_s::set_tracking_pll_bw(v);
    //TODO: update PLL bw on all slaves here
    return true;
}

bool nbrej::set_filter_alfa(const c_def::v_union & v)
{
    vfo_s::set_filter_alfa(v);
    //TODO: update filter alfa on all slaves here
    return true;
}
#endif
