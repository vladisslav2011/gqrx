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
        tmp[p]={0,1};
    }
    if(1)
    for(k=0;k<16;k++)
        for(j=k+1;j<16;j++)
        {
            uint16_t e=(1<<k)^(1<<j);
            uint32_t p=calc_syndrome(b,16)^calc_syndrome(b^e,16);
            tmp[p]={e,2};
        }
    if(1)
    for(j=3;j<6;j++)
//    for(j=3;j<4;j++)
        for(k=0;k<17-j;k++)
        {
            uint16_t e=(((1<<j)-1)<<k)&((1<<16)-1);
            uint32_t p=calc_syndrome(b,16)^calc_syndrome(b^e,16);
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
	set_output_multiple(104);  // 1 RDS datagroup = 104 bits
	message_port_register_out(pmt::mp("out"));
	enter_no_sync();
}

decoder_impl::~decoder_impl() {
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

void decoder_impl::decode_group(int bit_errors)
{
	// raw data bytes, as received from RDS.
	// 8 info bytes, followed by 4 RDS offset chars: ABCD/ABcD/EEEE (in US)
	unsigned char bytes[13];

	// RDS information words
	bytes[0] = (group[0] >> 8U) & 0xffU;
	bytes[1] = (group[0]      ) & 0xffU;
	bytes[2] = (group[1] >> 8U) & 0xffU;
	bytes[3] = (group[1]      ) & 0xffU;
	bytes[4] = (group[2] >> 8U) & 0xffU;
	bytes[5] = (group[2]      ) & 0xffU;
	bytes[6] = (group[3] >> 8U) & 0xffU;
	bytes[7] = (group[3]      ) & 0xffU;

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

/* the synchronization process is described in Annex C, page 66 of the standard */
    while (i<noutput_items) {
        group[0]=((group[0]<<1)|(group[1]>>25))&((1<<26)-1);
        group[1]=((group[1]<<1)|(group[2]>>25))&((1<<26)-1);
        group[2]=((group[2]<<1)|(group[3]>>25))&((1<<26)-1);
        group[3]=((group[3]<<1)|(group[4]>>25))&((1<<26)-1);
        group[4]=((group[4]<<1)|in[i])&((1<<26)-1);
        ++i;
        ++bit_counter;
        if(bit_counter>=26*5)
        {
            offset_chars[0]='A';
            offset_chars[1]='B';
            offset_chars[2]='C';
            offset_chars[3]='D';
            uint16_t locators[5]={0,0,0,0,0};
            uint16_t errors[5]={0,0,0,0,0};
            uint16_t s;
            uint16_t w;
            auto old_sync = d_state;
            s=calc_syndrome(group[0]>>10,16)^(group[0]&0x3ff);
            locators[0]=locator[s^offset_word[0]].l;
            if(locator[s^offset_word[0]].w>0)
                offset_chars[0]='x';
            errors[0]+=locator[s^offset_word[0]].w;
            errors[1]+=locator[s^offset_word[3]].w;
            errors[2]+=std::min(locator[s^offset_word[2]].w,locator[s^offset_word[4]].w);
            errors[3]+=locator[s^offset_word[1]].w;
            errors[4]+=locator[s^offset_word[0]].w;
            s=calc_syndrome(group[4]>>10,16)^(group[4]&0x3ff);
            locators[4]=locator[s^offset_word[0]].l;
            errors[4]=locator[s^offset_word[0]].w;
            if((errors[0]<2)&&(errors[4]<2)&&(((group[0]>>10)^locators[0]) == ((group[4]>>10)^locators[4])))
            {
                uint16_t pi = (group[0]>>10)^locators[0];
                if(d_corr)
                {
                    if((pi == d_prev_pi)&&(errors[0] == 0))
                    {
                        if(d_pi_cnt<3)
                            d_pi_cnt++;
                        else{
                            printf("+[%04x] %d, %d\n",pi,errors[0], d_pi_cnt);
                            group[0]=(group[0]>>10)^locators[0];
                            group[1]=(group[1]>>10)^locators[1];
                            group[2]=(group[2]>>10)^locators[2];
                            group[3]=(group[3]>>10)^locators[3];
                            decode_group(errors[0]);
                        }
                        continue;
                    }else{
                        d_prev_pi=pi;
                        d_pi_cnt=0;
                    }
                }else{
                    if(d_state != SYNC)
                        printf("+[%04x] %d\n",pi,errors[0]);
                    if(errors[0] == 0)
                        d_state = FORCE_SYNC;
                }
            }
            if(d_corr)
                continue;
            s=calc_syndrome(group[1]>>10,16)^(group[1]&0x3ff);
            locators[1]=locator[s^offset_word[1]].l;
            if(locator[s^offset_word[1]].w>d_ecc_max)
                offset_chars[1]='x';
            errors[0]+=locator[s^offset_word[1]].w;
            errors[1]+=locator[s^offset_word[0]].w;
            errors[2]+=locator[s^offset_word[3]].w;
            errors[3]+=std::min(locator[s^offset_word[2]].w,locator[s^offset_word[4]].w);
            errors[4]+=locator[s^offset_word[1]].w;

            s=calc_syndrome(group[2]>>10,16)^(group[2]&0x3ff);
            locators[2]=locator[s^offset_word[2]].l;
            w=locator[s^offset_word[2]].w;
            offset_chars[2]='C';
            if(w > locator[s^offset_word[4]].w)
            {
                offset_chars[2]='c';
                w=locator[s^offset_word[4]].w;
                locators[2]=locator[s^offset_word[4]].l;
                if(locator[s^offset_word[4]].w>d_ecc_max)
                    offset_chars[2]='x';
            }else if(locator[s^offset_word[2]].w>d_ecc_max)
                    offset_chars[2]='x';

            errors[0]+=w;
            errors[1]+=locator[s^offset_word[1]].w;
            errors[2]+=locator[s^offset_word[0]].w;
            errors[3]+=locator[s^offset_word[3]].w;
            errors[4]+=w;

            s=calc_syndrome(group[3]>>10,16)^(group[3]&0x3ff);
            if(locator[s^offset_word[3]].w>d_ecc_max)
                offset_chars[3]='x';
            locators[3]=locator[s^offset_word[3]].l;
            errors[0]+=locator[s^offset_word[3]].w;
            errors[1]+=std::min(locator[s^offset_word[2]].w,locator[s^offset_word[4]].w);
            errors[2]+=locator[s^offset_word[1]].w;
            errors[3]+=locator[s^offset_word[0]].w;
            errors[4]+=locator[s^offset_word[3]].w;
            #if 0
            s=calc_syndrome(group[4]>>10,16)^(group[4]&0x3ff);
            locators[4]=locator[s^offset_word[0]].l;
            errors[0]+=locator[s^offset_word[0]].w;
            errors[1]+=locator[s^offset_word[3]].w;
            errors[2]+=std::min(locator[s^offset_word[2]].w,locator[s^offset_word[4]].w);
            errors[3]+=locator[s^offset_word[1]].w;
            errors[4]+=locator[s^offset_word[0]].w;
            if(((group[0]>>10)^locators[0]) != ((group[4]>>10)^locators[4]))
                continue;
            #endif
            int bit_errors=
                __builtin_popcount(locators[0])+__builtin_popcount(locators[1])+__builtin_popcount(locators[2])+__builtin_popcount(locators[3]);
            if(d_state != FORCE_SYNC)
            {
                if((std::min(std::min(errors[0],errors[1]),std::min(errors[2],errors[3])) != errors[0])&&(d_state!=SYNC))
                    continue;
                if(errors[0] > 5)
                {
                    if(d_state!=SYNC)
                        continue;
                }else{
                    d_state = SYNC;
                    d_counter = errors[0]*2;
                }
                if((errors[0] > 15)&&(d_state==SYNC))
                {
                    d_counter ++;
                    if(d_counter > 12)
                    {
                        d_state=NO_SYNC;
                        printf("- NO Sync errors: %d\n",errors[0]);
                    }
                }
                if(d_state != SYNC)
                    continue;
            }else{
                d_state = SYNC;
                offset_chars[0]='F';
                d_counter = 0;
            }
//            if(bit_errors>5)
//                continue;
            group[0]=(group[0]>>10)^locators[0];
            group[1]=(group[1]>>10)^locators[1];
            group[2]=(group[2]>>10)^locators[2];
            group[3]=(group[3]>>10)^locators[3];
            decode_group(bit_errors);
            if(d_state != old_sync)
                printf("Sync %c %04x Corrected: %d %d\n",offset_chars[2],group[0],bit_errors,errors[0]);
            bit_counter=26;
        }
    }
    decode_group(127);
    return noutput_items;
    while (i<noutput_items) {
        reg=(reg<<1)|in[i];		// reg contains the last 26 rds bits
        switch (d_state) {
            case NO_SYNC:
                reg_syndrome = calc_syndrome(reg,26);
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
                            if ((block_distance*26)!=bit_distance) presync=false;
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
