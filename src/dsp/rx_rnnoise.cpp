/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2011 Alexandru Csete OZ9AEC.
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
#include <stdexcept>
#include <gnuradio/io_signature.h>
#include <gnuradio/gr_complex.h>
#include <dsp/rx_rnnoise.h>
#include <volk/volk.h>

#define P_GAIN 100000.0

rx_rnnoise_f_sptr make_rx_rnnoise_f(int sample_rate)
{
    return gnuradio::get_initial_sptr(new rx_rnnoise_f(sample_rate));
}

/**
 * \brief Create receiver rnnoice noice reduction filter object.
 *
 * Use make_rx_rnnoice_f() instead.
 */
rx_rnnoise_f::rx_rnnoise_f(int sample_rate)
    : gr::sync_block ("rx_rnnoise_f",
          gr::io_signature::make(2, 2, sizeof(float)),
          gr::io_signature::make(2, 2, sizeof(float))),
      d_sample_rate(sample_rate),
      d_enabled(false),
      d_gain(P_GAIN),
      d_gain_inv(1.0/P_GAIN)
{
#ifdef ENABLE_RNNOISE
    d_st0 = rnnoise_create(NULL);
    if(!d_st0)
        throw(std::runtime_error("Failed to create rnnoise state object"));
    d_st1 = rnnoise_create(NULL);
    if(!d_st1)
        throw(std::runtime_error("Failed to create rnnoise state object"));
    d_frame_size=rnnoise_get_frame_size();
#else
    d_frame_size = 256;
#endif
    set_output_multiple(d_frame_size);
    d_buf0.resize(d_frame_size);
    d_buf1.resize(d_frame_size);
}

rx_rnnoise_f::~rx_rnnoise_f()
{
#ifdef ENABLE_RNNOISE
  rnnoise_destroy(d_st0);
  rnnoise_destroy(d_st1);
#endif
}

/**
 * \brief Receiver rnnoise noice reduction work method.
 * \param mooutput_items
 * \param input_items
 * \param output_items
 */
int rx_rnnoise_f::work(int noutput_items,
                    gr_vector_const_void_star &input_items,
                    gr_vector_void_star &output_items)
{
    const float *in0 = (const float *) input_items[0];
    const float *in1 = (const float *) input_items[1];
    float *out0 = (float *) output_items[0];
    float *out1 = (float *) output_items[1];

    std::lock_guard<std::mutex> lock(d_mutex);
    int nframes = noutput_items / d_frame_size;
    for(int k = 0; k < nframes; k++)
    {
#ifdef ENABLE_RNNOISE
        if(d_enabled)
        {
            volk_32f_s32f_multiply_32f(out0, (float *)in0, d_gain, d_frame_size);
            volk_32f_s32f_multiply_32f(out1, (float *)in1, d_gain, d_frame_size);

            rnnoise_process_frame(d_st0, d_buf0.data(), out0);
            rnnoise_process_frame(d_st1, d_buf1.data(), out1);

            volk_32f_s32f_multiply_32f(out0, d_buf0.data(), d_gain_inv, d_frame_size);
            volk_32f_s32f_multiply_32f(out1, d_buf1.data(), d_gain_inv, d_frame_size);
        }else{
#else
        {
#endif
            std::memcpy(out0, in0, d_frame_size * sizeof(float));
            std::memcpy(out1, in1, d_frame_size * sizeof(float));
        }

        out0 += d_frame_size;
        out1 += d_frame_size;
        in0 += d_frame_size;
        in1 += d_frame_size;
    }
    return noutput_items;
}
