/*
 * Copyright (C) 2014 Bastian Bloessl <bloessl@ccs-labs.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define dout debug && std::cout
#define lout log && std::cout

#include "decoder_impl.h"
#include "constants.h"
#include <gnuradio/io_signature.h>
#include <array>
#include <volk/volk.h>

using namespace gr::rds;
const std::array<decoder_impl::bit_locator,1024> decoder_impl::locator(decoder_impl::build_locator());

std::array<decoder_impl::bit_locator,1024> decoder_impl::build_locator()
{
    std::array<decoder_impl::bit_locator,1024> tmp;
    uint32_t b=9876;
    unsigned k,j;
    for(k=0;k<1024;k++)
        tmp[k]={0xffff,16};
    tmp[0]={0,0};
    for(k=0;k<16;k++)
    {
        uint16_t e=(1<<k);
        uint32_t p=calc_syndrome(b,16)^calc_syndrome(b^e,16);
        tmp[p]={e,1};
        if(0)
        for(j=0;j<10;j++)
            tmp[p^(1<<j)]={e,2};
    }
    if(1)
    for(k=0;k<10;k++)
    {
        uint32_t p=calc_syndrome(b,16)^(1<<k);
        if(tmp[p].w==16)
            tmp[p]={0,1};
    }
    if(1)
    for(k=0;k<16;k++)
        for(j=k+1;j<16;j++)
        {
            uint16_t e=(1<<k)^(1<<j);
            uint32_t p=calc_syndrome(b,16)^calc_syndrome(b^e,16);
            if(tmp[p].w==16)
                tmp[p]={e,2};
        }
    if(1)
    for(j=3;j<6;j++)
//    for(j=3;j<4;j++)
        for(k=0;k<17-j;k++)
        {
            uint16_t e=(((1<<j)-1)<<k)&((1<<16)-1);
            uint32_t p=calc_syndrome(b,16)^calc_syndrome(b^e,16);
            if(tmp[p].w==16)
                tmp[p]={e,uint8_t(j)};
        }
    return tmp;
}

decoder::sptr
decoder::make(bool log, bool debug) {
  return gnuradio::get_initial_sptr(new decoder_impl(log, debug));
}


decoder_impl::decoder_impl(bool log, bool debug)
	: gr::sync_block ("gr_rds_decoder",
			gr::io_signature::make (1, 1, sizeof(float)),
			gr::io_signature::make (0, 0, 0)),
	log(log),
	debug(debug)
{
	bit_counter=0;
	set_output_multiple(GROUP_SIZE);  // 1 RDS datagroup = 104 bits
	set_history(1+BLOCK_SIZE*n_group*PS_SEARCH_MAX+1);
	message_port_register_out(pmt::mp("out"));
	enter_no_sync();
	prev_grp=&groups[0];
	group=&groups[n_group];
	next_grp=&groups[n_group*2];
}

decoder_impl::~decoder_impl() {
}

bool decoder_impl::start()
{
    bit_counter=-BLOCK_SIZE;
    d_bit_counter=0;
    d_valid_bits=0;
    memset(d_used_list,0,sizeof(d_used_list));
    return gr::block::start();
}

////////////////////////// HELPER FUNCTIONS /////////////////////////

void decoder_impl::enter_no_sync() {
	presync = false;
	d_state = NO_SYNC;
}

void decoder_impl::enter_sync(unsigned int sync_block_number) {
	wrong_blocks_counter   = 0;
	blocks_counter         = 0;
	block_bit_counter      = 0;
	block_number           = (sync_block_number + 1) % 4;
	group_assembly_started = false;
	d_state                = SYNC;
}

/* see Annex B, page 64 of the standard */
unsigned int decoder_impl::calc_syndrome(unsigned long message,
		unsigned char mlen) {
	unsigned long lreg = 0;
	unsigned int i;
	const unsigned long poly = 0x5B9;
	const unsigned char plen = 10;

	for (i = mlen; i > 0; i--)  {
		lreg = (lreg << 1) | ((message >> (i-1)) & 0x01);
		if (lreg & (1 << plen)) lreg = lreg ^ poly;
	}
	for (i = plen; i > 0; i--) {
		lreg = lreg << 1;
		if (lreg & (1<<plen))
		  lreg = lreg ^ poly;
	}
	return (lreg & ((1<<plen)-1));	// select the bottom plen bits of reg
}

int decoder_impl::process_group(unsigned * grp, int thr, unsigned char * offs_chars, uint16_t * loc, int * good_grp)
{
    int ret=0;
    if(offs_chars)
    {
        offs_chars[0]='A';
        offs_chars[1]='B';
        offs_chars[2]='C';
        offs_chars[3]='D';
    }
    if(loc)
        loc[0]=loc[1]=loc[2]=loc[3]=0;
    uint16_t s;
    int g1=0;
    int g2=0;

    s=calc_syndrome(grp[0]>>10,16)^(grp[0]&0x3ff);
    const bit_locator & bl=locator[s^offset_word[0]];
    if(loc && (bl.l!=0xffff))
        loc[0]=bl.l;
    if(offs_chars)
        if(bl.w>0)
            offs_chars[0]='x';
    if(bl.w==1)
        g1++;
    if(bl.w==2)
        g2++;
    ret+=bl.w;
    d_block0errs=bl.w;

    s=calc_syndrome(grp[1]>>10,16)^(grp[1]&0x3ff);
    const bit_locator & bl1=locator[s^offset_word[1]];
    if(loc && (bl.l!=0xffff))
        loc[1]=bl1.l;
    if(offs_chars)
        if(bl1.w>thr)
            offs_chars[1]='x';
    if(bl1.w==1)
        g1++;
    if(bl1.w==2)
        g2++;
    ret+=bl1.w;

    s=calc_syndrome(grp[2]>>10,16)^(grp[2]&0x3ff);
    const bit_locator & bl2=locator[s^offset_word[2]];
    const bit_locator & bl4=locator[s^offset_word[4]];
    if(bl2.w<bl4.w)
    {
        if(loc && (bl.l!=0xffff))
            loc[2]=bl2.l;
        if(offs_chars)
            if(bl2.w>thr)
                offs_chars[2]='x';
        if(bl2.w==1)
            g1++;
        if(bl2.w==2)
            g2++;
        ret+=bl2.w;
    }else{
        if(offs_chars)
            offs_chars[2]='c';
        if(loc && (bl.l!=0xffff))
            loc[2]=bl4.l;
        if(offs_chars)
            if(bl4.w>thr)
                offs_chars[2]='x';
        if(bl4.w==1)
            g1++;
        if(bl4.w==2)
            g2++;
        ret+=bl4.w;
    }

    s=calc_syndrome(grp[3]>>10,16)^(grp[3]&0x3ff);
    const bit_locator & bl3=locator[s^offset_word[3]];
    if(loc && (bl.l!=0xffff))
        loc[3]=bl3.l;
    if(offs_chars)
        if(bl3.w>thr)
            offs_chars[3]='x';
    if(bl3.w==1)
        g1++;
    if(bl3.w==2)
        g2++;
    ret+=bl3.w;

    if(good_grp)
        *good_grp=g1*3+g2;
    return ret;
}

void decoder_impl::decode_group(unsigned *grp, int bit_errors)
{
	// raw data bytes, as received from RDS.
	// 8 info bytes, followed by 4 RDS offset chars: ABCD/ABcD/EEEE (in US)
	unsigned char bytes[13];

	// RDS information words
	bytes[0] = (grp[0] >> 8U) & 0xffU;
	bytes[1] = (grp[0]      ) & 0xffU;
	bytes[2] = (grp[1] >> 8U) & 0xffU;
	bytes[3] = (grp[1]      ) & 0xffU;
	bytes[4] = (grp[2] >> 8U) & 0xffU;
	bytes[5] = (grp[2]      ) & 0xffU;
	bytes[6] = (grp[3] >> 8U) & 0xffU;
	bytes[7] = (grp[3]      ) & 0xffU;

	// RDS offset words
	bytes[8] = offset_chars[0];
	bytes[9] = offset_chars[1];
	bytes[10] = offset_chars[2];
	bytes[11] = offset_chars[3];
	bytes[12] = bit_errors;

	pmt::pmt_t data(pmt::make_blob(bytes, 13));
	pmt::pmt_t meta(pmt::PMT_NIL);

	pmt::pmt_t pdu(pmt::cons(meta, data));  // make PDU: (metadata, data) pair
	message_port_pub(pmt::mp("out"), pdu);
}

int decoder_impl::work (int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
{
    const float *in = (const float *) input_items[0];
    in += n_group*BLOCK_SIZE*PS_SEARCH_MAX+1;
    (void) output_items;

/*    dout << "RDS data decoder at work: input_items = "
        << noutput_items << ", /104 = "
        << noutput_items / 104 << std::endl;
*/
    int i=0;
    int ecc_max = d_ecc_max;
    uint16_t locators[5]={0,0,0,0,0};

/* the synchronization process is described in Annex C, page 66 of the standard */
    while (i<noutput_items) {
        if(in[i-1]==0.f)
        {
            if(in[i]!=0.f)
            {
                memset(d_accum.data(),0,d_accum.size()*sizeof(d_accum[0]));
                d_acc_cnt=0;
                d_valid_bits=0;
                memset(d_used_list,0,sizeof(d_used_list));
            }else{
                i++;
                continue;
            }
        }
        d_accum[d_acc_p+GROUP_SIZE]=d_accum[d_acc_p]+=(in[i]-d_accum[d_acc_p])*d_acc_alfa;
        d_acc_p++;
        if(d_valid_bits<PS_SEARCH_MAX*GROUP_SIZE)
            d_valid_bits++;
        memcpy(&d_used_list[1],&d_used_list[0],sizeof(d_used_list)-sizeof(d_used_list[0]));
        d_used_list[0]=0;

        d_acc_p%=GROUP_SIZE;
        if(d_acc_cnt<d_acc_lim)
            d_acc_cnt++;

        std::swap(group,prev_grp);
        std::memcpy(group,next_grp,sizeof(group[0])*n_group);
        d_weight+=(std::abs(in[i])-d_weight)/float(n_group*BLOCK_SIZE);
        //d_weight -= std::abs(in[i-n_group*BLOCK_SIZE]);
        //d_weight += std::abs(in[i]);
        int sample=in[i]>0.f;
        for(int j=0;j<n_group-1;j++)
            group[j]=((group[j]<<1)|(group[j+1]>>25))&((1<<26)-1);
        group[n_group-1]=((group[n_group-1]<<1)|sample)&((1<<26)-1);
        std::swap(group,next_grp);
        unsigned int  *good_group=group;
        d_prev_errs=d_curr_errs;
        d_curr_errs=d_next_errs;
        ++i;
        ++bit_counter;
        ++d_bit_counter;
        int pi=-1;
        if(d_integrate_pi != INTEGRATE_PI_COH)
            pi=pi_detect(next_grp,false);
        if(d_acc_cnt==d_acc_lim)
        {
            d_acc_cnt-=GROUP_SIZE;
            for(int k=0;k<104;k++)
            {
                int sample=d_accum[d_acc_p+k]>0.f;
                for(int j=0;j<n_group-1;j++)
                    d_acc_groups[j]=((d_acc_groups[j]<<1)|(d_acc_groups[j+1]>>25))&((1<<26)-1);
                d_acc_groups[n_group-1]=((d_acc_groups[n_group-1]<<1)|sample)&((1<<26)-1);
            }
            for(int k=0;k<GROUP_SIZE;k++)
            {
                int acc_pi=-1;
                if(d_integrate_pi != INTEGRATE_PI_NC)
                    acc_pi=pi_detect(d_acc_groups,true);
                if(acc_pi!=-1)
                    if((d_state!=SYNC)||(d_best_pi!=acc_pi)||(bit_counter%GROUP_SIZE!=GROUP_SIZE-k))
                    {
                        d_state=SYNC;
                        d_counter=0;
                        bit_counter=GROUP_SIZE-k;
                        d_best_pi=acc_pi;
                    }
                int sample=d_accum[d_acc_p+k]>0.f;
                for(int j=0;j<n_group-1;j++)
                    d_acc_groups[j]=((d_acc_groups[j]<<1)|(d_acc_groups[j+1]>>25))&((1<<26)-1);
                d_acc_groups[n_group-1]=((d_acc_groups[n_group-1]<<1)|sample)&((1<<26)-1);
            }
        }
        if(pi!=-1)
        {
            if((d_state!=SYNC)||(d_best_pi!=pi)||(bit_counter%GROUP_SIZE != 0))
            {
                d_state=SYNC;
                d_counter=0;
                bit_counter=0;
                d_best_pi=pi;
            }
        }
        if(bit_counter>=GROUP_SIZE+1)
        {
            int good_grp=0;
            int bit_errors=0;
            auto old_sync = d_state;
            if(d_state==SYNC)
            //if(0)
            {
                //handle +/- 1 bit shifts
                d_prev_errs=process_group(prev_grp);
                uint16_t prev_pi=prev_grp[0];
                d_curr_errs=process_group(group);
                d_next_errs=process_group(next_grp);
                uint16_t next_pi=next_grp[0];
                bit_counter=1;
                if((d_prev_errs<d_curr_errs)&&(d_prev_errs<d_next_errs)&&(d_prev_errs<12)&&(prev_pi==d_best_pi))
                {
                    good_group=prev_grp;
                    bit_counter=2;
                    printf("<->\n");
                }else if((d_next_errs<d_curr_errs)&&(d_next_errs<d_prev_errs)&&(d_next_errs<12)&&(next_pi==d_best_pi))
                {
                    good_group=next_grp;
                    bit_counter=0;
                    printf("<+>\n");
                }
            }
            bit_errors=process_group(good_group, ecc_max, offset_chars, locators, &good_grp);
            char sync_point='E';
            if(d_state != FORCE_SYNC)
            {
                int curr_pi=(good_group[0]>>10)^locators[0];
                if((d_best_pi==curr_pi)&&(d_block0errs<5))
                {
                    if(d_state!=SYNC)
                    {
                        sync_point='P';
                        d_state = SYNC;
                    }
                    d_counter = 0;
                    d_acc_cnt = 0;
                }

                if(d_state==SYNC)
                {
                    if(good_grp>=4)
                    {
                        d_counter>>=2;
                    }
                    if(good_grp>=3)
                    {
                        d_counter>>=1;
                    }
                    if(bit_errors > 15)
                    {
                        if(d_counter > 512*8)
                        {
                            d_state=NO_SYNC;
                            printf("- NO Sync errors: %d, %d\n",d_curr_errs,d_counter);
                            d_counter = 0;
                        }
                    }
                }
                d_counter +=bit_errors;
                if(d_state != SYNC)
                    continue;
            }else{
                d_state = SYNC;
                offset_chars[0]='F';
                sync_point='F';
                d_counter = 0;
            }
            if((bit_errors>0) && (d_integrate_ps_dist>=8) && d_integrate_ps)
            {
                int p=4;
                int bestp=0;
                int bestq=0;
                int best_errs=bit_errors;
                //float bestdev=10.f;
                int search_lim=std::min(d_valid_bits / GROUP_SIZE, d_integrate_ps_dist);
                unsigned char  offset_chars_a[4];
                uint16_t locators_a[5]={0,0,0,0,0};
                unsigned char  offset_chars_tmp[4];
                uint16_t locators_tmp[5]={0,0,0,0,0};
                uint32_t best_grp[4]{0};
                while(p<search_lim)
                {
                    int bitp=i-GROUP_SIZE-1;
                    int bito=i-GROUP_SIZE*(1+p)-1;
                    int q=0;
                    if(d_integrate_ps == INTEGRATE_PS_3)
                        q=std::max(p+4,p*2-4);
                    if(d_used_list[p])
                    {
                        p++;
                        continue;
                    }
                    while(q<search_lim)
                    {
                        int bit2=i-GROUP_SIZE*(1+q)-1;
                        //float stddev,mean;
                        if(!d_used_list[q])
                        {
                            volk_32f_x2_add_32f(&accum[0],&in[bitp],&in[bito],GROUP_SIZE);
                            if(q)
                                volk_32f_x2_add_32f(&accum[0],&accum[0],&in[bit2],GROUP_SIZE);
                            for(int h=0;h<GROUP_SIZE;h++)
                            {
                                const float flt=accum[h];
                                const int sample=flt>0.f;
                                accum_a[h]=std::abs(accum[h]);
                                for(int j=0;j<n_group-1;j++)
                                    prev_grp[j]=((prev_grp[j]<<1)|(prev_grp[j+1]>>25))&((1<<26)-1);
                                prev_grp[n_group-1]=((prev_grp[n_group-1]<<1)|sample)&((1<<26)-1);
                            }
                            //volk_32f_stddev_and_mean_32f_x2(&stddev,&mean,&accum_a[0],GROUP_SIZE);
                            //if(mean)
                            //    stddev/=mean;
                            int errs=process_group(prev_grp, ecc_max,  offset_chars_a, locators_a);
                            //if(stddev<bestdev)
                            if(errs<best_errs)
                            {
                                best_errs=errs;
                                bestp=p;
                                bestq=q;
                                //bestdev=stddev;
                                memcpy(best_grp,prev_grp,sizeof(best_grp));
                                memcpy(locators_tmp,locators_a,sizeof(locators_tmp));
                                memcpy(offset_chars_tmp,offset_chars_a,sizeof(offset_chars_tmp));
                            }
                            if(best_errs==0)
                                break;
                        }
                        if(d_integrate_ps == INTEGRATE_PS_2)
                            break;
                        if(q)
                            q++;
                        else
                            q=std::max(p+4,p*2-4);
                    }
                    if(best_errs==0)
                        break;
                    p++;
                }
                if(best_errs<bit_errors)
                {
                    if((offset_chars_tmp[1]!='x')&&(offset_chars_tmp[2]!='x')&&(offset_chars_tmp[3]!='x'))
                        best_errs=0;
                    bit_errors=best_errs;
                    good_group=&prev_grp[0];
                    memcpy(locators,locators_tmp,sizeof(locators));
                    memcpy(offset_chars,offset_chars_tmp,sizeof(offset_chars));
                    memcpy(prev_grp,best_grp,sizeof(best_grp));
                    offset_chars[0]='x';
                    if(best_errs==0)
                    {
                        d_used_list[0]=1;
                        d_used_list[bestp]=1;
                        d_used_list[bestq]=1;
                    }
                }
            }else{
                d_used_list[0]=1;
            }
            prev_grp[0]=(good_group[0]>>10)^locators[0];
            prev_grp[1]=(good_group[1]>>10)^locators[1];
            prev_grp[2]=(good_group[2]>>10)^locators[2];
            prev_grp[3]=(good_group[3]>>10)^locators[3];
            decode_group(prev_grp, bit_errors);
            if(d_state != old_sync)
                printf("Sync %c %04x/%04x Corrected: %d %d %c\n",offset_chars[2],prev_grp[0],d_best_pi,bit_errors,ecc_max,sync_point);
        }
    }
    decode_group(group, 127);
    return noutput_items;
}

int decoder_impl::pi_detect(uint32_t * p_grp, bool corr)
{
    uint16_t locators[5]={0,0,0,0,0};
    uint32_t tmp_grp[4];
    int errs=process_group(p_grp, d_ecc_max, offset_chars, locators);
    pi_stats       *pi_a=corr?d_pi_b:d_pi_a;
    uint16_t pi = (p_grp[0]>>10)^locators[0];
    if(d_block0errs<5)
    {
        static const uint8_t weights_corr[]={19,9,1,1,0};
        static const uint8_t weights_def[]={19,9,5,4,3};
        const uint8_t *weights=corr?weights_corr:weights_def;
        pi_a[pi].weight+=weights[d_block0errs];
        pi_a[pi].count++;
        int bit_offset=d_bit_counter-pi_a[pi].lastseen;
        pi_a[pi].lastseen=d_bit_counter;
        if(!corr)
        {
            if((pi_a[pi].weight>=22)&&((bit_offset+1)%GROUP_SIZE < 3))
                pi_a[pi].weight+=weights[d_block0errs]>>2;
            if((pi_a[pi].weight>=22)&&(bit_offset%GROUP_SIZE == 0))
                pi_a[pi].weight+=weights[d_block0errs]>>1;
        }
        int threshold=55;
        if(pi_a[pi].weight<threshold)
        {
            if(pi_a[pi].weight>d_max_weight[corr])
            {
                d_max_weight[corr]=pi_a[pi].weight;
                //printf("%c[%04x] %d (%d,%d) %d %d %6.2f\n",corr?':':'?',pi,((bit_offset+1)%GROUP_SIZE < 3),pi_a[pi].weight,pi_a[pi].count,d_block0errs,errs,double(d_weight));
                if(!corr)
                    d_matches[pi].push_back(grp_array(p_grp));
                tmp_grp[0]=pi;
                tmp_grp[1]=pi_a[pi].weight;
                offset_chars[0]='?';
                offset_chars[1]=offset_chars[2]=offset_chars[3]='x';
                decode_group(tmp_grp,errs);
            }
        }else{
            if((d_state==NO_SYNC)||(d_best_pi!=pi))
            {
                //printf("%c[%04x] %d (%d,%d) %d %d %6.2f!!\n",corr?'>':'+',pi,((bit_offset+1)%GROUP_SIZE < 3),pi_a[pi].weight,pi_a[pi].count,d_block0errs,errs,double(d_weight));
                if(!corr)
                {
                    auto & prv=d_matches[pi];
                    for(unsigned k=0;k<prv.size();k++)
                    {
                        std::memcpy(tmp_grp,prv[k].data,sizeof(prv[k].data));
                        d_prev_errs=process_group(tmp_grp, d_ecc_max, offset_chars, locators);
                        tmp_grp[0]=pi;
                        tmp_grp[1]=(tmp_grp[1]>>10)^locators[1];
                        tmp_grp[2]=(tmp_grp[2]>>10)^locators[2];
                        tmp_grp[3]=(tmp_grp[3]>>10)^locators[3];
                        offset_chars[0]='A';
                        decode_group(tmp_grp,errs);
                    }
                }
                errs=process_group(p_grp, d_ecc_max, offset_chars, locators);
                tmp_grp[0]=pi;
                tmp_grp[1]=(p_grp[1]>>10)^locators[1];
                tmp_grp[2]=(p_grp[2]>>10)^locators[2];
                tmp_grp[3]=(p_grp[3]>>10)^locators[3];
                offset_chars[0]='A';
                if(corr)
                    offset_chars[1]=offset_chars[2]=offset_chars[3]='x';
                decode_group(tmp_grp,errs);
                for(int jj=0;jj<65536;jj++)
                {
                    d_pi_b[jj].weight=0;
                    d_pi_b[jj].count=0;
                }
            }
            if(!corr)
                d_matches.clear();
            for(int jj=0;jj<65536;jj++)
                if(jj==pi)
                {
                    pi_a[jj].weight>>=1;
                    pi_a[jj].count>>=1;
                }else{
                    pi_a[jj].weight>>=2;
                    pi_a[jj].count>>=2;
                }
            d_pi_bitcnt[corr]=0;
            d_max_weight[corr]>>=1;
            //memset(d_accum.data(),0,d_accum.size()*sizeof(d_accum[0]));
            //d_acc_cnt=0;
            return pi;
        }
        d_pi_bitcnt[corr]++;
        if(d_pi_bitcnt[corr]>BLOCK_SIZE*(25+corr*25))
        {
            d_pi_bitcnt[corr]=0;
            for(int jj=0;jj<65536;jj++)
            {
                if(pi_a[jj].weight>0)
                    pi_a[jj].weight--;
                if(pi_a[jj].count>0)
                    pi_a[jj].count--;
            }
            d_max_weight[corr]--;
            //std::cout<<"d_pi_bitcnt--\n";
        }
    }
    return -1;
}

void decoder_impl::reset()
{
    reset_corr();
    d_state = NO_SYNC;
}

void decoder_impl::reset_corr()
{
    std::memset(d_pi_stats,0,sizeof(d_pi_stats));
    memset(d_accum.data(),0,d_accum.size()*sizeof(d_accum[0]));
    d_acc_cnt=0;
    d_bit_counter=0;
    d_max_weight[0]=d_max_weight[1]=0;
    d_best_pi=-1;
    d_pi_bitcnt[0]=d_pi_bitcnt[1]=0;
    d_valid_bits=0;
    d_matches.clear();
}
