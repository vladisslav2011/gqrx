/*
 * rtty demodulator block implementation
 *
 * Copyright 2022 Marc CAPDEVILLE F4JMZ
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include <gnuradio/io_signature.h>
#include <gnuradio/filter/firdes.h>
#include <dsp/rtty/rtty_demod.h>
#include <gnuradio/blocks/null_sink.h>

namespace gr {
    namespace rtty {

/* Create a new instance of rtty_demod and return a shared_ptr. */
rtty_demod::sptr rtty_demod::make(float quad_rate, float mark_freq, float space_freq,float baud_rate,enum rtty_mode mode,enum rtty_parity parity) {
    return gnuradio::get_initial_sptr(new rtty_demod(quad_rate, mark_freq, space_freq, baud_rate, mode, parity));
}

static const int MIN_IN = 1;  /* Mininum number of input streams. */
static const int MAX_IN = 1;  /* Maximum number of input streams. */
static const int MIN_OUT = 0; /* Minimum number of output streams. */
static const int MAX_OUT = 0; /* Maximum number of output streams. */

rtty_demod::rtty_demod(float quad_rate, float mark_freq, float space_freq, float baud_rate,enum rtty_mode mode,enum rtty_parity parity)
                        : gr::hier_block2 ("rtty_demod",
                          gr::io_signature::make (MIN_IN, MAX_IN, sizeof (gr_complex)),
                          gr::io_signature::make (MIN_OUT, MAX_OUT, sizeof (char))),
                          d_quad_rate(quad_rate),
                          d_mark_freq(mark_freq),
                          d_space_freq(space_freq),
                          d_baud_rate(baud_rate),
                          d_mode(mode),
                          d_parity(parity) {
    int word_len;
    enum gr::rtty::async_rx::async_rx_parity async_parity;

    /* demodulator */
//    d_fsk_demod = gr::rtty::fsk_demod::make(d_quad_rate,round(d_quad_rate/RTTY_DEMOD_RATE),d_baud_rate,mark_freq,space_freq);
    /* generate taps */
    double rate = d_baud_rate*RTTY_OSR/d_quad_rate;

    double cutoff = rate > 1.0 ? 0.4 : 0.4*rate;
    double trans_width = rate > 1.0 ? 0.2 : 0.2*rate;
    unsigned int flt_size = 32;

    d_taps = gr::filter::firdes::low_pass(flt_size, flt_size, cutoff, trans_width);

    /* create the filter */
    d_resampler = gr::filter::pfb_arb_resampler_ccf::make(rate, d_taps, flt_size);
    d_fsk_demod = gr::rtty::fsk_demod::make(baud_rate*RTTY_OSR,d_baud_rate,mark_freq,space_freq);

    /* Decode */
    switch (d_mode) {
        default:
        case RTTY_MODE_5BITS_BAUDOT:
            word_len = 5;
            break;
        case RTTY_MODE_7BITS_ASCII:
            word_len = 7;
            break;
        case RTTY_MODE_8BITS_ASCII:
            word_len = 8;
            break;
    }

    switch (d_parity) {
        default:
        case RTTY_PARITY_NONE:
            async_parity = gr::rtty::async_rx::ASYNC_RX_PARITY_NONE;
            break;
        case RTTY_PARITY_ODD:
            async_parity = gr::rtty::async_rx::ASYNC_RX_PARITY_ODD;
            break;
        case RTTY_PARITY_EVEN:
            async_parity = gr::rtty::async_rx::ASYNC_RX_PARITY_EVEN;
            break;
        case RTTY_PARITY_MARK:
            async_parity = gr::rtty::async_rx::ASYNC_RX_PARITY_MARK;
            break;
        case RTTY_PARITY_SPACE:
            async_parity = gr::rtty::async_rx::ASYNC_RX_PARITY_SPACE;
            break;
        case RTTY_PARITY_DONTCARE:
            async_parity = gr::rtty::async_rx::ASYNC_RX_PARITY_DONTCARE;
            break;
    }
    d_async = gr::rtty::async_rx::make(baud_rate*RTTY_OSR,d_baud_rate,word_len,async_parity);

    /* baudot decodding and storage */
    d_data = char_store::make(100,true);

    /* connect blocks */
    connect(self(), 0, d_resampler, 0);
    connect(d_resampler, 0, d_fsk_demod, 0);
    connect(d_fsk_demod, 0, d_async, 0);
    connect(d_fsk_demod, 1, d_async, 1);
    connect(d_fsk_demod, 2, d_async, 2);
    connect(d_fsk_demod, 3, d_async, 3);
    connect(d_async, 0, d_data, 0);
}

rtty_demod::~rtty_demod () {
}


void rtty_demod::update_settings()
{
    double rate = d_baud_rate*RTTY_OSR/d_quad_rate;
    double cutoff = rate > 1.0 ? 0.4 : 0.4*rate;
    double trans_width = rate > 1.0 ? 0.2 : 0.2*rate;
    unsigned int flt_size = 32;

    d_taps = gr::filter::firdes::low_pass(flt_size, flt_size, cutoff, trans_width);

    d_resampler->set_taps(d_taps);
    d_resampler->set_rate(rate);
    d_fsk_demod->set_symbol_rate(d_baud_rate);
    d_fsk_demod->set_sample_rate(d_baud_rate*RTTY_OSR);
    d_async->set_sample_rate(d_baud_rate*RTTY_OSR);
    d_async->set_bit_rate(d_baud_rate);
}

void  rtty_demod::set_quad_rate(float quad_rate) {
    d_quad_rate = quad_rate;
    update_settings();
}

float rtty_demod::quad_rate() const {
    return d_quad_rate;
}

void rtty_demod::set_mark_freq(float mark_freq) {
    d_mark_freq = mark_freq;
    d_fsk_demod->set_mark_freq(d_mark_freq);
}

float rtty_demod::mark_freq() {
    return d_fsk_demod->mark_freq();
}

void rtty_demod::set_space_freq(float space_freq) {
    d_space_freq = space_freq;
    d_fsk_demod->set_space_freq(d_space_freq);
}

float rtty_demod::space_freq() {
    return d_fsk_demod->space_freq();
}

void rtty_demod::set_baud_rate(float baud_rate) {
    if (!baud_rate)
        return;
    d_baud_rate = baud_rate;
    update_settings();
    d_async->reset();
}

float rtty_demod::baud_rate() {
    return d_async->bit_rate();
}

void rtty_demod::reset() {
    d_async->reset();
}

int rtty_demod::get_data(std::string &data) {

    return d_data->get_data(data);
}

void rtty_demod::set_mode(enum rtty_mode mode) {
    if (mode == d_mode)
        return;

    switch (mode) {
        case RTTY_MODE_5BITS_BAUDOT:
            d_async->set_word_len(5);
            d_data->set_baudot(true);
            d_async->reset();
            break;
        case RTTY_MODE_7BITS_ASCII:
            d_async->set_word_len(7);
            d_data->set_baudot(false);
            d_async->reset();
            break;
        case RTTY_MODE_8BITS_ASCII:
            d_async->set_word_len(8);
            d_data->set_baudot(false);
            d_async->reset();
            break;
        default:
            return;
    }

    d_mode = mode;
}

enum rtty_demod::rtty_mode rtty_demod::mode() {
    return d_mode;
}

void rtty_demod::set_parity(enum rtty_parity parity) {
    if (parity == d_parity)
        return;

    switch (parity) {
        default:
        case RTTY_PARITY_NONE:
            d_async->set_parity(gr::rtty::async_rx::ASYNC_RX_PARITY_NONE);
            break;
        case RTTY_PARITY_ODD:
            d_async->set_parity(gr::rtty::async_rx::ASYNC_RX_PARITY_ODD);
            break;
        case RTTY_PARITY_EVEN:
            d_async->set_parity(gr::rtty::async_rx::ASYNC_RX_PARITY_EVEN);
            break;
        case RTTY_PARITY_MARK:
            d_async->set_parity(gr::rtty::async_rx::ASYNC_RX_PARITY_MARK);
            break;
        case RTTY_PARITY_SPACE:
            d_async->set_parity(gr::rtty::async_rx::ASYNC_RX_PARITY_SPACE);
            break;
        case RTTY_PARITY_DONTCARE:
            d_async->set_parity(gr::rtty::async_rx::ASYNC_RX_PARITY_DONTCARE);
            break;
    }

    d_async->reset();

    d_parity = parity;
}

enum rtty_demod::rtty_parity rtty_demod::parity() {
    return d_parity;
}

}
}
