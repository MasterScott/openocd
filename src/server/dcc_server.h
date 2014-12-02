/***************************************************************************
 *   Copyright (C) 2014 by Elliot Buller                                   *
 *   Elliot.Buller@gmail.com                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.           *
 ***************************************************************************/

#ifndef DCC_SERVER_H
#define DCC_SERVER_H

#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <netinet/in.h>

#include <helper/command.h>
#include "server.h"

// DCC input buffer size
#define DCC_BUFFER_SIZE    (127)

/* Register dcc related commands */
int dcc_server_register_commands(struct command_context *command_context);

/* Write a packet to target */
int dcc_server_put_packet(struct connection *connection, char *buffer, int len);

/* Initialize dcc server */
int dcc_server_init (void);

#endif	/* DCC_SERVER_H */
