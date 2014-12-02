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

#include "dcc_server.h"

#include <string.h>
#include <stdbool.h>
#include <helper/list.h>

// Listening port
static char *dcc_port;

// Connection variables
struct dcc_connection {
  bool             enabled;
  bool             closed;
  struct list_head pkt_list;
};

struct dcc_packet {
  char              *buf;
  int               len;
  struct list_head  list;
};

int dcc_server_put_packet(struct connection *connection, char *buffer, int len)
{
  return ERROR_OK;
}

static int dcc_socket_write (struct connection *connection, const void *data,
			     int len)
{
  struct dcc_connection *dcc_cnx = connection->priv;

  if (dcc_cnx->closed)
    return ERROR_SERVER_REMOTE_CLOSED;

  // Try to write to socket
  if (connection_write(connection, data, len) == len)
    return ERROR_OK;

  // Else connection must have been closed
  dcc_cnx->closed = true;
  return ERROR_SERVER_REMOTE_CLOSED;
}


/*
 * Handle DCC connection related callbacks
 */
static int dcc_new_connection(struct connection *cnx)
{
  // Allocate dcc connection
  struct dcc_connection *dcc_cnx = malloc(sizeof(struct dcc_connection));
  if (dcc_cnx == NULL)
    return ERROR_FAIL;

  // Create port
  dcc_cnx->enabled = true;
  dcc_cnx->closed = false;
  INIT_LIST_HEAD (&dcc_cnx->pkt_list);

  // Save into connection
  cnx->priv = dcc_cnx;

  // Write out to socket
  dcc_socket_write (cnx, "Hello\r\n", 7);
  return ERROR_OK;
}

static int dcc_input(struct connection *cnx)
{
  int rv;
  struct dcc_packet *dcc_pkt;
  struct dcc_connection *dcc_cnx;

  // Get specific connection data
  dcc_cnx = cnx->priv;

  // Malloc a packet and buffer
  dcc_pkt = malloc(sizeof(struct dcc_packet));
  if (dcc_pkt == NULL)
    return ERROR_FAIL;
  dcc_pkt->buf = malloc (DCC_BUFFER_SIZE);
  if (dcc_pkt->buf == NULL)
    return ERROR_FAIL;

  // Read from connection
  dcc_pkt->len = connection_read(cnx, dcc_pkt->buf, DCC_BUFFER_SIZE);
  if (dcc_pkt->len == 0) {
    rv = ERROR_SERVER_REMOTE_CLOSED;
    goto fail;
  }

  // Init packet list
  INIT_LIST_HEAD (&dcc_pkt->list);

  // Add packet to linked list
  list_add_tail (&dcc_cnx->pkt_list, &dcc_pkt->list);
  return ERROR_OK;

 fail:
  if (dcc_pkt->buf)
    free (dcc_pkt->buf);
  if (dcc_pkt)
    free (dcc_pkt);
  return rv;
}

static int dcc_connection_closed(struct connection *cnx)
{
  return ERROR_OK;
}

int dcc_server_init (void)
{

  // Create new service
  return add_service ("dcc",
		      dcc_port,
		      1,
		      dcc_new_connection,
		      dcc_input,
		      dcc_connection_closed,
		      NULL);
}

/* DCC command handlers */
COMMAND_HANDLER(handle_dcc_enable_command)
{
  return ERROR_OK;
}

#if 0
COMMAND_HANDLER(handle_dcc_port_command)
{
  return ERROR_OK;
}
#endif

static const struct command_registration dcc_command_handlers[] = {
  {
    .name = "dcc",
    .handler = handle_dcc_enable_command,
    .mode = COMMAND_ANY,
    .help = "enable or disable dcc communications over TCP/IP port",
    .usage = "('enable'|'disable')",
  },
#if 0
  {
    .name = "dcc_port",
    .handler = handle_dcc_port_command,
    .mode = COMMAND_ANY,
    .help = "Set dcc communications TCP port number",
    .usage = "[port num]",
  },
#endif
  COMMAND_REGISTRATION_DONE
};

int dcc_server_register_commands(struct command_context *cmd_ctx)
{
  dcc_port = strdup("3532");
  return register_commands(cmd_ctx, NULL, dcc_command_handlers);
}
