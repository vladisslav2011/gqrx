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
#include <QMessageBox>
#include <QFileDialog>
#include <cmath>
#include <gnuradio/io_signature.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/blocks/wavfile_sink.h>
#include <gnuradio/blocks/complex_to_imag.h>
#include <gnuradio/blocks/complex_to_real.h>
#include <gnuradio/blocks/complex_to_magphase.h>
#include <iostream>
#include <stdio.h>
#include <stdarg.h>
#include "dsp/rx_rds.h"
#include <volk/volk.h>

#if GNURADIO_VERSION >= 0x030800
#include <gnuradio/digital/timing_error_detector_type.h>
#include <gnuradio/blocks/multiply_const.h>
#else
#include <gnuradio/blocks/multiply_const_ff.h>
#endif


class iir_corr : public gr::sync_block
{
protected:
    iir_corr(int period, double alfa):
        gr::sync_block ("iir_corr",
        gr::io_signature::make(1, 1, sizeof(gr_complex)),
        gr::io_signature::make(1, 1, sizeof(gr_complex))),
        d_alfa(alfa),
        d_ialfa(1.0-alfa),
        d_period(period),
        d_p(0),
        d_b(d_period)
    {
        for(int k=0;k<d_period;k++)
            d_b[k]=0.0;
    }

public:
#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<iir_corr> sptr;
#else
typedef std::shared_ptr<iir_corr> sptr;
#endif
    ~iir_corr()
    {
    }

    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items) override
    {
        const gr_complex *in = (const gr_complex *) input_items[0];
        gr_complex *out = (gr_complex *) output_items[0];
        for(int k=0;k<noutput_items;k++)
        {
            out[k]=d_b[d_p]=d_b[d_p]*d_ialfa+in[k]*d_alfa;
            d_p++;
            if(d_p>=d_period)
                d_p=0;
        }
        return noutput_items;
    }

    static sptr make(int period, double alfa)
    {
        return gnuradio::get_initial_sptr(new iir_corr(period,alfa));
    }

private:
    gr_complex      d_alfa;
    gr_complex      d_ialfa;
    int             d_period;
    int d_p;
    std::vector<gr_complex> d_b;
};

class dbpsk_det_cb: public gr::sync_decimator
{
protected:
    dbpsk_det_cb():
        gr::sync_decimator ("dbpsk_det_cb",
        gr::io_signature::make2(2, 2, sizeof(gr_complex),sizeof(uint8_t)),
        gr::io_signature::make(1, 1, sizeof(uint8_t)),2)
    {
//        set_output_multiple(2);
        set_history(8);
    }
public:
#if GNURADIO_VERSION < 0x030900
typedef boost::shared_ptr<dbpsk_det_cb> sptr;
#else
typedef std::shared_ptr<dbpsk_det_cb> sptr;
#endif
    ~dbpsk_det_cb()
    {
    }

    static sptr make()
    {
        return gnuradio::get_initial_sptr(new dbpsk_det_cb());
    }
    int work(int noutput_items,
             gr_vector_const_void_star &input_items,
             gr_vector_void_star &output_items) override
    {
        const gr_complex *in = (const gr_complex *) input_items[0];
        const uint8_t *in1 = (const uint8_t *) input_items[1];
        uint8_t *out = (uint8_t *) output_items[0];
        for(int k=0;k<noutput_items;k++)
        {
            const gr_complex *b=&in[k*2];
            if(in1[k*2])
                b++;
            #if 0
            constexpr float fv=0.6f;
            d_iir+=(real(b[0])-d_iir)*fv;
            int tmp=real(b[1])>d_iir;
            d_iir+=(real(b[1])-d_iir)*fv;
            out[k]=d_prev^tmp;
            d_prev=tmp;
            #elif 0
            auto m1_0=b[0]-b[1]-b[2]+b[3];
            auto m0_0=b[0]-b[1]+b[2]-b[3];
            int f0=std::abs(m1_0)>std::abs(m0_0);
            out[k]=f0;
           #else
            constexpr float fv=0.0f;
            float iir=d_iir;
            float p[4];
            for(int j=0;j<4;j++)
            {
                p[j]=real(b[j])-iir;
                iir+=(real(b[j])-iir)*fv;
            }
            d_iir+=(real(b[0])-d_iir)*fv;
            d_iir+=(real(b[1])-d_iir)*fv;
            auto m1_0=p[0]-p[1]-p[2]+p[3];
            auto m0_0=p[0]-p[1]+p[2]-p[3];
            auto f0=std::abs(m1_0)-std::abs(m0_0);
            out[k]=f0>0.f;
            #endif
            d_i_mag+=(log10f(std::max(std::abs(real(b[0])),0.0001f))*10.f-d_i_mag)*corr_alfa;
            d_q_mag+=(log10f(std::max(std::abs(imag(b[0])),0.0001f))*10.f-d_q_mag)*corr_alfa;
            d_i_mag+=(log10f(std::max(std::abs(real(b[1])),0.0001f))*10.f-d_i_mag)*corr_alfa;
            d_q_mag+=(log10f(std::max(std::abs(imag(b[1])),0.0001f))*10.f-d_q_mag)*corr_alfa;
        }
        return noutput_items;
    }

    float snr() const
    {
        return d_i_mag-d_q_mag;
    }


private:
    static constexpr float corr_alfa{0.001f};

    float d_iir{0.f};
    int d_prev{0};
    float d_i_mag{0.f};
    float d_q_mag{0.f};
};







class soft_bpsk: public gr::digital::constellation_bpsk
{
public:
    typedef boost::shared_ptr<soft_bpsk> sptr;

    // public constructor
    static sptr make()
    {
        return sptr(new soft_bpsk());
    }

    ~soft_bpsk(){};

    unsigned int decision_maker(const gr_complex* sample) override
    {
        constexpr float fv=.61f;
        acc+=(real(*sample)-acc)*fv;
        return real(*sample)>acc;
    }

protected:
    soft_bpsk(): constellation_bpsk()
    {
    };
    float acc{0.f};
};

static const int MIN_IN = 1;  /* Minimum number of input streams. */
static const int MAX_IN = 1;  /* Maximum number of input streams. */
static const int MIN_OUT = 1; /* Minimum number of output streams. */
static const int MAX_OUT = 1; /* Maximum number of output streams. */

/*
 * Create a new instance of rx_rds and return
 * a shared_ptr. This is effectively the public constructor.
 */
rx_rds_sptr make_rx_rds(double sample_rate, bool encorr)
{
    return gnuradio::get_initial_sptr(new rx_rds(sample_rate, encorr));
}

rx_rds::rx_rds(double sample_rate, bool encorr)
    : gr::hier_block2 ("rx_rds",
                      gr::io_signature::make (MIN_IN, MAX_IN, sizeof (float)),
                      gr::io_signature::make (MIN_OUT, MAX_OUT, sizeof (char))),
      d_sample_rate(sample_rate)
{
#if (GNURADIO_VERSION < 0x030800) || NEW_RDS
    constexpr int decim1=10;
#else
    constexpr int decim1=10;
#endif
    if (sample_rate < 128000.0) {
        throw std::invalid_argument("RDS sample rate not supported");
    }

//    d_fxff_tap = gr::filter::firdes::low_pass(1, d_sample_rate, 5500, 3500);
    d_fxff_tap = gr::filter::firdes::band_pass(1, d_sample_rate,
        2375.f/2.f-d_fxff_bw, 2375.f/2.f+d_fxff_bw, d_fxff_tw/*,gr::filter::firdes::WIN_BLACKMAN_HARRIS*/);
    d_fxff = gr::filter::freq_xlating_fir_filter_fcf::make(decim1, d_fxff_tap, 57000, d_sample_rate);
    update_fxff_taps();

#if GNURADIO_VERSION < 0x030900
    const double rate = (double) d_interpolation / (double) d_decimation;
    d_rsmp_tap = gr::filter::firdes::low_pass(d_interpolation, d_interpolation, rate * 0.45, rate * 0.1);
    d_rsmp = gr::filter::rational_resampler_base_ccf::make(d_interpolation, d_decimation, d_rsmp_tap);
#else
    d_rsmp = gr::filter::rational_resampler_ccf::make(d_interpolation, d_decimation);
#endif

    int n_taps = 151*5;
#if (GNURADIO_VERSION < 0x030800) || NEW_RDS
    d_rrcf = gr::filter::firdes::root_raised_cosine(1, ((float)d_sample_rate*d_interpolation)/(d_decimation*decim1), 2375.0, 1.2, n_taps);
#else
    d_rrcf = gr::filter::firdes::root_raised_cosine(1, ((float)d_sample_rate*d_interpolation)/(d_decimation*decim1), 2375.0, 1.0, n_taps);
#endif
//    auto tmp_rrcf=gr::filter::firdes::root_raised_cosine(1, (d_sample_rate*float(d_interpolation))/float(d_decimation*decim1), 2375.0*0.5, 1, n_taps);
//    volk_32f_x2_add_32f(d_rrcf.data(),d_rrcf.data(),tmp_rrcf.data(),n_taps);
    d_rrcf_manchester = std::vector<float>(n_taps-8);
    for (int n = 0; n < n_taps-8; n++) {
        d_rrcf_manchester[n] = d_rrcf[n] - d_rrcf[n+8];
    }

    auto corr=iir_corr::make(((float)d_sample_rate*d_interpolation*104.f*2.f)/(d_decimation*2375.f*decim1),0.1);
    int agc_samp = ((float)d_sample_rate*d_interpolation*0.8f)/(d_decimation*23750.f);

    d_costas_loop = gr::digital::costas_loop_cc::make(powf(10.f,-2.8f),2);
    //d_costas_loop->set_damping_factor(0.85);
    d_costas_loop->set_min_freq(-0.0003f*float(decim1));
    d_costas_loop->set_max_freq(0.0003f*float(decim1));
#if (GNURADIO_VERSION < 0x030800) || NEW_RDS
    gr::digital::constellation_sptr p_c = soft_bpsk::make()->base();
    d_bpf = gr::filter::fir_filter_ccf::make(1, d_rrcf);

    d_agc = make_rx_agc_cc(0,40, agc_samp, 0, agc_samp, 0);

    d_sync = clock_recovery_el_cc::make(((float)d_sample_rate*d_interpolation)/(d_decimation*(2375.f*decim1)), d_gain_omega, 0.5, d_gain_mu, d_omega_lim);

    d_koin = gr::blocks::keep_one_in_n::make(sizeof(unsigned char), 2);
#else
    gr::digital::constellation_sptr p_c = gr::digital::constellation_bpsk::make()->base();
    d_bpf = gr::filter::fir_filter_ccf::make(1, d_rrcf_manchester);

    d_agc = make_rx_agc_cc(0,40, agc_samp, 0, agc_samp*10, 0);

    d_sync = gr::digital::symbol_sync_cc::make(gr::digital::TED_ZERO_CROSSING,
        (d_sample_rate*d_interpolation*2.0)/(d_decimation*float(2375.f*decim1)), 0.01, 1, 1, 0.1, 1, p_c);
#endif

    d_mpsk = gr::digital::constellation_decoder_cb::make(p_c);

    d_ddbb = gr::digital::diff_decoder_bb::make(2);

    /* connect filter */
    connect(self(), 0, d_fxff, 0);
    if(d_decimation==d_interpolation)
    {
        connect(d_fxff, 0, d_agc, 0);
    }else{
        connect(d_fxff, 0, d_rsmp, 0);
        connect(d_rsmp, 0, d_agc, 0);
    }
#if 1
    connect(d_agc, 0, d_costas_loop, 0);
    connect(d_costas_loop, 0, d_bpf, 0);
#else
    connect(d_agc, 0, d_costas_loop, 0);
#endif
    if(encorr)
    {
        connect(d_bpf, 0, corr, 0);
        connect(corr, 0, d_sync, 0);
    }else
        connect(d_bpf, 0, d_sync, 0);
//    connect(d_agc, 0, d_sync, 0);
#if 0
    connect(d_sync, 0, d_mpsk, 0);

#if GNURADIO_VERSION < 0x030800
    connect(d_mpsk, 0, d_koin, 0);
    connect(d_koin, 0, d_ddbb, 0);
#else
    connect(d_mpsk, 0, d_ddbb, 0);
#endif

    connect(d_ddbb, 0, self(), 0);
#else
#if (GNURADIO_VERSION < 0x030800) || NEW_RDS
    d_det=dbpsk_det_cb::make();
    connect(d_sync, 0, d_det, 0);
    connect(d_sync, 1, d_det, 1);
    connect(d_det, 0, self(), 0);
#else
    connect(d_sync, 0, d_mpsk, 0);
    connect(d_mpsk, 0, d_ddbb, 0);
    connect(d_ddbb, 0, self(), 0);
#endif
#endif
    #if 0
    {
    auto w0=gr::blocks::wavfile_sink::make("/home/vlad/agc.wav",2,19000);
    auto cl0=gr::digital::costas_loop_cc::make(2.0*M_PI/2000.0,2);
    auto im0=gr::blocks::complex_to_imag::make();
    auto re0=gr::blocks::complex_to_real::make();
    connect(d_agc,0,cl0,0);
    connect(cl0,0,re0,0);
    connect(cl0,0,im0,0);
    connect(re0,0,w0,0);
    connect(im0,0,w0,1);
    }
    #endif
    #if 0
    {
    auto w1=gr::blocks::wavfile_sink::make("/home/vlad/agc.wav",2,19000);
    auto bb = gr::blocks::complex_to_magphase::make();
    auto mc=gr::blocks::multiply_const_ff::make(1.0/M_PI/2.0);
    connect(d_agc,0,bb,0);
    connect(bb,0,w1,0);
    connect(bb,1,mc,0);
    connect(mc,0,w1,1);
    }
    #endif
    #if 0
    {
    auto w2=gr::blocks::wavfile_sink::make("/home/vlad/rrcf_manchester.wav",2,19000);
    auto im2=gr::blocks::complex_to_imag::make();
    auto re2=gr::blocks::complex_to_real::make();
    auto bpf_manc=gr::filter::fir_filter_ccf::make(1, d_rrcf_manchester);
    auto cl2=gr::digital::costas_loop_cc::make(2.0*M_PI/20000.0,2);
    cl2->set_min_freq(-0.005);
    cl2->set_max_freq(0.005);
    connect(d_rsmp,0,bpf_manc,0);
    connect(bpf_manc,0,cl2,0);
    connect(cl2,0,re2,0);
    connect(cl2,0,im2,0);
    connect(re2,0,w2,0);
    connect(im2,0,w2,1);
    }
    #endif
    #if 0
    {
    auto w1=gr::blocks::wavfile_sink::make("/home/vlad/bpf.wav",2,19000);
    auto im1=gr::blocks::complex_to_imag::make();
    auto re1=gr::blocks::complex_to_real::make();
    auto cl1=gr::digital::costas_loop_cc::make(2.0*M_PI/20000.0,2);
    cl1->set_min_freq(-0.005);
    cl1->set_max_freq(0.005);
    connect(d_bpf,0,cl1,0);
    connect(cl1,0,re1,0);
    connect(cl1,0,im1,0);
    connect(re1,0,w1,0);
    connect(im1,0,w1,1);
    }
    #endif
    #if 0
    {
    auto w1=gr::blocks::wavfile_sink::make("/home/vlad/rsmp.wav",2,19000);
    auto bb = gr::blocks::complex_to_magphase::make();
    auto mc=gr::blocks::multiply_const_ff::make(1.0/M_PI/2.0);
    connect(d_rsmp,0,bb,0);
    connect(bb,0,w1,0);
    connect(bb,1,mc,0);
    connect(mc,0,w1,1);
    }
    #endif
    #if 0
    {
    auto w4=gr::blocks::wavfile_sink::make("/home/vlad/raw4.wav",2,19000.0/8.0);
    auto im4=gr::blocks::complex_to_imag::make();
    auto re4=gr::blocks::complex_to_real::make();
    auto cl4=gr::digital::costas_loop_cc::make(2.0*M_PI/200.0,2);
    connect(d_sync,0,cl4,0);
    connect(cl4,0,re4,0);
    connect(cl4,0,im4,0);
    connect(re4,0,w4,0);
    connect(im4,0,w4,1);
    }
    #endif
}

rx_rds::~rx_rds ()
{

}

void rx_rds::trig()
{
#if (GNURADIO_VERSION < 0x030800) || NEW_RDS
    changed_value(C_RDS_CR_OMEGA, d_index, d_sync->omega());
    changed_value(C_RDS_CR_MU, d_index, d_sync->mu());
    changed_value(C_RDS_CL_FREQ, d_index, d_costas_loop->get_frequency()*1000.f);
    changed_value(C_RDS_PHASE_SNR, d_index, phase_snr());
#else
    changed_value(C_RDS_CL_FREQ, d_index, d_costas_loop->get_frequency()*1000.);
#endif
}

void rx_rds::update_fxff_taps()
{
    //lock();
    #if 0
    d_fxff_tap = gr::filter::firdes::low_pass(1, d_sample_rate, d_fxff_bw+2375.f-1000.f, d_fxff_tw);
    #else
    d_fxff_tap = gr::filter::firdes::band_pass(1, d_sample_rate,
        2375.f/2.f-std::min(d_fxff_bw,1170.0f), 2375.f/2.f+std::min(d_fxff_bw,1170.0f), d_fxff_tw);
    #endif
    d_fxff->set_taps(d_fxff_tap);
    //unlock();
}

void rx_rds::set_omega_lim(float v)
{
#if (GNURADIO_VERSION < 0x030800) || NEW_RDS
    d_sync->set_omega_lim(d_omega_lim=v);
#endif
}

void rx_rds::set_dll_bw(float v)
{
#if (GNURADIO_VERSION < 0x030800) || NEW_RDS
    d_sync->set_dllbw(v);
#endif
}

void rx_rds::set_cl_bw(float v)
{
    d_costas_loop->set_loop_bandwidth(powf(10.f,v/10.f));
}

float rx_rds::phase_snr() const
{
#if (GNURADIO_VERSION < 0x030800) || NEW_RDS
    return dynamic_cast<dbpsk_det_cb *>(d_det.get())->snr();
#else
    return 0;
#endif
}
