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
#include <sstream>
#include <iostream>
#include <iomanip>

bool vfo_s::set_offset(int offset, bool locked)
{
    d_offset = offset;
    return false;
}

bool vfo_s::set_filter_low(const c_def::v_union & v)
{
    d_filter_low = v;
    return true;
}

bool vfo_s::set_filter_high(const c_def::v_union & v)
{
    d_filter_high = v;
    return true;
}

bool vfo_s::set_filter_shape(const c_def::v_union & v)
{
    d_filter_shape = Modulations::filter_shape(int(v));
    // Allow asymmetric filters (Shift+Wheel)
    #if 0
     Modulations::UpdateFilterRange(d_demod, d_filter_low, d_filter_high);
    #endif
    d_filter_tw = Modulations::TwFromFilterShape(d_filter_low, d_filter_high, d_filter_shape);
    return true;
}

void vfo_s::set_filter(int low, int high, Modulations::filter_shape shape)
{
    d_filter_low = low;
    d_filter_high = high;
    d_filter_shape = shape;
    d_filter_tw = Modulations::TwFromFilterShape(low, high, shape);
}

void vfo_s::filter_adjust()
{
    Modulations::UpdateFilterRange(d_demod, d_filter_low, d_filter_high);
    d_filter_tw = Modulations::TwFromFilterShape(d_filter_low, d_filter_high, d_filter_shape);
}

bool vfo_s::set_demod(const c_def::v_union & v)
{
    d_demod = Modulations::idx(int(v));
    return true;
}

void vfo_s::set_index(int index)
{
    d_index = index;
}

void vfo_s::set_autostart(bool v)
{
    d_locked = v;
}

bool vfo_s::set_freq_lock(const c_def::v_union & v)
{
    d_locked = v;
    return true;
}

bool vfo_s::set_sql_level(const c_def::v_union & v)
{
    d_level_db = v;
    return true;
}

bool vfo_s::set_sql_alpha(const c_def::v_union & v)
{
    d_alpha = v;
    return true;
}

bool vfo_s::get_audio_rec(c_def::v_union &) const
{
    return true;
}

bool vfo_s::get_audio_rec_filename(c_def::v_union &) const
{
    return true;
}

bool vfo_s::set_agc_on(const c_def::v_union & v)
{
    d_agc_on = v;
    return true;
}

bool vfo_s::set_agc_target_level(const c_def::v_union & v)
{
    d_agc_target_level = v;
    c_def::v_union tmp;
    get_agc_target_level_label(tmp);
    changed_value(C_AGC_TARGET_LABEL, d_index, tmp);
    return true;
}

bool vfo_s::get_agc_target_level_label(c_def::v_union & v) const
{
    std::stringstream tmp;
    tmp<<std::fixed<<d_agc_target_level;
    v=tmp.str();
    return true;
}

bool vfo_s::set_agc_manual_gain(const c_def::v_union & v)
{
    d_agc_manual_gain = v;
    c_def::v_union tmp;
    get_agc_manual_gain_label(tmp);
    changed_value(C_AGC_MAN_GAIN_LABEL, d_index, tmp);
    return true;
}

bool vfo_s::get_agc_manual_gain_label(c_def::v_union & v) const
{
    std::stringstream tmp;
    tmp<<std::fixed<<std::setprecision(1)<<d_agc_manual_gain;
    v=tmp.str();
    return true;
}

bool vfo_s::set_agc_max_gain(const c_def::v_union & v)
{
    d_agc_max_gain = v;
    c_def::v_union tmp;
    get_agc_max_gain_label(tmp);
    changed_value(C_AGC_MAX_GAIN_LABEL, d_index, tmp);
    return true;
}

bool vfo_s::get_agc_max_gain_label(c_def::v_union & v) const
{
    std::stringstream tmp;
    tmp<<std::fixed<<d_agc_max_gain;
    v=tmp.str();
    return true;
}

bool vfo_s::set_agc_attack(const c_def::v_union & v)
{
    d_agc_attack_ms = v;
    c_def::v_union tmp;
    get_agc_attack_label(tmp);
    changed_value(C_AGC_ATTACK_LABEL, d_index, tmp);
    return true;
}

bool vfo_s::get_agc_attack_label(c_def::v_union & v) const
{
    std::stringstream tmp;
    tmp<<std::fixed<<d_agc_attack_ms;
    v=tmp.str();
    return true;
}

bool vfo_s::set_agc_decay(const c_def::v_union & v)
{
    d_agc_decay_ms = v;
    c_def::v_union tmp;
    get_agc_decay_label(tmp);
    changed_value(C_AGC_DECAY_LABEL, d_index, tmp);
    return true;
}

bool vfo_s::get_agc_decay_label(c_def::v_union & v) const
{
    std::stringstream tmp;
    tmp<<std::fixed<<d_agc_decay_ms;
    v=tmp.str();
    return true;
}

bool vfo_s::set_agc_hang(const c_def::v_union & v)
{
    d_agc_hang_ms = v;
    c_def::v_union tmp;
    get_agc_hang_label(tmp);
    changed_value(C_AGC_HANG_LABEL, d_index, tmp);
    return true;
}

bool vfo_s::get_agc_hang_label(c_def::v_union & v) const
{
    std::stringstream tmp;
    tmp<<std::fixed<<d_agc_hang_ms;
    v=tmp.str();
    return true;
}

bool vfo_s::set_agc_panning(const c_def::v_union & v)
{
    d_agc_panning = v;
    c_def::v_union tmp;
    get_agc_panning_label(tmp);
    changed_value(C_AGC_PANNING_LABEL, d_index, tmp);
    return true;
}

bool vfo_s::get_agc_panning_label(c_def::v_union & v) const
{
    std::stringstream tmp;
    tmp<<std::fixed<<d_agc_panning;
    v=tmp.str();
    return true;
}

bool vfo_s::set_agc_panning_auto(const c_def::v_union & v)
{
    d_agc_panning_auto = v;
    return true;
}

bool vfo_s::set_agc_mute(const c_def::v_union & v)
{
    d_agc_mute = v;
    return true;
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

bool vfo_s::set_nb1_on(const c_def::v_union & v)
{
    d_nb_on[0] = v;
    return true;
}

bool vfo_s::set_nb2_on(const c_def::v_union & v)
{
    d_nb_on[1] = v;
    return true;
}

bool vfo_s::set_nb3_on(const c_def::v_union & v)
{
    d_nb_on[2] = v;
    return true;
}

bool vfo_s::set_nb1_threshold(const c_def::v_union & v)
{
    d_nb_threshold[0] = v;
    return true;
}

bool vfo_s::set_nb2_threshold(const c_def::v_union & v)
{
    d_nb_threshold[1] = v;
    return true;
}

bool vfo_s::set_nb3_gain(const c_def::v_union & v)
{
    d_nb_threshold[2] = v;
    return true;
}

bool vfo_s::set_audio_rec_dir(const c_def::v_union & v)
{
    d_rec_dir = std::string(v);
    return true;
}

bool vfo_s::set_audio_rec_sql_triggered(const c_def::v_union & v)
{
    d_rec_sql_triggered = v;
    return true;
}

bool vfo_s::set_audio_rec_min_time(const c_def::v_union & v)
{
    d_rec_min_time = v;
    return true;
}

bool vfo_s::set_audio_rec_max_gap(const c_def::v_union & v)
{
    d_rec_max_gap = v;
    return true;
}

bool vfo_s::set_audio_rec(const c_def::v_union & v)
{
    return true;
}

/* UDP streaming */
bool vfo_s::set_udp_host(const c_def::v_union & v)
{
    d_udp_host = std::string(v);
    return true;
}

bool vfo_s::set_udp_port(const c_def::v_union & v)
{
    d_udp_port = v;
    return true;
}

bool vfo_s::set_udp_stereo(const c_def::v_union & v)
{
    d_udp_stereo = v;
    return true;
}

bool vfo_s::set_udp_streaming(const c_def::v_union & v)
{
    d_udp_streaming = v;
    return true;
}

bool vfo_s::set_audio_dev(const c_def::v_union & v)
{
    d_audio_dev = std::string(v);
    return true;
}

bool vfo_s::set_dedicated_audio_sink(const c_def::v_union & v)
{
    d_dedicated_audio_sink = v;
    return true;
}

void vfo_s::restore_settings(vfo_s& from, bool force)
{
    c_def::v_union v(0);

    from.get_freq_lock(v);set_freq_lock(v);
    from.get_demod(v);set_demod(v);
    from.get_sql_level(v);set_sql_level(v);
    from.get_sql_alpha(v);set_sql_alpha(v);

    from.get_filter_low(v);set_filter_low(v);
    from.get_filter_high(v);set_filter_high(v);
    from.get_filter_shape(v);set_filter_shape(v);

    from.get_udp_host(v);
    set_udp_host(v);
    from.get_udp_port(v);
    set_udp_port(v);
    from.get_udp_stereo(v);
    set_udp_stereo(v);
    from.get_udp_streaming(v);
    set_udp_streaming(v);

    from.get_audio_rec_dir(v);
    if(force || !(v == ""))
        set_audio_rec_dir(v);
    from.get_audio_rec_min_time(v);
    if(force || (int(v) > 0))
        set_audio_rec_min_time(v);
    from.get_audio_rec_max_gap(v);
    if(force || (int(v) > 0))
        set_audio_rec_max_gap(v);
    from.get_audio_rec_sql_triggered(v);
    set_audio_rec_sql_triggered(v);

    from.get_audio_dev(v);
    set_audio_dev(v);
    from.get_dedicated_audio_sink(v);
    set_dedicated_audio_sink(v);

    from.get_agc_mute(v);set_agc_mute(v);

    from.get_agc_on(v);set_agc_on(v);
    from.get_agc_manual_gain(v);set_agc_manual_gain(v);
    from.get_agc_max_gain(v);set_agc_max_gain(v);
    from.get_agc_target_level(v);set_agc_target_level(v);
    from.get_agc_attack(v);set_agc_attack(v);
    from.get_agc_decay(v);set_agc_decay(v);
    from.get_agc_hang(v);set_agc_hang(v);
    from.get_agc_panning(v);set_agc_panning(v);
    from.get_agc_panning_auto(v);set_agc_panning_auto(v);

    from.get_nb1_on(v);set_nb1_on(v);
    from.get_nb2_on(v);set_nb2_on(v);
    from.get_nb3_on(v);set_nb3_on(v);
    from.get_nb1_threshold(v);set_nb1_threshold(v);
    from.get_nb2_threshold(v);set_nb2_threshold(v);
    from.get_nb3_gain(v);set_nb3_gain(v);

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

    setters[C_FREQ_LOCK]=&vfo_s::set_freq_lock;
    getters[C_FREQ_LOCK]=&vfo_s::get_freq_lock;

    setters[C_FILTER_SHAPE]=&vfo_s::set_filter_shape;
    getters[C_FILTER_SHAPE]=&vfo_s::get_filter_shape;
    setters[C_FILTER_LO]=&vfo_s::set_filter_low;
    getters[C_FILTER_LO]=&vfo_s::get_filter_low;
    setters[C_FILTER_HI]=&vfo_s::set_filter_high;
    getters[C_FILTER_HI]=&vfo_s::get_filter_high;
    setters[C_MODE]=&vfo_s::set_demod;
    getters[C_MODE]=&vfo_s::get_demod;
    //Squelch
    setters[C_SQUELCH_LEVEL]=&vfo_s::set_sql_level;
    getters[C_SQUELCH_LEVEL]=&vfo_s::get_sql_level;
    #if 0
    // Unimplemented
    setters[C_SQL_ALPHA]=&vfo_s::set_sql_alpha;
    getters[C_SQL_ALPHA]=&vfo_s::get_sql_alpha;
    #endif
    //AGC mute
    setters[C_AGC_MUTE]=&vfo_s::set_agc_mute;
    getters[C_AGC_MUTE]=&vfo_s::get_agc_mute;
    //Dedicated audio sink
    setters[C_AUDIO_DEDICATED_ON]=&vfo_s::set_dedicated_audio_sink;
    getters[C_AUDIO_DEDICATED_ON]=&vfo_s::get_dedicated_audio_sink;
    setters[C_AUDIO_DEDICATED_DEV]=&vfo_s::set_audio_dev;
    getters[C_AUDIO_DEDICATED_DEV]=&vfo_s::get_audio_dev;
    //Audio streaming parameters
    setters[C_AUDIO_UDP_HOST]=&vfo_s::set_udp_host;
    getters[C_AUDIO_UDP_HOST]=&vfo_s::get_udp_host;
    setters[C_AUDIO_UDP_PORT]=&vfo_s::set_udp_port;
    getters[C_AUDIO_UDP_PORT]=&vfo_s::get_udp_port;
    setters[C_AUDIO_UDP_STEREO]=&vfo_s::set_udp_stereo;
    getters[C_AUDIO_UDP_STEREO]=&vfo_s::get_udp_stereo;
    setters[C_AUDIO_UDP_STREAMING]=&vfo_s::set_udp_streaming;
    getters[C_AUDIO_UDP_STREAMING]=&vfo_s::get_udp_streaming;
    //Audio recording parameters
    setters[C_AUDIO_REC_DIR]=&vfo_s::set_audio_rec_dir;
    getters[C_AUDIO_REC_DIR]=&vfo_s::get_audio_rec_dir;
    setters[C_AUDIO_REC_SQUELCH_TRIGGERED]=&vfo_s::set_audio_rec_sql_triggered;
    getters[C_AUDIO_REC_SQUELCH_TRIGGERED]=&vfo_s::get_audio_rec_sql_triggered;
    setters[C_AUDIO_REC_MIN_TIME]=&vfo_s::set_audio_rec_min_time;
    getters[C_AUDIO_REC_MIN_TIME]=&vfo_s::get_audio_rec_min_time;
    setters[C_AUDIO_REC_MAX_GAP]=&vfo_s::set_audio_rec_max_gap;
    getters[C_AUDIO_REC_MAX_GAP]=&vfo_s::get_audio_rec_max_gap;
    setters[C_AUDIO_REC]=&vfo_s::set_audio_rec;
    getters[C_AUDIO_REC]=&vfo_s::get_audio_rec;
    getters[C_AUDIO_REC_FILENAME]=&vfo_s::get_audio_rec_filename;
    // AGC parameters
    setters[C_AGC_ON]=&vfo_s::set_agc_on;
    getters[C_AGC_ON]=&vfo_s::get_agc_on;
    setters[C_AGC_MAN_GAIN]=&vfo_s::set_agc_manual_gain;
    getters[C_AGC_MAN_GAIN]=&vfo_s::get_agc_manual_gain;
    getters[C_AGC_MAN_GAIN_LABEL]=&vfo_s::get_agc_manual_gain_label;
    setters[C_AGC_MAX_GAIN]=&vfo_s::set_agc_max_gain;
    getters[C_AGC_MAX_GAIN]=&vfo_s::get_agc_max_gain;
    getters[C_AGC_MAX_GAIN_LABEL]=&vfo_s::get_agc_max_gain_label;
    setters[C_AGC_TARGET]=&vfo_s::set_agc_target_level;
    getters[C_AGC_TARGET]=&vfo_s::get_agc_target_level;
    getters[C_AGC_TARGET_LABEL]=&vfo_s::get_agc_target_level_label;
    setters[C_AGC_ATTACK]=&vfo_s::set_agc_attack;
    getters[C_AGC_ATTACK]=&vfo_s::get_agc_attack;
    getters[C_AGC_ATTACK_LABEL]=&vfo_s::get_agc_attack_label;
    setters[C_AGC_DECAY]=&vfo_s::set_agc_decay;
    getters[C_AGC_DECAY]=&vfo_s::get_agc_decay;
    getters[C_AGC_DECAY_LABEL]=&vfo_s::get_agc_decay_label;
    setters[C_AGC_HANG]=&vfo_s::set_agc_hang;
    getters[C_AGC_HANG]=&vfo_s::get_agc_hang;
    getters[C_AGC_HANG_LABEL]=&vfo_s::get_agc_hang_label;
    setters[C_AGC_PANNING]=&vfo_s::set_agc_panning;
    getters[C_AGC_PANNING]=&vfo_s::get_agc_panning;
    getters[C_AGC_PANNING_LABEL]=&vfo_s::get_agc_panning_label;
    setters[C_AGC_PANNING_AUTO]=&vfo_s::set_agc_panning_auto;
    getters[C_AGC_PANNING_AUTO]=&vfo_s::get_agc_panning_auto;
    // NB parameters
    setters[C_NB1_ON]=&vfo_s::set_nb1_on;
    getters[C_NB1_ON]=&vfo_s::get_nb1_on;
    setters[C_NB2_ON]=&vfo_s::set_nb2_on;
    getters[C_NB2_ON]=&vfo_s::get_nb2_on;
    setters[C_NB3_ON]=&vfo_s::set_nb3_on;
    getters[C_NB3_ON]=&vfo_s::get_nb3_on;
    setters[C_NB1_THR]=&vfo_s::set_nb1_threshold;
    getters[C_NB1_THR]=&vfo_s::get_nb1_threshold;
    setters[C_NB2_THR]=&vfo_s::set_nb2_threshold;
    getters[C_NB2_THR]=&vfo_s::get_nb2_threshold;
    setters[C_NB3_GAIN]=&vfo_s::set_nb3_gain;
    getters[C_NB3_GAIN]=&vfo_s::get_nb3_gain;
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
