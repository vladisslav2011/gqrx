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

    float mu() const { return d_mu; }
    float omega() const { return d_omega; }
    float gain_mu() const { return d_gain_mu; }
    float gain_omega() const { return d_gain_omega; }

    void set_gain_mu(float gain_mu) { d_gain_mu = gain_mu; }
    void set_gain_omega(float gain_omega) { d_gain_omega = gain_omega; }
    void set_mu(float mu) { d_mu = mu; }
    void set_omega(float omega);

private:
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
    float d_l0acc{0.f};
    float d_l90acc{0.f};

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

#endif /* INCLUDED_CLOCK_RECOVERY_H */
