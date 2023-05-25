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
    .idx(C_NFM_MAXDEV)
    .name("Max dev")
    .title("Max dev")
    .title_placement(c_def::grid_placement(PLACE_NEXT,0))
    .placement(c_def::grid_placement(PLACE_SAME,PLACE_NEXT))
    .hint("NFM Max dev")
    .g_type(G_COMBO)
    .dock(D_RXOPT)
    .window(W_DEMOD_OPT)
    .scope(S_VFO)
    .demod_specific(true)
    .demodgroup(Modulations::GRP_NFM)
    .v3_config_group("receiver")
    .config_key("fm_maxdev")
    .bookmarks_column(18)
    .bookmarks_key("FM Max Deviation")
    .v_type(V_DOUBLE)
    .frac_digits(0)
    .def(2500.0)
    .min(10.0)
    .max(75000.0)
    .step(10.0)
    .presets(
    {
        {"0","Voice (2.5 kHz)",2500.0},
        {"1","Voice (5 kHz)",5000.0},
        {"2","APT (17 kHz)",17000.0},
        {"3","APT (25 kHz)",25000.0},
    })
    ,
c_def()
    .idx(C_NFMPLL_MAXDEV)
    .name("Max dev")
    .title("Max dev")
    .title_placement(c_def::grid_placement(PLACE_NEXT,0))
    .placement(c_def::grid_placement(PLACE_SAME,PLACE_NEXT))
    .hint("NFM Max dev")
    .g_type(G_DOUBLESPINBOX)
    .dock(D_RXOPT)
    .window(W_DEMOD_OPT)
    .scope(S_VFO)
    .demod_specific(true)
    .demodgroup(Modulations::GRP_NFMPLL)
    .v3_config_group("receiver")
    .config_key("")
    .bookmarks_column(-1)
    .bookmarks_key("")
    .v_type(V_DOUBLE)
    .frac_digits(0)
    .def(2500.0)
    .min(10.0)
    .max(75000.0)
    .step(10.0)
    .presets(
    {
        {"0","Voice (2.5 kHz)",2500.0},
        {"1","Voice (5 kHz)",5000.0},
        {"2","APT (17 kHz)",17000.0},
        {"3","APT (25 kHz)",25000.0},
    })
    ,
c_def()
    .idx(C_NFM_DEEMPH)
    .name("Deemphasis")
    .title("Deemphasis")
    .title_placement(c_def::grid_placement(PLACE_NEXT,0))
    .placement(c_def::grid_placement(PLACE_SAME,PLACE_NEXT))
    .hint("NFM Deemphasis")
    .g_type(G_COMBO)
    .dock(D_RXOPT)
    .window(W_DEMOD_OPT)
    .scope(S_VFO)
    .demod_specific(true)
    .demodgroup(Modulations::GRP_NFM)
    .v3_config_group("receiver")
    .config_key("fm_deemph")
    .bookmarks_column(19)
    .bookmarks_key("FM Deemphasis")
    .v_type(V_DOUBLE)
    .def(75.0)
    .min(0.0)
    .max(1000.0)
    .step(1.0)
    .presets(
    {
        {"0","Off",0.0},
        {"1","25 us",25.0},
        {"2","50 us",50.0},
        {"3","75 us",75.0},
        {"4","100 us",100.0},
        {"5","250 us",250.0},
        {"6","530 us",530.0},
        {"7","1 ms",1000.0},
    })
    ,
c_def()
    .idx(C_NFM_SUBTONE_FILTER)
    .name("Subtone filter")
    .title("Subtone filter")
    .title_placement(c_def::grid_placement(PLACE_NONE,0))
    .placement(c_def::grid_placement(PLACE_NEXT,1))
    .hint("NFM Subtone filter")
    .g_type(G_CHECKBOX)
    .dock(D_RXOPT)
    .window(W_DEMOD_OPT)
    .scope(S_VFO)
    .demod_specific(true)
    .demodgroup(Modulations::GRP_NFM)
    .v3_config_group("receiver")
    .config_key("")
    .bookmarks_column(-1)
    .bookmarks_key("")
    .v_type(V_BOOLEAN)
    .def(0)
    .min(0)
    .max(1)
    .step(1)
    ,
c_def()
    .idx(C_NFMPLL_DAMPING_FACTOR)
    .name("Damping factor")
    .title("Damping factor")
    .title_placement(c_def::grid_placement(PLACE_NEXT,0))
    .placement(c_def::grid_placement(PLACE_SAME,PLACE_NEXT))
    .hint("NFM PLL Damping factor")
    .g_type(G_DOUBLESPINBOX)
    .dock(D_RXOPT)
    .window(W_DEMOD_OPT)
    .scope(S_VFO)
    .demod_specific(true)
    .demodgroup(Modulations::GRP_NFMPLL)
    .v3_config_group("receiver")
    .config_key("fmpll_damping_factor")
    .bookmarks_column(-1)
    .bookmarks_key("")
    .v_type(V_DOUBLE)
    .frac_digits(2)
    .def(0.7)
    .min(0.01)
    .max(1.0)
    .step(0.01)
    ,
c_def()
    .idx(C_NFMPLL_PLLBW)
    .name("PLL")
    .title("PLL")
    .title_placement(c_def::grid_placement(PLACE_NEXT,0))
    .placement(c_def::grid_placement(PLACE_SAME,PLACE_NEXT))
    .hint("NFM PLL bandwidth")
    .g_type(G_DOUBLESPINBOX)
    .dock(D_RXOPT)
    .window(W_DEMOD_OPT)
    .scope(S_VFO)
    .demod_specific(true)
    .demodgroup(Modulations::GRP_NFMPLL)
    .v3_config_group("receiver")
    .config_key("nfmpll_bw")
    .bookmarks_column(31)
    .bookmarks_key("NFM PLL BW")
    .v_type(V_DOUBLE)
    .frac_digits(4)
    .def(0.027)
    .min(0.0001)
    .max(0.5)
    .step(0.001)
    ,
c_def()
    .idx(C_NFMPLL_SUBTONE_FILTER)
    .name("Subtone filter")
    .title("Subtone filter")
    .title_placement(c_def::grid_placement(PLACE_NONE,0))
    .placement(c_def::grid_placement(PLACE_NEXT,1))
    .hint("NFM PLL Subtone filter")
    .g_type(G_CHECKBOX)
    .dock(D_RXOPT)
    .window(W_DEMOD_OPT)
    .scope(S_VFO)
    .demod_specific(true)
    .demodgroup(Modulations::GRP_NFMPLL)
    .v3_config_group("receiver")
    .config_key("subtone_filter")
    .bookmarks_column(-1)
    .bookmarks_key("")
    .v_type(V_BOOLEAN)
    .def(0)
    .min(0)
    .max(1)
    .step(1)
    ,
c_def()
    .idx(C_AMSYNC_PLLBW)
    .name("PLL")
    .title("PLL")
    .title_placement(c_def::grid_placement(0,0))
    .placement(c_def::grid_placement(PLACE_SAME,PLACE_NEXT))
    .hint("AM SYNC PLL bandwidth")
    .g_type(G_COMBO)
    .dock(D_RXOPT)
    .window(W_DEMOD_OPT)
    .scope(S_VFO)
    .demod_specific(true)
    .demodgroup(Modulations::GRP_AM_SYNC)
    .v3_config_group("receiver")
    .config_key("pll_bw")
    .bookmarks_column(22)
    .bookmarks_key("PLL BW")
    .v_type(V_DOUBLE)
    .frac_digits(4)
    .def(0.001)
    .min(0.0001)
    .max(0.01)
    .step(0.0001)
    .presets(
    {
        {"Fast","Fast",0.01},
        {"Medium","Medium",0.001},
        {"Slow","Slow",0.0001},
    })
    ,
c_def()
    .idx(C_AMSYNC_DCR)
    .name("DCR")
    .title("DCR")
    .title_placement(c_def::grid_placement(PLACE_NONE,0))
    .placement(c_def::grid_placement(PLACE_NEXT,1))
    .hint("AM SYNC DC remove filter")
    .g_type(G_CHECKBOX)
    .dock(D_RXOPT)
    .window(W_DEMOD_OPT)
    .scope(S_VFO)
    .demod_specific(true)
    .demodgroup(Modulations::GRP_AM_SYNC)
    .v3_config_group("receiver")
    .config_key("amsync_dcr")
    .bookmarks_column(21)
    .bookmarks_key("AM SYNC DCR")
    .v_type(V_BOOLEAN)
    .def(1)
    .min(0)
    .max(1)
    .step(1)
    ,
c_def()
    .idx(C_AM_DCR)
    .name("DCR")
    .title("DCR")
    .title_placement(c_def::grid_placement(PLACE_NONE,0))
    .placement(c_def::grid_placement(PLACE_NEXT,1))
    .hint("AM DC remove filter")
    .g_type(G_CHECKBOX)
    .dock(D_RXOPT)
    .window(W_DEMOD_OPT)
    .scope(S_VFO)
    .demod_specific(true)
    .demodgroup(Modulations::GRP_AM)
    .v3_config_group("receiver")
    .config_key("am_dcr")
    .bookmarks_column(20)
    .bookmarks_key("AM DCR")
    .v_type(V_BOOLEAN)
    .def(1)
    .min(0)
    .max(1)
    .step(1)
    ,
c_def()
    .idx(C_CW_OFFSET)
    .name("CW offset")
    .title("CW offset")
    .title_placement(c_def::grid_placement(0,0))
    .placement(c_def::grid_placement(PLACE_SAME,PLACE_NEXT))
    .hint("CW offset, Hz")
    .g_type(G_SPINBOX)
    .dock(D_RXOPT)
    .window(W_DEMOD_OPT)
    .scope(S_VFO)
    .demod_specific(true)
    .demodgroup(Modulations::GRP_CW)
    .v3_config_group("receiver")
    .config_key("cwoffset")
    .bookmarks_column(17)
    .bookmarks_key("CW Offset")
    .v_type(V_INT)
    .def(700)
    .min(0)
    .max(2000)
    .step(1)
    ,
c_def()
    .idx(C_DEMOD_OFF_DUMMY)
    .name("No options for this demod")
    .title("No options for this demod")
    .title_placement(c_def::grid_placement(PLACE_SAME,0))
    .placement(c_def::grid_placement(PLACE_SAME,PLACE_NEXT))
    .hint("No options for this demod")
    .g_type(G_LABEL)
    .dock(D_RXOPT)
    .window(W_DEMOD_OPT)
    .scope(S_VFO)
    .v_type(V_STRING)
    .demod_specific(true)
    .demodgroup(Modulations::GRP_OFF)
    .def("")
    .min("")
    .max("")
    .step("")
    .writable(false)
    ,
c_def()
    .idx(C_RAWIQ_DUMMY)
    .name("No options for this demod")
    .title("No options for this demod")
    .title_placement(c_def::grid_placement(PLACE_SAME,0))
    .placement(c_def::grid_placement(PLACE_SAME,PLACE_NEXT))
    .hint("No options for this demod")
    .g_type(G_LABEL)
    .dock(D_RXOPT)
    .window(W_DEMOD_OPT)
    .scope(S_VFO)
    .v_type(V_STRING)
    .demod_specific(true)
    .demodgroup(Modulations::GRP_RAW)
    .def("")
    .min("")
    .max("")
    .step("")
    .writable(false)
    ,
c_def()
    .idx(C_SSB_DUMMY)
    .name("No options for this demod")
    .title("No options for this demod")
    .title_placement(c_def::grid_placement(PLACE_SAME,0))
    .placement(c_def::grid_placement(PLACE_SAME,PLACE_NEXT))
    .hint("No options for this demod")
    .g_type(G_LABEL)
    .dock(D_RXOPT)
    .window(W_DEMOD_OPT)
    .scope(S_VFO)
    .v_type(V_STRING)
    .demod_specific(true)
    .demodgroup(Modulations::GRP_SSB)
    .def("")
    .min("")
    .max("")
    .step("")
    .writable(false)
    ,
c_def()
    .idx(C_WFM_DEEMPH)
    .name("Deemphasis")
    .title("Deemphasis")
    .title_placement(c_def::grid_placement(PLACE_NEXT,0))
    .placement(c_def::grid_placement(PLACE_SAME,PLACE_NEXT))
    .hint("WFM Deemphasis")
    .g_type(G_COMBO)
    .dock(D_RXOPT)
    .window(W_DEMOD_OPT)
    .scope(S_VFO)
    .demod_specific(true)
    .demodgroup(Modulations::GRP_WFM_MONO)
    .v3_config_group("receiver")
    .config_key("wfm_deemph")
    .v_type(V_DOUBLE)
    .def(50.0)
    .min(0.0)
    .max(1000.0)
    .step(1.0)
    .presets(
    {
        {"0","Off",0.0},
        {"1","25 us",25.0},
        {"2","50 us",50.0},
        {"3","75 us",75.0},
        {"4","100 us",100.0},
        {"5","250 us",250.0},
        {"6","530 us",530.0},
        {"7","1 ms",1000.0},
    })
    ,
c_def()
    .idx(C_WFM_STEREO_DEEMPH)
    .name("Deemphasis")
    .title("Deemphasis")
    .title_placement(c_def::grid_placement(PLACE_NEXT,0))
    .placement(c_def::grid_placement(PLACE_SAME,PLACE_NEXT))
    .hint("WFM Deemphasis")
    .g_type(G_COMBO)
    .dock(D_RXOPT)
    .window(W_DEMOD_OPT)
    .scope(S_VFO)
    .demod_specific(true)
    .demodgroup(Modulations::GRP_WFM_STEREO)
    .v3_config_group("receiver")
    .config_key("wfm_deemph")
    .v_type(V_DOUBLE)
    .def(50.0)
    .min(0.0)
    .max(1000.0)
    .step(1.0)
    .presets(
    {
        {"0","Off",0.0},
        {"1","25 us",25.0},
        {"2","50 us",50.0},
        {"3","75 us",75.0},
        {"4","100 us",100.0},
        {"5","250 us",250.0},
        {"6","530 us",530.0},
        {"7","1 ms",1000.0},
    })
    ,
c_def()
    .idx(C_WFM_OIRT_DEEMPH)
    .name("Deemphasis")
    .title("Deemphasis")
    .title_placement(c_def::grid_placement(PLACE_NEXT,0))
    .placement(c_def::grid_placement(PLACE_SAME,PLACE_NEXT))
    .hint("WFM Deemphasis")
    .g_type(G_COMBO)
    .dock(D_RXOPT)
    .window(W_DEMOD_OPT)
    .scope(S_VFO)
    .demod_specific(true)
    .demodgroup(Modulations::GRP_WFM_STEREO_OIRT)
    .v3_config_group("receiver")
    .config_key("wfm_deemph")
    .v_type(V_DOUBLE)
    .def(50.0)
    .min(0.0)
    .max(1000.0)
    .step(1.0)
    .presets(
    {
        {"0","Off",0.0},
        {"1","25 us",25.0},
        {"2","50 us",50.0},
        {"3","75 us",75.0},
        {"4","100 us",100.0},
        {"5","250 us",250.0},
        {"6","530 us",530.0},
        {"7","1 ms",1000.0},
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
