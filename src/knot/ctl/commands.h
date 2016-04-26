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
 * \brief Server control commands.
 *
 * \addtogroup ctl
 * @{
 */

#pragma once

#include "libknot/libknot.h"
#include "knot/server/server.h"

/*! Control commands. */
typedef enum {
	CTL_NONE,

	CTL_STATUS,
	CTL_STOP,
	CTL_RELOAD,

	CTL_ZONE_STATUS,
	CTL_ZONE_RELOAD,
	CTL_ZONE_REFRESH,
	CTL_ZONE_RETRANSFER,
	CTL_ZONE_FLUSH,
	CTL_ZONE_SIGN,

	CTL_CONF_LIST,
	CTL_CONF_READ,
	CTL_CONF_BEGIN,
	CTL_CONF_COMMIT,
	CTL_CONF_ABORT,
	CTL_CONF_DIFF,
	CTL_CONF_GET,
	CTL_CONF_SET,
	CTL_CONF_UNSET,
} ctl_cmd_t;

/*! Control command parameters. */
typedef struct {
	knot_ctl_t *ctl;
	knot_ctl_type_t type;
	knot_ctl_data_t data;
	server_t *server;
} ctl_args_t;

/*!
 * Returns a string equivalent of the command.
 *
 * \param[in] cmd  Command.
 *
 * \return Command string or NULL.
 */
const char* ctl_cmd_to_str(ctl_cmd_t cmd);

/*!
 * Returns a command corresponding to the string.
 *
 * \param[in] cmd_str  Command string.
 *
 * \return Command.
 */
ctl_cmd_t ctl_str_to_cmd(const char *cmd_str);

/*!
 * Executes a control command.
 *
 * \param[in] cmd   Control command.
 * \param[in] args  Command arguments.
 *
 * \return Error code, KNOT_EOK if successful.
 */
int ctl_exec(ctl_cmd_t cmd, ctl_args_t *args);

/*! @} */