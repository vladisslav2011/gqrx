/* -*- c++ -*- */
/*
 * Copyright 2005,2006,2010-2012,2014 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */


#include "clock_recovery.h"
#include <gnuradio/io_signature.h>
#include <gnuradio/math.h>
#include <gnuradio/prefs.h>
#include <iomanip>
#include <sstream>
#include <stdexcept>

static constexpr int FUDGE = 16;

clock_recovery_el_cc::sptr clock_recovery_el_cc::make(
    float omega, float gain_omega, float mu, float gain_mu, float omega_relative_limit)
{
    return gnuradio::get_initial_sptr(new clock_recovery_el_cc(
        omega, gain_omega, mu, gain_mu, omega_relative_limit));
}

clock_recovery_el_cc::clock_recovery_el_cc(
    float omega, float gain_omega, float mu, float gain_mu, float omega_relative_limit)
    : block("clock_recovery_el_cc",
            gr::io_signature::make(1, 1, sizeof(gr_complex)),
            gr::io_signature::make2(1, 2, sizeof(gr_complex), sizeof(uint8_t))),
      d_mu(mu),
      d_omega(omega),
      d_gain_omega(gain_omega),
      d_omega_relative_limit(omega_relative_limit),
      d_gain_mu(gain_mu)
{
    if (omega <= 0.0)
        throw std::out_of_range("clock rate must be > 0");
    if (gain_mu < 0 || gain_omega < 0)
        throw std::out_of_range("Gains must be non-negative");

    set_omega(omega); // also sets min and max omega
    set_relative_rate(1.0/omega);
    set_history(d_interp.ntaps() + 2*(FUDGE + int(omega)));
    enable_update_rate(true); // fixes tag propagation through variable rate block
}

clock_recovery_el_cc::~clock_recovery_el_cc() {}

void clock_recovery_el_cc::forecast(int noutput_items,
                                         gr_vector_int& ninput_items_required)
{
    unsigned ninputs = ninput_items_required.size();
    for (unsigned i = 0; i < ninputs; i++)
        ninput_items_required[i] =
            (int)ceil((noutput_items * d_omega)) + history();
}

void clock_recovery_el_cc::set_omega(float omega)
{
    d_omega = omega;
    d_omega_mid = omega;
    d_omega_lim = d_omega_relative_limit * omega;
}

void clock_recovery_el_cc::set_omega_lim(float lim)
{
    d_omega_relative_limit = lim;
    d_omega_lim = d_omega_relative_limit * d_omega_mid;
}


inline float mag2(const gr_complex & in)
{
    //return std::abs(in);
    return in.real()*in.real()+in.imag()*in.imag();
}

int clock_recovery_el_cc::general_work(int noutput_items,
                                            gr_vector_int& ninput_items,
                                            gr_vector_const_void_star& input_items,
                                            gr_vector_void_star& output_items)
{
    const gr_complex* in = (const gr_complex*)input_items[0];
    gr_complex* out = (gr_complex*)output_items[0];
    uint8_t* out1 = (output_items.size()>1)?(uint8_t*)output_items[1]:nullptr;

    int ii = 1;                                          // input index
    int oo = 0;                                          // output index
    int ni = ninput_items[0] - history(); // don't use more input than this

    assert(d_mu >= 0.0);
    assert(d_mu <= 1.0);

    float mm_val = 0;
    constexpr float corr_alfa=0.001f;

    while (oo < noutput_items && ii < ni)
    {
        float e_mu0=d_mu-d_dllbw;
        float l_mu0=d_mu+d_dllbw;
        float e_mu90=d_omega*.5f+e_mu0;
        float l_mu90=d_omega*.5f+l_mu0;
        float c_mu90=d_omega*.5f+d_mu;
        float e_mu180=d_omega+e_mu0;
        float l_mu180=d_omega+l_mu0;
        float c_mu180=d_omega+d_mu;
        float e_mu270=d_omega*1.5f+e_mu0;
        float l_mu270=d_omega*1.5f+l_mu0;
        float c_mu270=d_omega*1.5f+d_mu;

        int ei0=ii+(int)floor(e_mu0);
        int li0=ii+(int)floor(l_mu0);
        l_mu0 -= floor(l_mu0);
        e_mu0 -= floor(e_mu0);
        int ei90=ii+(int)floor(e_mu90);
        int li90=ii+(int)floor(l_mu90);
        int ci90=ii+(int)floor(c_mu90);
        l_mu90 -= floor(l_mu90);
        e_mu90 -= floor(e_mu90);
        c_mu90 -= floor(c_mu90);
        int ei180=ii+(int)floor(e_mu180);
        int li180=ii+(int)floor(l_mu180);
        int ci180=ii+(int)floor(c_mu180);
        l_mu180 -= floor(l_mu180);
        e_mu180 -= floor(e_mu180);
        c_mu180 -= floor(c_mu180);
        int ei270=ii+(int)floor(e_mu270);
        int li270=ii+(int)floor(l_mu270);
        int ci270=ii+(int)floor(c_mu270);
        l_mu270 -= floor(l_mu270);
        e_mu270 -= floor(e_mu270);
        c_mu270 -= floor(c_mu270);
        if(d_skip)
        {
            d_skip=0;
            mm_val=0.f;
        }else{
            d_c0acc+=(mag2(d_interp.interpolate(&in[ii], d_mu))-d_c0acc)*d_dllalfa;
            d_c90acc+=(mag2(d_interp.interpolate(&in[ci90], c_mu90))-d_c90acc)*d_dllalfa;
            d_c180acc+=(mag2(d_interp.interpolate(&in[ci180], c_mu180))-d_c180acc)*d_dllalfa;
            d_c270acc+=(mag2(d_interp.interpolate(&in[ci270], c_mu270))-d_c270acc)*d_dllalfa;

            d_e0acc+=(mag2(d_interp.interpolate(&in[ei0], e_mu0))-d_e0acc)*d_dllalfa;
            d_e90acc+=(mag2(d_interp.interpolate(&in[ei90], e_mu90))-d_e90acc)*d_dllalfa;
            d_l0acc+=(mag2(d_interp.interpolate(&in[li0], l_mu0))-d_l0acc)*d_dllalfa;
            d_l90acc+=(mag2(d_interp.interpolate(&in[li90], l_mu90))-d_l90acc)*d_dllalfa;

            d_e180acc+=(mag2(d_interp.interpolate(&in[ei180], e_mu180))-d_e180acc)*d_dllalfa;
            d_e270acc+=(mag2(d_interp.interpolate(&in[ei270], e_mu270))-d_e270acc)*d_dllalfa;
            d_l180acc+=(mag2(d_interp.interpolate(&in[li180], l_mu180))-d_l180acc)*d_dllalfa;
            d_l270acc+=(mag2(d_interp.interpolate(&in[li270], l_mu270))-d_l270acc)*d_dllalfa;
            d_corr0+=((d_c0acc==0.f)||(d_c90acc==0.f))?0.f:(log10f(d_c0acc)-log10f(d_c90acc)-d_corr0)*corr_alfa;
            d_corr180+=((d_c180acc==0.f)||(d_c270acc==0.f))?0.f:(log10f(d_c180acc)-log10f(d_c270acc)-d_corr180)*corr_alfa;
            #if 0
            if(d_corr0>=d_corr180)
                mm_val=(d_l0acc-d_l90acc)-(d_e0acc-d_e90acc);
            else
                mm_val=(d_l180acc-d_l270acc)-(d_e180acc-d_e270acc);
            #else
            if(d_corr0>=d_corr180)
                mm_val=d_e90acc-d_l90acc;
            else
                mm_val=(d_e270acc-d_l270acc)*0.5f;
            #endif
            d_skip=1;
        }
        if(out1)
            out1[oo] = d_skip?0:1;
        if(d_corr0>=d_corr180)
        {
            out[oo++] = d_interp.interpolate(&in[ii], d_mu);
        }else{
            out[oo++] = d_interp.interpolate(&in[ci180], c_mu180);
        }

        d_omega = d_omega + d_gain_omega * mm_val;
        d_omega =
            d_omega_mid + gr::branchless_clip(d_omega - d_omega_mid, d_omega_lim);

        d_mu = d_mu + d_omega + gr::branchless_clip(d_gain_mu * mm_val, d_omega*0.25f);
        ii += (int)floor(d_mu);
        d_mu -= floor(d_mu);

        if (ii < 1) // clamp it.  This should only happen with bogus input
            ii = 1;
    }

    if (ii > 1)
    {
        assert(ii - 1<= ninput_items[0]);
        consume_each(ii-1);
    }

    return oo;
}
