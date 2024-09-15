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

wfmrx_sptr make_wfmrx(double quad_rate, float audio_rate, std::vector<receiver_base_cf_sptr> & rxes)
{
    return gnuradio::get_initial_sptr(new wfmrx(quad_rate, audio_rate, rxes));
}

wfmrx::wfmrx(double quad_rate, float audio_rate, std::vector<receiver_base_cf_sptr> & rxes)
    : receiver_base_cf("WFMRX", WFM_PREF_QUAD_RATE, quad_rate, audio_rate, rxes),
      d_running(false)
{

    filter = make_rx_filter((double)WFM_PREF_QUAD_RATE, -80000.0, 80000.0, 20000.0);
    /* demodulator */
    demod_fm = gr::analog::quadrature_demod_cf::make((double)WFM_PREF_QUAD_RATE / (2.0 * M_PI * 75000.0));
    stereo = make_stereo_demod(WFM_PREF_QUAD_RATE, d_audio_rate, true);
    stereo_oirt = make_stereo_demod(WFM_PREF_QUAD_RATE, d_audio_rate, true, true);
    mono = make_stereo_demod(WFM_PREF_QUAD_RATE, d_audio_rate, false);

    /* create rds blocks but dont connect them */
    rds = make_rx_rds((double)WFM_PREF_QUAD_RATE);
    rds_decoder = gr::rds::decoder::make();
    rds_parser = gr::rds::parser::make(0, 0, 0);

    connect_default();
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

bool wfmrx::set_au_flt_size(const c_def::v_union & v)
{
    receiver_base_cf::set_au_flt_size(v);
    switch (d_demod) {

    case Modulations::MODE_WFM_MONO:
    default:
        if(d_connected)
            lock();
        mono->set_au_flt_size(d_au_flt_size);
        if(d_connected)
            unlock();
        stereo->set_au_flt_size(d_au_flt_size);
        stereo_oirt->set_au_flt_size(d_au_flt_size);
        break;

    case Modulations::MODE_WFM_STEREO:
        mono->set_au_flt_size(d_au_flt_size);
        if(d_connected)
            lock();
        stereo->set_au_flt_size(d_au_flt_size);
        if(d_connected)
            unlock();
        stereo_oirt->set_au_flt_size(d_au_flt_size);
        break;

    case Modulations::MODE_WFM_STEREO_OIRT:
        mono->set_au_flt_size(d_au_flt_size);
        stereo->set_au_flt_size(d_au_flt_size);
        if(d_connected)
            lock();
        stereo_oirt->set_au_flt_size(d_au_flt_size);
        if(d_connected)
            unlock();
        break;
    }
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
    filter_adjust();
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
    mono->set_tau((double)tau);
    stereo->set_tau((double)tau);
    stereo_oirt->set_tau((double)tau);
    return true;
}

void wfmrx::set_index(int index)
{
    receiver_base_cf::set_index(index);
    rds_parser->set_index(index);
    rds->set_index(index);
}

void wfmrx::start_rds_decoder()
{
    lock();
    connect(demod_fm, 0, rds, 0);
    connect(rds, 0, rds_decoder, 0);
    msg_connect(rds_decoder, "out", rds_parser, "in");
    rds_parser->send_extra=[=]()
    {
        rds->trig();
    };
    unlock();
    rds_decoder->reset();
    rds_parser->reset();
}

void wfmrx::stop_rds_decoder()
{
    lock();
    disconnect(demod_fm, 0, rds, 0);
    disconnect(rds, 0, rds_decoder, 0);
    msg_disconnect(rds_decoder, "out", rds_parser, "in");
    rds_parser->send_extra=nullptr;
    unlock();
    rds_parser->clear();
}

bool wfmrx::set_wfm_raw(const c_def::v_union & v)
{
    receiver_base_cf::set_wfm_raw(v);
    if(d_demod == Modulations::MODE_WFM_MONO)
        lock();
    mono->set_raw(d_wfm_raw);
    if(d_demod == Modulations::MODE_WFM_MONO)
        unlock();
    return true;
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

bool wfmrx::get_rds_errors(c_def::v_union &to) const
{
    to=rds_parser->get_n_errors();
    return true;
}

bool wfmrx::get_rds_cl_freq(c_def::v_union &to) const
{
    to=0.;
    return true;
}

bool wfmrx::get_rds_phase_snr(c_def::v_union &to) const
{
    to=rds->phase_snr();
    return true;
}

bool wfmrx::set_rds_agc(const c_def::v_union & v)
{
    receiver_base_cf::set_rds_agc(v);
    rds->set_agc_rate(v);
    return true;
}

bool wfmrx::set_rds_gmu(const c_def::v_union & v)
{
    receiver_base_cf::set_rds_gmu(v);
    rds->set_gain_mu(powf(10.f, v));
    return true;
}

bool wfmrx::set_rds_gomega(const c_def::v_union & v)
{
    receiver_base_cf::set_rds_gomega(v);
    rds->set_gain_omega(powf(10.f, v));
    return true;
}

bool wfmrx::set_rds_fxff_bw(const c_def::v_union & v)
{
    receiver_base_cf::set_rds_fxff_bw(v);
    rds->set_fxff_bw(d_rds_fxff_bw);
    return true;
}

bool wfmrx::set_rds_fxff_tw(const c_def::v_union & v)
{
    receiver_base_cf::set_rds_fxff_tw(v);
    rds->set_fxff_tw(d_rds_fxff_tw);
    return true;
}

bool wfmrx::set_rds_ecc_max(const c_def::v_union & v)
{
    receiver_base_cf::set_rds_ecc_max(v);
    rds_decoder->set_ecc_max(d_rds_ecc_max);
    return true;
}

bool wfmrx::set_rds_omega_lim(const c_def::v_union & v)
{
    receiver_base_cf::set_rds_omega_lim(v);
    rds->set_omega_lim(d_rds_omega_lim);
    return true;
}

bool wfmrx::set_rds_dll_bw(const c_def::v_union & v)
{
    receiver_base_cf::set_rds_dll_bw(v);
    rds->set_dll_bw(d_rds_dll_bw);
    return true;
}

bool wfmrx::set_rds_cl_bw(const c_def::v_union & v)
{
    receiver_base_cf::set_rds_cl_bw(v);
    rds->set_cl_bw(d_rds_cl_bw);
    return true;
}

bool wfmrx::set_rds_cl_lim(const c_def::v_union & v)
{
    receiver_base_cf::set_rds_cl_lim(v);
    rds->set_cl_lim(d_rds_cl_lim);
    return true;
}
