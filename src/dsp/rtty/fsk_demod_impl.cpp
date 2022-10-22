/*
 * fsk/afsk demodulator block implementation
 *
 * Copyright 2022 Marc CAPDEVILLE F4JMZ
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include <gnuradio/io_signature.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/math.h>
#include <complex>
#include <volk/volk.h>
#include "fsk_demod_impl.h"

#ifndef GR_M_PI
#define GR_M_PI   3.14159265358979323846 /* pi */
#endif

#define FSK_DEMOD_ALPHA    (0.35)
#define FSK_DEMOD_NTAPS (3*d_sample_rate/d_symbol_rate)

namespace gr {
    namespace rtty {

        fsk_demod_impl::fsk_demod_impl(float sample_rate, float symbol_rate, float mark_freq, float space_freq) :
                    gr::sync_block("fsk_demod",
                                       gr::io_signature::make(1, 1, sizeof(gr_complex)),
                                       gr::io_signature::make(4, 4, sizeof(float))),
#if GNURADIO_VERSION < 0x030900
                    d_mark_fir(1,{}),
                    d_space_fir(1,{}),
#else
                    d_mark_fir({}),
                    d_space_fir({}),
#endif
                    d_sample_rate(sample_rate),
                    d_symbol_rate(symbol_rate),
                    d_mark_freq(mark_freq),
                    d_space_freq(space_freq),
                    d_symbol_size(std::round(d_sample_rate/d_symbol_rate))
        {

            set_mark_freq(d_mark_freq);
            set_space_freq(d_space_freq);
            update_history();
            fd[0]=fopen("/home/vlad/iq/fsk0.raw","w+");
            fd[1]=fopen("/home/vlad/iq/fsk1.raw","w+");
            fd[2]=fopen("/home/vlad/iq/fsk2.raw","w+");
            fd[3]=fopen("/home/vlad/iq/fsk3.raw","w+");
        }

        fsk_demod::sptr fsk_demod::make(float sample_rate, float symbol_rate, float mark_freq, float space_freq) {
            return gnuradio::get_initial_sptr (new fsk_demod_impl(sample_rate, symbol_rate, mark_freq,space_freq));
        }

        fsk_demod_impl::~fsk_demod_impl() {
            for(int p=0;p<4;p++)
                if(fd[p])
                    fclose(fd[p]);
        }

        void fsk_demod_impl::update_history()
        {
            set_history(std::max(d_mark_taps.size(), d_space_taps.size()) + d_symbol_size);
        }

        void fsk_demod_impl::set_sample_rate(float sample_rate) {
            std::lock_guard<std::recursive_mutex> lock(d_mutex);

            d_sample_rate = sample_rate;
            d_symbol_size=std::round(d_sample_rate/d_symbol_rate);

            set_mark_freq(d_mark_freq);
            set_space_freq(d_space_freq);
            update_history();
        }

        float fsk_demod_impl::sample_rate() const {
            return d_sample_rate;
        }

        void fsk_demod_impl::set_symbol_rate(float symbol_rate) {
            std::lock_guard<std::recursive_mutex> lock(d_mutex);

            d_symbol_rate = symbol_rate;
            d_symbol_size=std::round(d_sample_rate/d_symbol_rate);

            set_mark_freq(d_mark_freq);
            set_space_freq(d_space_freq);
            update_history();
        }

        float fsk_demod_impl::symbol_rate() const {
            return d_symbol_rate;
        }

        void fsk_demod_impl::set_mark_freq(float mark_freq) {
            std::lock_guard<std::recursive_mutex> lock(d_mutex);

            d_mark_freq = mark_freq;
            d_mark_taps = gr::filter::firdes::complex_band_pass(1.0, d_sample_rate, d_mark_freq - d_symbol_rate, d_mark_freq + d_symbol_rate, d_symbol_rate/4);
            d_mark_fir.set_taps(d_mark_taps);
            update_history();
        }

        float fsk_demod_impl::mark_freq() const {
            return d_mark_freq;
        }

        void fsk_demod_impl::set_space_freq(float space_freq) {
            std::lock_guard<std::recursive_mutex> lock(d_mutex);

            d_space_freq = space_freq;
            d_space_taps = gr::filter::firdes::complex_band_pass(1.0, d_sample_rate, d_space_freq - d_symbol_rate, d_space_freq + d_symbol_rate, d_symbol_rate/4);
            d_space_fir.set_taps(d_space_taps);
            update_history();
        }

        float fsk_demod_impl::space_freq() const {
            return d_space_freq;
        }

        int fsk_demod_impl::work (int noutput_items,
                gr_vector_const_void_star& input_items,
                gr_vector_void_star& output_items) {

            std::lock_guard<std::recursive_mutex> lock(d_mutex);
            float * out0 = reinterpret_cast<float*>(output_items[0]);
            float * out1 = reinterpret_cast<float*>(output_items[1]);
            float * out2 = reinterpret_cast<float*>(output_items[2]);
            float * out3 = reinterpret_cast<float*>(output_items[3]);
            const gr_complex * in = reinterpret_cast<const gr_complex*>(input_items[0]);
            int al = volk_get_alignment();
            gr_complex * mark = (gr_complex*)volk_malloc(sizeof(gr_complex)*(noutput_items+d_symbol_size),al);
            gr_complex * space = (gr_complex*)volk_malloc(sizeof(gr_complex)*(noutput_items+d_symbol_size),al);
            float * mark_mag = (float*)volk_malloc(sizeof(float)*(noutput_items+d_symbol_size),al);
            float * space_mag = (float*)volk_malloc(sizeof(float)*(noutput_items+d_symbol_size),al);

            int i;

            for (i=0;i<noutput_items+d_symbol_size;i++) {
                mark[i] = d_mark_fir.filter(&in[i]);
                space[i] = d_space_fir.filter(&in[i]);
            }

            volk_32fc_magnitude_32f(mark_mag,mark,noutput_items+d_symbol_size);
            volk_32fc_magnitude_32f(space_mag,space,noutput_items+d_symbol_size);
//            volk_32fc_s32f_power_spectrum_32f(mark_mag,mark,1.0,noutput_items+d_symbol_size);
//            volk_32fc_s32f_power_spectrum_32f(space_mag,space,1.0,noutput_items+d_symbol_size);

            for (i=0;i<noutput_items;i++) {
                volk_32f_stddev_and_mean_32f_x2(&out0[i],&out1[i],&mark_mag[i],d_symbol_size/2);
                volk_32f_stddev_and_mean_32f_x2(&out2[i],&out3[i],&space_mag[i],d_symbol_size/2);
                /*int k;
                float acc;
                float macc;
                int n=d_symbol_size/2;
                float nm=1.0/float(n);
                acc=0;
                for(k=0;k<n;k++)
                    acc+=mark_mag[i+k];
                acc*=nm;
                macc=0;
                for(k=0;k<n;k++)
                    macc+=std::abs(mark_mag[i+k]-acc);
                macc*=nm;
                out0[i]=macc;
                out1[i]=acc;

                acc=0;
                for(k=0;k<n;k++)
                    acc+=space_mag[i+k];
                acc*=nm;
                macc=0;
                for(k=0;k<n;k++)
                    macc+=std::abs(space_mag[i+k]-acc);
                macc*=nm;
                out2[i]=macc;
                out3[i]=acc;*/
            }

            int p = 0;
            for(p=0;p<4;p++)
                if(fd[p])
                        fwrite(output_items[p],sizeof(float),noutput_items,fd[p]);
            volk_free(space_mag);
            volk_free(mark_mag);
            volk_free(space);
            volk_free(mark);

            return noutput_items;
        }

    } /* namespace rtty */
} /* namespace gr */
