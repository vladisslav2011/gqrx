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

static const int FUDGE = 16;

clock_recovery_el_cc::sptr clock_recovery_el_cc::make(
    float omega, float gain_omega, float mu, float gain_mu, float omega_relative_limit)
{
    return gnuradio::get_initial_sptr(new clock_recovery_el_cc(
        omega, gain_omega, mu, gain_mu, omega_relative_limit));
}

clock_recovery_el_cc::clock_recovery_el_cc(
    float omega, float gain_omega, float mu, float gain_mu, float omega_relative_limit)
    : block("clock_recovery_mm_cc",
            gr::io_signature::make(1, 1, sizeof(gr_complex)),
            gr::io_signature::make2(1, 1, sizeof(gr_complex), sizeof(float))),
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
    set_history(d_interp.ntaps() + FUDGE + 2*int(omega));
    enable_update_rate(true); // fixes tag propagation through variable rate block
}

clock_recovery_el_cc::~clock_recovery_el_cc() {}

void clock_recovery_el_cc::forecast(int noutput_items,
                                         gr_vector_int& ninput_items_required)
{
    unsigned ninputs = ninput_items_required.size();
    for (unsigned i = 0; i < ninputs; i++)
        ninput_items_required[i] =
            (int)ceil((noutput_items * d_omega) + d_interp.ntaps()) + FUDGE;
}

void clock_recovery_el_cc::set_omega(float omega)
{
    d_omega = omega;
    d_omega_mid = omega;
    d_omega_lim = d_omega_relative_limit * omega;
}

constexpr float mag2(const gr_complex & in)
{
    return in.real()*in.real()+in.imag()*in.imag();
}

int clock_recovery_el_cc::general_work(int noutput_items,
                                            gr_vector_int& ninput_items,
                                            gr_vector_const_void_star& input_items,
                                            gr_vector_void_star& output_items)
{
    const gr_complex* in = (const gr_complex*)input_items[0];
    gr_complex* out = (gr_complex*)output_items[0];

    int ii = 1;                                          // input index
    int oo = 0;                                          // output index
    int ni = ninput_items[0] - d_interp.ntaps() - FUDGE; // don't use more input than this

    assert(d_mu >= 0.0);
    assert(d_mu <= 1.0);

    float mm_val = 0;
    constexpr float dllbw=0.4f;
    constexpr float dllalfa=0.1f;

    while (oo < noutput_items && ii < ni)
    {
        float e_mu0=d_mu-dllbw;
        float l_mu0=d_mu+dllbw;
        float e_mu90=d_omega*.5f+d_mu-dllbw;
        float l_mu90=d_omega*.5f+d_mu+dllbw;
        int ei0=1;
        int li0=1;
        int ei90=1+(int)floor(e_mu90);
        int li90=1+(int)floor(l_mu90);
        l_mu90 -= floor(l_mu90);
        e_mu90 -= floor(e_mu90);
        if(e_mu0<0.f)
        {
            e_mu0+=1.f;
            ei0=0;
        }
        if(l_mu0>1.f)
        {
            li0+=1;
            l_mu0-=1;
        }
        d_e0acc+=(mag2(d_interp.interpolate(&in[ei0], e_mu0))-d_e0acc)*dllalfa;
        d_e90acc+=(mag2(d_interp.interpolate(&in[ei90], e_mu90))-d_e90acc)*dllalfa;
        d_l0acc+=(mag2(d_interp.interpolate(&in[li0], l_mu0))-d_l0acc)*dllalfa;
        d_l90acc+=(mag2(d_interp.interpolate(&in[li90], l_mu90))-d_l90acc)*dllalfa;
        mm_val=(d_l0acc-d_l90acc)-(d_e0acc-d_e90acc);
        out[oo++] = d_interp.interpolate(&in[ii], d_mu);

        d_omega = d_omega + d_gain_omega * mm_val;
        d_omega =
            d_omega_mid + gr::branchless_clip(d_omega - d_omega_mid, d_omega_lim);

        d_mu = d_mu + d_omega + d_gain_mu * mm_val;
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