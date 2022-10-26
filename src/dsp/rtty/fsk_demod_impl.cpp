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
#define ITEMS_MAX 16384

namespace gr {
    namespace rtty {

        fsk_demod_impl::fsk_demod_impl(float sample_rate, float symbol_rate, int decimation, float mark_freq, float space_freq) :
                    gr::sync_decimator("fsk_demod",
                                       gr::io_signature::make(1, 1, sizeof(gr_complex)),
                                       gr::io_signature::make(2, 2, sizeof(float)),
                                       decimation),
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
                    d_decimation(decimation)
        {

            int al = volk_get_alignment();
            d_mark = (gr_complex*)volk_malloc(sizeof(gr_complex)*(ITEMS_MAX+2),al);
            d_space = (gr_complex*)volk_malloc(sizeof(gr_complex)*(ITEMS_MAX+2),al);
            d_fmark = (float*)volk_malloc(sizeof(float)*(ITEMS_MAX+2),al);
            d_fspace = (float*)volk_malloc(sizeof(float)*(ITEMS_MAX+2),al);
            if(!(d_mark&&d_space&&d_fmark&&d_fspace))
                throw std::bad_alloc();
            set_mark_freq(d_mark_freq);
            set_space_freq(d_space_freq);
            update_history();
        }

        fsk_demod::sptr fsk_demod::make(float sample_rate, float symbol_rate, int decimation, float mark_freq, float space_freq) {
            return gnuradio::get_initial_sptr (new fsk_demod_impl(sample_rate, symbol_rate, decimation, mark_freq,space_freq));
        }

        fsk_demod_impl::~fsk_demod_impl() {
            volk_free(d_fspace);
            volk_free(d_fmark);
            volk_free(d_space);
            volk_free(d_mark);
        }

        void fsk_demod_impl::update_history()
        {
            set_history(std::max(d_mark_taps.size(), d_space_taps.size())+d_decimation*2);
        }

        void fsk_demod_impl::set_sample_rate(float sample_rate) {
            std::lock_guard<std::recursive_mutex> lock(d_mutex);

            d_sample_rate = sample_rate;

            set_mark_freq(d_mark_freq);
            set_space_freq(d_space_freq);
            update_history();
        }

        float fsk_demod_impl::sample_rate() const {
            return d_sample_rate;
        }

        void fsk_demod_impl::set_decimation(unsigned int decimation) {
            std::lock_guard<std::recursive_mutex> lock(d_mutex);
            d_decimation = decimation;

            gr::sync_decimator::set_decimation(decimation);

            set_mark_freq(d_mark_freq);
            set_space_freq(d_space_freq);
            update_history();
        }

        int fsk_demod_impl::decimation() const {
            return d_decimation;
        }

        void fsk_demod_impl::set_symbol_rate(float symbol_rate) {
            std::lock_guard<std::recursive_mutex> lock(d_mutex);

            d_symbol_rate = symbol_rate;

            set_mark_freq(d_mark_freq);
            set_space_freq(d_space_freq);
            update_history();
        }

        float fsk_demod_impl::symbol_rate() const {
            return d_symbol_rate;
        }

        void fsk_demod_impl::set_mark_freq(float mark_freq) {
            std::lock_guard<std::recursive_mutex> lock(d_mutex);

            if(std::abs(mark_freq)+d_symbol_rate<d_sample_rate*0.5)
            {
                d_mark_freq = mark_freq;
                double bw=d_symbol_rate;
                double tw=std::abs(d_mark_freq-d_space_freq)-bw*2.0;
                if(tw<bw*0.5)
                    tw=bw*0.5;
                d_mark_taps = gr::filter::firdes::complex_band_pass(1.0, d_sample_rate, d_mark_freq - bw, d_mark_freq + bw, tw,gr::filter::firdes::WIN_HANN);
                d_mark_fir.set_taps(d_mark_taps);
                update_history();
            }else{
                fprintf(stderr,"skipping fsk_demod_impl::set_mark_freq(%f) %f %f\n",mark_freq, d_symbol_rate,d_sample_rate);
            }
        }

        float fsk_demod_impl::mark_freq() const {
            return d_mark_freq;
        }

        void fsk_demod_impl::set_space_freq(float space_freq) {
            std::lock_guard<std::recursive_mutex> lock(d_mutex);

            if(std::abs(space_freq)+d_symbol_rate<d_sample_rate*0.5)
            {
                d_space_freq = space_freq;
                double bw=d_symbol_rate;
                double tw=std::abs(d_mark_freq-d_space_freq)-bw*2.0;
                if(tw<bw*0.5)
                    tw=bw*0.5;
                d_space_taps = gr::filter::firdes::complex_band_pass(1.0, d_sample_rate, d_space_freq - bw, d_space_freq + bw, tw,gr::filter::firdes::WIN_HANN);
                d_space_fir.set_taps(d_space_taps);
                update_history();
            }else{
                fprintf(stderr,"skipping fsk_demod_impl::set_space_freq(%f) %f %f\n",space_freq, d_symbol_rate,d_sample_rate);
            }
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
            const gr_complex * in = reinterpret_cast<const gr_complex*>(input_items[0]);

            if(noutput_items>ITEMS_MAX)
                noutput_items=ITEMS_MAX;
            d_mark_fir.filterNdec (d_mark, in, noutput_items+2, d_decimation);
            d_space_fir.filterNdec (d_space, in, noutput_items+2, d_decimation);

            #if 0
            volk_32fc_magnitude_32f(out0,d_mark,noutput_items);
            volk_32fc_magnitude_32f(out1,d_space,noutput_items);
            #else
            volk_32fc_s32f_power_spectrum_32f(d_fmark,d_mark,1.0,noutput_items+2);
            volk_32fc_s32f_power_spectrum_32f(d_fspace,d_space,1.0,noutput_items+2);
            for(int k=0;k<noutput_items+2;k++)
                d_fmark[k]+=150.0f;
            for(int k=0;k<noutput_items+2;k++)
                d_fspace[k]+=150.0f;
            for(int k=0;k<noutput_items;k++)
                if( (d_fmark[k]+d_fmark[k+2])*0.5f-d_fmark[k+1] > std::abs(d_fmark[k]-d_fmark[k+2])*2.0f)
                    out0[k]=(d_fmark[k]+d_fmark[k+2])*0.5f;
                else
                    out0[k]=d_fmark[k+1];
            for(int k=0;k<noutput_items;k++)
                if( (d_fspace[k]+d_fspace[k+2])*0.5f-d_fspace[k+1] > std::abs(d_fspace[k]-d_fspace[k+2])*2.0f)
                    out1[k]=(d_fspace[k]+d_fspace[k+2])*0.5f;
                else
                    out1[k]=d_fspace[k+1];
            #endif


            return noutput_items;
        }

    } /* namespace rtty */
} /* namespace gr */
