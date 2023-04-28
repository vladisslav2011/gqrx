/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2012 Alexandru Csete OZ9AEC.
 * FM stereo implementation by Alex Grinkov a.grinkov(at)gmail.com.
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
#include <cmath>
#include <iostream>
#include <QDebug>
#include "receivers/wfmrx.h"

wfmrx_sptr make_wfmrx(float quad_rate, float audio_rate)
{
    return gnuradio::get_initial_sptr(new wfmrx(quad_rate, audio_rate));
}

wfmrx::wfmrx(float quad_rate, float audio_rate)
    : receiver_base_cf("WFMRX", WFM_PREF_QUAD_RATE, quad_rate, audio_rate),
      d_running(false)
{

    filter = make_rx_filter(WFM_PREF_QUAD_RATE, -80000.0, 80000.0, 20000.0);
    /* demodulator */
    demod_fm = gr::analog::quadrature_demod_cf::make(WFM_PREF_QUAD_RATE / (2.0 * M_PI * 75000.0));
    stereo = make_stereo_demod(WFM_PREF_QUAD_RATE, d_audio_rate, true);
    stereo_oirt = make_stereo_demod(WFM_PREF_QUAD_RATE, d_audio_rate, true, true);
    mono = make_stereo_demod(WFM_PREF_QUAD_RATE, d_audio_rate, false);

    /* create rds blocks but dont connect them */
    rds = make_rx_rds(WFM_PREF_QUAD_RATE);
    rds_decoder = gr::rds::decoder::make(1, 1);
    rds_parser = gr::rds::parser::make(0, 0, 0);

    connect(ddc, 0, iq_resamp, 0);
    connect(iq_resamp, 0, filter, 0);
    connect(filter, 0, meter, 0);
    connect(filter, 0, sql, 0);
    connect(sql, 0, demod_fm, 0);
    connect(demod_fm, 0, mono, 0);
    connect(mono, 0, output, 0); // left  channel
    connect(mono, 1, output, 1); // right channel
}

wfmrx::~wfmrx()
{

}

bool wfmrx::start()
{
    d_running = true;

    return true;
}

bool wfmrx::stop()
{
    d_running = false;

    return true;
}

void wfmrx::set_audio_rate(int audio_rate)
{
    receiver_base_cf::set_audio_rate(audio_rate);
    switch (d_demod) {

    case Modulations::MODE_WFM_MONO:
    default:
        mono->set_audio_rate(audio_rate);
        break;

    case Modulations::MODE_WFM_STEREO:
        stereo->set_audio_rate(audio_rate);
        break;

    case Modulations::MODE_WFM_STEREO_OIRT:
        stereo_oirt->set_audio_rate(audio_rate);
        break;
    }
}

void wfmrx::set_filter(int low, int high, Modulations::filter_shape shape)
{
    receiver_base_cf::set_filter(low, high, shape);
    if(d_demod!=Modulations::MODE_OFF)
        filter->set_param(double(d_filter_low), double(d_filter_high), double(d_filter_tw));
}

bool wfmrx::set_filter_shape(const c_def::v_union & v)
{
    receiver_base_cf::set_filter_shape(v);
    if(d_demod!=Modulations::MODE_OFF)
        filter->set_param(double(d_filter_low), double(d_filter_high), double(d_filter_tw));
    return true;
}

bool wfmrx::set_demod(const c_def::v_union & v)
{
    Modulations::idx new_demod = Modulations::idx(int(v));
    /* check if new demodulator selection is valid */
    if ((new_demod < Modulations::MODE_WFM_MONO) || (new_demod > Modulations::MODE_WFM_STEREO_OIRT))
        return true;

    if (new_demod == d_demod) {
        /* nothing to do */
        return true;
    }

    /* disconnect current demodulator */
    switch (d_demod) {

    case Modulations::MODE_WFM_MONO:
    default:
        disconnect(demod_fm, 0, mono, 0);
        disconnect(mono, 0, output, 0); // left  channel
        disconnect(mono, 1, output, 1); // right channel
        break;

    case Modulations::MODE_WFM_STEREO:
        disconnect(demod_fm, 0, stereo, 0);
        disconnect(stereo, 0, output, 0); // left  channel
        disconnect(stereo, 1, output, 1); // right channel
        break;

    case Modulations::MODE_WFM_STEREO_OIRT:
        disconnect(demod_fm, 0, stereo_oirt, 0);
        disconnect(stereo_oirt, 0, output, 0); // left  channel
        disconnect(stereo_oirt, 1, output, 1); // right channel
        break;
    }

    switch (new_demod) {

    case Modulations::MODE_WFM_MONO:
    default:
        connect(demod_fm, 0, mono, 0);
        connect(mono, 0, output, 0); // left  channel
        connect(mono, 1, output, 1); // right channel
        mono->set_audio_rate(d_audio_rate);
        break;

    case Modulations::MODE_WFM_STEREO:
        connect(demod_fm, 0, stereo, 0);
        connect(stereo, 0, output, 0); // left  channel
        connect(stereo, 1, output, 1); // right channel
        stereo->set_audio_rate(d_audio_rate);
        break;

    case Modulations::MODE_WFM_STEREO_OIRT:
        connect(demod_fm, 0, stereo_oirt, 0);
        connect(stereo_oirt, 0, output, 0); // left  channel
        connect(stereo_oirt, 1, output, 1); // right channel
        stereo_oirt->set_audio_rate(d_audio_rate);
        break;
    }
    return receiver_base_cf::set_demod(v);
}

bool wfmrx::set_wfm_deemph(const c_def::v_union & v)
{
    receiver_base_cf::set_wfm_deemph(v);
    const float tau = 1e-6f * d_wfm_deemph;
    mono->set_tau(tau);
    stereo->set_tau(tau);
    stereo_oirt->set_tau(tau);
    return true;
}

void wfmrx::set_index(int index)
{
    receiver_base_cf::set_index(index);
    rds_parser->set_index(index);
}

void wfmrx::start_rds_decoder()
{
    lock();
    connect(demod_fm, 0, rds, 0);
    connect(rds, 0, rds_decoder, 0);
    msg_connect(rds_decoder, "out", rds_parser, "in");
    unlock();
    rds_parser->reset();
}

void wfmrx::stop_rds_decoder()
{
    lock();
    disconnect(demod_fm, 0, rds, 0);
    disconnect(rds, 0, rds_decoder, 0);
    msg_disconnect(rds_decoder, "out", rds_parser, "in");
    rds_parser->clear();
    unlock();
}

bool wfmrx::set_rds_on(const c_def::v_union & v)
{
    if(d_rds_on == bool(v))
        return true;
    receiver_base_cf::set_rds_on(v);
    if(d_rds_on)
        start_rds_decoder();
    else
        stop_rds_decoder();
    return true;
}

bool wfmrx::get_rds_pi(c_def::v_union &to) const
{
    to=rds_parser->get_last(gr::rds::parser::PI);
    return true;
}

bool wfmrx::get_rds_ps(c_def::v_union &to) const
{
    to=rds_parser->get_last(gr::rds::parser::PS);
    return true;
}

bool wfmrx::get_rds_pty(c_def::v_union &to) const
{
    to=rds_parser->get_last(gr::rds::parser::PTY);
    return true;
}

bool wfmrx::get_rds_flagstring(c_def::v_union &to) const
{
    to=rds_parser->get_last(gr::rds::parser::FLAGSTRING);
    return true;
}

bool wfmrx::get_rds_rt(c_def::v_union &to) const
{
    to=rds_parser->get_last(gr::rds::parser::RT);
    return true;
}

bool wfmrx::get_rds_clock(c_def::v_union &to) const
{
    to=rds_parser->get_last(gr::rds::parser::CLOCK);
    return true;
}

bool wfmrx::get_rds_af(c_def::v_union &to) const
{
    to=rds_parser->get_last(gr::rds::parser::AF);
    return true;
}
