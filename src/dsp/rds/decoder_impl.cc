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
decoder::make(bool corr, bool log, bool debug) {
  return gnuradio::get_initial_sptr(new decoder_impl(corr, log, debug));
}

decoder_impl::decoder_impl(bool corr, bool log, bool debug)
	: gr::sync_block ("gr_rds_decoder",
			gr::io_signature::make (1, 1, sizeof(char)),
			gr::io_signature::make (0, 0, 0)),
	log(log),
	debug(debug),
	d_corr(corr)
{
	bit_counter=0;
	set_output_multiple(GROUP_SIZE);  // 1 RDS datagroup = 104 bits
	message_port_register_out(pmt::mp("out"));
	enter_no_sync();
	prev_grp=&groups[0];
	group=&groups[4];
	next_grp=&groups[8];
}

decoder_impl::~decoder_impl() {
}

bool decoder_impl::start()
{
    bit_counter=-BLOCK_SIZE;
    d_bit_counter=0;
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
    if(loc)
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
    if(loc)
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
        if(loc)
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
        if(loc)
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
    if(loc)
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
    const bool *in = (const bool *) input_items[0];
    (void) output_items;

/*    dout << "RDS data decoder at work: input_items = "
        << noutput_items << ", /104 = "
        << noutput_items / 104 << std::endl;
*/
    int i=0,j;
    unsigned long bit_distance, block_distance;
    unsigned int block_calculated_crc, block_received_crc, checkword,dataword;
    unsigned int reg_syndrome;
    unsigned char offset_char('x');  // x = error while decoding the word offset
    int ecc_max = d_ecc_max;

/* the synchronization process is described in Annex C, page 66 of the standard */
    while (i<noutput_items) {
        std::swap(group,prev_grp);
        std::memcpy(group,next_grp,sizeof(group[0])*4);
        group[0]=((group[0]<<1)|(group[1]>>25))&((1<<26)-1);
        group[1]=((group[1]<<1)|(group[2]>>25))&((1<<26)-1);
        group[2]=((group[2]<<1)|(group[3]>>25))&((1<<26)-1);
//        group[3]=((group[3]<<1)|(group[4]>>25))&((1<<26)-1);
//        group[4]=((group[4]<<1)|in[i])&((1<<26)-1);
        group[3]=((group[3]<<1)|in[i])&((1<<26)-1);
        std::swap(group,next_grp);
    	unsigned int  *good_group=group;
        ++i;
        ++bit_counter;
        ++d_bit_counter;
        if(bit_counter>=GROUP_SIZE+1)
        {
            uint16_t locators[5]={0,0,0,0,0};
            int good_grp=0;
            int bit_errors=0;
            auto old_sync = d_state;
            if(d_state==SYNC)
            {
                //handle +/- 1 bit shifts
                d_prev_errs=process_group(prev_grp);
                d_curr_errs=process_group(group);
                d_next_errs=process_group(next_grp);
                bit_counter=1;
                if((d_prev_errs<d_curr_errs)&&(d_prev_errs<d_next_errs)&&(d_prev_errs<12))
                {
                    good_group=prev_grp;
                    bit_counter=2;
                    printf("<->\n");
                }else if((d_next_errs<d_curr_errs)&&(d_next_errs<d_prev_errs)&&(d_next_errs<12))
                {
                    good_group=next_grp;
                    bit_counter=0;
                    printf("<+>\n");
                }/*else if((curr_errs<prev_errs)&&(curr_errs<next_errs))*/
                 reset_corr();
            }else{
                d_prev_errs=d_curr_errs;
                d_curr_errs=d_next_errs;
                d_next_errs=process_group(next_grp, ecc_max, offset_chars, locators);
                uint16_t pi = (next_grp[0]>>10)^locators[0];
                if(d_block0errs<5)
                {
                    constexpr uint8_t weights[]={19,9,5,4,3};
                    d_pi_a[pi].weight+=weights[d_block0errs];
                    d_pi_a[pi].count++;
                    int bit_offset=d_bit_counter-d_pi_a[pi].lastseen;
                    d_pi_a[pi].lastseen=d_bit_counter;
                    /*
                    if((d_pi_a[pi].weight>=22)&&(d_block0errs<3)&&((bit_offset+1)%GROUP_SIZE < 3))
                        d_pi_a[pi].weight+=2;
                    if((d_pi_a[pi].weight>=22)&&(d_block0errs<5)&&(bit_offset%GROUP_SIZE == 0))
                        d_pi_a[pi].weight+=4;
                    if((d_pi_a[pi].weight>=22)&&(d_block0errs<3)&&(bit_offset%GROUP_SIZE == 0))
                        d_pi_a[pi].weight+=6;
                    if((d_pi_a[pi].weight>=22)&&(d_block0errs==0)&&(bit_offset%GROUP_SIZE == 0))
                        d_pi_a[pi].weight+=15;
                        */
                    if((d_pi_a[pi].weight>=22)&&((bit_offset+1)%GROUP_SIZE < 3))
                        d_pi_a[pi].weight+=weights[d_block0errs]>>2;
                    if((d_pi_a[pi].weight>=22)&&(bit_offset%GROUP_SIZE == 0))
                        d_pi_a[pi].weight+=weights[d_block0errs]>>1;
                    if(d_pi_a[pi].weight<55)
                    {
                        if(d_pi_a[pi].weight>d_max_weight)
                        {
                            d_max_weight=d_pi_a[pi].weight;
                            printf("?[%04x] %d (%d,%d) %d %d\n",pi,((bit_offset+1)%GROUP_SIZE < 3),d_pi_a[pi].weight,d_pi_a[pi].count,d_block0errs,d_next_errs);
                            d_matches[pi].push_back(grp_array(next_grp));
                            prev_grp[0]=pi;
                            prev_grp[1]=d_pi_a[pi].weight;
                            offset_chars[0]='?';
                            offset_chars[1]=offset_chars[2]=offset_chars[3]='x';
                            decode_group(prev_grp,d_next_errs);
                        }
                    }else{
                        printf("+[%04x] %d (%d,%d) %d %d!!\n",pi,((bit_offset+1)%GROUP_SIZE < 3),d_pi_a[pi].weight,d_pi_a[pi].count,d_block0errs,d_next_errs);
                        auto & prv=d_matches[pi];
                        for(unsigned k=0;k<prv.size();k++)
                        {
                            std::memcpy(prev_grp,prv[k].data,sizeof(prv[k].data));
                            d_prev_errs=process_group(prev_grp, ecc_max, offset_chars, locators);
                            prev_grp[0]=pi;
                            prev_grp[1]=(prev_grp[1]>>10)^locators[1];
                            prev_grp[2]=(prev_grp[2]>>10)^locators[2];
                            prev_grp[3]=(prev_grp[3]>>10)^locators[3];
                            offset_chars[0]='A';
                            decode_group(prev_grp,0);
                        }
                        d_matches.clear();
                        d_next_errs=process_group(next_grp, ecc_max, offset_chars, locators);
                        prev_grp[0]=pi;
                        prev_grp[1]=(next_grp[1]>>10)^locators[1];
                        prev_grp[2]=(next_grp[2]>>10)^locators[2];
                        prev_grp[3]=(next_grp[3]>>10)^locators[3];
                        offset_chars[0]='A';
                        decode_group(prev_grp,0);
                        for(int jj=0;jj<65536;jj++)
                            if(jj==pi)
                            {
                                d_pi_a[jj].weight>>=1;
                                d_pi_a[jj].count>>=1;
                            }else{
                                d_pi_a[jj].weight>>=2;
                                d_pi_a[jj].count>>=2;
                            }
                        d_pi_bitcnt=0;
                        d_max_weight>>=1;
                        d_state=SYNC;
                        d_best_pi=pi;
                        bit_counter=1;
                        continue;
                    }
                    d_pi_bitcnt++;
                    if(d_pi_bitcnt>BLOCK_SIZE*50)
                    {
                        d_pi_bitcnt=0;
                        for(int jj=0;jj<65536;jj++)
                        {
                            if(d_pi_a[jj].weight>0)
                                d_pi_a[jj].weight--;
                            if(d_pi_a[jj].count>0)
                                d_pi_a[jj].count--;
                        }
                        d_max_weight--;
                        std::cout<<"d_pi_bitcnt--\n";
                    }
                }
                if(d_corr)
                    continue;
            }
            bit_errors=process_group(good_group, ecc_max, offset_chars, locators, &good_grp);
            char sync_point='E';
            if(d_state != FORCE_SYNC)
            {
                //if(((d_curr_errs>d_prev_errs)||(d_curr_errs>d_next_errs))&&(d_state!=SYNC))
                //    continue;
                if((d_best_pi==int((good_group[0]>>10)^locators[0]))&&(d_block0errs<5))
                {
                    if(d_state!=SYNC)
                    {
                        sync_point='P';
                        d_state = SYNC;
                    }
                    d_counter = 0;
                }
                if(bit_errors <= 1)
                {
                    if(d_state != SYNC)
                    {
                        d_best_pi=(good_group[0]>>10)^locators[0];
                        d_state = SYNC;
                        sync_point='N';
                    }
                }
                if(d_state==SYNC)
                {
                    if(good_grp>=4)
                    {
                        d_counter>>=2;
                        //d_counter=0;
                    }
                    if(good_grp>=3)
                    {
                        d_counter>>=1;
                    }
                    if(bit_errors > 15)
                    {
                        if(d_counter > 512)
                        {
                            d_state=NO_SYNC;
                            d_counter = 0;
                            printf("- NO Sync errors: %d\n",d_curr_errs);
                        }
                    }                }
                d_counter +=bit_errors;
                if(d_state != SYNC)
                    continue;
            }else{
                d_state = SYNC;
                offset_chars[0]='F';
                sync_point='F';
                d_counter = 0;
            }
//            if(bit_errors>5)
//                continue;
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

    while (i<noutput_items) {
        reg=(reg<<1)|in[i];		// reg contains the last 26 rds bits
        switch (d_state) {
            case NO_SYNC:
                reg_syndrome = calc_syndrome(reg,BLOCK_SIZE);
                for (j=0;j<5;j++) {
                    if (reg_syndrome==syndrome[j]) {
                        if (!presync) {
                            lastseen_offset=j;
                            lastseen_offset_counter=bit_counter;
                            presync=true;
                        }
                        else {
                            bit_distance=bit_counter-lastseen_offset_counter;
                            if (offset_pos[lastseen_offset]>=offset_pos[j])
                                block_distance=offset_pos[j]+4-offset_pos[lastseen_offset];
                            else
                                block_distance=offset_pos[j]-offset_pos[lastseen_offset];
                            if ((block_distance*BLOCK_SIZE)!=bit_distance) presync=false;
                            else {
                                lout << "@@@@@ Sync State Detected" << std::endl;
                                enter_sync(j);
                            }
                        }
                    break; //syndrome found, no more cycles
                    }
                }
            break;
            case SYNC:
/* wait until 26 bits enter the buffer */
                if (block_bit_counter<25) block_bit_counter++;
                else {
                    good_block=false;
                    dataword=(reg>>10) & 0xffff;
                    block_calculated_crc=calc_syndrome(dataword,16);
                    checkword=reg & 0x3ff;
/* manage special case of C or C' offset word */
                    if (block_number==2) {
                        block_received_crc=checkword^offset_word[block_number];
                        if (block_received_crc==block_calculated_crc) {
                            good_block=true;
                            offset_char = 'C';
                        } else {
                            block_received_crc=checkword^offset_word[4];
                            if (block_received_crc==block_calculated_crc) {
                                good_block=true;
                                offset_char = 'c';  // C' (C-Tag)
                            } else {
                                wrong_blocks_counter++;
                                good_block=false;
                            }
                        }
                    }
                    else {
                        block_received_crc=checkword^offset_word[block_number];
                        if (block_received_crc==block_calculated_crc) {
                            good_block=true;
                            if (block_number==0) offset_char = 'A';
                            else if (block_number==1) offset_char = 'B';
                            else if (block_number==3) offset_char = 'D';
                        } else {
                            wrong_blocks_counter++;
                            good_block=false;
                        }
                    }
/* done checking CRC */
                    if (block_number==0 && good_block) {
                        group_assembly_started=true;
                        group_good_blocks_counter=1;
                    }
                    if (group_assembly_started) {
                        if (!good_block) group_assembly_started=false;
                        else {
                            group[block_number]=dataword;
                            offset_chars[block_number] = offset_char;
                            group_good_blocks_counter++;
                        }
//                        if (group_good_blocks_counter==5) decode_group(group);
                    }
                    block_bit_counter=0;
                    block_number=(block_number+1) % 4;
                    blocks_counter++;
/* 1187.5 bps / 104 bits = 11.4 groups/sec, or 45.7 blocks/sec */
                    if (blocks_counter==50) {
                        if (wrong_blocks_counter>35) {
                            lout << "@@@@@ Lost Sync (Got " << wrong_blocks_counter
                                << " bad blocks on " << blocks_counter
                                << " total)" << std::endl;
                            enter_no_sync();
                        } else {
                            lout << "@@@@@ Still Sync-ed (Got " << wrong_blocks_counter
                                << " bad blocks on " << blocks_counter
                                << " total)" << std::endl;
                        }
                        blocks_counter=0;
                        wrong_blocks_counter=0;
                    }
                }
            break;
            default:
                d_state=NO_SYNC;
            break;
        }
        i++;
        bit_counter++;
    }
    return noutput_items;
}

void decoder_impl::reset_corr()
{
    std::memset(d_pi_a,0,sizeof(d_pi_a));
    d_bit_counter=0;
    d_max_weight=0;
    d_best_pi=-1;
    d_pi_bitcnt=0;
    d_matches.clear();
}
