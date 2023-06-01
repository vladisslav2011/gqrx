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
#include <exception>

static const int MIN_IN = 1;  /* Minimum number of input streams. */
static const int MAX_IN = 1;  /* Maximum number of input streams. */
static const int MIN_OUT = 2; /* Minimum number of output streams. */
static const int MAX_OUT = 2; /* Maximum number of output streams. */

receiver_base_cf::receiver_base_cf(std::string src_name, float pref_quad_rate, double quad_rate, int audio_rate)
    : gr::hier_block2 (src_name,
                      gr::io_signature::make (MIN_IN, MAX_IN, sizeof(gr_complex)),
                      gr::io_signature::make (MIN_OUT, MAX_OUT, sizeof(float))),
      vfo_s(),
      d_connected(false),
      d_port(-1),
      d_decim_rate(quad_rate),
      d_quad_rate(0),
      d_ddc_decim(1),
      d_audio_rate(audio_rate),
      d_center_freq(145500000.0),
      d_pref_quad_rate(pref_quad_rate),
      d_audio_filename(""),
      d_udp_streaming(false)
{
    d_ddc_decim = std::max(1, (int)(d_decim_rate / TARGET_QUAD_RATE));
    d_quad_rate = d_decim_rate / d_ddc_decim;
    ddc = make_downconverter_cc(d_ddc_decim, 0.0, d_decim_rate);
    connect(self(), 0, ddc, 0);

    iq_resamp = make_resampler_cc(d_pref_quad_rate/d_quad_rate);
    agc = make_rx_agc_2f(d_audio_rate, d_agc_on, d_agc_target_level,
                         d_agc_manual_gain, d_agc_max_gain, d_agc_attack_ms,
                         d_agc_decay_ms, d_agc_hang_ms, d_agc_panning);
    sql = make_rx_sql_cc(d_level_db, d_alpha);
    meter = make_rx_meter_c(d_pref_quad_rate);
    wav_sink = wavfile_sink_gqrx::make(0, 2, (unsigned int) d_audio_rate,
                                       wavfile_sink_gqrx::FORMAT_WAV,
                                       wavfile_sink_gqrx::FORMAT_PCM_16);
    audio_udp_sink = make_udp_sink_f();
    audio_rnnoise =  make_rx_rnnoise_f(audio_rate);

    output = audio_rnnoise;
    connect(audio_rnnoise, 0, agc, 0);
    connect(audio_rnnoise, 1, agc, 1);
    connect(agc, 0, wav_sink, 0);
    connect(agc, 1, wav_sink, 1);
    connect(agc, 0, audio_udp_sink, 0);
    connect(agc, 1, audio_udp_sink, 1);
    connect(agc, 2, self(), 0);
    connect(agc, 3, self(), 1);
    wav_sink->set_rec_event_handler(std::bind(rec_event, this, std::placeholders::_1,
                                    std::placeholders::_2));
}

receiver_base_cf::~receiver_base_cf()
{
    //Prevent segfault
    if (wav_sink)
        wav_sink->set_rec_event_handler(nullptr);
}

void receiver_base_cf::set_demod(Modulations::idx demod)
{
    if ((get_demod() == Modulations::MODE_OFF) && (demod != Modulations::MODE_OFF))
    {
        qDebug() << "Changing RX quad rate:"  << d_decim_rate << "->" << d_quad_rate;
        lock();
        ddc->set_decim_and_samp_rate(d_ddc_decim, d_decim_rate);
        iq_resamp->set_rate(d_pref_quad_rate/d_quad_rate);
        unlock();
    }
    vfo_s::set_demod(demod);
}

void receiver_base_cf::set_quad_rate(double quad_rate)
{
    if (std::abs(d_decim_rate-quad_rate) > 0.5)
    {
        d_decim_rate = quad_rate;
        d_ddc_decim = std::max(1, (int)(d_decim_rate / TARGET_QUAD_RATE));
        d_quad_rate = d_decim_rate / d_ddc_decim;
        //avoid triggering https://github.com/gnuradio/gnuradio/issues/5436
        if (get_demod() != Modulations::MODE_OFF)
        {
            qDebug() << "Changing RX quad rate:"  << d_decim_rate << "->" << d_quad_rate;
            lock();
            ddc->set_decim_and_samp_rate(d_ddc_decim, d_decim_rate);
            iq_resamp->set_rate(d_pref_quad_rate/d_quad_rate);
            unlock();
        }
    }
}
void receiver_base_cf::set_audio_rate(int audio_rate)
{
    if(d_audio_rate != audio_rate)
    {
        d_audio_rate = audio_rate;
        agc->set_sample_rate(audio_rate);
        if(d_dedicated_audio_sink)
        {
            disconnect(agc, 0, audio_snk, 0);
            disconnect(agc, 1, audio_snk, 1);
            audio_snk.reset();
            audio_snk = create_audio_sink(d_audio_dev, d_audio_rate, "rx" + std::to_string(d_port));
            connect(agc, 0, audio_snk, 0);
            connect(agc, 1, audio_snk, 1);
        }
    }
}

void receiver_base_cf::set_center_freq(double center_freq)
{
    d_center_freq = center_freq;
    wav_sink->set_center_freq(center_freq);
}

void receiver_base_cf::set_offset(int offset)
{
    vfo_s::set_offset(offset);
    ddc->set_center_freq(offset);
    wav_sink->set_offset(offset);
}

void receiver_base_cf::set_port(int port)
{
    //lock();
    if(d_port != port)
    {
        if( !!audio_snk )
        {
            disconnect(agc, 0, audio_snk, 0);
            disconnect(agc, 1, audio_snk, 1);
            audio_snk.reset();
            disconnect(self(), 0, ddc, 0);
            connect(self(), 0, ddc, 0);
        }
        d_port = port;
        if( d_port != -1 && d_dedicated_audio_sink)
        {
            audio_snk = create_audio_sink(d_audio_dev, d_audio_rate, "rx" + std::to_string(d_port));
            connect(agc, 0, audio_snk, 0);
            connect(agc, 1, audio_snk, 1);
        }
    }
    //unlock();
}

bool receiver_base_cf::set_audio_rec_dir(const c_def::v_union & v)
{
    vfo_s::set_audio_rec_dir(v);
    wav_sink->set_rec_dir(v);
    return true;
}

bool receiver_base_cf::set_audio_rec_sql_triggered(const c_def::v_union & v)
{
    vfo_s::set_audio_rec_sql_triggered(v);
    sql->set_impl(bool(v) ? rx_sql_cc::SQL_PWR : rx_sql_cc::SQL_SIMPLE);
    wav_sink->set_sql_triggered(bool(v));
    return true;
}

bool receiver_base_cf::set_audio_rec_min_time(const c_def::v_union & v)
{
    vfo_s::set_audio_rec_min_time(v);
    wav_sink->set_rec_min_time(v);
    return true;
}

bool receiver_base_cf::set_audio_rec_max_gap(const c_def::v_union & v)
{
    vfo_s::set_audio_rec_max_gap(v);
    wav_sink->set_rec_max_gap(v);
    return true;
}

float receiver_base_cf::get_signal_level()
{
    return meter->get_level_db();
}

bool receiver_base_cf::has_nb()
{
    return false;
}

void receiver_base_cf::set_sql_level(double level_db)
{
    sql->set_threshold(level_db);
    vfo_s::set_sql_level(level_db);
}

void receiver_base_cf::set_sql_alpha(double alpha)
{
    sql->set_alpha(alpha);
    vfo_s::set_sql_alpha(alpha);
}

bool receiver_base_cf::set_agc_on(const c_def::v_union & v)
{
    vfo_s::set_agc_on(v);
    agc->set_agc_on(d_agc_on);
    return true;
}

bool receiver_base_cf::set_agc_target_level(const c_def::v_union & v)
{
    vfo_s::set_agc_target_level(v);
    agc->set_target_level(d_agc_target_level);
    return true;
}

bool receiver_base_cf::set_agc_manual_gain(const c_def::v_union & v)
{
    vfo_s::set_agc_manual_gain(v);
    agc->set_manual_gain(d_agc_manual_gain);
    return true;
}

bool receiver_base_cf::set_agc_max_gain(const c_def::v_union & v)
{
    vfo_s::set_agc_max_gain(v);
    agc->set_max_gain(d_agc_max_gain);
    return true;
}

bool receiver_base_cf::set_agc_attack(const c_def::v_union & v)
{
    vfo_s::set_agc_attack(v);
    agc->set_attack(d_agc_attack_ms);
    return true;
}

bool receiver_base_cf::set_agc_decay(const c_def::v_union & v)
{
    vfo_s::set_agc_decay(v);
    agc->set_decay(d_agc_decay_ms);
    return true;
}

bool receiver_base_cf::set_agc_hang(const c_def::v_union & v)
{
    vfo_s::set_agc_hang(v);
    agc->set_hang(d_agc_hang_ms);
    return true;
}

bool receiver_base_cf::set_agc_panning(const c_def::v_union & v)
{
    vfo_s::set_agc_panning(v);
    agc->set_panning(d_agc_panning);
    return true;
}

void receiver_base_cf::set_agc_mute(bool agc_mute)
{
    agc->set_mute(agc_mute);
    vfo_s::set_agc_mute(agc_mute);
}

float receiver_base_cf::get_agc_gain()
{
    return agc->get_current_gain();
}

/* UDP streaming */
bool receiver_base_cf::set_udp_host(const c_def::v_union & v)
{
    if(d_udp_host == std::string(v))
        return true;
    return vfo_s::set_udp_host(v);
}

bool receiver_base_cf::set_udp_port(const c_def::v_union & v)
{
    if(d_udp_port == int(v))
        return true;
    return vfo_s::set_udp_port(v);
}

bool receiver_base_cf::set_udp_stereo(const c_def::v_union & v)
{
    if(d_udp_stereo == bool(v))
        return true;
    return vfo_s::set_udp_stereo(v);
}

bool receiver_base_cf::set_udp_streaming(bool streaming)
{
    if(d_udp_streaming == streaming)
        return true;
    try{
        if(!d_udp_streaming && streaming)
            audio_udp_sink->start_streaming(d_udp_host, d_udp_port, d_udp_stereo);
        if(d_udp_streaming && !streaming)
            audio_udp_sink->stop_streaming();
    }catch(std::exception & e)
    {
        return false;
    }
    d_udp_streaming = streaming;
    return true;
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
    if (from.get() == this)
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
    if (self->d_rec_event)
    {
        self->d_rec_event(self->get_index(), filename, is_running);
        std::cerr<<"d_rec_event("<<self->get_index()<<","<<filename<<","<<is_running<<")\n";
    }
}

void receiver_base_cf::restore_settings(receiver_base_cf& from)
{
    vfo_s::restore_settings(from);

    set_center_freq(from.d_center_freq);
}

bool receiver_base_cf::set_dedicated_audio_sink(const c_def::v_union & v)
{
    if(d_dedicated_audio_sink == bool(v))
        return true;
    if(d_connected)
        lock();
    if( !!audio_snk )
    {
        disconnect(agc, 0, audio_snk, 0);
        disconnect(agc, 1, audio_snk, 1);
        audio_snk.reset();
        disconnect(self(), 0, ddc, 0);
        connect(self(), 0, ddc, 0);
    }
    d_dedicated_audio_sink = v;
    if( d_port != -1 && d_dedicated_audio_sink)
    {
        try
        {
            audio_snk = create_audio_sink(d_audio_dev, d_audio_rate, "rx" + std::to_string(d_port));
            connect(agc, 0, audio_snk, 0);
            connect(agc, 1, audio_snk, 1);
            changed_value(C_AUDIO_DEDICATED_ERROR, d_index, "");
        }catch(std::exception &e)
        {
            audio_snk.reset();
            d_dedicated_audio_sink = false;
            changed_value(C_AUDIO_DEDICATED_ON, d_index, d_dedicated_audio_sink);
            changed_value(C_AUDIO_DEDICATED_ERROR, d_index, e.what());
        }
    }
    if(d_connected)
        unlock();
    return true;
}
