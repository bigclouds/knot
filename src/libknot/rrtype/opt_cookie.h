/*  Copyright (C) 2016 CZ.NIC, z.s.p.o. <knot-dns@labs.nic.cz>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/*!
 * \file
 *
 * \brief Functions for manipulating the EDNS OPT pseudo-RR.
 *
 * \addtogroup libknot
 * @{
 */

#pragma once

#define KNOT_OPT_COOKIE_MIN  8
#define KNOT_OPT_COOKIE_CLNT KNOT_OPT_COOKIE_MIN
#define KNOT_OPT_COOKIE_SRVR_MIN 8
#define KNOT_OPT_COOKIE_SRVR_MAX 32

/*----------------------------------------------------------------------------*/
/* EDNS Cookie option handling functions.                                     */
/*----------------------------------------------------------------------------*/

#define srvr_cookie_len_ok(sc_len) \
	((sc_len == 0) || \
	 ((sc_len) >= KNOT_OPT_COOKIE_SRVR_MIN && (sc_len) <= KNOT_OPT_COOKIE_SRVR_MAX))

/*!
 * \brief Returns the size of the buffer required to store the cookie.
 *
 * \note The value of \a sc_len must be within defined limits.
 *
 * \param sc_len Server cookie portion length.
 *
 * \retval <> 0 if the supplied arguments are within limits
 * \retval 0 if the supplied parameters violate the requirements
 */
_mustcheck_
static inline uint16_t knot_edns_opt_cookie_data_len(uint16_t sc_len)
{
	/* + size of client cookie */
	return srvr_cookie_len_ok(sc_len) ? (sc_len + KNOT_OPT_COOKIE_CLNT) : 0;
}

/*!
 * \brief Creates cookie wire data.
 *
 * \param cc       Client cookie.
 * \param sc       Server cookie.
 * \param sc_len   Server cookie size.
 * \param data     Output data buffer.
 * \param data_len Size of output data buffer/written data.
 *
 * \retval KNOT_EOK
 * \retval KNOT_EINVAL
 * \retval KNOT_ESPACE
 */
int knot_edns_opt_cookie_create(const uint8_t cc[KNOT_OPT_COOKIE_CLNT],
                                const uint8_t *sc, const uint16_t sc_len,
                                uint8_t *data, uint16_t *data_len);

/*!
 * \brief Parse cookie wire data.
 *
 * \param data     Input data buffer containing whole cookie option.
 * \param data_len Length of input data buffer.
 * \param cc       Client cookie.
 * \param cc_len   Client cookie size.
 * \param sc       Server cookie.
 * \param sc_len   Server cookie size.
 *
 * \return KNOT_EOK
 * \return KNOT_EINVAL
 * \return KNOT_ESPACE
 */
int knot_edns_opt_cookie_parse(const uint8_t *data, const uint16_t data_len,
                               uint8_t *cc, uint16_t cc_len,
                               uint8_t *sc, uint16_t sc_len);

/*! @} */
