/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2023 vladisslav2011@gmail.com.
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

#include "dcontrols.h"

static const std::array<c_def,C_COUNT> store{

c_def()
    .idx(C_TEST)
    .name("test")
    .title("test ctl")
    .title_placement(c_def::grid_placement(PLACE_SAME,0))
    .placement(c_def::grid_placement(PLACE_SAME,PLACE_NEXT))
    .hint("widget test value")
//    .g_type(G_TOGGLEBUTTON)
//    .g_type(G_SLIDER)
//    .g_type(G_SPINBOX)
    .g_type(G_DOUBLESPINBOX)
    .dock(D_RDS)
    .scope(S_VFO)
    .v3_config_group("receiver")
    .v4_config_group("test")
    .config_key("test")
//    .tab("Tab0")
    .bookmarks_column(-1)
    .bookmarks_key("-1")
    .v_type(V_INT)
    .def(0)
    .min(0)
    .max(1000)
    .step(10)
    .frac_digits(1)
    .readable(true)
    .writable(true)
    .event(false)
    .presets(
    {
    })
    ,

};

std::array<std::function<void (const int, const c_def::v_union &)>, C_COUNT> conf_base::observers{};

bool c_def::clip(c_def::v_union & v) const
{
    if(v_type()==V_STRING)
        return false;
    if(v>d_max)
    {
        v=d_max;
        return true;
    }
    if(v<d_min)
    {
        v=d_min;
        return true;
    }
    return false;
}

const std::array<c_def,C_COUNT> & c_def::all()
{
    return store;
}

bool conf_base::register_observer(c_id optid, std::function<void (const int, const c_def::v_union &)> pevent)
{
    observers[optid]=pevent;
    return true;
}

const std::function<void (const int, const c_def::v_union &)> & conf_base::observer(c_id id) const
{
    return observers[id];
}

void conf_base::changed_value(c_id id, const int rx, const c_def::v_union & value) const
{
    auto & handler = observer(id);
    if(handler)
        handler(rx, value);
}
