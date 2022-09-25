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
#ifndef RX_RNNOISE_H
#define RX_RNNOISE_H

#include <mutex>
#include <vector>
#include <gnuradio/sync_block.h>
#include <gnuradio/gr_complex.h>
#ifdef ENABLE_RNNOISE
#include <rnnoise.h>
#endif
class rx_rnnoise_f;

#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<rx_rnnoise_f> rx_rnnoise_f_sptr;
#else
typedef std::shared_ptr<rx_rnnoise_f> rx_rnnoise_f_sptr;
#endif


/**
 * \brief Return a shared_ptr to a new instance of rx_rnnoise_f.
 * \param sample_rate  The sample rate (default = 48000).
 *
 * This is effectively the public constructor for a new block.
 * To avoid accidental use of raw pointers, the rx_rnnoise_f constructor is private.
 * make_rx_rnnoise_f is the public interface for creating new instances.
 */
rx_rnnoise_f_sptr make_rx_rnnoise_f(int sample_rate = 48000);

/**
 * \brief Experimental AGC block for analog voice modes (AM, SSB, CW).
 * \ingroup DSP
 *
 * This block performs automatic gain control.
 * To be written...
 */
class rx_rnnoise_f : public gr::sync_block
{
    friend rx_rnnoise_f_sptr make_rx_rnnoise_f(int sample_rate);

protected:
    rx_rnnoise_f(int sample_rate);

public:
    ~rx_rnnoise_f();

    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items) override;

    void set_sample_rate(int sample_rate);
    void set_enabled(bool enabled)
    {
        std::unique_lock<std::mutex> lock(d_mutex);
        d_enabled = enabled;
    }
    void set_gain(float gain)
    {
        std::unique_lock<std::mutex> lock(d_mutex);
        d_gain = gain;
        d_gain_inv = 1.f / gain;
    }
    bool get_enabled() { return d_enabled; }
    float get_gain() { return d_gain; }
private:
    std::mutex      d_mutex;  /*! Used to lock internal data while processing or setting parameters. */

    int             d_sample_rate;   /*! Current sample rate. */
    int             d_frame_size;
    bool            d_enabled;
    float           d_gain;
    float           d_gain_inv;
#ifdef ENABLE_RNNOISE
    DenoiseState    *d_st0;
    DenoiseState    *d_st1;
#endif
    std::vector<float> d_buf0;
    std::vector<float> d_buf1;
private:
};

#endif /* RX_RNNOISE_H */
