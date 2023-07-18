/**
 *  Drag Race Tree 4.0
 *  Author: Michael Kohn
 *   Email: mike@mikekohn.net
 *     Web: https://www.mikekohn.net/
 * License: BSD
 *
 * Copyright 2023 by Michael Kohn
 *
 * https://www.mikekohn.net/micro/drag_racing_tree.php
 *
 */

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "Network.h"

Network::Network()
{
}

Network::~Network()
{
}

int Network::net_send(int socket_id, const uint8_t *buffer, int length)
{
  struct timeval tv;
  fd_set writeset;
    
  int bytes_sent = 0;

  while (bytes_sent < length)
  {
    FD_ZERO(&writeset);
    FD_SET(socket_id, &writeset);

    tv.tv_sec = 10;
    tv.tv_usec = 0;

    int n = select(socket_id + 1, NULL, &writeset, NULL, &tv);

    if (n == -1)
    {
      if (errno == EINTR) { continue; }

      perror("Problem with select in net_send");
      return -1;
    }

    if (n == 0) { return -3; }

    n = send(socket_id, buffer + bytes_sent, length - bytes_sent, 0);
    if (n < 0) { return -4; }

    bytes_sent += n;
  }

  return bytes_sent;
}

int Network::net_recv(int socket_id, uint8_t *buffer, int length)
{
  struct timeval tv;
  fd_set readset;
  int count = 0;

  while (count < 5)
  {
    FD_ZERO(&readset);
    FD_SET(socket_id, &readset);

    tv.tv_sec = 2;
    tv.tv_usec = 0;
    count++;

    int n = select(socket_id + 1, &readset, NULL, NULL, &tv);

    if (n == -1)
    {
      if (errno == EINTR) { continue; }
      return -1;
    }

    // Timeout on select().
    if (n == 0) { continue; }

    if (FD_ISSET(socket_id, &readset))
    {
      return recv(socket_id, buffer, length, 0);
    }
  }

  return -5;
}

void Network::net_close(int socket_id)
{ 
  if (socket_id != -1)
  {
    close(socket_id);
    socket_id = -1;
  }
}

