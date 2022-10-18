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

        fsk_demod_impl::fsk_demod_impl(float sample_rate, unsigned int decimation, float symbol_rate, float mark_freq, float space_freq) :
                    gr::sync_decimator("fsk_demod",
                                       gr::io_signature::make(1, 1, sizeof(gr_complex)),
                                       gr::io_signature::make(1, 1, sizeof(float)),
                                       decimation ),
#if GNURADIO_VERSION < 0x030900
                    d_mark_fir(1,{}),
                    d_space_fir(1,{}),
#else
                    d_mark_fir({}),
                    d_space_fir({}),
#endif
                    d_sample_rate(sample_rate),
                    d_decimation(decimation),
                    d_symbol_rate(symbol_rate),
                    d_mark_freq(mark_freq),
                    d_space_freq(space_freq) {

            d_rrc_taps = gr::filter::firdes::root_raised_cosine(1.0, d_sample_rate, d_symbol_rate, FSK_DEMOD_ALPHA, FSK_DEMOD_NTAPS);
            set_history(d_rrc_taps.size());

            set_mark_freq(d_mark_freq);
            set_space_freq(d_space_freq);
        }

        fsk_demod::sptr fsk_demod::make(float sample_rate, unsigned int decimation, float symbol_rate, float mark_freq, float space_freq) {
            return gnuradio::get_initial_sptr (new fsk_demod_impl(sample_rate, decimation, symbol_rate, mark_freq,space_freq));
        }

        fsk_demod_impl::~fsk_demod_impl() {
        }

        void fsk_demod_impl::set_sample_rate(float sample_rate) {
            std::lock_guard<std::recursive_mutex> lock(d_mutex);

            d_sample_rate = sample_rate;

            d_rrc_taps = gr::filter::firdes::root_raised_cosine(1.0, d_sample_rate, d_symbol_rate, FSK_DEMOD_ALPHA, FSK_DEMOD_NTAPS);
            set_history(d_rrc_taps.size());

            set_mark_freq(d_mark_freq);
            set_space_freq(d_space_freq);

        }

        float fsk_demod_impl::sample_rate() const {
            return d_sample_rate;
        }

        void fsk_demod_impl::set_decimation(unsigned int decimation) {
            std::lock_guard<std::recursive_mutex> lock(d_mutex);
            d_decimation = decimation;

            gr::sync_decimator::set_decimation(decimation);

            d_rrc_taps = gr::filter::firdes::root_raised_cosine(1.0, d_sample_rate, d_symbol_rate, FSK_DEMOD_ALPHA, FSK_DEMOD_NTAPS);
            set_history(d_rrc_taps.size());

            set_mark_freq(d_mark_freq);
            set_space_freq(d_space_freq);
        }

        int fsk_demod_impl::decimation() const {
            return d_decimation;
        }

        void fsk_demod_impl::set_symbol_rate(float symbol_rate) {
            std::lock_guard<std::recursive_mutex> lock(d_mutex);

            d_symbol_rate = symbol_rate;

            d_rrc_taps = gr::filter::firdes::root_raised_cosine(1.0, d_sample_rate, d_symbol_rate, FSK_DEMOD_ALPHA, FSK_DEMOD_NTAPS);
            set_history(d_rrc_taps.size());

            set_mark_freq(d_mark_freq);
            set_space_freq(d_space_freq);
        }

        float fsk_demod_impl::symbol_rate() const {
            return d_symbol_rate;
        }

        void fsk_demod_impl::set_mark_freq(float mark_freq) {
            std::lock_guard<std::recursive_mutex> lock(d_mutex);

            std::vector<gr_complex> ctaps(d_rrc_taps.size());
            float fwT0 = 2* GR_M_PI * (mark_freq/d_sample_rate);


            for (unsigned int i=0; i< d_rrc_taps.size(); i++) {
                ctaps[i] = d_rrc_taps[i] * exp(gr_complex(0, i * fwT0));
            }

            d_mark_fir.set_taps(ctaps);
            d_mark_freq = mark_freq;

        }

        float fsk_demod_impl::mark_freq() const {
            return d_mark_freq;
        }

        void fsk_demod_impl::set_space_freq(float space_freq) {
            std::lock_guard<std::recursive_mutex> lock(d_mutex);

            std::vector<gr_complex> ctaps(d_rrc_taps.size());
            float fwT0 = 2* GR_M_PI * (space_freq/d_sample_rate);


            for (unsigned int i=0; i< d_rrc_taps.size(); i++) {
                ctaps[i] = d_rrc_taps[i] * exp(gr_complex(0, i * fwT0));
            }

            d_space_fir.set_taps(ctaps);
            d_space_freq = space_freq;

        }

        float fsk_demod_impl::space_freq() const {
            return d_space_freq;
        }

        int fsk_demod_impl::work (int noutput_items,
                gr_vector_const_void_star& input_items,
                gr_vector_void_star& output_items) {

            std::lock_guard<std::recursive_mutex> lock(d_mutex);
            float * out = reinterpret_cast<float*>(output_items[0]);
            const gr_complex * in = reinterpret_cast<const gr_complex*>(input_items[0]);
            int al = volk_get_alignment();
            std::vector<gr_complex> vmark(noutput_items+al);
            std::vector<gr_complex> vspace(noutput_items+al);
            std::vector<float> vmark_mag(noutput_items+al);
            std::vector<float> vspace_mag(noutput_items+al);
            std::vector<float> vsum(noutput_items+al);
            gr_complex * mark;
            gr_complex * space;
	    float * mark_mag;
	    float * space_mag;
	    float * sum;

            mark =   (gr_complex*)((((size_t)vmark.data())+(al-1)) & ~(al-1));
            space =  (gr_complex*)((((size_t)vspace.data())+(al-1)) & ~(al-1));
            mark_mag =  (float*)((((size_t)vmark_mag.data())+(al-1)) & ~(al-1));
            space_mag =  (float*)((((size_t)vspace_mag.data())+(al-1)) & ~(al-1));
            sum =  (float*)((((size_t)vsum.data())+(al-1)) & ~(al-1));

            int i,j;

            for (i=0,j=0;i<noutput_items;i++,j+=d_decimation) {
                mark[i] = d_mark_fir.filter(&in[j]);
                space[i] = d_space_fir.filter(&in[j]);
            }
	    
            volk_32fc_magnitude_squared_32f(mark_mag,mark,noutput_items);
            volk_32fc_magnitude_squared_32f(space_mag,space,noutput_items);

            // Normalize level
#if VOLK_VERSION < 020300
            for (i=0;i<noutput_items;i++) {
                mark_mag[i]+=1e-31f;
                space_mag[i]+=1e-31f;
            }
#else
            volk_32f_s32f_add_32f(mark_mag,mark_mag,1e-31f,noutput_items); // Avoid divde by 0
            volk_32f_s32f_add_32f(space_mag,space_mag,1e-31f,noutput_items); // Avoid divde by 0
#endif
            volk_32f_x2_add_32f(sum,mark_mag,space_mag,noutput_items);
            volk_32f_x2_divide_32f(mark_mag,mark_mag,sum,noutput_items);        
            volk_32f_x2_divide_32f(space_mag,space_mag,sum,noutput_items);        

            volk_32f_x2_subtract_32f(out,mark_mag,space_mag,noutput_items);


            return noutput_items;
        }

    } /* namespace rtty */
} /* namespace gr */
