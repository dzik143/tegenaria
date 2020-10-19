/**************************************************************************/
/*                                                                        */
/* This file is part of Tegenaria project.                                */
/* Copyright (c) 2010, 2015 Sylwester Wysocki (sw143@wp.pl).              */
/*                                                                        */
/* The Tegenaria library and any derived work however based on this       */
/* software are copyright of Sylwester Wysocki. Redistribution and use of */
/* the present software is allowed according to terms specified in the    */
/* file LICENSE which comes in the source distribution.                   */
/*                                                                        */
/* All rights reserved.                                                   */
/*                                                                        */
/**************************************************************************/

#include <cstdio>
#include <string.h>
#include <Tegenaria/Net.h>
#include <Tegenaria/Debug.h>

using namespace Tegenaria;

//
// Entry point.
//

int main(int argc, char **argv)
{
  //
  // No arguments.
  //
  // - Wait for connection on 0.0.0.0:6666.
  //

  if (argc < 3)
  {
    char buf[64] = {0};
    
    int readed = 0;
    
    DBG_HEAD("DIRLIGO-SERVER-0.1\nBuild [%s, %s]\n", __DATE__, __TIME__);

    NetConnection *nc = NetAccept(6666);
    
    FAIL(nc == NULL);

    readed = nc -> read(buf, sizeof(buf));

    nc -> release();
    
    printf("Readed [%d] bytes [%s].\n", readed, buf);
  }
  
  //
  // 'libnet-example-single <ip> <port>'.
  // Connect to server listening on ip:port.
  //
  
  else
  {
    char buf[64] = "Hello from client.\n";

    DBG_HEAD("DIRLIGO-CLIENT-0.1\nBuild [%s, %s]\n", __DATE__, __TIME__);
    
    const char *ip = argv[1];

    int port = atoi(argv[2]);
    
    int written = 0;
    
    NetConnection *nc = NetConnect(ip, port);
        
    FAIL(nc == NULL);

    written = nc -> write(buf, strlen(buf));
    
    printf("Written [%d] bytes [%s].\n", written, buf);
    
    nc -> release();
  }

  //
  // Clean Up.
  //
  
  fail:

  return 0;
}
