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


static const int MIN_IN = 1;  /* Minimum number of input streams. */
static const int MAX_IN = 1;  /* Maximum number of input streams. */
static const int MIN_OUT = 2; /* Minimum number of output streams. */
static const int MAX_OUT = 2; /* Maximum number of output streams. */

receiver_base_cf::receiver_base_cf(std::string src_name, float pref_quad_rate, float quad_rate, int audio_rate)
    : gr::hier_block2 (src_name,
                      gr::io_signature::make (MIN_IN, MAX_IN, sizeof(gr_complex)),
                      gr::io_signature::make (MIN_OUT, MAX_OUT, sizeof(float))),
      d_quad_rate(quad_rate),
      d_audio_rate(audio_rate),
      d_pref_quad_rate(pref_quad_rate)
{
    iq_resamp = make_resampler_cc(d_pref_quad_rate/d_quad_rate);
    agc = make_rx_agc_2f(d_audio_rate, false, 0, 0, 100, 500, 500, 0);
    sql = gr::analog::simple_squelch_cc::make(-150.0, 0.001);
    meter = make_rx_meter_c(d_pref_quad_rate);
}

receiver_base_cf::~receiver_base_cf()
{

}

void receiver_base_cf::set_quad_rate(float quad_rate)
{
    if (std::abs(d_quad_rate-quad_rate) > 0.5)
    {
        qDebug() << "Changing RX quad rate:"  << d_quad_rate << "->" << quad_rate;
        d_quad_rate = quad_rate;
        lock();
        iq_resamp->set_rate(d_pref_quad_rate/d_quad_rate);
        unlock();
    }
}

void receiver_base_cf::set_center_freq(double center_freq)
{
    d_center_freq = center_freq;
}

void receiver_base_cf::set_offset(double offset)
{
    d_offset = offset;
}

void receiver_base_cf::set_rec_dir(std::string dir)
{
    d_rec_dir = dir;
}

float receiver_base_cf::get_signal_level()
{
    return meter->get_level_db();
}

bool receiver_base_cf::has_nb()
{
    return false;
}

void receiver_base_cf::set_nb_on(int nbid, bool on)
{
    (void) nbid;
    (void) on;
}

void receiver_base_cf::set_nb_threshold(int nbid, float threshold)
{
    (void) nbid;
    (void) threshold;
}

bool receiver_base_cf::has_sql()
{
    return false;
}

void receiver_base_cf::set_sql_level(double level_db)
{
    sql->set_threshold(level_db);
}

void receiver_base_cf::set_sql_alpha(double alpha)
{
    sql->set_alpha(alpha);
}

bool receiver_base_cf::has_agc()
{
    return false;
}

void receiver_base_cf::set_agc_on(bool agc_on)
{
    agc->set_agc_on(agc_on);
}

void receiver_base_cf::set_agc_target_level(int target_level)
{
    agc->set_target_level(target_level);
}

void receiver_base_cf::set_agc_manual_gain(float gain)
{
    agc->set_manual_gain(gain);
}

void receiver_base_cf::set_agc_max_gain(int gain)
{
    agc->set_max_gain(gain);
}

void receiver_base_cf::set_agc_attack(int attack_ms)
{
    agc->set_attack(attack_ms);
}

void receiver_base_cf::set_agc_decay(int decay_ms)
{
    agc->set_decay(decay_ms);
}

void receiver_base_cf::set_agc_hang(int hang_ms)
{
    agc->set_hang(hang_ms);
}

float receiver_base_cf::get_agc_gain()
{
    return agc->get_current_gain();
}

bool receiver_base_cf::has_fm()
{
    return false;
}

void receiver_base_cf::set_fm_maxdev(float maxdev_hz)
{
    (void) maxdev_hz;
}

void receiver_base_cf::set_fm_deemph(double tau)
{
    (void) tau;
}

bool receiver_base_cf::has_am()
{
    return false;
}

void receiver_base_cf::set_am_dcr(bool enabled)
{
    (void) enabled;
}

bool receiver_base_cf::has_amsync()
{
    return false;
}

void receiver_base_cf::set_amsync_dcr(bool enabled)
{
    (void) enabled;
}

void receiver_base_cf::set_amsync_pll_bw(float pll_bw)
{
    (void) pll_bw;
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
    // FIXME: option to use local time
    // use toUTC() function compatible with older versions of Qt.
    QString file_name = QDateTime::currentDateTime().toUTC().toString("gqrx_yyyyMMdd_hhmmss");
    QString filename = QString("%1/%2_%3.wav").arg(QString(d_rec_dir.data())).arg(file_name).arg(qint64(d_center_freq + d_offset));
    d_audio_filename = filename.toStdString();

    // if this fails, we don't want to go and crash now, do we
    try {
#if GNURADIO_VERSION < 0x030900
        wav_sink = gr::blocks::wavfile_sink::make(d_audio_filename.c_str(), 2,
                                                  (unsigned int) d_audio_rate,
                                                  16);
#else
        wav_sink = gr::blocks::wavfile_sink::make(d_audio_filename.c_str(), 2,
                                                  (unsigned int) d_audio_rate,
                                                  gr::blocks::FORMAT_WAV, gr::blocks::FORMAT_PCM_16);
#endif
    }
    catch (std::runtime_error &e) {
        std::cout << "Error opening " << d_audio_filename << ": " << e.what() << std::endl;
        return 1;
    }

    lock();
    connect(agc, 0, wav_sink, 0);
    connect(agc, 1, wav_sink, 1);
    unlock();
    return 0;
}

void receiver_base_cf::stop_audio_recording()
{
    // not strictly necessary to lock but I think it is safer
    lock();
    wav_sink->close();
    disconnect(agc, 0, wav_sink, 0);
    disconnect(agc, 1, wav_sink, 1);

    unlock();
    wav_sink.reset();
}

//FIXME Reimplement wavfile_sink correctly to make this work as expected
void receiver_base_cf::continue_audio_recording(receiver_base_cf_sptr from)
{
    if(from.get() == this)
        return;
    from->disconnect(from->agc, 0, from->wav_sink, 0);
    from->disconnect(from->agc, 1, from->wav_sink, 1);
    wav_sink = from->wav_sink;
    connect(agc, 0, wav_sink, 0);
    connect(agc, 1, wav_sink, 1);
    from->wav_sink.reset();
}

std::string receiver_base_cf::get_last_audio_filename()
{
    return d_audio_filename;
}
