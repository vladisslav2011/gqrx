/*
 * asynchronous receiver block implementation
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
#include <volk/volk.h>
#include "async_rx_impl.h"

#define SYMBOLS_MAX 24

namespace gr {
namespace rtty {

async_rx::sptr async_rx::make(float sample_rate, float bit_rate, char word_len, enum async_rx_parity parity) {
    return gnuradio::get_initial_sptr(new async_rx_impl(sample_rate, bit_rate, word_len, parity));
}

async_rx_impl::async_rx_impl(float sample_rate, float bit_rate, char word_len, enum async_rx_parity parity) :
            gr::block("async_rx",
            gr::io_signature::make(2, 2, sizeof(float)),
            gr::io_signature::make(1, 1, sizeof(int))),
            d_sample_rate(sample_rate),
            d_bit_rate(bit_rate),
            d_word_len(word_len),
            d_parity(parity),
            d_start_thr(1.0),
            d_bit_thr(1.0),
            d_in_offset(0)
{
    bit_len = std::round(sample_rate / bit_rate);
    bit_len2 = bit_len/2;
    bit_len4 = bit_len/4;
    threshold = 0.0;
    snr_mark=snr_space=0.0;
    d_raw_len = d_word_len+2+(d_parity?1:0);
    set_history(bit_len*SYMBOLS_MAX);
    #if DEBUG_ASYNC
    fd[0]=fopen("/home/vlad/iq/fsk0.raw","w+");
    fd[1]=fopen("/home/vlad/iq/fsk1.raw","w+");
    fd[2]=fopen("/home/vlad/iq/corr.raw","w+");
    #endif
}

async_rx_impl::~async_rx_impl() {
    #if DEBUG_ASYNC
    for(int p=0;p<3;p++)
        if(fd[p])
            fclose(fd[p]);
    #endif
}

void async_rx_impl::set_word_len(char word_len) {
    std::lock_guard<std::mutex> lock(d_mutex);

    d_word_len = word_len;
    d_raw_len = d_word_len+2+(d_parity?1:0);
}

char async_rx_impl::word_len() const {
    return d_word_len;
}

void async_rx_impl::set_sample_rate(float sample_rate) {
    std::lock_guard<std::mutex> lock(d_mutex);

    d_sample_rate = sample_rate;
    bit_len = (d_sample_rate / d_bit_rate);
    bit_len2 = bit_len/2;
    bit_len4 = bit_len/4;
    set_history(bit_len*SYMBOLS_MAX);
}

float async_rx_impl::sample_rate() const {
    return d_sample_rate;
}

void async_rx_impl::set_bit_rate(float bit_rate) {
    std::lock_guard<std::mutex> lock(d_mutex);

    d_bit_rate = bit_rate;
    bit_len = (d_sample_rate / d_bit_rate);
    bit_len2 = bit_len/2;
    bit_len4 = bit_len/4;
    set_history(bit_len*SYMBOLS_MAX);
}

float async_rx_impl::bit_rate() const {
    return d_bit_rate;
}

void async_rx_impl::set_parity(enum async_rx::async_rx_parity parity) {
    std::lock_guard<std::mutex> lock(d_mutex);

    d_parity = parity;
    d_raw_len = d_word_len+2+(d_parity?1:0);
}

enum async_rx::async_rx_parity async_rx_impl::parity() const {
    return d_parity;
}

void async_rx_impl::reset() {
    std::lock_guard<std::mutex> lock(d_mutex);

    threshold = 0;
    d_in_offset=0;
}

void async_rx_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required) {
    std::lock_guard<std::mutex> lock(d_mutex);

    ninput_items_required[0] =
    ninput_items_required[1] = noutput_items * (d_word_len+2+(d_parity==ASYNC_RX_PARITY_NONE?0:1)) * bit_len + history();
}

float async_rx_impl::avg(const float *in,int cc)
{
    float acc=0.0;
    float dev=0.0;
    for(int k=0;k<cc;k++)
        acc+=in[k];
    acc/=float(cc);
    for(int k=0;k<cc;k++)
        dev+=std::abs(acc-in[k]);
    dev/=float(cc);
    return acc-dev;
}

void async_rx_impl::minmax(const float *in,int cc,int l,float &a_min, float &a_max)
{
    a_min=1e7;
    a_max=-1e7;
    float a_buf=0.0;
//    for(int k=0;k<l;k++)
//        a_buf+=in[k];
    for(int k=0;k<cc;k++)
    {
        a_buf=avg(&in[k],l);
        if(a_min>a_buf)
            a_min=a_buf;
        if(a_max<a_buf)
            a_max=a_buf;
        //a_buf+=in[k+l]-in[k];
    }
//    a_min/=float(l);
//    a_max/=float(l);
}

float async_rx_impl::correlate_word(const float *mark,const float *space,int ofs,int &out,bool d)
{
    float corr_acc=0.0;
    float corr=0.0;
    float prev_mark=0.0;
    float prev_space=0.0;
    float last_mark=0.0;
    float last_space=0.0;
    float thr_mark=0.0;
    float thr_space=0.0;
    float min_mark=0.0;
    float max_mark=0.0;
    float min_space=0.0;
    float max_space=0.0;
    int k=0;
    out=-1;
    int p=0;
    int c=0;
    int par=0;
    int parity_point=100;
    int stop_point=d_word_len;
    int nbits=d_word_len+1;
    int mask=1<<d_word_len;
    if(d_parity)
    {
        nbits++;
        parity_point=stop_point;
        stop_point++;
    }
    int avg_level=bit_len2;
    int avg_trans=bit_len4;
    int avg_initial=bit_len2;

    minmax(&mark[ofs],(nbits+2)*bit_len*2.0f,avg_initial,min_mark,max_mark);
    minmax(&space[ofs],(nbits+2)*bit_len*2.0f,avg_initial,min_space,max_space);
    snr_mark=max_mark-min_mark;
    snr_space=max_space-min_space;
    thr_mark=(min_mark+max_mark)*0.5;
    thr_space=(min_space+max_space)*0.5;
    //correlate mark
    prev_mark=avg(&mark[ofs+p],avg_level);
    prev_space=avg(&space[ofs+p],avg_level);
    if(d)printf("thr=%1.5f %1.5f %1.5f; %1.5f %1.5f %1.5f; %1.5f %1.5f  ",min_mark,thr_mark,max_mark,min_space,thr_space,max_space,prev_mark,prev_space);
    corr+= prev_mark - thr_mark + thr_space - prev_space;
    //correlate transition
    p=bit_len2;
    for(int j=0;j<bit_len2;j++)
        corr+=avg(&mark[ofs+p+j],avg_trans)-avg(&mark[ofs+p+j+1],avg_trans)+avg(&space[ofs+p+j+1],avg_trans)-avg(&space[ofs+p+j],avg_trans);
    //correlate space
    p=bit_len;
    last_space=avg(&space[ofs+p],avg_level);
    last_mark=avg(&mark[ofs+p],avg_level);
    corr+= last_space - prev_space;
    corr+= prev_mark - last_mark;
    //correlate stop
    corr+=avg(&mark[ofs+bit_len*(stop_point+2)],avg_level)-avg(&space[ofs+bit_len*(stop_point+2)],avg_level);
    corr_acc=corr-std::max(max_mark,max_space);
    thr_mark=(prev_mark+last_mark)*0.5;
    thr_space=(prev_space+last_space)*0.5;
    if(d)printf("c=%1.5f %1.5f %1.5f ",corr,prev_mark,last_space);
    prev_mark=last_mark;
    prev_space=last_space;
    if(corr>0.0)
    {
        out=0;
        for(k=0;k<nbits;k++)
        {
            corr=0;
            p=bit_len*(k+2);
            last_space=avg(&space[ofs+p],avg_level);
            last_mark=avg(&mark[ofs+p],avg_level);
            if(c==0)
            {
                //space-mark
                corr=(thr_space-last_space)+(last_mark-thr_mark);
//                corr=(thr_space-last_space)*thr_space/thr_mark+(last_mark-thr_mark)*thr_mark/thr_space;
            }else{
                //mark-space
                corr=(last_space-thr_space)+(thr_mark-last_mark);
//                corr=(last_space-thr_space)*thr_space/thr_mark+(thr_mark-last_mark)*thr_mark/thr_space;
            }
            if(corr>0)
            {
                c^=1;
                //corr_acc+=corr;
                thr_mark=(thr_mark+(prev_mark+last_mark)*0.5)*0.5;
                thr_space=(thr_space+(prev_space+last_space)*0.5)*0.5;
                prev_mark=last_mark;
                prev_space=last_space;
            }
            if(k==stop_point)
            {
                if(!c)
                {
                    if(d)printf("[S] ");
                    out =-1;
                    corr_acc*=0.5;
                }
            }else if(k<parity_point)
            {
                 if(d)printf("%d",c);
            #if 1
                if(c)
                    out|=mask;
                out>>=1;
            #else
                out<<=1;
                out|=c;
            #endif
                par^=c;
            }
            if(k==parity_point)
            {
                if(d_parity==ASYNC_RX_PARITY_ODD)
                    par^=1;
                if(d_parity==ASYNC_RX_PARITY_MARK)
                    par=1;
                if(d_parity==ASYNC_RX_PARITY_SPACE)
                    par=0;
                if(d_parity!=ASYNC_RX_PARITY_DONTCARE)
                {
                    if(c!=par)
                    {
                        out =-1;
                        if(d)printf("[P] ");
                        corr_acc/=2.0;
                    }
                }
            }
        }
        if(d)printf("d=%d %1.5f %1.5f",out,thr_mark,thr_space);
    }
    if(d)printf("\n");
    return corr_acc;
}

int async_rx_impl::general_work (int noutput_items,
        gr_vector_int &ninput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items) {

    std::lock_guard<std::mutex> lock(d_mutex);
    int in_max = noutput_items*d_raw_len*bit_len;
    int in_count = d_in_offset;
    int out_count = 0;
    const float *mark = reinterpret_cast<const float*>(input_items[0]);
    const float *space = reinterpret_cast<const float*>(input_items[1]);
    int *out = reinterpret_cast<int*>(output_items[0]);
    int chars[3];
    float corr[3]={0.0,0.0,0.0};
    int p = 0;
    corr[0]=correlate_word(mark,space,0,chars[0]);
    corr[2]=corr[1]=corr[0];
    chars[2]=chars[1]=chars[0];
    #if DEBUG_ASYNC
    if(fd[2])
    {
        float * x=(float*)malloc(in_max*sizeof(float));
    #endif
        for(p=0;p<in_max;p++)
        {
            corr[0]=corr[1];
            corr[1]=corr[2];
            chars[0]=chars[1];
            chars[1]=chars[2];
        #if DEBUG_ASYNC
            x[p]=
        #endif
            corr[2]=correlate_word(mark,space,p,chars[2],false);
            if(p>=in_count)
            {
                if(corr[0]<corr[1] && corr[2]<corr[1] && corr[1] > threshold && corr[0] > threshold && corr[2] > threshold && chars[1]!=-1 && chars[0]==chars[1] && chars[1]==chars[2])
                {
                    in_count=p+(d_word_len+1+(d_parity==ASYNC_RX_PARITY_NONE?0:1)) * bit_len;//last mark may be the first mark of the new word, so keep it
                    out_count++;
                    *out=chars[1];
                    out++;
//                    printf("+ %d %3.1f %3.1f\n",chars[1], snr_mark, snr_space);
                    break;
                }
            }
        }
    #if DEBUG_ASYNC
        fwrite(x,sizeof(float),p,fd[2]);
        free(x);
    }
    for(int j=0;j<2;j++)
        if(fd[j])
                fwrite(input_items[j],sizeof(float),p,fd[j]);
    #endif
    d_in_offset=in_count-p;
    consume_each(p);
    return out_count;
}

}
}
