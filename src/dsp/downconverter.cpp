/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2020 Clayton Smith VE3IRR.
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
#include <math.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/pm_remez.h>
#include <gnuradio/io_signature.h>
#include <gnuradio/blocks/null_sink.h>
#include <chrono>

#include "downconverter.h"

#define LPF_CUTOFF 120e3

downconverter_cc_sptr make_downconverter_cc(unsigned int decim, double center_freq, double samp_rate)
{
    return gnuradio::get_initial_sptr(new downconverter_cc(decim, center_freq, samp_rate));
}

downconverter_cc::downconverter_cc(unsigned int decim, double center_freq, double samp_rate)
    : gr::hier_block2("downconverter_cc",
          gr::io_signature::make(1, 1, sizeof(gr_complex)),
          gr::io_signature::make(1, 1, sizeof(gr_complex))),
      d_decim(decim),
      d_center_freq(center_freq),
      d_samp_rate(samp_rate),
      d_proto_taps({1})
{
    connect_all();
    update_proto_taps();
    update_phase_inc();
}

downconverter_cc::~downconverter_cc()
{

}

void downconverter_cc::set_decim_and_samp_rate(unsigned int decim, double samp_rate)
{
    if((d_samp_rate==samp_rate)&&(decim==d_decim))
        return;
    d_samp_rate = samp_rate;
    if (decim != d_decim)
    {
        d_decim = decim;
        lock();
        disconnect_all();
        connect_all();
        unlock();
    }
    update_proto_taps();
    update_phase_inc();
}

void downconverter_cc::set_center_freq(double center_freq)
{
    d_center_freq = center_freq;
    update_phase_inc();
}

void downconverter_cc::connect_all()
{
    if (d_decim > 1)
    {
        filt = gr::filter::freq_xlating_fir_filter_ccf::make(d_decim, {1}, 0.0, d_samp_rate);
        connect(self(), 0, filt, 0);
        if(tap_filt)
        {
            connect(self(), 0, tap_filt, 0);
            connect(tap_filt,0,tap_block,0);
        }
        connect(filt, 0, self(), 0);
    }
    else
    {
        rot = gr::blocks::rotator_cc::make(0.0);
        connect(self(), 0, rot, 0);
        connect(rot, 0, self(), 0);
    }
}

void downconverter_cc::connect_fir_tap(gr::basic_block_sptr to)
{
    if(to)
    {
        if(!tap_filt)
        {
            tap_block=to;
            disconnect_all();
            tap_filt = gr::filter::freq_xlating_fir_filter_ccf::make(1, {1}, 0.0, 1.0);
            connect_all();
        }
    }else{
        if(tap_filt)
        {
            disconnect_all();
            tap_filt.reset();
            tap_block.reset();
            connect_all();
        }
    }
    update_proto_taps();
    update_phase_inc();
}

void downconverter_cc::update_proto_taps() noexcept
{
    if (d_decim > 1)
    {
    std::chrono::time_point<std::chrono::steady_clock> now = std::chrono::steady_clock::now();
        double out_rate = d_samp_rate / d_decim;
        if(d_wintype < 8)// 8 is equiripple filter
        {
            d_proto_taps = (d_att < 20.0)
                ?gr::filter::firdes::low_pass(1.0, d_samp_rate, LPF_CUTOFF, out_rate - 2.0 * LPF_CUTOFF, wintype(d_wintype), d_beta)
                :gr::filter::firdes::low_pass_2(1.0, d_samp_rate, LPF_CUTOFF, out_rate - 2.0 * LPF_CUTOFF, d_att, wintype(d_wintype), d_beta);
            std::chrono::duration<double> diff = std::chrono::steady_clock::now() - now;
            printf("downconverter_cc: taps.size=%ld in %lf us\n", d_proto_taps.size(),diff.count()*1000000.0);
        }else{
            d_proto_taps = (d_att < 20.0)
                ?gr::filter::firdes::low_pass(1.0, d_samp_rate, LPF_CUTOFF, out_rate - 2.0 * LPF_CUTOFF, wintype(6), d_beta)
                :gr::filter::firdes::low_pass_2(1.0, d_samp_rate, LPF_CUTOFF, out_rate - 2.0 * LPF_CUTOFF, d_att, wintype(6), d_beta);
            now = std::chrono::steady_clock::now();
            #if 0
            std::vector<double> coarse_taps=gr::filter::pm_remez(454, {0.0, 0.0075, 0.0175, 1.0}, {std::sqrt(2.0)/2.0, std::sqrt(2.0)/2.0, 0.0, 0.0}, {1.0, 46222.0}, "bandpass");
            std::vector<float> & fine_taps = d_proto_taps;
            unsigned sz=455.0*d_samp_rate/32000000.0;
            if(!(sz&1))
                sz|=1;
            if(sz<5)
                sz=5;
            fine_taps.resize(sz);
            unsigned cd=(sz-1)/2;
            unsigned cs=454/2;
            fine_taps[cd]=coarse_taps[cs];
            for(unsigned k=1;k<=cd;k++)
            {
                double dp=double(k)/double(sz)*455.0;
                unsigned ia=std::floor(dp);
                unsigned ib=std::ceil(dp);
                double scale=dp-double(ia);
                fine_taps[cd-k]=fine_taps[cd+k]=(coarse_taps[ia+cs]+(coarse_taps[ib+cs]-coarse_taps[ia+cs])*scale)*std::sqrt(32000000.0/double(d_samp_rate));
            }
            #else
            double att = 60.0;
            if(d_att >= 20.0)
                att = d_att;
            unsigned ntaps=(unsigned)(att * d_samp_rate / (22.0 * (out_rate - 2.0 * LPF_CUTOFF)));
            // pm_remez requires ntaps >= 5
            if(ntaps < 5)
                ntaps = 5;
            if(!(ntaps & 1))
                ntaps++;
            auto sb_dev = pow(10.0, -att * (d_beta / 5.8) / 20.0);
            auto pb_dev = (pow(10.0, d_ripple / 20.0) - 1.0) / (pow(10.0, d_ripple / 20.0) + 1.0);
            auto max_dev = std::max(sb_dev, pb_dev);
            sb_dev = max_dev / sb_dev;
            pb_dev = max_dev / pb_dev;
            auto pb_end = 2.0 * LPF_CUTOFF / d_samp_rate;
            auto sb_start = 2.0 * (out_rate - LPF_CUTOFF) / d_samp_rate;
            std::vector<double> remez_taps {};
            try{
                remez_taps = gr::filter::pm_remez(ntaps - 1, {0.0, pb_end, sb_start, 1.0}, {std::sqrt(2.0)/2.0, std::sqrt(2.0)/2.0, 0.0, 0.0}, {pb_dev, sb_dev}, "bandpass");
                if(d_proto_taps.size() < ntaps)
                    d_proto_taps.resize(ntaps);
                for(unsigned k = 0; k < ntaps; k++)
                    d_proto_taps[k] = remez_taps[k];
                std::chrono::duration<double> diff = std::chrono::steady_clock::now() - now;
                printf("downconverter_cc: taps.size=%d sb_dev=%lf pb_dev=%lf pb_end=%lf sb_start=%lf in %lf us\n", ntaps, sb_dev, pb_dev, pb_end, sb_start,diff.count()*1000000.0);
            }catch(...){
                std::chrono::duration<double> diff = std::chrono::steady_clock::now() - now;
                printf("FAILED downconverter_cc: taps.size=%d sb_dev=%lf pb_dev=%lf pb_end=%lf sb_start=%lf in %lf us\n", ntaps, sb_dev, pb_dev, pb_end, sb_start,diff.count()*1000000.0);
            }
            #endif
        }
        filt->set_taps(d_proto_taps);
        if(tap_filt)
            tap_filt->set_taps(d_proto_taps);
        d_proto_taps.resize(0);
    }
}

void downconverter_cc::update_phase_inc()
{
    if (d_decim > 1)
    {
        filt->set_center_freq(d_center_freq);
        if(tap_filt)
            tap_filt->set_center_freq(d_center_freq/d_samp_rate);
    }else
        rot->set_phase_inc(-2.0 * M_PI * d_center_freq / d_samp_rate);
}
