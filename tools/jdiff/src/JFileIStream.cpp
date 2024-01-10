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

#include <stdlib.h>
#include <stdio.h>

#include "JFileIStream.h"
#include "JDebug.h"

namespace JojoDiff {
JFileIStream::JFileIStream(istream &apFil, char const * const asFid, const bool abSeq)
:JFile(asFid, abSeq), mpFil(apFil), mzPosInp(0)
{
    chkSeq() ;  // Check if file is sequential or not
}

JFileIStream::~JFileIStream() {
}

/**
* @brief Seek EOF
*
* @param EXI_OK (0) or EXI_SEK in cae of error
*/
off_t JFileIStream::jeofpos() {
    mpFil.seekg(0, mpFil.end) ;
    if (mpFil.fail()){
        mpFil.clear();
        return EXI_SEK;
    }
    else {
        off_t lzEof = mpFil.tellg();
        mpFil.seekg(0, mpFil.beg) ;
        return lzEof ;
    }
} ;

/**
 * @brief Set lookahead base: soft lookahead will fail when reading after base + buffer size
 *
 * Attention: the base will be implicitly changed by get on non-buffered reads too !
 *
 * @param   azBse	base position
 */
void JFileIStream::set_lookahead_base (
    const off_t azBse	/* new base position */
) {
    // no need to do anything
}

/**
 * @brief Get next byte
 *
 * Soft read ahead will return an EOB when date is not available in the buffer.
 *
 * @param   aiSft	soft reading type: 0=read, 1=hard read ahead, 2=soft read ahead
 * @return 			the read character or EOF or EOB.
 */
int JFileIStream::get (
    const eAhead aiSft   /* 0=read, 1=hard ahead, 2=soft ahead  */
) {
    return get(mzPosInp, aiSft);
}

/**
 * Gets one byte from the file.
 */
int JFileIStream:: get (
    const off_t &azPos,    	/* position to read from                */
    const eAhead aiSft      /* 0=read, 1=hard ahead, 2=soft ahead   */
) {
    if (azPos != mzPosInp){
        if (mbSeq){
            if (aiSft == Read )
                return EXI_SEK ;
            else
                return EOB ;    // on a sequential file, we cannot seek
        } else {
            mlFabSek++;
            if (mpFil.eof())
                mpFil.clear();
            mpFil.seekg(azPos, std::ios::beg); // may throw an exception
        }
    }
    mzPosInp = azPos + 1;
    return mpFil.get();
} /* function get */
} /* namespace JojoDiff */
