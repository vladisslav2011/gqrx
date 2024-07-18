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


nbrej::sptr nbrej::make(double quad_rate, float audio_rate, std::vector<receiver_base_cf_sptr> & rxes)
{
    return gnuradio::get_initial_sptr(new nbrej(quad_rate, audio_rate, rxes));
}

nbrej::nbrej(double quad_rate, float audio_rate, std::vector<receiver_base_cf_sptr> & rxes)
    : receiver_base_cf("NBREJ", NB_PREF_QUAD_RATE, quad_rate, audio_rate, rxes)
{
    set_demod(Modulations::MODE_NB_REJECTOR);
    for(size_t k=0;k<d_rxes.size();k++)
        if(d_rxes[k] && d_rxes[k]->get_demod() != Modulations::MODE_NB_REJECTOR)
            d_rxes[k]->d_rejector_count++;
}

nbrej::~nbrej()
{
    for(size_t k=0;k<d_rxes.size();k++)
        if(d_rxes[k] && d_rxes[k]->get_demod() != Modulations::MODE_NB_REJECTOR)
        {
            if(std::abs(d_rxes[k]->get_offset() - get_offset()) * 2 < d_rxes[k]->get_pref_quad_rate())
               d_rxes[k]->remove_rejector(this);
            d_rxes[k]->d_rejector_count--;
        }
}

void nbrej::set_filter(int low, int high, Modulations::filter_shape shape)
{
    int offset=get_offset();
    receiver_base_cf::set_filter(low, high, shape);
    for(size_t k=0;k<d_rxes.size();k++)
        if(d_rxes[k] && d_rxes[k]->get_demod() != Modulations::MODE_NB_REJECTOR)
            if(std::abs(d_rxes[k]->get_offset() - offset) * 2 < d_rxes[k]->get_pref_quad_rate())
               d_rxes[k]->update_rejector(this);
}

bool nbrej::set_offset(int offset, bool locked)
{
    int old_offset=get_offset();
    if(offset==old_offset)
        return false;
    vfo_s::set_offset(offset, locked);
    bool needslock = false;
    size_t k = 0;
    if(!locked)
        for(k=0;k<d_rxes.size();k++)
            if(d_rxes[k] && d_rxes[k]->get_demod() != Modulations::MODE_NB_REJECTOR)
            {
                if(std::abs(d_rxes[k]->get_offset() - offset) * 2 < d_rxes[k]->get_pref_quad_rate())
                    needslock |= d_rxes[k]->find_rejector(this) == -1;
                else if(std::abs(d_rxes[k]->get_offset() - old_offset) * 2 < d_rxes[k]->get_pref_quad_rate())
                    needslock |= d_rxes[k]->find_rejector(this) != -1;
                if(needslock)
                    break;
            }
    for(size_t t=0;t<d_rxes.size();t++)
        if(d_rxes[t] && d_rxes[t]->get_demod() != Modulations::MODE_NB_REJECTOR)
        {
            if(std::abs(d_rxes[t]->get_offset() - offset) * 2 < d_rxes[t]->get_pref_quad_rate())
                d_rxes[t]->update_rejector(this);
            else if(std::abs(d_rxes[t]->get_offset() - old_offset) * 2 < d_rxes[t]->get_pref_quad_rate())
                d_rxes[t]->remove_rejector(this);
        }
    return needslock;
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
