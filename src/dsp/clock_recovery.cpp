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

static constexpr int N_SAMPLE = 10;
static constexpr int FUDGE = N_SAMPLE+3;
static constexpr int s_size = 104;
//#define ONESHOT_CR_EL

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
    if (omega <= 0.f)
        throw std::out_of_range("clock rate must be > 0");
    if (gain_mu < 0.f || gain_omega < 0.f)
        throw std::out_of_range("Gains must be non-negative");

    set_omega(omega); // also sets min and max omega
    set_relative_rate(1.f/omega);
    pp[0].omega_scale=0.f;
    pp[0].omega_add=0.f;
    pp[1].omega_scale=0.5f;
    pp[1].omega_add=0.f;
    pp[2].omega_scale=1.0f;
    pp[2].omega_add=0.f;
    pp[3].omega_scale=1.5f;
    pp[3].omega_add=0.f;
    pp[4].omega_scale=0.5f;
    pp[4].omega_add=-d_dllbw;
    pp[5].omega_scale=0.5f;
    pp[5].omega_add=d_dllbw;
    pp[6].omega_scale=1.5f;
    pp[6].omega_add=-d_dllbw;
    pp[7].omega_scale=1.5f;
    pp[7].omega_add=d_dllbw;
    #ifdef ONESHOT_CR_EL
    set_history(d_interp.ntaps() + (s_size+FUDGE)*int(omega));
    set_output_multiple(s_size);
    #else
    set_history(d_interp.ntaps() + FUDGE*int(omega));
    #endif
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

float clock_recovery_el_cc::estimate(float mu, float omega, int n, const gr_complex * buf)
{
    int oo=0;
    int ii=0;
    float acc=0.f;
    while(oo<n)
    {
        ii += (int)floorf(mu);
        mu -= floorf(mu);
        if(!(oo&1))
            acc+=mag2(d_interp.interpolate(&buf[ii], mu));
        mu = mu + omega;
        oo++;
    }
    return acc/float(n);
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
    #ifdef ONESHOT_CR_EL
    int dm=ceilf(d_omega_mid*.5f);
    float corr_max=-1.f;
    int corr_k=0;
    bool corr180=false;
    float mu;
    float dllbw=d_dllbw/s_size;
    for(int k=-dm;k<=dm;k++)
    {
        mu=d_mu+k;
        d_c0acc=estimate(mu,d_omega_mid,s_size,&in[ii]);
        d_c90acc=estimate(mu+d_omega_mid*.5f,d_omega_mid,s_size,&in[ii]);
        d_c180acc=estimate(mu+d_omega_mid,d_omega_mid,s_size,&in[ii]);
        d_c270acc=estimate(mu+d_omega_mid*1.5f,d_omega_mid,s_size,&in[ii]);
        d_corr0=((d_c0acc==0.f)||(d_c90acc==0.f))?0.f:log10f(d_c0acc)-log10f(d_c90acc);
        d_corr180=((d_c180acc==0.f)||(d_c270acc==0.f))?0.f:log10f(d_c180acc)-log10f(d_c270acc);
        if(std::max(d_corr0,d_corr180)>corr_max)
        {
            corr_max=std::max(d_corr0,d_corr180);
            corr_k=k;
            corr180=d_corr0<d_corr180;
        }
    }
    d_mu+=corr_k;
    printf("corr_k=%d %8.2f\n",corr_k,corr_max);
    if(corr180)
    {
        d_e270acc=estimate(d_omega*1.5f+d_mu-dllbw,d_omega,s_size,&in[ii]);
        d_l270acc=estimate(d_omega*1.5f+d_mu+dllbw,d_omega,s_size,&in[ii]);
        mm_val=(d_e270acc-d_l270acc)*0.5f;
    }else{
        d_e90acc=estimate(d_omega*.5f+d_mu-dllbw,d_omega,s_size,&in[ii]);
        d_l90acc=estimate(d_omega*.5f+d_mu+dllbw,d_omega,s_size,&in[ii]);
        mm_val=d_e90acc-d_l90acc;
    }
    while (oo < s_size && ii < ni)
    {
        ii += (int)floorf(d_mu);
        d_mu -= floorf(d_mu);

        if (ii < 1) // clamp it.  This should only happen with bogus input
            ii = 1;
        float c_mu180=d_omega+d_mu;
        int ci180=ii+(int)floorf(c_mu180);
        c_mu180 -= floorf(c_mu180);
    
        if(out1)
            out1[oo] = d_skip^(d_corr0>=d_corr180);
        gr_complex outval=d_interp.interpolate(&in[ii], d_mu);
        out[oo++]=outval;
        d_mu = d_mu + d_omega;
    }
    d_omega = d_omega + d_gain_omega * mm_val;
    d_omega =
        d_omega_mid + gr::branchless_clip(d_omega - d_omega_mid, d_omega_lim);

    d_mu = d_mu + d_omega + gr::branchless_clip(d_gain_mu * mm_val, d_omega*0.25f);
    ii += (int)floorf(d_mu);
    d_mu -= floorf(d_mu);
    if (ii > 1)
    {
        assert(ii - 1<= ninput_items[0]);
        consume_each(ii-1);
    }
    #else
    
    pp[4].omega_add=-d_dllbw;
    pp[5].omega_add=d_dllbw;
    pp[6].omega_add=-d_dllbw;
    pp[7].omega_add=d_dllbw;
    while (oo < noutput_items && ii < ni)
    {
        for(int k=0;k<8;k++)
        {
            pp[k].mu=d_mu+d_omega*pp[k].omega_scale+pp[k].omega_add;
            pp[k].i=ii+(int)floorf(pp[k].mu);
            pp[k].i-=floorf(pp[k].mu);
        }
        if(d_skip)
        {
            d_skip=0;
            mm_val=0.f;
        }else{
            for(int k=0;k<8;k++)
                pp[k].acc+=(estimate(pp[k].mu,d_omega,N_SAMPLE,&in[pp[k].i])-pp[k].acc)*d_dllalfa;

            d_corr0+=((pp[0].acc==0.f)||(pp[1].acc==0.f))?0.f:(log10f(pp[0].acc)-log10f(pp[1].acc)-d_corr0)*corr_alfa;
            d_corr180+=((pp[2].acc==0.f)||(pp[3].acc==0.f))?0.f:(log10f(pp[2].acc)-log10f(pp[3].acc)-d_corr180)*corr_alfa;
            if(d_corr0>=d_corr180)
                mm_val=pp[4].acc-pp[5].acc;
            else
                mm_val=(pp[6].acc-pp[7].acc)*0.5f;
            d_skip=1;
        }
        if(out1)
        {
            if(std::abs(d_corr0-d_corr180)>corr_flip_threshold)
            {
                if(d_offs!=(d_corr0>=d_corr180))
                    printf("Flip !\n");
                d_offs=(d_corr0>=d_corr180);
            }
            out1[oo] = d_skip^d_offs;
        }
        gr_complex outval=d_interp.interpolate(&in[ii], d_mu);
        out[oo++]=outval;

        d_omega = d_omega + d_gain_omega * mm_val;
        d_omega =
            d_omega_mid + gr::branchless_clip(d_omega - d_omega_mid, d_omega_lim);

        d_mu = d_mu + d_omega + gr::branchless_clip(d_gain_mu * mm_val, d_omega*0.25f);
        ii += (int)floorf(d_mu);
        d_mu -= floorf(d_mu);

        if (ii < 1) // clamp it.  This should only happen with bogus input
            ii = 1;
    }

    if (ii > 1)
    {
        assert(ii - 1<= ninput_items[0]);
        consume_each(ii-1);
    }
    #endif

    return oo;
}


#include <volk/volk.h>
#define test_extra 128*8*3

bpsk_phase_sync_cc::sptr bpsk_phase_sync_cc::make(int block_size, float bw, float thr)
{
    return gnuradio::get_initial_sptr(new bpsk_phase_sync_cc(block_size,bw,thr));
}

bpsk_phase_sync_cc::bpsk_phase_sync_cc(int block_size, float bw, float thr): sync_block("bpsk_phase_sync_cc",
            gr::io_signature::make(1, 1, sizeof(gr_complex)),
            gr::io_signature::make(1, 1, sizeof(gr_complex)))
{
    d_size=block_size;
    set_bw(bw);
    d_threshold=thr;
    set_history(1+std::max(test_extra,d_size*2));
    set_output_multiple(d_size);
    d_buf.resize(d_size);
}

bpsk_phase_sync_cc::~bpsk_phase_sync_cc()
{
}

float bpsk_phase_sync_cc::estimate(float phase, float incr, int len, const gr_complex * buf)
{
    return estimate(std::polar(1.f,phase),std::polar(1.f,incr),len,buf);
}

float bpsk_phase_sync_cc::estimate(gr_complex phase, gr_complex incr, int len, const gr_complex * buf)
{
    float i_acc=0.f;
    float q_acc=0.f;
    set_phase(phase);
    set_incr(incr);
    for(int k=0;k<len;k++)
    {
        gr_complex x=rotate(buf[k]);
        i_acc+=std::abs(real(x));
        q_acc+=std::abs(imag(x));
    }
    return std::max(i_acc,0.00001f)/std::max(q_acc,0.00001f);
}

template<typename T> T max3(const T &a, const T &b, const T &c)
{
    return std::max(a,std::max(b,c));
}

bool bpsk_phase_sync_cc::phase_incr_oneshot(float & phase, float & incr, int size, const gr_complex * buf)
{
    #if 1
    //4 point coarse initial phase estimation search
    //TODO: switch to 3 point search?
    const float search_lim=d_scaled_bw;
    const int s_size=size;
    const float p0=phase;
    const float p90=float(M_PI*0.5)+p0;
    const float p_90=float(-M_PI*0.5)+p0;
    const float p45=float(M_PI*0.25)+p0;
    const float p_45=float(-M_PI*0.25)+p0;
    float e0,e90,e45,e_45;
    float beste=0.f;
    int bestj=0;
    const float step=0.5f/float(s_size);
    const int NN=std::floor(0.005f/step);
    for(int j=-NN;j<=NN;j++)
    {
        e0=estimate(p0,float(j)*step,s_size,buf);
        e90=estimate(p90,float(j)*step,s_size,buf);
        e45=estimate(p45,float(j)*step,s_size,buf);
        e_45=estimate(p_45,float(j)*step,s_size,buf);
        float de=std::max(std::max(e0,e90),std::max(e45,e_45))-std::min(std::min(e0,e90),std::min(e45,e_45));
        if(de>beste)
        {
            beste=de;
            bestj=j;
        }
        //printf("de[%8.8f]=%8.8f\n",double(float(j)*step),double(de));
    }
    float start_inc=float(bestj)*step;
    //printf("desync: %8.3f %d %8.3f\n",double(beste),bestj,log10(prompt)*10.);
    e0=estimate(p0,start_inc,s_size,buf);
    e90=estimate(p90,start_inc,s_size,buf);
    e45=estimate(p45,start_inc,s_size,buf);
    e_45=estimate(p_45,start_inc,s_size,buf);
    float a1=0.f;
    float a2=0.f;
    float ea1=0.f;
    float ea2=0.f;
    float pm,em;
    float pf;
    if(std::min(e_45,e0)>std::max(e90,e45))
    {
        a1=p_45;
        a2=p0;
        ea1=e_45;
        ea2=e0;
    }else
    if(std::min(e45,e0)>std::max(e90,e_45))
    {
        a1=p0;
        a2=p45;
        ea1=e0;
        ea2=e45;
    }else
    if(std::min(e45,e90)>std::max(e0,e_45))
    {
        a1=p45;
        a2=p90;
        ea1=e45;
        ea2=e90;
    }else
    //if(std::min(e_45,e90)>std::max(e0,e45))
    {
        a1=p_90;
        a2=p_45;
        ea1=e90;
        ea2=e_45;
    }
    #if 0
    if(e0>max3(e90,e45,e_45))
    {
        a1=p_45;
        a2=p45;
        ea1=e_45;
        ea2=e45;
        printf("j=%d\n",bestj);
    }else
    if(e90>max3(e0,e45,e_45))
    {
        a1=p45;
        a2=p_45;
        ea1=e45;
        ea2=e_45;
    }else
    if(e45>max3(e90,e0,e_45))
    {
        a1=p_45;
        a2=p45;
        ea1=e_45;
        ea2=e45;
    }else
//    if(e_45>max3(e90,e45,e0))
    {
        a1=p_90;
        a2=p0;
        ea1=e90;
        ea2=e0;
    }
    #endif
    //fine binary search of initial phase estimation
    while(a2-a1>search_lim)
    {
        pm=0.5f*(a1+a2);
        em=estimate(pm,start_inc,s_size,buf);
        //printf("pm=%8.8f em=%8.8f\n",double(pm),double(em));
        if(ea1>ea2)
        {
            ea2=em;
            a2=pm;
        }else{
            ea1=em;
            a1=pm;
        }
    }
    if(ea1>ea2)
    {
        pf=a1;
    }else{
        pf=a2;
    }
    a1=pf-d_coarse_incr_bw+start_inc*d_size;
    a2=pf+d_coarse_incr_bw+start_inc*d_size;
    ea1=estimate(a1,start_inc,s_size,&buf[d_size]);
    ea2=estimate(a2,start_inc,s_size,&buf[d_size]);
    //second block phase search to estimate the frequency
    while(a2-a1>search_lim)
    {
        pm=0.5f*(a1+a2);
        em=estimate(pm,start_inc,s_size,&buf[d_size]);
        //printf("incr=%8.8f em=%8.8f\n",double(pm-pf)/double(d_size),double(em));
        if(ea1>ea2)
        {
            ea2=em;
            a2=pm;
        }else{
            ea1=em;
            a1=pm;
        }
    }
    if(ea1>ea2)
    {
        em=ea1;
        pm=a1;
    }else{
        em=ea2;
        pm=a2;
    }
    float found_incr=(pm-pf)/float(d_size);
    pf+=0.5f*start_inc;
    pf-=0.5f*found_incr;
    phase=pf;
    incr=found_incr;
    #endif
    #if 0
    //4 point coarse initial phase estimation search
    //TODO: switch to 3 point search?
    const gr_complex p0=phase;
    const gr_complex p90=std::polar(1.f,float(M_PI*0.5))*phase;
    const gr_complex p45=std::polar(1.f,float(M_PI*0.25))*phase;
    const gr_complex p_45=std::polar(1.f,float(-M_PI*0.25))*phase;
    const float e0=estimate(p0,gr_complex(1.f,0),d_size,&in[k]);
    const float e90=estimate(p90,gr_complex(1.f,0),d_size,&in[k]);
    const float e45=estimate(p45,gr_complex(1.f,0),d_size,&in[k]);
    const float e_45=estimate(p_45,gr_complex(1.f,0),d_size,&in[k]);
    gr_complex a1;
    gr_complex a2;
    float ea1;
    float ea2;
    gr_complex pm;
    float em;
    gr_complex pf;
    if(std::min(e_45,e0)>std::max(e90,e45))
    {
        a1=p_45;
        a2=p0;
        ea1=e_45;
        ea2=e0;
    }else
    if(std::min(e45,e0)>std::max(e90,e_45))
    {
        a1=p0;
        a2=p45;
        ea1=e0;
        ea2=e45;
    }else
    if(std::min(e45,e90)>std::max(e0,e_45))
    {
        a1=p45;
        a2=p90;
        ea1=e45;
        ea2=e90;
    }else
    //if(std::min(e_45,e90)>std::max(e0,e45))
    {
        a1=std::polar(1.f,float(-M_PI*0.5))*phase;
        a2=p_45;
        ea1=e90;
        ea2=e_45;
    }
    //fine binary search of initial phase estimation
    int nn=0;
    while(std::abs(a2-a1)>d_scaled_bw)
    {
        pm=a1+a2;
        pm/=std::abs(pm);
        em=estimate(pm,0,d_size,&in[k]);
        if(ea1>ea2)
        {
            ea2=em;
            a2=pm;
        }else{
            ea1=em;
            a1=pm;
        }
        nn++;
    }
    printf("nn=%d\n",nn);
    if(ea1>ea2)
    {
        pf=a1;
    }else{
        pf=a2;
    }
    a1=pf*std::polar(1.f,-d_coarse_incr_bw);
    ea1=estimate(a1,0,d_size,&in[k+d_size]);
    a2=pf*std::polar(1.f,d_coarse_incr_bw);
    ea2=estimate(a2,0,d_size,&in[k+d_size]);
    //second block phase search to estimate the frequency
    while(std::abs(a2-a1)>d_scaled_bw)
    {
        pm=a1+a2;
        pm/=std::abs(pm);
        em=estimate(pm,0,&in[k+d_size]);
        if(ea1>ea2)
        {
            ea2=em;
            a2=pm;
        }else{
            ea1=em;
            a1=pm;
        }
    }
    if(ea1>ea2)
    {
        em=ea1;
        pm=a1;
    }else{
        em=ea2;
        pm=a2;
    }
    float found_incr=std::arg(pm)-std::arg(pf);
    found_incr/=float(d_size);
    pf*=std::polar(1.f,-0.5f*found_incr);
    d_phase=pf;
    d_incr=std::polar(1.f,found_incr);
    rotateN(&out[k],&in[k],d_size);
    k+=d_size;
    #endif
    return true;
}


int bpsk_phase_sync_cc::work(int noutput_items,
            gr_vector_const_void_star &input_items,
            gr_vector_void_star &output_items)
{
    const gr_complex* in = (const gr_complex*)input_items[0];
    gr_complex* out = (gr_complex*)output_items[0];
    int k=0;
    while(k<noutput_items)
    {
        gr_complex phase=d_phase;
        gr_complex incr=d_incr;
        float early=estimate(phase,incr*d_early,d_size*2.f,&in[k]);
        float late=estimate(phase,incr*d_late,d_size*2.f,&in[k]);
        float test=estimate(phase,incr,d_size+test_extra,&in[k]);
        float prompt=estimate(phase,incr,d_size,&in[k]);
        float dd=(log10f(late)-log10f(early));
        float e_phase=std::arg(phase),e_incr=std::arg(incr);
        if(std::max(prompt,test)>1.f)//in sync
//        if(0)
        {
            //phase_incr_oneshot(e_phase,e_incr,d_size,&in[k]);
            //d_incr=incr;
            d_phase=phase;//*std::polar(1.f,d_scaled_bw*dd);
            d_incr=incr*std::polar(1.f,d_scaled_bw*dd);
            d_incr/=std::abs(d_incr);
            //float dd=late-early;
            //d_phase*=std::polar(1.f,d_bw*dd);
//            printf("ok: %8.8f %8.8f\n", double(std::arg(d_phase)-e_phase),double(std::arg(d_incr)-e_incr));
        }else{
            if(phase_incr_oneshot(e_phase,e_incr,d_size*2,&in[k]))
            {
                float newtest=estimate(e_phase,e_incr,d_size+test_extra,&in[k]);
                if(newtest>test)
                {
                    //printf("resync: %8.8f\n", double(std::arg(d_phase)-e_phase));
                    d_phase=std::polar(1.f,e_phase);
                    d_incr=std::polar(1.f,e_incr);
                }
            }
        }
        d_freq=d_incr;
        const int remaining=std::min(noutput_items-k,d_size);
        rotateN(&out[k],&in[k],remaining);
        k+=remaining;
    }
    return noutput_items;
}

gr_complex bpsk_phase_sync_cc::rotate(gr_complex in)
{
    d_rot_n++;

    gr_complex z = in * d_phase; // rotate in by phase
    d_phase *= d_incr;     // incr our phase (complex mult == add phases)

    if ((d_rot_n % 512) == 0)
        d_phase /=
            std::abs(d_phase); // Normalize to ensure multiplication is rotation

    return z;
}

void bpsk_phase_sync_cc::rotateN(gr_complex * out, const gr_complex * in,int n)
{
#ifdef APPLY_BROKEN_ROTATOR_WORKAROUND
        for (int i = 0; i < n; i++) {
            out[i] = rotate(in[i]);
        }
#else
        volk_32fc_s32fc_x2_rotator_32fc(out, in, d_incr, &d_phase, n);
#endif
}
void bpsk_phase_sync_cc::set_bw(float bw)
{
    d_bw = bw;
    d_scaled_bw=d_bw/float(d_size);
//    d_early=std::polar(1.f,-d_scaled_bw);
//    d_late=std::polar(1.f,d_scaled_bw);
    d_early=std::polar(1.f,-0.6f/float(d_size));
    d_late= std::polar(1.f, 0.6f/float(d_size));
//    printf("bpsk_phase_sync_cc::set_bw %8.8f => %8.8f %8.8f %8.8f\n",bw,d_scaled_bw,imag(d_early),imag(d_late));
}
