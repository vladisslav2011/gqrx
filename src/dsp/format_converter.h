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

class any_to_any_base: virtual public gr::sync_block
{
public:
#if GNURADIO_VERSION < 0x030900
    typedef boost::shared_ptr<any_to_any_base> sptr;
#else
    typedef std::shared_ptr<any_to_any_base> sptr;
#endif

    void set_decimation(unsigned decimation)
    {
        d_decimation=decimation;
    }

    void set_interpolation(unsigned interpolation)
    {
        d_interpolation=interpolation;
    }

    unsigned decimation()
    {
        return d_decimation;
    }

    unsigned interpolation()
    {
        return d_interpolation;
    }

    int fixed_rate_ninput_to_noutput(int ninput) override
    {
        return std::max(0, ninput - (int)history() + 1) * interpolation() / decimation();
    }

    int fixed_rate_noutput_to_ninput(int noutput) override
    {
        return noutput * decimation() / interpolation() + history() - 1;
    }

    virtual void convert(const void *in, void *out, int noutput_items) = 0;

    int general_work(int noutput_items,
                                    gr_vector_int& ninput_items,
                                    gr_vector_const_void_star& input_items,
                                    gr_vector_void_star& output_items) override
    {
        int r = work(noutput_items, input_items, output_items);
        if (r > 0)
            consume_each(r * decimation() / interpolation());
        return r;
    }
    private:
    unsigned d_decimation;
    unsigned d_interpolation;
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

    static sptr make(dispatcher::tag<any_to_any<gr_complex, std::array<int8_t,5>>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(512.0f,2,1));
    }

    static sptr make(dispatcher::tag<any_to_any<std::array<int8_t,5>, gr_complex>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(512.0f,1,2));
    }

    static sptr make(dispatcher::tag<any_to_any<gr_complex, std::array<int8_t,3>>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(2048.0f));
    }

    static sptr make(dispatcher::tag<any_to_any<std::array<int8_t,3>, gr_complex>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(2048.0f));
    }

    static sptr make(dispatcher::tag<any_to_any<gr_complex, std::array<int8_t,7>>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(8192.0f,2,1));
    }

    static sptr make(dispatcher::tag<any_to_any<std::array<int8_t,7>, gr_complex>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(8192.0f,1,2));
    }

    any_to_any(const double scale, unsigned decimation=1, unsigned interpolation=1):sync_block("any_to_any",
                gr::io_signature::make (1, 1, sizeof(T_IN)),
                gr::io_signature::make (1, 1, sizeof(T_OUT)))
    {
        d_scale = scale;
        d_scale_i = 1.0 / scale;
        set_decimation(decimation);
        set_interpolation(interpolation);
        set_output_multiple(interpolation);
    }
    int work(int noutput_items, gr_vector_const_void_star &input_items,
    gr_vector_void_star &output_items) override
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

    void convert(const gr_complex *in, std::array<int8_t,5> * out, int noutput_items)
    {
        while(noutput_items)
        {
            int i;
            int q;
            uint8_t * p = (uint8_t *) &(*out)[0];
            i = std::round(in->real()*d_scale);
            q = std::round(in->imag()*d_scale);
            in++;
            p[0] = i & 0xff;
            p[1] = ((i & 0x0300) >> 8) | ((q & 0x3f) << 2);
            p[2] = (q >> 6) & 0x0f;
            i = std::round(in->real()*d_scale);
            q = std::round(in->imag()*d_scale);
            in++;
            p[2] |= (i & 0x0f) << 4;
            p[3] = ((i & 0x3f0) >> 4) | ((q & 0x03) << 6);
            p[4] = q >> 2;
            out++;
            noutput_items--;
        }
    }

    void convert(const std::array<int8_t,5> *in, gr_complex * out, int noutput_items)
    {
        noutput_items&=-2;
        while(noutput_items)
        {
            int i;
            int q;
            uint8_t * p = (uint8_t *) &(*in)[0];
            i = p[0] | ((p[1] & 0x03) << 8);
            q = (p[1] >> 2) | ((p[2] & 0x0f) << 6);
            *out=gr_complex(float((i&(1<<9))?i-1024:i)*d_scale_i, float((q&(1<<9))?q-1024:q)*d_scale_i);
            out++;
            i = (p[2] >> 4) | ((p[3] & 0x3f) << 4);
            q = (p[3] >> 6) | (p[4] << 2);
            *out=gr_complex(float((i&(1<<9))?i-1024:i)*d_scale_i, float((q&(1<<9))?q-1024:q)*d_scale_i);
            out++;
            in++;
            noutput_items-=2;
        }
    }

    void convert(const gr_complex *in, std::array<int8_t,3> * out, int noutput_items)
    {
        while(noutput_items)
        {
            int i;
            int q;
            uint8_t * p = (uint8_t *) &(*out)[0];
            i = std::round(in->real()*d_scale);
            q = std::round(in->imag()*d_scale);
            in++;
            p[0] = i & 0xff;
            p[1] = (i & 0x0f00) >> 8 | (q & 0x0f)<<4;
            p[2] = q >> 4;
            out++;
            noutput_items--;
        }
    }

    void convert(const std::array<int8_t,3> *in, gr_complex * out, int noutput_items)
    {
        while(noutput_items)
        {
            int i;
            int q;
            uint8_t * p = (uint8_t *) &(*in)[0];
            i = p[0] | (p[1] & 0x0f) << 8;
            q = p[1] >> 4 | p[2] << 4;
            *out=gr_complex(float((i&(1<<11))?i-4096:i)*d_scale_i, float((q&(1<<11))?q-4096:q)*d_scale_i);
            out++;
            in++;
            noutput_items--;
        }
    }

    void convert(const gr_complex *in, std::array<int8_t,7> * out, int noutput_items)
    {
        while(noutput_items)
        {
            int i;
            int q;
            uint8_t * p = (uint8_t *) &(*out)[0];
            i = std::round(in->real()*d_scale);
            q = std::round(in->imag()*d_scale);
            in++;
            p[0] = i & 0xff;
            p[1] = ((i & 0x3f00) >> 8) | ((q & 0x03) << 6);
            p[2] = (q >> 2) & 0xff;
            p[3] = (q >> 10) & 0x0f;
            i = std::round(in->real()*d_scale);
            q = std::round(in->imag()*d_scale);
            in++;
            p[3] |= (i & 0x0f) << 4;
            p[4] = (i >> 4) & 0xff;
            p[5] = ((i >> 12) & 0x03) | ((q & 0x3f) << 2);
            p[6] = q >> 6;
            out++;
            noutput_items--;
        }
    }

    void convert(const std::array<int8_t,7> *in, gr_complex * out, int noutput_items)
    {
        noutput_items&=-2;
        while(noutput_items)
        {
            int i;
            int q;
            uint8_t * p = (uint8_t *) &(*in)[0];
            i = p[0] | ((p[1] & 0x3f) << 8);
            q = (p[1] >> 6) | (p[2] << 2) | ((p[3] & 0x0f) << 10);
            *out=gr_complex(float((i&(1<<13))?i-16384:i)*d_scale_i, float((q&(1<<13))?q-16384:q)*d_scale_i);
            out++;
            i = (p[3] >> 4) | (p[4] << 4) | ((p[5] & 0x03) << 12);
            q = (p[5] >> 2) | (p[6] << 6);
            *out=gr_complex(float((i&(1<<13))?i-16384:i)*d_scale_i, float((q&(1<<13))?q-16384:q)*d_scale_i);
            out++;
            in++;
            noutput_items-=2;
        }
    }


};

#endif /* INCLUDED_FORMAT_CONVERTER_H */
