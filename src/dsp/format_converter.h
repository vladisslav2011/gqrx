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
#include <limits.h>
#include <volk/volk.h>
#include <array>
#include <x86intrin.h>

namespace dispatcher
{
    template <typename> struct tag{};
}

template <typename T_IN, typename T_OUT> class any_to_any_32;
template <typename T_IN, typename T_OUT> class any_to_any_bmi32;
template <typename T_IN, typename T_OUT> class any_to_any_64;
template <typename T_IN, typename T_OUT> class any_to_any_bmi64;

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
        set_output_multiple(interpolation);
    }

    void set_scale(float scale)
    {
        d_scale = scale;
        d_scale_i = 1.0 / scale;
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
    protected:
    float d_scale;
    float d_scale_i;
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

using any_to_any_base::sptr;

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
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(1.0,1,1,"f32f32c"));
    }

    static sptr make(dispatcher::tag<any_to_any<gr_complex, std::complex<int32_t>>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT32_MIN),1,1,"f32s32c"));
    }

    static sptr make(dispatcher::tag<any_to_any<std::complex<int32_t>, gr_complex>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT32_MIN),1,1,"s32f32c"));
    }

    static sptr make(dispatcher::tag<any_to_any<gr_complex, std::complex<uint32_t>>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT32_MIN),1,1,"f32u32c"));
    }

    static sptr make(dispatcher::tag<any_to_any<std::complex<uint32_t>, gr_complex>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT32_MIN),1,1,"u32f32c"));
    }

    static sptr make(dispatcher::tag<any_to_any<gr_complex, std::complex<int16_t>>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT16_MIN),1,1,"f32s16c"));
    }

    static sptr make(dispatcher::tag<any_to_any<std::complex<int16_t>, gr_complex>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT16_MIN),1,1,"s16f32c"));
    }

    static sptr make(dispatcher::tag<any_to_any<gr_complex, std::complex<uint16_t>>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT16_MIN),1,1,"f32u16c"));
    }

    static sptr make(dispatcher::tag<any_to_any<std::complex<uint16_t>, gr_complex>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT16_MIN),1,1,"u16f32c"));
    }

    static sptr make(dispatcher::tag<any_to_any<gr_complex, std::complex<int8_t>>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT8_MIN),1,1,"f32s8c"));
    }

    static sptr make(dispatcher::tag<any_to_any<std::complex<int8_t>, gr_complex>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT8_MIN),1,1,"s8f32c"));
    }

    static sptr make(dispatcher::tag<any_to_any<gr_complex, std::complex<uint8_t>>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT8_MIN),1,1,"f32u8c"));
    }

    static sptr make(dispatcher::tag<any_to_any<std::complex<uint8_t>, gr_complex>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(-float(INT8_MIN),1,1,"u8f32c"));
    }

    static sptr make(dispatcher::tag<any_to_any<gr_complex, std::array<int8_t,40>>>)
    {
        if(UINTPTR_MAX == 0xffffffffffffffff)
        {
            #ifdef __BMI2__
            if( __builtin_cpu_supports("bmi2"))
                return gnuradio::get_initial_sptr(new any_to_any_bmi64<T_IN, T_OUT>(-float(INT16_MIN),16,1,"f32s10c"));
            #endif
            return gnuradio::get_initial_sptr(new any_to_any_64<T_IN, T_OUT>(-float(INT16_MIN),16,1,"f32s10c"));
        }else{
            return gnuradio::get_initial_sptr(new any_to_any_32<T_IN, T_OUT>(-float(INT16_MIN),16,1,"f32s10c"));
        }
    }

    static sptr make(dispatcher::tag<any_to_any<std::array<int8_t,40>, gr_complex>>)
    {
        if(UINTPTR_MAX == 0xffffffffffffffff)
        {
            #ifdef __BMI2__
            if( __builtin_cpu_supports("bmi2"))
                return gnuradio::get_initial_sptr(new any_to_any_bmi64<T_IN, T_OUT>(-float(INT16_MIN),1,16,"s10f32c"));
            #endif
            return gnuradio::get_initial_sptr(new any_to_any_64<T_IN, T_OUT>(-float(INT16_MIN),1,16,"s10f32c"));
        }else{
            return gnuradio::get_initial_sptr(new any_to_any_32<T_IN, T_OUT>(-float(INT16_MIN),1,16,"s10f32c"));
        }
    }

    static sptr make(dispatcher::tag<any_to_any<gr_complex, std::array<int8_t,3>>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(2048.0f,1,1,"f32s12c"));
    }

    static sptr make(dispatcher::tag<any_to_any<std::array<int8_t,3>, gr_complex>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(2048.0f,1,1,"s12f32c"));
    }

    static sptr make(dispatcher::tag<any_to_any<gr_complex, std::array<int8_t,7>>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(8192.0f,2,1,"f32s14c"));
    }

    static sptr make(dispatcher::tag<any_to_any<std::array<int8_t,7>, gr_complex>>)
    {
        return gnuradio::get_initial_sptr(new any_to_any<T_IN, T_OUT>(8192.0f,1,2,"s14f32c"));
    }

    any_to_any(const double scale, unsigned decimation, unsigned interpolation, const std::string bname):sync_block(bname,
                gr::io_signature::make (1, 1, sizeof(T_IN)),
                gr::io_signature::make (1, 1, sizeof(T_OUT)))
    {
        set_scale(scale);
        set_decimation(decimation);
        set_interpolation(interpolation);
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
#if 1
    void convert(const gr_complex *in, std::array<int8_t,40> * out, int noutput_items)
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
#endif
#if 1
    void convert(const std::array<int8_t,40> *in, gr_complex * out, int noutput_items)
    {
        uint8_t * p = (uint8_t *) &(*in)[0];
        while(noutput_items)
        {
            int i;
            int q;
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
#endif
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

////////////////////////////////
// 32 bit accelerated converters
////////////////////////////////

template <typename T_IN, typename T_OUT> class BLOCKS_API any_to_any_32 : virtual public any_to_any_base
{
public:

    any_to_any_32(const double scale, unsigned decimation, unsigned interpolation, const std::string bname):sync_block(bname,
                gr::io_signature::make (1, 1, sizeof(T_IN)),
                gr::io_signature::make (1, 1, sizeof(T_OUT)))
    {
        set_scale(scale);
        set_decimation(decimation);
        set_interpolation(interpolation);
    }

    int work(int noutput_items, gr_vector_const_void_star &input_items,
    gr_vector_void_star &output_items) override
    {
        return work(noutput_items, input_items, output_items, dispatcher::tag<any_to_any_32>());
    }

    void convert(const void *in, void *out, int noutput_items) override
    {
        convert(in, out, noutput_items, dispatcher::tag<any_to_any_32>());
    }

private:
    template<typename U_IN, typename U_OUT> int work(int noutput_items, gr_vector_const_void_star &input_items,
    gr_vector_void_star &output_items, dispatcher::tag<any_to_any_32<U_IN, U_OUT>>)
    {
        const U_IN *in = (const U_IN *) input_items[0];
        U_OUT *out = (U_OUT *) output_items[0];

        convert(in, out, noutput_items);
        return noutput_items;
    }

    template<typename U_IN, typename U_OUT> void convert(const void *in, void *out, int noutput_items, dispatcher::tag<any_to_any_32<U_IN, U_OUT>>)
    {
        const U_IN *u_in = (const U_IN *) in;
        U_OUT *u_out = (U_OUT *) out;

        convert(u_in, u_out, noutput_items);
    }

   void convert(const gr_complex *in, std::array<int8_t,40> * out, int noutput_items)
    {
        uint8_t buf[8*2*2];
        uint32_t * p = (uint32_t *) &(*out)[0];
        noutput_items *= 2;
        while(noutput_items)
        {
            volk_32f_s32f_convert_16i((int16_t *)buf, (const float *)in, d_scale, 16);
            uint32_t * r = (uint32_t *) buf;
            *p = ((*r & 0x0000ffc0) >> 6)|
                 ((*r & 0xffc00000) >> 12);
            r++;
            *p|= ((*r & 0x0000ffc0) << 14)|//6>>20
                 ((*r & 0xffc00000) << 8);//22>>30
            p++;
            *p = ((*r & 0xffc00000) >> 24);//22>>-2
            r++;
            *p|= ((*r & 0x0000ffc0) << 2)|//6>>8
                 ((*r & 0xffc00000) >> 4);//22>>18
            r++;
            *p|= ((*r & 0x0000ffc0) << 22);//6>>28
            p++;
            *p = ((*r & 0x0000ffc0) >> 10)|//6>>-4
                 ((*r & 0xffc00000) >> 16);//22>>6
            r++;
            *p|= ((*r & 0x0000ffc0) << 10)|//6>>16
                 ((*r & 0xffc00000) << 4);//22>>26
            p++;
            *p = ((*r & 0xffc00000) >> 28);//22>>-6
            r++;
            *p|= ((*r & 0x0000ffc0) >> 2)|//6>>4
                 ((*r & 0xffc00000) >> 8);//22>>14
            r++;
            *p|= ((*r & 0x0000ffc0) << 18);//6>>24
            p++;
            *p = ((*r & 0x0000ffc0) >> 14)|//6>>-8
                 ((*r & 0xffc00000) >> 20);//22>>2
            r++;
            *p|= ((*r & 0x0000ffc0) << 6)|//6>>12
                 (*r & 0xffc00000);//22>>22
            p++;
            in+=8;
            noutput_items--;
        }
    }

    void convert(const std::array<int8_t,40> *in, gr_complex * out, int noutput_items)
    {
        uint8_t buf[8*2*2];
        uint32_t * i = (uint32_t *) &(*in)[0];
        while(noutput_items)
        {
            uint32_t * o = (uint32_t *) buf;
            *o = (*i & 0x000003ff) << 6;
            *o|= (*i & 0x000ffc00) << 12;
            o++;
            *o = (*i & 0x3ff00000) >> 14;
            *o|= (*i & 0xc0000000) >> 8;
            i++;
            *o|= (*i & 0x000000ff) << 24;
            o++;
            *o = (*i & 0x0003ff00) >> 2;
            *o|= (*i & 0x0ffc0000) << 4;
            o++;
            *o = (*i & 0xf0000000) >> 22;
            i++;
            *o|= (*i & 0x0000003f) << 10;
            *o|= (*i & 0x0000ffc0) << 16;
            o++;
            *o = (*i & 0x03ff0000) >> 10;
            *o|= (*i & 0xfc000000) >> 4;
            i++;
            *o|= (*i & 0x0000000f) << 28;
            o++;
            *o = (*i & 0x00003ff0) << 2;
            *o|= (*i & 0x00ffc000) << 8;
            o++;
            *o = (*i & 0xff000000) >> 18;
            i++;
            *o|= (*i & 0x00000003) << 14;
            *o|= (*i & 0x00000ffc) << 20;
            o++;
            *o = (*i & 0x003ff000) >> 6;
            *o|= (*i & 0xffc00000);
            i++;

            volk_16i_s32f_convert_32f((float *)out, (const int16_t*)buf,d_scale,16);
            out+=8;
            noutput_items-=8;
        }
    }

};

////////////////////////////////
// 64 bit accelerated converters
////////////////////////////////

template <typename T_IN, typename T_OUT> class BLOCKS_API any_to_any_64 : virtual public any_to_any_base
{
public:

    any_to_any_64(const double scale, unsigned decimation, unsigned interpolation, const std::string bname):sync_block(bname,
                gr::io_signature::make (1, 1, sizeof(T_IN)),
                gr::io_signature::make (1, 1, sizeof(T_OUT)))
    {
        set_scale(scale);
        set_decimation(decimation);
        set_interpolation(interpolation);
    }

    int work(int noutput_items, gr_vector_const_void_star &input_items,
    gr_vector_void_star &output_items) override
    {
        return work(noutput_items, input_items, output_items, dispatcher::tag<any_to_any_64>());
    }

    void convert(const void *in, void *out, int noutput_items) override
    {
        convert(in, out, noutput_items, dispatcher::tag<any_to_any_64>());
    }

private:
    template<typename U_IN, typename U_OUT> int work(int noutput_items, gr_vector_const_void_star &input_items,
    gr_vector_void_star &output_items, dispatcher::tag<any_to_any_64<U_IN, U_OUT>>)
    {
        const U_IN *in = (const U_IN *) input_items[0];
        U_OUT *out = (U_OUT *) output_items[0];

        convert(in, out, noutput_items);
        return noutput_items;
    }

    template<typename U_IN, typename U_OUT> void convert(const void *in, void *out, int noutput_items, dispatcher::tag<any_to_any_64<U_IN, U_OUT>>)
    {
        const U_IN *u_in = (const U_IN *) in;
        U_OUT *u_out = (U_OUT *) out;

        convert(u_in, u_out, noutput_items);
    }

   void convert(const gr_complex *in, std::array<int8_t,40> * out, int noutput_items)
    {
        uint8_t buf[16*2*2];
        uint64_t * p = (uint64_t *) &(*out)[0];
        while(noutput_items)
        {
            volk_32f_s32f_convert_16i((int16_t *)buf, (const float *)in, d_scale, 32);
            uint64_t * r = (uint64_t *) buf;
            *p = ((*r & 0x000000000000ffc0) >> 6)|
                 ((*r & 0x00000000ffc00000) >> 12)|
                 ((*r & 0x0000ffc000000000) >> 18)|
                 ((*r & 0xffc0000000000000) >> 24);
            r++;
            *p|= ((*r & 0x000000000000ffc0) << 34)|//6>>40
                 ((*r & 0x00000000ffc00000) << 28)|//22>>50
                 ((*r & 0x0000ffc000000000) << 22);//38>>60
            p++;
            *p = ((*r & 0x0000ffc000000000) >> 42)|//38>>-4
                 ((*r & 0xffc0000000000000) >> 48);//54>>6
            r++;
            *p|= ((*r & 0x000000000000ffc0) << 10)|//6>>16
                 ((*r & 0x00000000ffc00000) << 4)|//22>>26
                 ((*r & 0x0000ffc000000000) >> 2)|//38>>36
                 ((*r & 0xffc0000000000000) >> 8);//54>>46
            r++;
            *p|= ((*r & 0x000000000000ffc0) << 50);//6>>56
            p++;
            *p = ((*r & 0x000000000000ffc0) >> 14)|//6>>-8
                 ((*r & 0x00000000ffc00000) >> 20)|//22>>2
                 ((*r & 0x0000ffc000000000) >> 26)|//38>>12
                 ((*r & 0xffc0000000000000) >> 32);//54>>22
            r++;
            *p|= ((*r & 0x000000000000ffc0) << 26)|//6>>32
                 ((*r & 0x00000000ffc00000) << 20)|//22>>42
                 ((*r & 0x0000ffc000000000) << 14)|//38>>52
                 ((*r & 0xffc0000000000000) << 8);//54>>62
            p++;
            *p = ((*r & 0xffc0000000000000) >> 56);//54>>-2
            r++;
            *p|= ((*r & 0x000000000000ffc0) << 2)|//6>>8
                 ((*r & 0x00000000ffc00000) >> 4)|//22>>18
                 ((*r & 0x0000ffc000000000) >> 10)|//38>>28
                 ((*r & 0xffc0000000000000) >> 16);//54>>38
            r++;
            *p|= ((*r & 0x000000000000ffc0) << 42)|//6>>48
                 ((*r & 0x00000000ffc00000) << 36);//22>>58
            p++;
            *p = ((*r & 0x00000000ffc00000) >> 28)|//22>>-6
                 ((*r & 0x0000ffc000000000) >> 34)|//38>>4
                 ((*r & 0xffc0000000000000) >> 40);//54>>14
            r++;
            *p|= ((*r & 0x000000000000ffc0) << 18)|//6>>24
                 ((*r & 0x00000000ffc00000) << 12)|//22>>34
                 ((*r & 0x0000ffc000000000) << 6)|//38>>44
                 ((*r & 0xffc0000000000000)     );//54>>54
            p++;
            in+=16;
            noutput_items--;
        }
    }

    void convert(const std::array<int8_t,40> *in, gr_complex * out, int noutput_items)
    {
        uint8_t buf[16*2*2];
        noutput_items&=-2;
        while(noutput_items)
        {
            uint64_t * i = (uint64_t *) &(*in)[0];
            uint64_t * o = (uint64_t *) buf;
            *o = (*i & 0x00000000000003ff) << 6;
            *o|= (*i & 0x00000000000ffc00) << 12;
            *o|= (*i & 0x000000003ff00000) << 18;
            *o|= (*i & 0x000000ffc0000000) << 24;
            o++;
            *o = (*i & 0x0003ff0000000000) >> 34;
            *o|= (*i & 0x0ffc000000000000) >> 28;
            *o|= (*i & 0xf000000000000000) >> 22;
            i++;
            *o|= (*i & 0x000000000000003f) << 42;
            *o|= (*i & 0x000000000000ffc0) << 48;
            o++;
            *o = (*i & 0x0000000003ff0000) >> 10;
            *o|= (*i & 0x0000000ffc000000) >> 4;
            *o|= (*i & 0x00003ff000000000) << 2;
            *o|= (*i & 0x00ffc00000000000) << 8;
            o++;
            *o = (*i & 0xff00000000000000) >> 50;
            i++;
            *o|= (*i & 0x0000000000000003) << 14;
            *o|= (*i & 0x0000000000000ffc) << 20;
            *o|= (*i & 0x00000000003ff000) << 26;
            *o|= (*i & 0x00000000ffc00000) << 32;
            o++;
            *o = (*i & 0x000003ff00000000) >> 26;
            *o|= (*i & 0x000ffc0000000000) >> 20;
            *o|= (*i & 0x3ff0000000000000) >> 14;
            *o|= (*i & 0xc000000000000000) >> 8;
            i++;
            *o|= (*i & 0x00000000000000ff) << 56;
            o++;
            *o = (*i & 0x000000000003ff00) >> 2;
            *o|= (*i & 0x000000000ffc0000) << 4;
            *o|= (*i & 0x0000003ff0000000) << 10;
            *o|= (*i & 0x0000ffc000000000) << 16;
            o++;
            *o = (*i & 0x03ff000000000000) >> 42;
            *o|= (*i & 0xfc00000000000000) >> 36;
            i++;
            *o|= (*i & 0x000000000000000f) << 28;
            *o|= (*i & 0x0000000000003ff0) << 34;
            *o|= (*i & 0x0000000000ffc000) << 40;
            o++;
            *o = (*i & 0x00000003ff000000) >> 18;
            *o|= (*i & 0x00000ffc00000000) >> 12;
            *o|= (*i & 0x003ff00000000000) >> 6;
            *o|= (*i & 0xffc0000000000000);

            volk_16i_s32f_convert_32f((float *)out, (const int16_t*)buf,d_scale,32);
            out+=16;
            in++;
            noutput_items-=16;
        }
    }

};

#ifdef __BMI2__
/////////////////////////////////////
// 64 bit bmi2 accelerated converters
/////////////////////////////////////

template <typename T_IN, typename T_OUT> class BLOCKS_API any_to_any_bmi64 : virtual public any_to_any_base
{
public:

    any_to_any_bmi64(const double scale, unsigned decimation, unsigned interpolation, const std::string bname):sync_block(bname,
                gr::io_signature::make (1, 1, sizeof(T_IN)),
                gr::io_signature::make (1, 1, sizeof(T_OUT)))
    {
        set_scale(scale);
        set_decimation(decimation);
        set_interpolation(interpolation);
    }

    int work(int noutput_items, gr_vector_const_void_star &input_items,
    gr_vector_void_star &output_items) override
    {
        return work(noutput_items, input_items, output_items, dispatcher::tag<any_to_any_bmi64>());
    }

    void convert(const void *in, void *out, int noutput_items) override
    {
        convert(in, out, noutput_items, dispatcher::tag<any_to_any_bmi64>());
    }

private:
    template<typename U_IN, typename U_OUT> int work(int noutput_items, gr_vector_const_void_star &input_items,
    gr_vector_void_star &output_items, dispatcher::tag<any_to_any_bmi64<U_IN, U_OUT>>)
    {
        const U_IN *in = (const U_IN *) input_items[0];
        U_OUT *out = (U_OUT *) output_items[0];

        convert(in, out, noutput_items);
        return noutput_items;
    }

    template<typename U_IN, typename U_OUT> void convert(const void *in, void *out, int noutput_items, dispatcher::tag<any_to_any_bmi64<U_IN, U_OUT>>)
    {
        const U_IN *u_in = (const U_IN *) in;
        U_OUT *u_out = (U_OUT *) out;

        convert(u_in, u_out, noutput_items);
    }

   void convert(const gr_complex *in, std::array<int8_t,40> * out, int noutput_items)
    {
        uint8_t buf[16*2*2];
        uint64_t * p = (uint64_t *) &(*out)[0];
        while(noutput_items)
        {
            volk_32f_s32f_convert_16i((int16_t *)buf, (const float *)in, d_scale, 32);
            uint64_t * r = (uint64_t *) buf;
            uint64_t t;
            uint64_t b;
            b=_pext_u64(*r,0xffc0ffc0ffc0ffc0);
            r++;
            t=_pext_u64(*r,0xffc0ffc0ffc0ffc0);
            r++;
            b|=t<<40;

            *p=b;
            p++;

            b=_pext_u64(*r,0xffc0ffc0ffc0ffc0)<<16;//64-40-16=8
            r++;
            b|=t>>24;//40-24=16
            t=_pext_u64(*r,0xffc0ffc0ffc0ffc0);
            r++;
            b|=t<<56;//40-8=32

            *p=b;
            p++;

            b=t>>8;//32
            t=_pext_u64(*r,0xffc0ffc0ffc0ffc0);
            r++;
            b|=t<<32;//8

            *p=b;
            p++;

            b=_pext_u64(*r,0xffc0ffc0ffc0ffc0)<<8;//48
            r++;
            b|=t>>32;
            t=_pext_u64(*r,0xffc0ffc0ffc0ffc0);
            r++;
            b|=t<<48;//40-16=24

            *p=b;
            p++;

            b=t>>16;//24
            t=_pext_u64(*r,0xffc0ffc0ffc0ffc0);
            r++;
            b|=t<<24;

            *p=b;
            p++;

            in+=16;
            noutput_items--;
        }
    }

    void convert(const std::array<int8_t,40> *in, gr_complex * out, int noutput_items)
    {
        uint8_t buf[16*2*2];
        noutput_items&=-2;
        uint64_t t;
        uint64_t * i = (uint64_t *) &(*in)[0];
        while(noutput_items)
        {
            uint64_t * o = (uint64_t *) buf;
            *o = _pdep_u64(*i, 0xffc0ffc0ffc0ffc0);
            o++;
            t = *i >> 40;
            i++;
            t |= *i << 24;
            *o = _pdep_u64(t, 0xffc0ffc0ffc0ffc0);
            o++;
            t = *i >> 16;
            *o = _pdep_u64(t, 0xffc0ffc0ffc0ffc0);
            o++;
            t = *i >> 56;
            i++;
            t |= *i << 8;
            *o = _pdep_u64(t, 0xffc0ffc0ffc0ffc0);
            o++;
            t = *i >> 32;
            i++;
            t |= *i << 32;
            *o = _pdep_u64(t, 0xffc0ffc0ffc0ffc0);
            o++;
            t = *i >> 8;
            *o = _pdep_u64(t, 0xffc0ffc0ffc0ffc0);
            o++;
            t = *i >> 48;
            i++;
            t |= *i << 16;
            *o = _pdep_u64(t, 0xffc0ffc0ffc0ffc0);
            o++;
            t = *i >> 24;
            *o = _pdep_u64(t, 0xffc0ffc0ffc0ffc0);
            i++;
            volk_16i_s32f_convert_32f((float *)out, (const int16_t*)buf,d_scale,32);
            out+=16;
            noutput_items-=16;
        }
    }

};
#endif



#endif /* INCLUDED_FORMAT_CONVERTER_H */
