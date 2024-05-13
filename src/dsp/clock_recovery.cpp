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
    if (omega <= 0.f)
        throw std::out_of_range("clock rate must be > 0");
    if (gain_mu < 0.f || gain_omega < 0.f)
        throw std::out_of_range("Gains must be non-negative");

    set_omega(omega); // also sets min and max omega
    set_relative_rate(1.f/omega);
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

        int ei0=ii+(int)floorf(e_mu0);
        int li0=ii+(int)floorf(l_mu0);
        l_mu0 -= floorf(l_mu0);
        e_mu0 -= floorf(e_mu0);
        int ei90=ii+(int)floorf(e_mu90);
        int li90=ii+(int)floorf(l_mu90);
        int ci90=ii+(int)floorf(c_mu90);
        l_mu90 -= floorf(l_mu90);
        e_mu90 -= floorf(e_mu90);
        c_mu90 -= floorf(c_mu90);
        int ei180=ii+(int)floorf(e_mu180);
        int li180=ii+(int)floorf(l_mu180);
        int ci180=ii+(int)floorf(c_mu180);
        l_mu180 -= floorf(l_mu180);
        e_mu180 -= floorf(e_mu180);
        c_mu180 -= floorf(c_mu180);
        int ei270=ii+(int)floorf(e_mu270);
        int li270=ii+(int)floorf(l_mu270);
        int ci270=ii+(int)floorf(c_mu270);
        l_mu270 -= floorf(l_mu270);
        e_mu270 -= floorf(e_mu270);
        c_mu270 -= floorf(c_mu270);
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
        gr_complex outval=(d_corr0>=d_corr180)?d_interp.interpolate(&in[ii], d_mu):d_interp.interpolate(&in[ci180], c_mu180);
        out[oo++]=outval;
        d_am_i+=(log10f(std::max(std::abs(real(outval)),0.00001f))-d_am_i)*corr_alfa;
        d_am_q+=(log10f(std::max(std::abs(imag(outval)),0.00001f))-d_am_q)*corr_alfa;

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
    set_history(1+test_extra);
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

gr_complex bpsk_phase_sync_cc::sum2(gr_complex incr, int len, const gr_complex * buf)
{
    gr_complex acc=0;
    gr_complex ph_acc{1.f,0};
    if(incr==ph_acc)
        for(int k=0;k<len;k++)
        {
            const gr_complex m=buf[k];
            acc+=m*(m/std::abs(m));
        }
    else
        for(int k=0;k<len;k++)
        {
            const gr_complex m=buf[k]*ph_acc;
            ph_acc*=incr;
            acc+=m*(m/std::abs(m));
        }
    return acc;
}

float bpsk_phase_sync_cc::incr_estim(int block, int len, gr_complex incr, const gr_complex * buf)
{
    int n=len/block;
    gr_complex a=sum2(gr_complex(1.f,0),block,&buf[0]);
    float acc=0.f;
    float sc=1.f/float(block)*0.5f/float(n);
    float maga=0.f;
    for(int k=1;k<n;k++)
    {
        maga+=std::abs(a);
        gr_complex b=sum2(incr,block,&buf[k*block]);
        acc+=std::arg(b)*sc-std::arg(a)*sc;
        a=b;
    }
    printf("maga=%8.8f i=%8.8f\n",double(maga),double(acc));
    if(std::isfinite(acc))
        return acc;
    return 0.f;
}



int bpsk_phase_sync_cc::work(int noutput_items,
            gr_vector_const_void_star &input_items,
            gr_vector_void_star &output_items)
{
    const gr_complex* in = (const gr_complex*)input_items[0];
    gr_complex* out = (gr_complex*)output_items[0];
    gr_complex phase=d_phase;
    gr_complex incr=d_incr;
    int k=0;
    while(k<noutput_items)
    {
        float early=estimate(phase,incr*d_early,d_size,&in[k]);
        float late=estimate(phase,incr*d_late,d_size,&in[k]);
        float test=estimate(phase,incr,d_size+test_extra,&in[k]);
        float prompt=estimate(phase,incr,d_size,&in[k]);
        float dd=(log10f(late)-log10f(early));
        if(std::max(prompt,test)>1.f)//in sync
        //if(1)
        {
            d_phase=phase;
            //d_incr=incr;
            d_incr=incr*std::polar(1.f,d_scaled_bw*dd);
            d_incr/=std::abs(d_incr);
            rotateN(&out[k],&in[k],d_size);
            //float dd=late-early;
            //d_phase*=std::polar(1.f,d_bw*dd);
            k+=d_size;
        }else{
            #if 1
            //4 point coarse initial phase estimation search
            //TODO: switch to 3 point search?
            const float search_lim=0.1f/float(d_size);
            const float p0=std::arg(phase);
            const float p90=float(M_PI*0.5)+p0;
            const float p_90=float(-M_PI*0.5)+p0;
            const float p45=float(M_PI*0.25)+p0;
            const float p_45=float(-M_PI*0.25)+p0;
            float e0,e90,e45,e_45;
            #if 1
            float beste=0.f;
            int bestj=0;
            const float step=0.5f/float(d_size);
            const int NN=std::floor(0.005f/step);
            for(int j=-NN;j<=NN;j++)
            {
                e0=estimate(p0,float(j)*step,d_size,&in[k]);
                e90=estimate(p90,float(j)*step,d_size,&in[k]);
                e45=estimate(p45,float(j)*step,d_size,&in[k]);
                e_45=estimate(p_45,float(j)*step,d_size,&in[k]);
                float de=std::max(std::max(e0,e90),std::max(e45,e_45))-std::min(std::min(e0,e90),std::min(e45,e_45));
                if(de>beste)
                {
                    beste=de;
                    bestj=j;
                }
                //printf("de[%8.8f]=%8.8f\n",double(float(j)*step),double(de));
            }
            float start_inc=float(bestj)*step;
            printf("desync: %8.3f %d %8.3f\n",double(beste),bestj,log10(prompt)*10.);
            #else
            float start_inc=-incr_estim(32,d_size,gr_complex(1.f),in);
            //start_inc=-incr_estim(16,d_size,std::polar(1.f,start_inc),in);
            incr_estim(32,d_size,std::polar(1.f,0.f),&in[2]);
            incr_estim(32,d_size,std::polar(1.f,0.f),&in[4]);
            incr_estim(32,d_size,std::polar(1.f,0.f),&in[6]);
            start_inc=0.005;
            incr_estim(32,d_size,std::polar(1.f,start_inc),&in[6]);
            printf("desync: %8.3f %8.3f %8.3f %8.8f\n",log10(early)*10.,log10(prompt)*10.,log10(late)*10., double(start_inc));
            #endif
            e0=estimate(p0,start_inc,d_size,&in[k]);
            e90=estimate(p90,start_inc,d_size,&in[k]);
            e45=estimate(p45,start_inc,d_size,&in[k]);
            e_45=estimate(p_45,start_inc,d_size,&in[k]);
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
            //fine binary search of initial phase estimation
            while(a2-a1>search_lim)
            {
                pm=0.5f*(a1+a2);
                em=estimate(pm,start_inc,d_size,&in[k]);
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
            ea1=estimate(a1,start_inc,d_size,&in[k+d_size]);
            a2=pf+d_coarse_incr_bw+start_inc*d_size;
            ea2=estimate(a2,start_inc,d_size,&in[k+d_size]);
            //second block phase search to estimate the frequency
            while(a2-a1>search_lim)
            {
                pm=0.5f*(a1+a2);
                em=estimate(pm,start_inc,d_size,&in[k+d_size]);
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
            d_phase=std::polar(1.f,pf);
            d_incr=std::polar(1.f,found_incr);
            printf("desync: %8.8f\n", double(found_incr));
            rotateN(&out[k],&in[k],std::min(noutput_items-k,d_size));
            k+=d_size;
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
        }
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
