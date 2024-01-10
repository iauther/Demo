/*
 * JFileIStream.cpp
 *
 * Copyright (C) 2002-2020 Joris Heirbaut
 *
 * This file is part of JojoDiff.
 *
 * JojoDiff is free software: you can redistribute it and/or modify
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

#ifndef JFILEISTREAM_H_
#define JFILEISTREAM_H_

#include <istream>
using namespace std ;

#include "JDefs.h"
#include "JFile.h"

namespace JojoDiff {

/*
 * Unbuffered IStream access: all calls to JFile go straight through to istream.
 */
class JFileIStream: public JFile {
public:
    /**
     * Construct an unbuffered JFile on an istream.
     */
	JFileIStream(istream &apFil, char const * const asFid, const bool abSeq = false);

	/**
	 * Destroy the JFile.
	 */
	virtual ~JFileIStream();

	/**
	 * Get one byte from the file from the given position.
	 */
	int get (
		    const off_t &azPos,	        /* position to read from                */
		    const eAhead aiSft = Read   /* 0=read, 1=hard ahead, 2=soft ahead   */
		);

	/**
	 * @brief Get next byte
	 *
	 * Soft read ahead will return an EOB when date is not available in the buffer.
	 *
	 * @param   aiSft	soft reading type: 0=read, 1=hard read ahead, 2=soft read ahead
	 * @return 			the read character or EOF or EOB.
	 */
	virtual int get (
	    const eAhead aiSft = Read   /* 0=read, 1=hard ahead, 2=soft ahead   */
	) ;

	/**
	 * @brief Set lookahead base: soft lookahead will fail when reading after base + buffer size
	 *
	 * Attention: the base will be implicitly changed by get on non-buffered reads too !
	 *
	 * @param   azBse	base position
	 */
	virtual void set_lookahead_base (
	    const off_t azBse	/* new base position for soft lookahead */
	) ;

protected:

    /**
    * @brief Seek EOF
    *
    * @return >= 0: EOF position, EXI_SEK in case of error
    */
    off_t jeofpos() ;

private:
	/* Context */
    istream    &mpFil;      /**< file handle                            */

    /* State */
    off_t mzPosInp;         /**< current position in file                   */

};
}
#endif /* JFILEISTREAM_H_ */
