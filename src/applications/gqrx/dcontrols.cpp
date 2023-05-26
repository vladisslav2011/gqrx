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
    
c_def()
    .idx(C_RDS_ON)
    .name("Enable RDS")
    .title("Enable RDS")
    .title_placement(c_def::grid_placement(PLACE_NONE,PLACE_NONE))
    .placement(c_def::grid_placement(PLACE_NEXT,0,1,2))
    .hint("Enable RDS decoder")
    .g_type(G_CHECKBOX)
    .dock(D_RDS)
    .scope(S_VFO)
    .v3_config_group("receiver")
    .config_key("rds")
    .v_type(V_INT)
    .def(0)
    .min(0)
    .max(1)
    .step(1)
    ,
c_def()
    .idx(C_RDS_PS)
    .name("Station name")
    .title("Station Name:")
    .title_placement(c_def::grid_placement(PLACE_NEXT,0))
    .placement(c_def::grid_placement(PLACE_SAME,PLACE_NEXT))
    .hint("RDS Station name")
    .g_type(G_LABEL)
    .dock(D_RDS)
    .scope(S_VFO)
    .v_type(V_STRING)
    .def("")
    .min("")
    .max("")
    .step("")
    .readable(true)
    .writable(false)
    .event(true)
    ,
c_def()
    .idx(C_RDS_PTY)
    .name("Program type")
    .title("Program Type:")
    .title_placement(c_def::grid_placement(PLACE_NEXT,0))
    .placement(c_def::grid_placement(PLACE_SAME,PLACE_NEXT))
    .hint("RDS Program type")
    .g_type(G_LABEL)
    .dock(D_RDS)
    .scope(S_VFO)
    .v_type(V_STRING)
    .def("")
    .min("")
    .max("")
    .step("")
    .readable(true)
    .writable(false)
    .event(true)
    ,
c_def()
    .idx(C_RDS_PI)
    .name("Program ID")
    .title("Program ID:")
    .title_placement(c_def::grid_placement(PLACE_NEXT,0))
    .placement(c_def::grid_placement(PLACE_SAME,PLACE_NEXT))
    .hint("RDS Program ID")
    .g_type(G_LABEL)
    .dock(D_RDS)
    .scope(S_VFO)
    .v_type(V_STRING)
    .def("")
    .min("")
    .max("")
    .step("")
    .readable(true)
    .writable(false)
    .event(true)
    ,
c_def()
    .idx(C_RDS_RADIOTEXT)
    .name("Radio text")
    .title("Radio Text:")
    .title_placement(c_def::grid_placement(PLACE_NEXT,0))
    .placement(c_def::grid_placement(PLACE_SAME,PLACE_NEXT))
    .hint("RDS Radio text")
    .g_type(G_LABEL)
    .dock(D_RDS)
    .scope(S_VFO)
    .v_type(V_STRING)
    .def("")
    .min("")
    .max("")
    .step("")
    .readable(true)
    .writable(false)
    .event(true)
    ,
c_def()
    .idx(C_RDS_CLOCKTIME)
    .name("Clock time")
    .title("Clock Time:")
    .title_placement(c_def::grid_placement(PLACE_NEXT,0))
    .placement(c_def::grid_placement(PLACE_SAME,PLACE_NEXT))
    .hint("RDS Clock time")
    .g_type(G_LABEL)
    .dock(D_RDS)
    .scope(S_VFO)
    .v_type(V_STRING)
    .def("")
    .min("")
    .max("")
    .step("")
    .readable(true)
    .writable(false)
    .event(true)
    ,
c_def()
    .idx(C_RDS_ALTFREQ)
    .name("Alt Frequencies")
    .title("Alt. Frequencies:")
    .title_placement(c_def::grid_placement(PLACE_NEXT,0))
    .placement(c_def::grid_placement(PLACE_SAME,PLACE_NEXT))
    .hint("RDS Alternative frequencies list")
    .g_type(G_LABEL)
    .dock(D_RDS)
    .scope(S_VFO)
    .v_type(V_STRING)
    .def("")
    .min("")
    .max("")
    .step("")
    .readable(true)
    .writable(false)
    .event(true)
    ,
c_def()
    .idx(C_RDS_FLAGS)
    .name("Flags")
    .title("Flags:")
    .title_placement(c_def::grid_placement(PLACE_NEXT,0))
    .placement(c_def::grid_placement(PLACE_SAME,PLACE_NEXT))
    .hint("RDS Flags")
    .g_type(G_LABEL)
    .dock(D_RDS)
    .scope(S_VFO)
    .v_type(V_STRING)
    .def("")
    .min("")
    .max("")
    .step("")
    .readable(true)
    .writable(false)
    .event(true)
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
