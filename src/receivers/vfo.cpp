/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
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
#include "vfo.h"

void vfo_s::set_offset(int offset)
{
    d_offset = offset;
}

void vfo_s::set_filter_low(int low)
{
    d_filter_low = low;
}

void vfo_s::set_filter_high(int high)
{
    d_filter_high = high;
}

void vfo_s::set_filter_tw(int tw)
{
    d_filter_tw = tw;
}

void vfo_s::set_filter(int low, int high, int tw)
{
    d_filter_low = low;
    d_filter_high = high;
    d_filter_tw = tw;
}

void vfo_s::filter_adjust()
{
    Modulations::UpdateFilterRange(d_demod, d_filter_low, d_filter_high);
    Modulations::UpdateTw(d_filter_low, d_filter_high, d_filter_tw);
}

void vfo_s::set_demod(Modulations::idx demod)
{
    d_demod = demod;
}

void vfo_s::set_index(int index)
{
    d_index = index;
}

void vfo_s::set_freq_lock(bool on)
{
    d_locked = on;
}

void vfo_s::set_sql_level(double level_db)
{
    d_level_db = level_db;
}

void vfo_s::set_sql_alpha(double alpha)
{
    d_alpha = alpha;
}

void vfo_s::set_agc_on(bool agc_on)
{
    d_agc_on = agc_on;
}

void vfo_s::set_agc_target_level(int target_level)
{
    d_agc_target_level = target_level;
}

void vfo_s::set_agc_manual_gain(float gain)
{
    d_agc_manual_gain = gain;
}

void vfo_s::set_agc_max_gain(int gain)
{
    d_agc_max_gain = gain;
}

void vfo_s::set_agc_attack(int attack_ms)
{
    d_agc_attack_ms = attack_ms;
}

void vfo_s::set_agc_decay(int decay_ms)
{
    d_agc_decay_ms = decay_ms;
}

void vfo_s::set_agc_hang(int hang_ms)
{
    d_agc_hang_ms = hang_ms;
}

void vfo_s::set_agc_panning(int panning)
{
    d_agc_panning = panning;
}

void vfo_s::set_agc_panning_auto(bool mode)
{
    d_agc_panning_auto = mode;
}

void vfo_s::set_agc_mute(bool agc_mute)
{
    d_agc_mute = agc_mute;
}

bool vfo_s::set_cw_offset(const c_def::v_union & v)
{
    d_cw_offset = v;
    return true;
}

bool vfo_s::set_fm_maxdev(const c_def::v_union & v)
{
    d_fm_maxdev = v;
    return true;
}

bool vfo_s::set_fm_deemph(const c_def::v_union & v)
{
    d_fm_deemph = v;
    return true;
}

bool vfo_s::set_fmpll_damping_factor(const c_def::v_union & v)
{
    d_fmpll_damping_factor = v;
    return true;
}

bool vfo::set_subtone_filter(const c_def::v_union & v)
{
    d_subtone_filter = v;
    return true;
}

bool vfo_s::set_am_dcr(const c_def::v_union & v)
{
    d_am_dcr = v;
    return true;
}

bool vfo_s::set_amsync_dcr(const c_def::v_union & v)
{
    d_amsync_dcr = v;
    return true;
}

bool vfo_s::set_amsync_pll_bw(const c_def::v_union & v)
{
    d_pll_bw = v;
    return true;
}

bool vfo_s::set_pll_bw(const c_def::v_union & v)
{
    d_fmpll_bw = v;
    return true;
}

bool vfo_s::set_wfm_deemph(const c_def::v_union & v)
{
    d_wfm_deemph = v;
    return true;
}

void vfo_s::set_nb_on(int nbid, bool on)
{
    if (nbid - 1 < RECEIVER_NB_COUNT)
        d_nb_on[nbid - 1] = on;
}

bool vfo_s::get_nb_on(int nbid) const
{
    if (nbid - 1 < RECEIVER_NB_COUNT)
        return d_nb_on[nbid - 1];
    return false;
}

void vfo_s::set_nb_threshold(int nbid, float threshold)
{
    if (nbid - 1 < RECEIVER_NB_COUNT)
        d_nb_threshold[nbid - 1] = threshold;
}

float vfo_s::get_nb_threshold(int nbid) const
{
    if (nbid - 1 < RECEIVER_NB_COUNT)
        return d_nb_threshold[nbid - 1];
    return 0.0;
}

void vfo_s::set_audio_rec_dir(const std::string& dir)
{
    d_rec_dir = dir;
}

void vfo_s::set_audio_rec_sql_triggered(bool enabled)
{
    d_rec_sql_triggered = enabled;
}

void vfo_s::set_audio_rec_min_time(const int time_ms)
{
    d_rec_min_time = time_ms;
}

void vfo_s::set_audio_rec_max_gap(const int time_ms)
{
    d_rec_max_gap = time_ms;
}

/* UDP streaming */
bool vfo_s::set_udp_host(const std::string &host)
{
    d_udp_host = host;
    return true;
}

bool vfo_s::set_udp_port(int port)
{
    d_udp_port = port;
    return true;
}

bool vfo_s::set_udp_stereo(bool stereo)
{
    d_udp_stereo = stereo;
    return true;
}

void vfo_s::restore_settings(vfo_s& from, bool force)
{
    set_freq_lock(from.get_freq_lock());
    set_demod(from.get_demod());
    set_sql_level(from.get_sql_level());
    set_sql_alpha(from.get_sql_alpha());

    set_agc_on(from.get_agc_on());
    set_agc_target_level(from.get_agc_target_level());
    set_agc_manual_gain(from.get_agc_manual_gain());
    set_agc_max_gain(from.get_agc_max_gain());
    set_agc_attack(from.get_agc_attack());
    set_agc_decay(from.get_agc_decay());
    set_agc_hang(from.get_agc_hang());
    set_agc_panning(from.get_agc_panning());
    set_agc_panning_auto(from.get_agc_panning_auto());

    set_filter(from.get_filter_low(), from.get_filter_high(), from.get_filter_tw());

    for (int k = 0; k < RECEIVER_NB_COUNT; k++)
    {
        set_nb_on(k + 1, from.get_nb_on(k + 1));
        set_nb_threshold(k + 1, from.get_nb_threshold(k + 1));
    }
    if (force || (from.get_audio_rec_dir() != ""))
        set_audio_rec_dir(from.get_audio_rec_dir());
    if (force || (from.get_audio_rec_min_time() > 0))
        set_audio_rec_min_time(from.get_audio_rec_min_time());
    if (force || (from.get_audio_rec_max_gap() > 0))
        set_audio_rec_max_gap(from.get_audio_rec_max_gap());
    set_audio_rec_sql_triggered(from.get_audio_rec_sql_triggered());
    set_udp_host(from.get_udp_host());
    set_udp_port(from.get_udp_port());
    set_udp_stereo(from.get_udp_stereo());

    c_def::v_union v(0);
    from.get_fm_maxdev(v);set_fm_maxdev(v);
    from.get_fm_deemph(v);set_fm_deemph(v);
    from.get_fmpll_damping_factor(v);set_fmpll_damping_factor(v);
    from.get_subtone_filter(v);set_subtone_filter(v);
    from.get_pll_bw(v);set_pll_bw(v);
    from.get_amsync_pll_bw(v);set_amsync_pll_bw(v);
    from.get_amsync_dcr(v);set_amsync_dcr(v);
    from.get_am_dcr(v);set_am_dcr(v);
    from.get_cw_offset(v);set_cw_offset(v);
    from.get_wfm_deemph(v);set_wfm_deemph(v);
    from.get_rds_on(v);set_rds_on(v);
    from.get_test(v);set_test(v);
}

bool vfo_s::set_test(const c_def::v_union & v)
{
    if(d_testval==int(v))
        return true;
    printf("--------------------- vfo_s::set_test %d %d\n",d_index,(int)v);
    d_testval=v;
    return true;
}

bool vfo_s::get_test(c_def::v_union & v) const
{
    printf("--------------------- vfo_s::get_test\n");
    v=d_testval;
    return true;
}

bool vfo_s::set_rds_on(const c_def::v_union & v) {d_rds_on=v; return true;}
bool vfo_s::get_rds_on(c_def::v_union & v) const {v=d_rds_on; return true;}
bool vfo_s::get_rds_pi(c_def::v_union &) const {return false;}
bool vfo_s::get_rds_ps(c_def::v_union &) const {return false;}
bool vfo_s::get_rds_pty(c_def::v_union &) const {return false;}
bool vfo_s::get_rds_flagstring(c_def::v_union &) const {return false;}
bool vfo_s::get_rds_rt(c_def::v_union &) const {return false;}
bool vfo_s::get_rds_clock(c_def::v_union &) const {return false;}
bool vfo_s::get_rds_af(c_def::v_union &) const {return false;}

int vfo_s::conf_initializer()
{
    setters[C_TEST]=&vfo_s::set_test;
    getters[C_TEST]=&vfo_s::get_test;

    // NFM parameters
    setters[C_NFM_MAXDEV]=&vfo_s::set_fm_maxdev;
    getters[C_NFM_MAXDEV]=&vfo_s::get_fm_maxdev;
    setters[C_NFM_DEEMPH]=&vfo_s::set_fm_deemph;
    getters[C_NFM_DEEMPH]=&vfo_s::get_fm_deemph;
    setters[C_NFM_SUBTONE_FILTER]=&vfo_s::set_subtone_filter;
    getters[C_NFM_SUBTONE_FILTER]=&vfo_s::get_subtone_filter;
    // NFM PLL parameters
    setters[C_NFMPLL_MAXDEV]=&vfo_s::set_fm_maxdev;
    getters[C_NFMPLL_MAXDEV]=&vfo_s::get_fm_maxdev;
    setters[C_NFMPLL_SUBTONE_FILTER]=&vfo_s::set_subtone_filter;
    getters[C_NFMPLL_SUBTONE_FILTER]=&vfo_s::get_subtone_filter;
    setters[C_NFMPLL_DAMPING_FACTOR]=&vfo_s::set_fmpll_damping_factor;
    getters[C_NFMPLL_DAMPING_FACTOR]=&vfo_s::get_fmpll_damping_factor;
    setters[C_NFMPLL_PLLBW]=&vfo_s::set_pll_bw;
    getters[C_NFMPLL_PLLBW]=&vfo_s::get_pll_bw;
    // AM SYNC parameters
    setters[C_AMSYNC_DCR]=&vfo_s::set_amsync_dcr;
    getters[C_AMSYNC_DCR]=&vfo_s::get_amsync_dcr;
    setters[C_AMSYNC_PLLBW]=&vfo_s::set_amsync_pll_bw;
    getters[C_AMSYNC_PLLBW]=&vfo_s::get_amsync_pll_bw;
    // AM parameters
    setters[C_AM_DCR]=&vfo_s::set_am_dcr;
    getters[C_AM_DCR]=&vfo_s::get_am_dcr;
    // CW parameters
    setters[C_CW_OFFSET]=&vfo_s::set_cw_offset;
    getters[C_CW_OFFSET]=&vfo_s::get_cw_offset;
    // WFM parameters
    setters[C_WFM_DEEMPH]=&vfo_s::set_wfm_deemph;
    getters[C_WFM_DEEMPH]=&vfo_s::get_wfm_deemph;
    setters[C_WFM_STEREO_DEEMPH]=&vfo_s::set_wfm_deemph;
    getters[C_WFM_STEREO_DEEMPH]=&vfo_s::get_wfm_deemph;
    setters[C_WFM_OIRT_DEEMPH]=&vfo_s::set_wfm_deemph;
    getters[C_WFM_OIRT_DEEMPH]=&vfo_s::get_wfm_deemph;

    getters[C_RDS_ON]=&vfo_s::get_rds_on;
    setters[C_RDS_ON]=&vfo_s::set_rds_on;
    getters[C_RDS_PI]=&vfo_s::get_rds_pi;
    getters[C_RDS_PS]=&vfo_s::get_rds_ps;
    getters[C_RDS_PTY]=&vfo_s::get_rds_pty;
    getters[C_RDS_FLAGS]=&vfo_s::get_rds_flagstring;
    getters[C_RDS_RADIOTEXT]=&vfo_s::get_rds_rt;
    getters[C_RDS_CLOCKTIME]=&vfo_s::get_rds_clock;
    getters[C_RDS_ALTFREQ]=&vfo_s::get_rds_af;
    return 0;
}

template<> int conf_dispatchers<vfo_s>::conf_dispatchers_init_dummy(vfo_s::conf_initializer());
