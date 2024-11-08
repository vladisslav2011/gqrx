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
#ifndef INCLUDED_RDS_PARSER_H
#define INCLUDED_RDS_PARSER_H

#include "dsp/rds/api.h"
#include <gnuradio/block.h>

namespace gr {
namespace rds {

class RDS_API parser : virtual public gr::block
{
public:
#if GNURADIO_VERSION < 0x030900
	typedef boost::shared_ptr<parser> sptr;
#else
	typedef std::shared_ptr<parser> sptr;
#endif
	static sptr make(bool log, bool debug, unsigned char pty_locale);

	virtual void reset() = 0;
};

} // namespace rds
} // namespace gr

#endif /* INCLUDED_RDS_PARSER_H */

