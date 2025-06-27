/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2025 vladisslav2011@gmail.com.
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
#ifndef RX_MMSE_NR_H
#define RX_MMSE_NR_H

#include "receivers/defines.h"
#include <mutex>
#include <vector>
#include <gnuradio/sync_block.h>
#include <gnuradio/gr_complex.h>
#include <gnuradio/fft/fft.h>
#include <gnuradio/filter/firdes.h>       /* contains enum win_type */

class rx_mmse_nr_f;
class min_buffer_n;

#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<rx_mmse_nr_f> rx_mmse_nr_f_sptr;
#else
typedef std::shared_ptr<rx_mmse_nr_f> rx_mmse_nr_f_sptr;
#endif


/**
 * \brief Return a shared_ptr to a new instance of rx_mmse_nr_f.
 * \param sample_rate  The sample rate (default = 48000).
 *
 * This is effectively the public constructor for a new block.
 * To avoid accidental use of raw pointers, the rx_mmse_nr_f constructor is private.
 * make_rx_mmse_nr_f is the public interface for creating new instances.
 */
rx_mmse_nr_f_sptr make_rx_mmse_nr_f(int sample_rate = 48000);

/**
 * \brief Experimental NR block.
 * \ingroup DSP
 *
 * This block performs noise reduction.
 * To be written...
 */
class rx_mmse_nr_f : public gr::sync_block
{
    friend rx_mmse_nr_f_sptr make_rx_mmse_nr_f(int sample_rate);

protected:
    rx_mmse_nr_f(int sample_rate);

public:
    ~rx_mmse_nr_f();

    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items) override;

    void set_sample_rate(int sample_rate);
    void set_enabled(bool enabled)
    {
        std::unique_lock<std::mutex> lock(d_mutex);
        d_enabled = enabled;
        d_init = 0;
        d_init_ksi = false;
        fv_clear(d_old);
        fv_clear(d_noise_mean);
        fv_clear(d_Xk_prev);
    }
    void set_threshold(float val)
    {
        std::unique_lock<std::mutex> lock(d_mutex);
        d_thr = val;
    }
    void set_ofs(float val)
    {
        std::unique_lock<std::mutex> lock(d_mutex);
        d_ofs = val;
    }
    void set_strength(float val)
    {
        std::unique_lock<std::mutex> lock(d_mutex);
        d_strength = val;
    }
    bool get_enabled() { return d_enabled; }
    float get_threshold() { return d_thr; }
    float get_ofs() { return d_ofs; }
    float get_strength() { return d_strength; }
private:
    template <typename T> void fv_clear(std::vector<T> & v)
    {
        std::memset(v.data(),0,v.size()*sizeof(v[0]));
    }
int mmse_nr(int noutput_items,
                    const float *in0,
                    float * out0);

int dumb_nr(int noutput_items,
                    const float *in0,
                    float * out0);
float get_peak(unsigned n);
void update_buffer(unsigned n,unsigned p);

    std::mutex      d_mutex;  /*! Used to lock internal data while processing or setting parameters. */

    int             d_sample_rate;   /*! Current sample rate. */
    int             d_frame_size;
    bool            d_enabled;
    float           d_thr;
    float           d_ofs;
    float           d_strength;
    int             d_len1{0};
    int             d_len2{0};
    float           d_type{0};
    int             d_init{0};
    bool            d_init_ksi{false};
    int             d_fft_size{0};
    int             d_fft_rsize{0};

    gr::fft::fft_real_fwd *d_fft{nullptr};
    gr::fft::fft_real_rev *d_fft_r{nullptr};
    std::vector<float> d_window;
    std::vector<float> d_noise_mean;
    std::vector<float> d_noise_mu;
    std::vector<float> d_old;
    std::vector<float> d_ksi;
    std::vector<float> d_Xk_prev;
    std::vector<gr_complex> d_prev;
    float d_avg{0.f};
    float d_havg{0.f};
    float d_lavg{0.f};
    std::vector<std::vector<float>> d_mag_buf{};
    unsigned d_buf_size{0};
    unsigned d_mag_idx{0};
    unsigned d_mag_p{0};
private:
};

#endif /* RX_RNNOISE_H */
