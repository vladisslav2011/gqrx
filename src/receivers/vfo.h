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
#ifndef VFO_H
#define VFO_H

#if GNURADIO_VERSION < 0x030900
    #include <boost/shared_ptr.hpp>
#endif

#include "receivers/defines.h"
#include "receivers/modulations.h"
#include "applications/gqrx/dcontrols.h"


class vfo_s;
typedef class vfo_s: public conf_dispatchers<vfo_s>
{
public:
#if GNURADIO_VERSION < 0x030900
    typedef boost::shared_ptr<vfo_s> sptr;
#else
    typedef std::shared_ptr<vfo_s> sptr;
#endif
    struct comp;
    typedef std::set<sptr, comp> set;
public:
    static sptr make()
    {
        return sptr(new vfo_s());
    }

    vfo_s():
        d_offset(0),
        d_filter_low(-5000),
        d_filter_high(5000),
        d_filter_tw(100),
        d_filter_shape(Modulations::FILTER_SHAPE_NORMAL),
        d_demod(Modulations::MODE_OFF),
        d_index(-1),
        d_locked(false),
        d_level_db(-150),
        d_alpha(0.001),
        d_agc_on(true),
        d_agc_target_level(0),
        d_agc_manual_gain(0),
        d_agc_max_gain(100),
        d_agc_attack_ms(20),
        d_agc_decay_ms(500),
        d_agc_hang_ms(0),
        d_agc_panning(0),
        d_agc_panning_auto(false),
        d_agc_mute(false),
        d_cw_offset(700),
        d_fm_maxdev(2500),
        d_fm_deemph(75),
        d_fmpll_damping_factor(0.7),
        d_fmpll_bw(0.027),
        d_fmpll_subtone_filter(false),
        d_subtone_filter(false),
        d_am_dcr(true),
        d_amsync_dcr(true),
        d_pll_bw(0.01),
        d_wfm_deemph(50.0),
        d_rec_dir(""),
        d_rec_sql_triggered(false),
        d_rec_min_time(0),
        d_rec_max_gap(0),
        d_udp_host("127.0.0.1"),
        d_udp_port(7355),
        d_udp_stereo(false),
        d_udp_streaming(false),
        d_dedicated_audio_sink(false),
        d_audio_dev(""),
        d_rds_on(false),
        d_testval(0)
    {
        for (int k = 0; k < RECEIVER_NB_COUNT; k++)
        {
            d_nb_on[k] = false;
            d_nb_threshold[k] = 2;
        }
    }

    virtual ~vfo_s()
    {
    }

    //getters
    inline int get_offset() const { return d_offset; }
    /* Filter parameter */
    inline int get_filter_low() const { return d_filter_low;}
    inline int get_filter_high() const { return d_filter_high;}
    bool         get_filter_low(c_def::v_union & v) const { v=d_filter_low; return true; }
    bool         get_filter_high(c_def::v_union & v) const { v=d_filter_high; return true; }
    bool         get_filter_shape(c_def::v_union & v) const { v=d_filter_shape; return true; }
    inline void get_filter(int &low, int &high, Modulations::filter_shape &shape) const
    {
        low = d_filter_low;
        high = d_filter_high;
        shape = d_filter_shape;
    }

    Modulations::idx get_demod() const { return d_demod; }

    bool         get_demod(c_def::v_union & v) const { v=d_demod; return true; }
    inline int get_index() const { return d_index; }
    bool         get_autostart() const { return d_locked; }
    bool         get_freq_lock() const { return d_locked; }
    bool         get_freq_lock(c_def::v_union & v) const { v=d_locked; return true; }
    /* Squelch parameter */
    bool         get_sql_level(c_def::v_union & v) const { v=d_level_db; return true; }
    bool         get_sql_alpha(c_def::v_union & v) const { v=d_alpha; return true; }
    /* AGC */
    bool         get_agc_on(c_def::v_union & v) const { v=d_agc_on; return true; }
    bool         get_agc_target_level(c_def::v_union & v) const { v=d_agc_target_level; return true; }
    bool         get_agc_target_level_label(c_def::v_union & v) const;
    bool         get_agc_manual_gain(c_def::v_union & v) const { v=d_agc_manual_gain; return true; }
    bool         get_agc_manual_gain_label(c_def::v_union & v) const;
    bool         get_agc_max_gain(c_def::v_union & v) const { v=d_agc_max_gain; return true; }
    bool         get_agc_max_gain_label(c_def::v_union & v) const;
    bool         get_agc_attack(c_def::v_union & v) const { v=d_agc_attack_ms; return true; }
    bool         get_agc_attack_label(c_def::v_union & v) const;
    bool         get_agc_decay(c_def::v_union & v) const { v=d_agc_decay_ms; return true; }
    bool         get_agc_decay_label(c_def::v_union & v) const;
    bool         get_agc_hang(c_def::v_union & v) const { v=d_agc_hang_ms; return true; }
    bool         get_agc_hang_label(c_def::v_union & v) const;
    bool         get_agc_panning(c_def::v_union & v) const { v=d_agc_panning; return true; }
    bool         get_agc_panning_label(c_def::v_union & v) const;
    bool         get_agc_panning_auto(c_def::v_union & v) const { v=d_agc_panning_auto; return true; }
    bool         get_agc_mute(c_def::v_union & v) const { v=d_agc_mute; return true; }
    /* CW parameters */
    bool         get_cw_offset(c_def::v_union & v) const { v=d_cw_offset; return true; }
    /* FM parameters */
    bool         get_fm_maxdev(c_def::v_union & v) const { v=d_fm_maxdev; return true; }
    bool         get_fm_deemph(c_def::v_union & v) const { v=d_fm_deemph; return true; }
    bool         get_subtone_filter(c_def::v_union & v) const { v=d_subtone_filter;  return true; }
    /* AM parameters */
    bool         get_am_dcr(c_def::v_union & v) const { v=d_am_dcr; return true; }
    /* AM-Sync parameters */
    bool         get_amsync_dcr(c_def::v_union & v) const { v=d_amsync_dcr; return true; }
    bool         get_amsync_pll_bw(c_def::v_union & v) const { v=d_pll_bw; return true; }
    /* NFM PLL parameters */
    bool         get_pll_bw(c_def::v_union & v) const { v=d_fmpll_bw; return true; }
    bool         get_fmpll_damping_factor(c_def::v_union & v) const { v=d_fmpll_damping_factor; return true; }
    /* WFM parameters */
    bool         get_wfm_deemph(c_def::v_union & v) const { v=d_wfm_deemph; return true; }
    /* Noise blanker */
    bool         get_nb1_on(c_def::v_union & v) const { v=d_nb_on[0]; return true; }
    bool         get_nb2_on(c_def::v_union & v) const { v=d_nb_on[1]; return true; }
    bool         get_nb3_on(c_def::v_union & v) const { v=d_nb_on[2]; return true; }
    bool         get_nb1_threshold(c_def::v_union & v) const { v=d_nb_threshold[0]; return true; }
    bool         get_nb2_threshold(c_def::v_union & v) const { v=d_nb_threshold[1]; return true; }
    bool         get_nb3_gain(c_def::v_union & v) const { v=d_nb_threshold[2]; return true; }
    /* Audio recorder */
    bool         get_audio_rec_dir(c_def::v_union & v) const { v=d_rec_dir; return true; }
    bool         get_audio_rec_sql_triggered(c_def::v_union & v) const { v=d_rec_sql_triggered; return true; }
    bool         get_audio_rec_min_time(c_def::v_union & v) const { v=d_rec_min_time; return true; }
    bool         get_audio_rec_max_gap(c_def::v_union & v) const { v=d_rec_max_gap; return true; }
    virtual bool get_audio_rec(c_def::v_union &) const;
    virtual bool get_audio_rec_filename(c_def::v_union &) const;
    /* UDP streaming */
    bool         get_udp_host(c_def::v_union & v) const { v=d_udp_host; return true; }
    bool         get_udp_port(c_def::v_union & v) const { v=d_udp_port; return true; }
    bool         get_udp_stereo(c_def::v_union & v) const { v=d_udp_stereo; return true; }
    bool         get_udp_streaming(c_def::v_union & v) const { v=d_udp_streaming; return true; }
    /* Dedicated audio sink */
    bool         get_audio_dev(c_def::v_union & v) const { v=d_audio_dev; return true; }
    bool         get_dedicated_audio_sink(c_def::v_union & v) const { v=d_dedicated_audio_sink; return true; }

    /* GUI */
    bool         get_fft_center(c_def::v_union & v) const { v=d_fft_center; return true; }
    bool         get_fft_zoom(c_def::v_union & v) const { v=d_fft_zoom; return true; }

    //setters
    virtual bool set_offset(int offset, bool locked);
    /* Filter parameter */
    virtual bool set_filter_low(const c_def::v_union &);
    virtual bool set_filter_high(const c_def::v_union &);
    virtual bool set_filter_shape(const c_def::v_union &);
    virtual void set_filter(int low, int high, Modulations::filter_shape shape);
    virtual void filter_adjust();

    virtual bool set_demod(const c_def::v_union &);
    virtual void set_index(int index);
    virtual void set_autostart(bool v);
    virtual bool set_freq_lock(const c_def::v_union &);
    /* Squelch parameter */
    virtual bool  set_sql_level(const c_def::v_union &);
    virtual bool  set_sql_alpha(const c_def::v_union &);
    /* AGC */
    virtual bool  set_agc_on(const c_def::v_union &);
    virtual bool  set_agc_target_level(const c_def::v_union &);
    virtual bool  set_agc_manual_gain(const c_def::v_union &);
    virtual bool  set_agc_max_gain(const c_def::v_union &);
    virtual bool  set_agc_attack(const c_def::v_union &);
    virtual bool  set_agc_decay(const c_def::v_union &);
    virtual bool  set_agc_hang(const c_def::v_union &);
    virtual bool  set_agc_panning(const c_def::v_union &);
    virtual bool  set_agc_panning_auto(const c_def::v_union &);
    virtual bool  set_agc_mute(const c_def::v_union &);
    /* CW parameters */
    virtual bool set_cw_offset(const c_def::v_union &);
    /* FM parameters */
    virtual bool  set_fm_maxdev(const c_def::v_union &);
    virtual bool  set_fm_deemph(const c_def::v_union &);
    virtual bool  set_subtone_filter(const c_def::v_union &);
    /* FM PLL parameters */
    virtual bool  set_fmpll_damping_factor(const c_def::v_union &);
    virtual bool  set_pll_bw(const c_def::v_union &);
    /* AM parameters */
    virtual bool  set_am_dcr(const c_def::v_union &);
    /* AM-Sync parameters */
    virtual bool  set_amsync_dcr(const c_def::v_union &);
    virtual bool  set_amsync_pll_bw(const c_def::v_union &);
    /* WFM parameters */
    virtual bool  set_wfm_deemph(const c_def::v_union &);
    /* Noise blanker */
    virtual bool set_nb1_on(const c_def::v_union &);
    virtual bool set_nb2_on(const c_def::v_union &);
    virtual bool set_nb3_on(const c_def::v_union &);
    virtual bool set_nb1_threshold(const c_def::v_union &);
    virtual bool set_nb2_threshold(const c_def::v_union &);
    virtual bool set_nb3_gain(const c_def::v_union &);

    /* Audio recorder */
    virtual bool set_audio_rec_dir(const c_def::v_union &);
    virtual bool set_audio_rec_sql_triggered(const c_def::v_union &);
    virtual bool set_audio_rec_min_time(const c_def::v_union &);
    virtual bool set_audio_rec_max_gap(const c_def::v_union &);
    virtual bool set_audio_rec(const c_def::v_union &);

    /* UDP streaming */
    virtual bool set_udp_host(const c_def::v_union &);
    virtual bool set_udp_port(const c_def::v_union &);
    virtual bool set_udp_stereo(const c_def::v_union &);
    virtual bool set_udp_streaming(const c_def::v_union &);

    /* Dedicated audio sink */
    virtual bool set_audio_dev(const c_def::v_union &);
    virtual bool set_dedicated_audio_sink(const c_def::v_union &);

    virtual void restore_settings(vfo_s& from, bool force = true);

    static int conf_initializer();
    virtual bool set_test(const c_def::v_union &);
    virtual bool get_test(c_def::v_union &) const;

    virtual bool set_rds_on(const c_def::v_union &);
    virtual bool get_rds_on(c_def::v_union &) const;
    virtual bool get_rds_pi(c_def::v_union &) const;
    virtual bool get_rds_ps(c_def::v_union &) const;
    virtual bool get_rds_pty(c_def::v_union &) const;
    virtual bool get_rds_flagstring(c_def::v_union &to) const;
    virtual bool get_rds_rt(c_def::v_union &) const;
    virtual bool get_rds_clock(c_def::v_union &) const;
    virtual bool get_rds_af(c_def::v_union &) const;
    /* GUI */
    virtual bool set_fft_center(const c_def::v_union & v);
    virtual bool set_fft_zoom(const c_def::v_union & v);

public:
    struct comp
    {
        inline bool operator()(const sptr lhs, const sptr rhs) const
        {
            const vfo_s *a = lhs.get();
            const vfo_s *b = rhs.get();
            return (a->d_offset == b->d_offset) ? (a->d_index < b->d_index) : (a->d_offset < b->d_offset);
        }
    };

protected:
    int              d_offset;
    int              d_filter_low;
    int              d_filter_high;
    int              d_filter_tw;
    Modulations::filter_shape d_filter_shape;
    Modulations::idx d_demod;
    int              d_index;
    bool             d_locked;

    double           d_level_db;
    double           d_alpha;

    bool             d_agc_on;
    int              d_agc_target_level;
    float            d_agc_manual_gain;
    int              d_agc_max_gain;
    int              d_agc_attack_ms;
    int              d_agc_decay_ms;
    int              d_agc_hang_ms;
    int              d_agc_panning;
    int              d_agc_panning_auto;
    bool             d_agc_mute;

    int              d_cw_offset;        /*!< CW offset */

    float            d_fm_maxdev;
    double           d_fm_deemph;
    float            d_fmpll_damping_factor;
    float            d_fmpll_bw;
    bool             d_fmpll_subtone_filter;
    bool             d_subtone_filter;
    bool             d_am_dcr;
    bool             d_amsync_dcr;
    float            d_pll_bw;
    float            d_wfm_deemph;

    bool             d_nb_on[RECEIVER_NB_COUNT];
    float            d_nb_threshold[RECEIVER_NB_COUNT];

    std::string      d_rec_dir;
    bool             d_rec_sql_triggered;
    int              d_rec_min_time;
    int              d_rec_max_gap;

    std::string      d_udp_host;
    int              d_udp_port;
    bool             d_udp_stereo;
    bool             d_udp_streaming;

    bool             d_dedicated_audio_sink;
    std::string      d_audio_dev;

    bool             d_rds_on;
    int              d_testval;

    /* GUI */
    double           d_fft_zoom{1.};
    int              d_fft_center{0};
} vfo;


#endif //VFO_H