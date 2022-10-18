/*
 * asynchronous receiver block implementation
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
#include <volk/volk.h>
#include "async_rx_impl.h"

namespace gr {
namespace rtty {

async_rx::sptr async_rx::make(float sample_rate, float bit_rate, char word_len, enum async_rx_parity parity) {
    return gnuradio::get_initial_sptr(new async_rx_impl(sample_rate, bit_rate, word_len, parity));
}

async_rx_impl::async_rx_impl(float sample_rate, float bit_rate, char word_len, enum async_rx_parity parity) :
            gr::block("async_rx",
            gr::io_signature::make(1, 1, sizeof(float)),
            gr::io_signature::make(1, 1, sizeof(unsigned char))),
            d_sample_rate(sample_rate),
            d_bit_rate(bit_rate),
            d_word_len(word_len),
            d_parity(parity) {
    bit_len = (sample_rate / bit_rate);
    state = ASYNC_WAIT_IDLE;
    threshold = 0;
}

async_rx_impl::~async_rx_impl() {
}

void async_rx_impl::set_word_len(char word_len) {
    std::lock_guard<std::mutex> lock(d_mutex);

    d_word_len = word_len;
}

char async_rx_impl::word_len() const {
    return d_word_len;
}

void async_rx_impl::set_sample_rate(float sample_rate) {
    std::lock_guard<std::mutex> lock(d_mutex);

    d_sample_rate = sample_rate;
    bit_len = (sample_rate / d_bit_rate);
}

float async_rx_impl::sample_rate() const {
    return d_sample_rate;
}

void async_rx_impl::set_bit_rate(float bit_rate) {
    std::lock_guard<std::mutex> lock(d_mutex);

    d_bit_rate = bit_rate;
    bit_len = (d_sample_rate / bit_rate);
}

float async_rx_impl::bit_rate() const {
    return d_bit_rate;
}

void async_rx_impl::set_parity(enum async_rx::async_rx_parity parity) {
    std::lock_guard<std::mutex> lock(d_mutex);

    d_parity = parity;
}

enum async_rx::async_rx_parity async_rx_impl::parity() const {
    return d_parity;
}

void async_rx_impl::reset() {
    std::lock_guard<std::mutex> lock(d_mutex);

    max0=0;
    max1=0;
    threshold = (max0+max1)/2;
    state = ASYNC_WAIT_IDLE;
}

void async_rx_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required) {
    std::lock_guard<std::mutex> lock(d_mutex);

    ninput_items_required[0] = noutput_items * (d_word_len+2+(d_parity==ASYNC_RX_PARITY_NONE?0:1)) * bit_len;
}

int async_rx_impl::general_work (int noutput_items,
        gr_vector_int &ninput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items) {

    std::lock_guard<std::mutex> lock(d_mutex);
    float in_count = 0;
    int out_count = 0;
    const float *in = reinterpret_cast<const float*>(input_items[0]);
    unsigned char *out = reinterpret_cast<unsigned char*>(output_items[0]);
    float InAcc;

    while( (out_count < noutput_items) && (((int)roundf(in_count)) < (ninput_items[0]-bit_len))) {
        volk_32f_accumulator_s32f(&InAcc,&in[(int)roundf(in_count)],bit_len);
    //InAcc = in[(int)round(in_count)];
        switch (state) {
            case ASYNC_IDLE:    // Wait for MARK to SPACE transition
                if (InAcc<=threshold) { // transition detected
                    in_count+=bit_len/2;
                    state = ASYNC_CHECK_START;
                } else
                    in_count++;
                break;
            case ASYNC_CHECK_START:    // Check start bit
                if (InAcc<=threshold) { // Start bit verified
                    in_count += bit_len;
                    state = ASYNC_GET_BIT;
                    bit_pos = 0;
                    bit_count = 0;
                    word = 0;
                    max0 = InAcc;
                    threshold = (max1+max0)/2;
                } else { // Noise detection on start
                    in_count -= bit_len/2 -1;
                    state = ASYNC_IDLE;
                }
                break;
            case ASYNC_GET_BIT:
                if (InAcc>threshold) {
                    word |= 1<<bit_pos;
                    bit_count++;
                    max1 = InAcc;
                }
                else
                    max0 = InAcc;
                threshold = (max1+max0)/2;
                in_count+=bit_len;
                bit_pos++;
                if (bit_pos == d_word_len) {
                    if (d_parity == ASYNC_RX_PARITY_NONE)
                        state = ASYNC_CHECK_STOP;
                    else
                        state = ASYNC_CHECK_PARITY;
                }
                break;
            case ASYNC_CHECK_PARITY: // Check parity bit
                switch (d_parity) {
                    default:
                    case ASYNC_RX_PARITY_NONE:
                        state = ASYNC_CHECK_STOP;
                        break;
                    case ASYNC_RX_PARITY_ODD:
                        if ((InAcc<=threshold && (bit_count&1)) || (InAcc>threshold && !(bit_count&1))) {
                            in_count += bit_len;
                            state = ASYNC_CHECK_STOP;
                            if (bit_count&1)
                                max0=InAcc;
                            else
                                max1=InAcc;
                        }
                        else {
                            if (InAcc>threshold) {
                                max1 = InAcc;
                                state = ASYNC_IDLE;
                            }
                            else
                                state = ASYNC_WAIT_IDLE;
                            in_count++;
                        }
                        break;
                    case ASYNC_RX_PARITY_EVEN:
                        if ((InAcc<=threshold && !(bit_count&1)) || (InAcc>threshold && (bit_count&1))) {
                            in_count += bit_len;
                            state = ASYNC_CHECK_STOP;
                        if (bit_count&1)
                            max1=InAcc;
                        else
                            max0=InAcc;
                        }
                        else {
                            if (InAcc>threshold) {
                                state = ASYNC_IDLE;
                                max1 = InAcc;
                            }
                            else
                                state = ASYNC_WAIT_IDLE;
                            in_count++;
                        }
                        break;
                    case ASYNC_RX_PARITY_MARK:
                        if (InAcc>threshold) {
                            in_count += bit_len;
                            state = ASYNC_CHECK_STOP;
                            max1=InAcc;
                        }
                        else {
                            state = ASYNC_WAIT_IDLE;
                            in_count++;
                        }
                        break;
                    case ASYNC_RX_PARITY_SPACE:
                        if (InAcc<=threshold) {
                            in_count += bit_len;
                            state = ASYNC_CHECK_STOP;
                            max0=InAcc;
                        }
                        else {
                            state = ASYNC_IDLE;
                            in_count++;
                        }
                        break;
                    case ASYNC_RX_PARITY_DONTCARE:
                        in_count += bit_len;
                        state = ASYNC_CHECK_STOP;
                        if (InAcc>threshold)
                            max1 = InAcc;
                        else
                            max0 = InAcc;
                        break;
                }
                threshold = (max0+max1)/2;
                break;
            case ASYNC_CHECK_STOP: // Check stop bit
                if (InAcc>threshold) { // Stop bit verified
                    *out = word;
                    out++;
                    out_count++;
                    state = ASYNC_IDLE;
                    max1=InAcc;
                    threshold = (max0+max1)/2;
                } else { // Framming error
                    state = ASYNC_WAIT_IDLE;
                }
                in_count+=1;
                break;
            default:
            case ASYNC_WAIT_IDLE:    // Wait for SPACE to MARK transition
                if (InAcc>threshold) { // transition detected
                    in_count+=bit_len/2;
                    state = ASYNC_CHECK_IDLE;
                } else {
                    in_count++;
                }
                break;
            case ASYNC_CHECK_IDLE:    // Check idle
                if (InAcc>threshold) { // Idle for 1 bit verified
                    in_count += 1;
                    state = ASYNC_IDLE;
                    bit_pos = 0;
                    bit_count = 0;
                    word = 0;
                    max1 = InAcc;
                    threshold = (max0+max1)/2;
                } else { // Noise detection on stop
                    in_count -= bit_len/2 -1;
                    state = ASYNC_WAIT_IDLE;
                }
                break;
        }
    }

    consume_each ((int)roundf(in_count));

    return (out_count);
}

}
}
