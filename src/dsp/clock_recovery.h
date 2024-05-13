/* -*- c++ -*- */
/*
 * Copyright 2004,2011,2012,2014 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#ifndef INCLUDED_CLOCK_RECOVERY_H
#define INCLUDED_CLOCK_RECOVERY_H

#include <gnuradio/block.h>
#include <gnuradio/filter/mmse_fir_interpolator_cc.h>

class clock_recovery_el_cc : virtual public gr::block
{
public:
#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<clock_recovery_el_cc> sptr;
#else
typedef std::shared_ptr<clock_recovery_el_cc> sptr;
#endif
    static sptr make(float omega,
                              float gain_omega,
                              float mu,
                              float gain_mu,
                              float omega_relative_limit);
    clock_recovery_el_cc(float omega,
                              float gain_omega,
                              float mu,
                              float gain_mu,
                              float omega_relative_limit);
    ~clock_recovery_el_cc() override;

    void forecast(int noutput_items, gr_vector_int& ninput_items_required) override;
    int general_work(int noutput_items,
                     gr_vector_int& ninput_items,
                     gr_vector_const_void_star& input_items,
                     gr_vector_void_star& output_items) override;

    float mu() const { return (std::abs(d_corr0)>std::abs(d_corr180))?d_corr0*10.f:-d_corr180*10.f; }
//    float mu() const { return (std::abs(d_corr0)>std::abs(d_corr180))?(d_am_i-d_am_q)*10.f:(d_am_q-d_am_i)*10.f; }
    float omega() const { return d_omega; }
    float gain_mu() const { return d_gain_mu; }
    float gain_omega() const { return d_gain_omega; }
    float dllbw() const { return d_dllbw; }
    float dllalfa() const { return d_dllalfa; }

    void set_gain_mu(float gain_mu) { d_gain_mu = gain_mu; }
    void set_gain_omega(float gain_omega) { d_gain_omega = gain_omega; }
    void set_mu(float mu) { d_mu = mu; }
    void set_omega(float omega);
    void set_omega_lim(float lim);
    void set_dllbw(float v) { d_dllbw=v; }
    void set_dllalfa(float v) { d_dllalfa=v; }

private:
    static constexpr float corr_alfa{0.001f};

    float d_mu;                   // fractional sample position [0.0, 1.0]
    float d_omega;                // nominal frequency
    float d_gain_omega;           // gain for adjusting omega
    float d_omega_relative_limit; // used to compute min and max omega
    float d_omega_mid;            // average omega
    float d_omega_lim;            // actual omega clipping limit
    float d_gain_mu;              // gain for adjusting mu

    gr::filter::mmse_fir_interpolator_cc d_interp;

    float d_e0acc{0.f};
    float d_e90acc{0.f};
    float d_e180acc{0.f};
    float d_e270acc{0.f};
    float d_l0acc{0.f};
    float d_l90acc{0.f};
    float d_l180acc{0.f};
    float d_l270acc{0.f};
    float d_c0acc{0.f};
    float d_c90acc{0.f};
    float d_c180acc{0.f};
    float d_c270acc{0.f};
    int d_skip{0};
    float d_corr0{0.0};
    float d_corr180{0.0};
    float d_dllbw{0.4f};
    float d_dllalfa{0.2f};
    float d_am_i{0.f};
    float d_am_q{0.f};

    gr_complex slicer_0deg(gr_complex sample)
    {
        float real = 0.0f, imag = 0.0f;

        if (sample.real() > 0.0f)
            real = 1.0f;
        if (sample.imag() > 0.0f)
            imag = 1.0f;
        return gr_complex(real, imag);
    }

};

#include <gnuradio/sync_block.h>

class bpsk_phase_sync_cc : public gr::sync_block
{
public:
#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<bpsk_phase_sync_cc> sptr;
#else
typedef std::shared_ptr<bpsk_phase_sync_cc> sptr;
#endif
    static sptr make(int block_size=16, float bw=0.1, float thr=0.1);
    bpsk_phase_sync_cc(int block_size, float bw, float thr);
    ~bpsk_phase_sync_cc() override;

    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items) override;

    float bw() const { return d_bw; }
    float thr() const { return d_threshold; }

    void set_bw(float bw);
    void set_thr(float thr) { d_threshold = thr; }
//    float get_frequency() const { return std::arg(d_incr)*float(d_size)*0.5f/float(M_PI); }
    float get_frequency() const { return std::arg(d_incr); }

private:
    float estimate(float phase, float incr, int len, const gr_complex * buf);
    float estimate(gr_complex phase, gr_complex incr, int len, const gr_complex * buf);
    gr_complex rotate(gr_complex in);
    void rotateN(gr_complex * out, const gr_complex * in,int n);
    gr_complex sum2(gr_complex incr, int len, const gr_complex * buf);
    float incr_estim(int block, int len, gr_complex incr, const gr_complex * buf);
    void set_phase(gr_complex in)
    {
        d_phase=in/std::abs(in);
        d_rot_n=0;
    }
    void set_incr(gr_complex in)
    {
        d_incr=in/std::abs(in);
    }

    gr_complex d_phase{1.f,0.f};
    gr_complex d_incr{1.f,0.f};
    int d_rot_n;

    int d_size;
    float d_coarse_incr_bw{M_PI*0.25};
    float d_bw;
    float d_scaled_bw;
    float d_threshold;
    gr_complex d_early;
    gr_complex d_late;
    std::vector<gr_complex> d_buf;
};

#endif /* INCLUDED_CLOCK_RECOVERY_H */
