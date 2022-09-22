/* -*- c++ -*- */
/*
 * Gqrx SDR: Software defined radio receiver powered by GNU Radio and Qt
 *           https://gqrx.dk/
 *
 * Copyright 2021 vladisslav2011@gmail.com.
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
#ifndef INCLUDED_FORMAT_CONVERTER_H
#define INCLUDED_FORMAT_CONVERTER_H

#include <gnuradio/blocks/api.h>
#include <gnuradio/sync_interpolator.h>
#include <volk/volk.h>
#include <iostream>

namespace dispatcher
{
    template <typename> struct tag{};
}

class any_to_any_base: virtual public gr::sync_interpolator
{
public:
#if GNURADIO_VERSION < 0x030900
    typedef boost::shared_ptr<any_to_any_base> sptr;
#else
    typedef std::shared_ptr<any_to_any_base> sptr;
#endif

    virtual void convert(const void *in, void *out, int noutput_items) = 0;
};
    /*!
     * \brief Convert stream of one format to a stream of another format.
     * \ingroup type_converters_blk
     *
     * \details
     * The output stream contains chars with twice as many output
     * items as input items. For every complex input item, we produce
     * two output chars that contain the real part and imaginary part
     * converted to chars:
     *
     * \li output[0][n] = static_cast<char>(input[0][m].real());
     * \li output[0][n+1] = static_cast<char>(input[0][m].imag());
     */
template <typename T_IN, typename T_OUT> class BLOCKS_API any_to_any : virtual public any_to_any_base
{
public:
#if GNURADIO_VERSION < 0x030900
    typedef boost::shared_ptr<any_to_any<T_IN, T_OUT>> sptr;
#else
    typedef std::shared_ptr<any_to_any<T_IN, T_OUT>> sptr;
#endif


    /*!
    * Build a any-to-any.
    */
    static sptr make(const double scale)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(scale));
    }

    static sptr make()
    {
        return make(dispatcher::tag<any_to_any>());
    }

    static sptr make(dispatcher::tag<any_to_any<gr_complex, gr_complex>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(1.0));
    }

    static sptr make(dispatcher::tag<any_to_any<gr_complex, std::complex<int32_t>>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT32_MIN)));
    }

    static sptr make(dispatcher::tag<any_to_any<std::complex<int32_t>, gr_complex>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT32_MIN)));
    }

    static sptr make(dispatcher::tag<any_to_any<gr_complex, std::complex<uint32_t>>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT32_MIN)));
    }

    static sptr make(dispatcher::tag<any_to_any<std::complex<uint32_t>, gr_complex>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT32_MIN)));
    }

    static sptr make(dispatcher::tag<any_to_any<gr_complex, std::complex<int16_t>>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT16_MIN)));
    }

    static sptr make(dispatcher::tag<any_to_any<std::complex<int16_t>, gr_complex>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT16_MIN)));
    }

    static sptr make(dispatcher::tag<any_to_any<gr_complex, std::complex<uint16_t>>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT16_MIN)));
    }

    static sptr make(dispatcher::tag<any_to_any<std::complex<uint16_t>, gr_complex>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT16_MIN)));
    }

    static sptr make(dispatcher::tag<any_to_any<gr_complex, std::complex<int8_t>>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT8_MIN)));
    }

    static sptr make(dispatcher::tag<any_to_any<std::complex<int8_t>, gr_complex>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT8_MIN)));
    }

    static sptr make(dispatcher::tag<any_to_any<gr_complex, std::complex<uint8_t>>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT8_MIN)));
    }

    static sptr make(dispatcher::tag<any_to_any<std::complex<uint8_t>, gr_complex>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT8_MIN)));
    }

    any_to_any(const double scale):sync_interpolator("any_to_any",
                gr::io_signature::make (1, 1, sizeof(T_IN)),
                gr::io_signature::make (1, 1, sizeof(T_OUT)), 1)
    {
        d_scale = scale;
        d_scale_i = 1.0 / scale;
   }
    int work(int noutput_items, gr_vector_const_void_star &input_items,
    gr_vector_void_star &output_items)
    {
        return work(noutput_items, input_items, output_items, dispatcher::tag<any_to_any>());
    }

    void convert(const void *in, void *out, int noutput_items) override
    {
        convert(in, out, noutput_items, dispatcher::tag<any_to_any>());
    }

private:
    float d_scale;
    float d_scale_i;

    template<typename U_IN, typename U_OUT> int work(int noutput_items, gr_vector_const_void_star &input_items,
    gr_vector_void_star &output_items, dispatcher::tag<any_to_any<U_IN, U_OUT>>)
    {
        const U_IN *in = (const U_IN *) input_items[0];
        U_OUT *out = (U_OUT *) output_items[0];

        convert(in, out, noutput_items);
        return noutput_items;
    }

    template<typename U_IN, typename U_OUT> void convert(const void *in, void *out, int noutput_items, dispatcher::tag<any_to_any<U_IN, U_OUT>>)
    {
        const U_IN *u_in = (const U_IN *) in;
        U_OUT *u_out = (U_OUT *) out;

        convert(u_in, u_out, noutput_items);
    }

    void convert(const gr_complex *in, gr_complex * out, int noutput_items)
    {
        memcpy(out, in, noutput_items * sizeof(gr_complex));
    }

    void convert(const gr_complex *in, std::complex<int32_t> * out, int noutput_items)
    {
        volk_32f_s32f_convert_32i((int32_t *)out, (const float *)in, d_scale, noutput_items * 2);
    }

    void convert(const std::complex<int32_t> *in, gr_complex * out, int noutput_items)
    {
        volk_32i_s32f_convert_32f((float *)out, (const int32_t *)in, d_scale, noutput_items * 2);
    }

    void convert(const gr_complex *in, std::complex<uint32_t> * out, int noutput_items)
    {
        volk_32f_s32f_convert_32i((int32_t *)out, (const float *)in, d_scale, noutput_items * 2);
        uint32_t *out_u = (uint32_t *) out;
        for(int k = 0;k < noutput_items;k++)
        {
            *(out_u++) += uint32_t(INT32_MAX) + 1;
            *(out_u++) += uint32_t(INT32_MAX) + 1;
        }
    }

    void convert(const std::complex<uint32_t> *in, gr_complex * out, int noutput_items)
    {

        for(int k = 0;k < noutput_items;k++,in++,out++)
        {
            *out = gr_complex((float(in->real())+float(INT32_MIN)) * d_scale_i, (float(in->imag())+float(INT32_MIN)) * d_scale_i);
        }
    }

    void convert(const gr_complex *in, std::complex<int16_t> * out, int noutput_items)
    {
        volk_32f_s32f_convert_16i((int16_t *)out, (const float *)in, d_scale, noutput_items * 2);
    }

    void convert(const std::complex<int16_t> *in, gr_complex * out, int noutput_items)
    {
        volk_16i_s32f_convert_32f((float *)out, (const int16_t *)in, d_scale, noutput_items * 2);
    }

    void convert(const gr_complex *in, std::complex<uint16_t> * out, int noutput_items)
    {
        uint16_t *out_u = (uint16_t *) out;
        volk_32f_s32f_convert_16i((int16_t *)out, (const float *)in, d_scale, noutput_items * 2);
        for(int k = 0;k < noutput_items;k++)
        {
            *(out_u++) += INT16_MAX + 1;
            *(out_u++) += INT16_MAX + 1;
        }
    }

    void convert(const std::complex<uint16_t> *in, gr_complex * out, int noutput_items)
    {
        for(int k = 0;k < noutput_items;k++,in++,out++)
        {
            *out = gr_complex((float(in->real())+float(INT16_MIN)) * d_scale_i, (float(in->imag())+float(INT16_MIN)) * d_scale_i);
        }
    }

    void convert(const gr_complex *in, std::complex<int8_t> * out, int noutput_items)
    {
        volk_32f_s32f_convert_8i((int8_t *)out, (const float *)in, d_scale, noutput_items * 2);
    }

    void convert(const std::complex<int8_t> *in, gr_complex * out, int noutput_items)
    {
        volk_8i_s32f_convert_32f((float *)out, (const int8_t *)in, d_scale, noutput_items * 2);
    }

    void convert(const gr_complex *in, std::complex<uint8_t> * out, int noutput_items)
    {
        uint8_t *out_u = (uint8_t *) out;
        volk_32f_s32f_convert_8i((int8_t *)out, (const float *)in, d_scale, noutput_items * 2);
        for(int k = 0;k < noutput_items;k++)
        {
            *(out_u++) += INT8_MAX + 1;
            *(out_u++) += INT8_MAX + 1;
        }
    }

    void convert(const std::complex<uint8_t> *in, gr_complex * out, int noutput_items)
    {
        for(int k = 0;k < noutput_items;k++,in++,out++)
        {
            *out = gr_complex((float(in->real())+float(INT8_MIN)) * d_scale_i, (float(in->imag())+float(INT8_MIN)) * d_scale_i);
        }
    }


};

#endif /* INCLUDED_FORMAT_CONVERTER_H */
