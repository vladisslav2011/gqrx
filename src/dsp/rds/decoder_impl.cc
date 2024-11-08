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

int decoder_impl::group_ecc::process(unsigned * grp, int thr)
{
    int ret=0;
    offset_chars[0]='A';
    offset_chars[1]='B';
    offset_chars[2]='C';
    offset_chars[3]='D';
    locators[0]=locators[1]=locators[2]=locators[3]=0;
    uint16_t s;
    int g1=0;
    int g2=0;

    s=decoder_impl::calc_syndrome(grp[0]>>10,16)^(grp[0]&0x3ff);
    const bit_locator & bl=decoder_impl::locator[s^offset_word[0]];
    if(bl.l!=0xffff)
        locators[0]=bl.l;
    if(bl.w>0)
        offset_chars[0]='x';
    if(bl.w==1)
        g1++;
    if(bl.w==2)
        g2++;
    ret+=bl.w;
    errs[0]=bl.w;

    s=calc_syndrome(grp[1]>>10,16)^(grp[1]&0x3ff);
    const bit_locator & bl1=decoder_impl::locator[s^offset_word[1]];
    if(bl.l!=0xffff)
        locators[1]=bl1.l;
    if(bl1.w>thr)
        offset_chars[1]='x';
    if(bl1.w==1)
        g1++;
    if(bl1.w==2)
        g2++;
    ret+=bl1.w;

    s=calc_syndrome(grp[2]>>10,16)^(grp[2]&0x3ff);
    const bit_locator & bl2=decoder_impl::locator[s^offset_word[2]];
    const bit_locator & bl4=decoder_impl::locator[s^offset_word[4]];
    if(bl2.w<bl4.w)
    {
        if(bl.l!=0xffff)
            locators[2]=bl2.l;
        if(bl2.w>thr)
            offset_chars[2]='x';
        if(bl2.w==1)
            g1++;
        if(bl2.w==2)
            g2++;
        ret+=bl2.w;
    }else{
        offset_chars[2]='c';
        if(bl.l!=0xffff)
            locators[2]=bl4.l;
        if(bl4.w>thr)
            offset_chars[2]='x';
        if(bl4.w==1)
            g1++;
        if(bl4.w==2)
            g2++;
        ret+=bl4.w;
    }

    s=calc_syndrome(grp[3]>>10,16)^(grp[3]&0x3ff);
    const bit_locator & bl3=decoder_impl::locator[s^offset_word[3]];
    if(bl.l!=0xffff)
        locators[3]=bl3.l;
    if(bl3.w>thr)
        offset_chars[3]='x';
    if(bl3.w==1)
        g1++;
    if(bl3.w==2)
        g2++;
    ret+=bl3.w;

    good_grp=g1*3+g2;
    for(int k=0;k<4;k++)
        res[k]=(grp[k]>>10)^locators[k];
    return ret;
}

void decoder_impl::decode_group(group_ecc &ecc, int bit_errors)
{
	// raw data bytes, as received from RDS.
	// 8 info bytes, followed by 4 RDS offset chars: ABCD/ABcD/EEEE (in US)
	unsigned char bytes[13];

	// RDS information words
	bytes[0] = (ecc.res[0] >> 8U) & 0xffU;
	bytes[1] = (ecc.res[0]      ) & 0xffU;
	bytes[2] = (ecc.res[1] >> 8U) & 0xffU;
	bytes[3] = (ecc.res[1]      ) & 0xffU;
	bytes[4] = (ecc.res[2] >> 8U) & 0xffU;
	bytes[5] = (ecc.res[2]      ) & 0xffU;
	bytes[6] = (ecc.res[3] >> 8U) & 0xffU;
	bytes[7] = (ecc.res[3]      ) & 0xffU;

	// RDS offset words
	bytes[8] = ecc.offset_chars[0];
	bytes[9] = ecc.offset_chars[1];
	bytes[10] = ecc.offset_chars[2];
	bytes[11] = ecc.offset_chars[3];
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
    group_ecc ecc;

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
                d_prev_errs=ecc.process(prev_grp);
                uint16_t prev_pi=ecc.res[0];
                d_curr_errs=ecc.process(group);
                d_next_errs=ecc.process(next_grp);
                uint16_t next_pi=ecc.res[0];
                bit_counter=1;
                if((d_prev_errs<d_curr_errs)&&(d_prev_errs<d_next_errs)&&(d_prev_errs<12)&&(prev_pi==d_best_pi))
                {
                    good_group=prev_grp;
                    bit_counter=2;
                    if(log||debug)
                        printf("<->\n");
                }else if((d_next_errs<d_curr_errs)&&(d_next_errs<d_prev_errs)&&(d_next_errs<12)&&(next_pi==d_best_pi))
                {
                    good_group=next_grp;
                    bit_counter=0;
                    if(log||debug)
                        printf("<+>\n");
                }
            }
            bit_errors=ecc.process(good_group, ecc_max);
            good_grp=ecc.good_grp;
            char sync_point='E';
            if(d_state != FORCE_SYNC)
            {
                int curr_pi=ecc.res[0];
                if((d_best_pi==curr_pi)&&(ecc.errs[0]<5))
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
                            if(log||debug)
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
                sync_point='F';
                d_counter = 0;
            }
            if((bit_errors>0) && (d_integrate_ps_dist>=8) && d_integrate_ps)
            {
                int p=4;
                int bestp=0;
                int bestq=0;
                int best_errs=bit_errors;
                int search_lim=std::min(d_valid_bits / GROUP_SIZE, d_integrate_ps_dist);
                group_ecc iecc;
                group_ecc tecc;
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
                        if(!d_used_list[q])
                        {
                            volk_32f_x2_add_32f(&accum[0],&in[bitp],&in[bito],GROUP_SIZE);
                            if(q)
                                volk_32f_x2_add_32f(&accum[0],&accum[0],&in[bit2],GROUP_SIZE);
                            for(int h=0;h<GROUP_SIZE;h++)
                            {
                                const float flt=accum[h];
                                const int sample=flt>0.f;
                                for(int j=0;j<n_group-1;j++)
                                    prev_grp[j]=((prev_grp[j]<<1)|(prev_grp[j+1]>>25))&((1<<26)-1);
                                prev_grp[n_group-1]=((prev_grp[n_group-1]<<1)|sample)&((1<<26)-1);
                            }
                            int errs=iecc.process(prev_grp, ecc_max);
                            if(errs<best_errs)
                            {
                                best_errs=errs;
                                bestp=p;
                                bestq=q;
                                tecc=iecc;
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
                    if((tecc.offset_chars[1]!='x')&&(tecc.offset_chars[2]!='x')&&(tecc.offset_chars[3]!='x'))
                        best_errs=0;
                    bit_errors=best_errs;
                    ecc=tecc;
                    ecc.offset_chars[0]='x';
                    if(d_integrate_ps < INTEGRATE_PS_23_PLUS && best_errs==0)
                    {
                        d_used_list[0]=1;
                        d_used_list[bestp]=1;
                        d_used_list[bestq]=1;
                    }
                }
            }else{
                if(d_integrate_ps < INTEGRATE_PS_23_PLUS)
                    d_used_list[0]=1;
            }
            decode_group(ecc, bit_errors);
            if(d_state != old_sync && (log||debug))
                printf("Sync %c %04x/%04x Corrected: %d %d %c\n",ecc.offset_chars[2],ecc.res[0],d_best_pi,bit_errors,ecc_max,sync_point);
        }
    }
    decode_group(ecc, 127);
    return noutput_items;
}

int decoder_impl::pi_detect(uint32_t * p_grp, bool corr)
{
    return pi_detect(p_grp,pi_int[0],corr);
}

int decoder_impl::pi_detect(uint32_t * p_grp, decoder_impl::integr_ctx &x, bool corr)
{
    int errs=x.ecc.process(p_grp, d_ecc_max);
    pi_stats       *pi_a=corr?x.pi_b:x.pi_a;
    uint16_t pi = (p_grp[0]>>10)^x.ecc.locators[0];
    if(x.ecc.errs[0]<5)
    {
        static const uint8_t weights_corr[]={19,9,1,1,0};
        static const uint8_t weights_def[]={19,9,5,4,3};
        const uint8_t *weights=corr?weights_corr:weights_def;
        pi_a[pi].weight+=weights[x.ecc.errs[0]];
        pi_a[pi].count++;
        int bit_offset=d_bit_counter-pi_a[pi].lastseen;
        pi_a[pi].lastseen=d_bit_counter;
        if(!corr)
        {
            if((pi_a[pi].weight>=22)&&((bit_offset+1)%GROUP_SIZE < 3))
                pi_a[pi].weight+=weights[x.ecc.errs[0]]>>2;
            if((pi_a[pi].weight>=22)&&(bit_offset%GROUP_SIZE == 0))
                pi_a[pi].weight+=weights[x.ecc.errs[0]]>>1;
        }
        int threshold=55;
        if(pi_a[pi].weight<threshold)
        {
            if(pi_a[pi].weight>x.max_weight[corr])
            {
                x.max_weight[corr]=pi_a[pi].weight;
                if(log||debug)
                    printf("%c[%04x] %d (%d,%d) %d %d %6.2f\n",
                        corr?':':'?',
                        pi,
                        ((bit_offset+1)%GROUP_SIZE < 3),
                        pi_a[pi].weight,
                        pi_a[pi].count,
                        x.ecc.errs[0],
                        errs,
                        double(d_weight)
                    );
                if(!corr)
                    x.matches[pi].push_back(grp_array(p_grp));
                x.ecc.res[0]=pi;
                x.ecc.res[1]=pi_a[pi].weight;
                x.ecc.offset_chars[0]='?';
                x.ecc.offset_chars[1]=x.ecc.offset_chars[2]=x.ecc.offset_chars[3]='x';
                decode_group(x.ecc,errs);
            }
        }else{
            if((d_state==NO_SYNC)||(d_best_pi!=pi))
            {
                if(log||debug)
                    printf("%c[%04x] %d (%d,%d) %d %d %6.2f!!\n",
                        corr?'>':'+',
                        pi,
                        ((bit_offset+1)%GROUP_SIZE < 3),
                        pi_a[pi].weight,
                        pi_a[pi].count,
                        x.ecc.errs[0],
                        errs,
                        double(d_weight)
                    );
                if(!corr)
                {
                    auto & prv=x.matches[pi];
                    for(unsigned k=0;k<prv.size();k++)
                    {
                        d_prev_errs=x.ecc.process(prv[k].data, d_ecc_max);
                        x.ecc.res[0]=pi;
                        x.ecc.offset_chars[0]='A';
                        decode_group(x.ecc,errs);
                    }
                }
                errs=x.ecc.process(p_grp, d_ecc_max);
                x.ecc.res[0]=pi;
                x.ecc.offset_chars[0]='A';
                if(corr)
                    x.ecc.offset_chars[1]=x.ecc.offset_chars[2]=x.ecc.offset_chars[3]='x';
                decode_group(x.ecc,errs);
                for(int jj=0;jj<65536;jj++)
                {
                    x.pi_b[jj].weight=0;
                    x.pi_b[jj].count=0;
                }
            }
            if(!corr)
                x.matches.clear();
            for(int jj=0;jj<65536;jj++)
                if(jj==pi)
                {
                    pi_a[jj].weight>>=1;
                    pi_a[jj].count>>=1;
                }else{
                    pi_a[jj].weight>>=2;
                    pi_a[jj].count>>=2;
                }
            x.pi_bitcnt[corr]=0;
            x.max_weight[corr]>>=1;
            //memset(d_accum.data(),0,d_accum.size()*sizeof(d_accum[0]));
            //d_acc_cnt=0;
            return pi;
        }
        x.pi_bitcnt[corr]++;
        if(x.pi_bitcnt[corr]>BLOCK_SIZE*(25+corr*25))
        {
            x.pi_bitcnt[corr]=0;
            for(int jj=0;jj<65536;jj++)
            {
                if(pi_a[jj].weight>0)
                    pi_a[jj].weight--;
                if(pi_a[jj].count>0)
                    pi_a[jj].count--;
            }
            x.max_weight[corr]--;
            if(log||debug)
                std::cout<<"d_pi_bitcnt--\n";
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
    for(int k=0;k<2;k++)
    {
        std::memset(pi_int[k].pi_stats,0,sizeof(pi_int[k].pi_stats));
        pi_int[k].max_weight[0]=pi_int[k].max_weight[1]=0;
        pi_int[k].pi_bitcnt[0]=pi_int[k].pi_bitcnt[1]=0;
    }
    memset(d_accum.data(),0,d_accum.size()*sizeof(d_accum[0]));
    d_acc_cnt=0;
    d_bit_counter=0;
    d_best_pi=-1;
    d_valid_bits=0;
    d_matches.clear();
}
