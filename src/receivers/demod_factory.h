/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2011-2016 Alexandru Csete OZ9AEC.
 * Copyright 2022 vladisslav2011@gmail.com.
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
#ifndef DEMOD_FACTORY_H
#define DEMOD_FACTORY_H
#include "modulations.h"
#include "nbrx.h"
#include "wfmrx.h"
#include "nbrej.h"

class Demod_Factory
{
public:
    static receiver_base_cf_sptr make(Modulations::idx demod, double quad_rate, float audio_rate, std::vector<receiver_base_cf_sptr> & rxes)
    {
        Modulations::rx_chain rxc = Modulations::get_rxc(demod);
        switch (rxc)
        {
        case Modulations::RX_CHAIN_NBRX:
        case Modulations::RX_CHAIN_NONE:
            return make_nbrx(quad_rate, audio_rate, rxes);
        case Modulations::RX_CHAIN_WFMRX:
            return make_wfmrx(quad_rate, audio_rate, rxes);
        case Modulations::RX_CHAIN_REJECTOR:
            return nbrej::make(quad_rate, audio_rate, rxes);
        default:
            return nullptr;
        }
    }
};

#endif // DEMOD_FACTORY_H
