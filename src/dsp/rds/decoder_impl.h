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
#ifndef INCLUDED_RDS_DECODER_IMPL_H
#define INCLUDED_RDS_DECODER_IMPL_H

#include "dsp/rds/decoder.h"
#include <atomic>
#include <vector>
#include <map>

namespace gr {
namespace rds {

class decoder_impl : public decoder
{
public:
	decoder_impl(bool corr, bool log, bool debug);
	void set_ecc_max(int n) {d_ecc_max = n;}
	void reset_corr();

private:
    constexpr static int BLOCK_SIZE{26};
    constexpr static int GROUP_SIZE{26*4};
    struct bit_locator
    {
        uint16_t l;
        uint8_t w;
    };
    struct pi_stats
    {
        char count;
        char weight;
        unsigned lastseen;
    };
    struct grp_array
    {
        unsigned int data[4];
        grp_array(const unsigned int * from)
        {
            std::memcpy(data,from,sizeof(data));
        }
    };
	~decoder_impl();

	int work(int noutput_items,
			gr_vector_const_void_star &input_items,
			gr_vector_void_star &output_items);
    bool start() override;

	void enter_no_sync();
	void enter_sync(unsigned int);
	static unsigned int calc_syndrome(unsigned long, unsigned char);
	void decode_group(unsigned *, int);
	static std::array<bit_locator,1024> build_locator();
    int process_group(unsigned * grp, int thr=0, unsigned char * offs_chars=nullptr, uint16_t * loc=nullptr);

	int            bit_counter;
	unsigned long  lastseen_offset_counter, reg;
	unsigned int   block_bit_counter;
	unsigned int   wrong_blocks_counter;
	unsigned int   blocks_counter;
	unsigned int   group_good_blocks_counter;
	unsigned int   groups[4*3];
	unsigned int  *prev_grp;
	unsigned int  *group;
	unsigned int  *next_grp;
	unsigned char  offset_chars[4];  // [ABCcDEx] (x=error)
	bool           log;
	bool           debug;
	bool           presync;
	bool           good_block;
	bool           group_assembly_started;
	unsigned char  lastseen_offset;
	unsigned char  block_number;
	enum { NO_SYNC, SYNC, FORCE_SYNC } d_state;
	static const std::array<bit_locator,1024> locator;
	bool           d_corr{false};
	uint16_t       d_prev_pi{0};
	int            d_pi_cnt{0};
	int            d_counter{0};
	std::atomic<int> d_ecc_max{0};
	pi_stats       d_pi_a[65536]{};
	int            d_bit_counter;
	int            d_pi_bitcnt{0};
	char           d_max_weight{0};
    int            d_prev_errs{0};
    int            d_curr_errs{0};
    int            d_next_errs{0};
    int            d_block0errs{0};
    std::map<uint16_t,std::vector<grp_array>> d_matches{};
    int            d_best_pi{-1};

};

} /* namespace rds */
} /* namespace gr */

#endif /* INCLUDED_RDS_DECODER_IMPL_H */
