/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2012 Alexandru Csete OZ9AEC.
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
#include <gnuradio/io_signature.h>
#include "receivers/receiver_base.h"
#include <QDebug>
#include <QDateTime>
#include <QDir>
#include <functional>


static const int MIN_IN = 1;  /* Minimum number of input streams. */
static const int MAX_IN = 1;  /* Maximum number of input streams. */
static const int MIN_OUT = 2; /* Minimum number of output streams. */
static const int MAX_OUT = 2; /* Maximum number of output streams. */

receiver_base_cf::receiver_base_cf(std::string src_name, float pref_quad_rate, double quad_rate, int audio_rate)
    : gr::hier_block2 (src_name,
                      gr::io_signature::make (MIN_IN, MAX_IN, sizeof(gr_complex)),
                      gr::io_signature::make (MIN_OUT, MAX_OUT, sizeof(float))),
      d_connected(false),
      d_decim_rate(quad_rate),
      d_quad_rate(0),
      d_cw_offset(700),
      d_ddc_decim(1),
      d_audio_rate(audio_rate),
      d_center_freq(145500000.0),
      d_filter_tw(100),
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
      d_fm_maxdev(2500),
      d_fm_deemph(75),
      d_am_dcr(true),
      d_amsync_dcr(true),
      d_amsync_pll_bw(0.01),
      d_pref_quad_rate(pref_quad_rate)
{
    d_vfo = vfo::make(0, -5000, 5000, Modulations::MODE_OFF, -1, false);
    for (int k = 0; k < RECEIVER_NB_COUNT; k++)
    {
        d_nb_on[k] = false;
        d_nb_threshold[k] = 2;
    }
    d_ddc_decim = std::max(1, (int)(d_decim_rate / TARGET_QUAD_RATE));
    d_quad_rate = d_decim_rate / d_ddc_decim;
    ddc = make_downconverter_cc(d_ddc_decim, 0.0, d_decim_rate);
    connect(self(), 0, ddc, 0);

    iq_resamp = make_resampler_cc(d_pref_quad_rate/d_quad_rate);
    agc = make_rx_agc_2f(d_audio_rate, false, 0, 0, 100, 500, 500, 0, 0);
    sql = make_rx_sql_cc(-150.0, 0.001);
    meter = make_rx_meter_c(d_pref_quad_rate);
    wav_sink = wavfile_sink_gqrx::make(0, 2, (unsigned int) d_audio_rate,
                                       wavfile_sink_gqrx::FORMAT_WAV,
                                       wavfile_sink_gqrx::FORMAT_PCM_16);
    connect(agc, 0, wav_sink, 0);
    connect(agc, 1, wav_sink, 1);
    wav_sink->set_rec_event_handler(std::bind(rec_event, this, std::placeholders::_1,
                                    std::placeholders::_2));
}

receiver_base_cf::~receiver_base_cf()
{
    //Prevent segfault
    if(wav_sink)
        wav_sink->set_rec_event_handler(nullptr);
}

void receiver_base_cf::set_index(int index)
{
    d_vfo->index = index;
}

int  receiver_base_cf::get_index()
{
    return d_vfo->index;
}

void receiver_base_cf::set_demod(Modulations::idx demod)
{
    if ((d_vfo->mode == Modulations::MODE_OFF) && (demod != Modulations::MODE_OFF))
    {
        qDebug() << "Changing RX quad rate:"  << d_decim_rate << "->" << d_quad_rate;
        lock();
        ddc->set_decim_and_samp_rate(d_ddc_decim, d_decim_rate);
        iq_resamp->set_rate(d_pref_quad_rate/d_quad_rate);
        unlock();
    }
    d_vfo->mode = demod;
}

Modulations::idx receiver_base_cf::get_demod()
{
    return d_vfo->mode;
}

void receiver_base_cf::set_quad_rate(double quad_rate)
{
    if (std::abs(d_decim_rate-quad_rate) > 0.5)
    {
        d_decim_rate = quad_rate;
        d_ddc_decim = std::max(1, (int)(d_decim_rate / TARGET_QUAD_RATE));
        d_quad_rate = d_decim_rate / d_ddc_decim;
        //avoid triggering https://github.com/gnuradio/gnuradio/issues/5436
        if (d_vfo->mode != Modulations::MODE_OFF)
        {
            qDebug() << "Changing RX quad rate:"  << d_decim_rate << "->" << d_quad_rate;
            lock();
            ddc->set_decim_and_samp_rate(d_ddc_decim, d_decim_rate);
            iq_resamp->set_rate(d_pref_quad_rate/d_quad_rate);
            unlock();
        }
    }
}

void receiver_base_cf::set_center_freq(double center_freq)
{
    d_center_freq = center_freq;
    wav_sink->set_center_freq(center_freq);
}

void receiver_base_cf::set_offset(double offset)
{
    d_vfo->offset = offset;
    ddc->set_center_freq(offset - d_cw_offset);
    wav_sink->set_offset(offset);
}

double receiver_base_cf::get_offset()
{
    return d_vfo->offset;
}

void receiver_base_cf::set_freq_lock(bool on)
{
    d_vfo->locked = on;
}

bool receiver_base_cf::get_freq_lock()
{
    return d_vfo->locked;
}

void receiver_base_cf::set_cw_offset(double offset)
{
    d_cw_offset = offset;
    ddc->set_center_freq(d_vfo->offset - d_cw_offset);
}

double receiver_base_cf::get_cw_offset()
{
    return d_cw_offset;
}

void receiver_base_cf::set_rec_dir(std::string dir)
{
    d_rec_dir = dir;
    wav_sink->set_rec_dir(dir);
}

void receiver_base_cf::set_audio_rec_sql_triggered(bool enabled)
{
    sql->set_impl(enabled ? rx_sql_cc::SQL_PWR : rx_sql_cc::SQL_SIMPLE);
    wav_sink->set_sql_triggered(enabled);
}

void receiver_base_cf::set_audio_rec_min_time(const int time_ms)
{
    wav_sink->set_rec_min_time(time_ms);
}

void receiver_base_cf::set_audio_rec_max_gap(const int time_ms)
{
    wav_sink->set_rec_max_gap(time_ms);
}

float receiver_base_cf::get_signal_level()
{
    return meter->get_level_db();
}

void receiver_base_cf::set_filter(double low, double high, double tw)
{
    d_vfo->low = low;
    d_vfo->high = high;
    d_filter_tw = tw;
}

void receiver_base_cf::get_filter(double &low, double &high, double &tw)
{
    low = d_vfo->low;
    high = d_vfo->high;
    tw = d_filter_tw;
}

bool receiver_base_cf::has_nb()
{
    return false;
}

void receiver_base_cf::set_nb_on(int nbid, bool on)
{
    if (nbid - 1 < RECEIVER_NB_COUNT)
        d_nb_on[nbid - 1] = on;
}

bool receiver_base_cf::get_nb_on(int nbid)
{
    if (nbid - 1 < RECEIVER_NB_COUNT)
        return d_nb_on[nbid - 1];
    return false;
}

void receiver_base_cf::set_nb_threshold(int nbid, float threshold)
{
    if (nbid - 1 < RECEIVER_NB_COUNT)
        d_nb_threshold[nbid -1] = threshold;
}

float receiver_base_cf::get_nb_threshold(int nbid)
{
    if (nbid - 1 < RECEIVER_NB_COUNT)
        return d_nb_threshold[nbid -1];
    return 0.0;
}

void receiver_base_cf::set_sql_level(double level_db)
{
    sql->set_threshold(level_db);
    d_level_db = level_db;
}

double receiver_base_cf::get_sql_level()
{
    return d_level_db;
}

void receiver_base_cf::set_sql_alpha(double alpha)
{
    sql->set_alpha(alpha);
    d_alpha = alpha;
}

double receiver_base_cf::get_sql_alpha()
{
    return d_alpha;
}

void receiver_base_cf::set_agc_on(bool agc_on)
{
    agc->set_agc_on(agc_on);
    d_agc_on = agc_on;
}

bool receiver_base_cf::get_agc_on()
{
    return d_agc_on;
}

void receiver_base_cf::set_agc_target_level(int target_level)
{
    agc->set_target_level(target_level);
    d_agc_target_level = target_level;
}

int receiver_base_cf::get_agc_target_level()
{
    return d_agc_target_level;
}

void receiver_base_cf::set_agc_manual_gain(float gain)
{
    agc->set_manual_gain(gain);
    d_agc_manual_gain = gain;
}

float receiver_base_cf::get_agc_manual_gain()
{
    return d_agc_manual_gain;
}

void receiver_base_cf::set_agc_max_gain(int gain)
{
    agc->set_max_gain(gain);
    d_agc_max_gain = gain;
}

int receiver_base_cf::get_agc_max_gain()
{
    return d_agc_max_gain;
}

void receiver_base_cf::set_agc_attack(int attack_ms)
{
    agc->set_attack(attack_ms);
    d_agc_attack_ms = attack_ms;
}

int receiver_base_cf::get_agc_attack()
{
    return d_agc_attack_ms;
}

void receiver_base_cf::set_agc_decay(int decay_ms)
{
    agc->set_decay(decay_ms);
    d_agc_decay_ms = decay_ms;
}

int receiver_base_cf::get_agc_decay()
{
    return d_agc_decay_ms;
}

void receiver_base_cf::set_agc_hang(int hang_ms)
{
    agc->set_hang(hang_ms);
    d_agc_hang_ms = hang_ms;
}

int receiver_base_cf::get_agc_hang()
{
    return d_agc_hang_ms;
}

void receiver_base_cf::set_agc_panning(int panning)
{
    agc->set_panning(panning);
    d_agc_panning = panning;
}

int receiver_base_cf::get_agc_panning()
{
    return d_agc_panning;
}

void receiver_base_cf::set_agc_panning_auto(bool mode)
{
    //TODO: implement auto panning
    d_agc_panning_auto = mode;
}

bool receiver_base_cf::get_agc_panning_auto()
{
    return d_agc_panning_auto;
}


float receiver_base_cf::get_agc_gain()
{
    return agc->get_current_gain();
}

void receiver_base_cf::set_fm_maxdev(float maxdev_hz)
{
    d_fm_maxdev = maxdev_hz;
}

float receiver_base_cf::get_fm_maxdev()
{
    return d_fm_maxdev;
}

void receiver_base_cf::set_fm_deemph(double tau)
{
    d_fm_deemph = tau;
}

double receiver_base_cf::get_fm_deemph()
{
    return d_fm_deemph;
}

void receiver_base_cf::set_am_dcr(bool enabled)
{
    d_am_dcr = enabled;
}

bool receiver_base_cf::get_am_dcr()
{
    return d_am_dcr;
}

void receiver_base_cf::set_amsync_dcr(bool enabled)
{
    d_amsync_dcr = enabled;
}

bool receiver_base_cf::get_amsync_dcr()
{
    return d_amsync_dcr;
}

void receiver_base_cf::set_amsync_pll_bw(float pll_bw)
{
    d_amsync_pll_bw = pll_bw;
}

float receiver_base_cf::get_amsync_pll_bw()
{
    return d_amsync_pll_bw;
}

void receiver_base_cf::get_rds_data(std::string &outbuff, int &num)
{
        (void) outbuff;
        (void) num;
}

void receiver_base_cf::start_rds_decoder()
{
}

void receiver_base_cf::stop_rds_decoder()
{
}

void receiver_base_cf::reset_rds_parser()
{
}

bool receiver_base_cf::is_rds_decoder_active()
{
    return false;
}

int receiver_base_cf::start_audio_recording()
{
    return wav_sink->open_new();
}

bool receiver_base_cf::get_audio_recording()
{
    return wav_sink->is_active();
}

void receiver_base_cf::stop_audio_recording()
{
    wav_sink->close();
}

//FIXME Reimplement wavfile_sink correctly to make this work as expected
void receiver_base_cf::continue_audio_recording(receiver_base_cf_sptr from)
{
    if(from.get() == this)
        return;
    from->disconnect(from->agc, 0, from->wav_sink, 0);
    from->disconnect(from->agc, 1, from->wav_sink, 1);
    wav_sink = from->wav_sink;
    wav_sink->set_rec_event_handler(std::bind(rec_event, this, std::placeholders::_1,
                                    std::placeholders::_2));
    connect(agc, 0, wav_sink, 0);
    connect(agc, 1, wav_sink, 1);
    from->wav_sink.reset();
}

std::string receiver_base_cf::get_last_audio_filename()
{
    return d_audio_filename;
}

void receiver_base_cf::rec_event(receiver_base_cf * self, std::string filename, bool is_running)
{
    self->d_audio_filename = filename;
    if(self->d_rec_event)
        self->d_rec_event(self->d_vfo->index, filename, is_running);
}

void receiver_base_cf::restore_settings(receiver_base_cf_sptr from)
{
    set_offset(from->d_vfo->offset);
    set_center_freq(from->d_center_freq);
    set_filter(from->d_vfo->low, from->d_vfo->high, from->d_filter_tw);
    set_freq_lock(from->get_freq_lock());
    set_cw_offset(from->d_cw_offset);
    for (int k = 0; k < RECEIVER_NB_COUNT; k++)
    {
        set_nb_on(k + 1, from->d_nb_on[k]);
        set_nb_threshold(k + 1, from->d_nb_threshold[k]);
    }
    set_sql_level(from->d_level_db);
    set_sql_alpha(from->d_alpha);
    set_agc_on(from->d_agc_on);
    set_agc_target_level(from->d_agc_target_level);
    set_agc_manual_gain(from->d_agc_manual_gain);
    set_agc_max_gain(from->d_agc_max_gain);
    set_agc_attack(from->d_agc_attack_ms);
    set_agc_decay(from->d_agc_decay_ms);
    set_agc_hang(from->d_agc_hang_ms);

    set_fm_maxdev(from->d_fm_maxdev);
    set_fm_deemph(from->d_fm_deemph);
    set_am_dcr(from->d_am_dcr);
    set_amsync_dcr(from->d_amsync_dcr);
    set_amsync_pll_bw(from->d_amsync_pll_bw);

    set_audio_rec_sql_triggered(from->get_audio_rec_sql_triggered());
    set_audio_rec_min_time(from->get_audio_rec_min_time());
    set_audio_rec_max_gap(from->get_audio_rec_max_gap());
    set_rec_dir(from->get_rec_dir());
    set_agc_panning(from->get_agc_panning());
    set_agc_panning_auto(from->get_agc_panning_auto());
}