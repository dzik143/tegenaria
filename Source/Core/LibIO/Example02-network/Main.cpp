/******************************************************************************/
/*                                                                            */
/* Copyright (c) 2010, 2014 Sylwester Wysocki <sw143@wp.pl>                   */
/*                                                                            */
/* Permission is hereby granted, free of charge, to any person obtaining a    */
/* copy of this software and associated documentation files (the "Software"), */
/* to deal in the Software without restriction, including without limitation  */
/* the rights to use, copy, modify, merge, publish, distribute, sublicense,   */
/* and/or sell copies of the Software, and to permit persons to whom the      */
/* Software is furnished to do so, subject to the following conditions:       */
/*                                                                            */
/* The above copyright notice and this permission notice shall be included in */
/* all copies or substantial portions of the Software.                        */
/*                                                                            */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR */
/* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   */
/* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    */
/* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER */
/* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    */
/* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        */
/* DEALINGS IN THE SOFTWARE.                                                  */
/*                                                                            */
/******************************************************************************/

#include <cstdio>
#include <unistd.h>
#include <cstring>

#include <Tegenaria/Net.h>
#include <Tegenaria/Debug.h>
#include <Tegenaria/Process.h>
#include <Tegenaria/IOMixer.h>

using namespace Tegenaria;

// ##########################################################################
// ################### S E R V E R    S I D E    C O D E  ###################
// ##########################################################################

//
//
// Callback handler to process one incoming connection.
//
// This function is runned in another thred created
// by main server loop.
//

int ServeClient(NetConnection *nc)
{
  IOMixer *iomixer = NULL;

  int slave1Fd[2];
  int slave2Fd[2];
  int slave3Fd[2];
  int slave4Fd[2];

  char slave1Msg[64] = {0};
  char slave2Msg[64] = {0};
  char slave3Msg[64] = {0};
  char slave4Msg[64] = {0};

  //
  // Wrap connected socket into IOMixer object.
  //

  iomixer = new IOMixer(nc -> readCallback, nc -> writeCallback, nc, nc);

  //
  // Open four channels.
  //

  iomixer -> addSlave(slave1Fd, 1);
  iomixer -> addSlave(slave2Fd, 2);
  iomixer -> addSlave(slave3Fd, 3);
  iomixer -> addSlave(slave4Fd, 4);

  //
  // Start listening on master IN.
  //

  iomixer -> start();

  //
  // Read four messages from client - each on different channel.
  //

  slave1Msg[0] = 0;
  slave2Msg[0] = 0;
  slave3Msg[0] = 0;
  slave4Msg[0] = 0;

  read(slave1Fd[0], slave1Msg, sizeof(slave1Msg));
  read(slave2Fd[0], slave2Msg, sizeof(slave2Msg));
  read(slave3Fd[0], slave3Msg, sizeof(slave3Msg));
  read(slave4Fd[0], slave4Msg, sizeof(slave4Msg));

  printf("--------------------\n");
  printf(" Connection socket   : [%d]\n", nc -> getSocket());
  printf(" Client info         : [%s]\n", nc -> getClientInfo());
  printf(" Message on slave #1 : [%s].\n", slave1Msg);
  printf(" Message on slave #2 : [%s].\n", slave2Msg);
  printf(" Message on slave #3 : [%s].\n", slave3Msg);
  printf(" Message on slave #4 : [%s].\n", slave4Msg);
  printf("--------------------\n");

  //
  // Send four answers to server - each on different channel.
  //

  strcpy(slave1Msg, "Answer from server #1");
  strcpy(slave2Msg, "Answer from server #2");
  strcpy(slave3Msg, "Answer from server #3");
  strcpy(slave4Msg, "Answer from server #4");

  write(slave1Fd[1], slave1Msg, strlen(slave1Msg));
  write(slave2Fd[1], slave2Msg, strlen(slave2Msg));
  write(slave3Fd[1], slave3Msg, strlen(slave3Msg));
  write(slave4Fd[1], slave4Msg, strlen(slave4Msg));

  //
  // Close all slaves.
  //

  close(slave1Fd[0]);
  close(slave2Fd[0]);
  close(slave3Fd[0]);
  close(slave4Fd[0]);

  close(slave1Fd[1]);
  close(slave2Fd[1]);
  close(slave3Fd[1]);
  close(slave4Fd[1]);

  //
  // Close our side of master socket.
  //
  // It means:
  //
  // - we don't want to read anymore
  //
  // but it does NOT mean:
  //
  // - we don't want to write anymore, because
  //   slaves can still contains some not flushed
  //   data to send to other side.
  //

  iomixer -> shutdown();

  //
  // Wait until all IO pending flushed inside IOMixer.
  //

  iomixer -> join();

  //
  // Release IOMixer object.
  //

  iomixer -> release();

  return 0;
};

//
// Entry point.
//

int main(int argc, char **argv)
{
  NetConnection *nc = NULL;

  //
  // No arguments.
  // Create server in background thread, which:
  //
  // - uses our ServerClient() routine to process incoming connections.
  // - listens on port 6666
  //

  if (argc < 3)
  {
    DBG_INIT("server.log");
    DBG_HEAD("DIRLIGO-SERVER-0.1\nBuild [%s, %s]\n", __DATE__, __TIME__);

    nc = NetServerCreate(6666, ServeClient);

    FAIL(nc == NULL);

    printf("Server created with handle [%p].\n", nc);

    //
    // Wait until server finishes.
    //

    nc -> join();
  }

// ##########################################################################
// ################### C L I E N T    S I D E    C O D E  ###################
// ##########################################################################

  //
  // 'libnet-example <ip> <port>'.
  // Client mode.
  //

  else
  {
    DBG_INIT("client.log");
    DBG_HEAD("DIRLIGO-CLIENT-0.1\nBuild [%s, %s]\n", __DATE__, __TIME__);

    const char *ip = argv[1];

    int port = atoi(argv[2]);

    int written = 0;

    int slave1Fd[2];
    int slave2Fd[2];
    int slave3Fd[2];
    int slave4Fd[2];

    char slave1Msg[64] = "Hello from client slave #1.";
    char slave2Msg[64] = "Hello from client slave #2.";
    char slave3Msg[64] = "Hello from client slave #3.";
    char slave4Msg[64] = "Hello from client slave #4.";

    IOMixer *iomixer = NULL;

    //
    // Connect to server listening on given <ip>:<port>.
    //

    nc = NetConnect(ip, port);

    FAIL(nc == NULL);

    //
    // Wrap connected socket into IOMixer object.
    //

    //IOMixer iomixer(sock, sock, IOMIXER_TYPE_SOCKET, IOMIXER_TYPE_SOCKET);

    iomixer = new IOMixer(nc -> readCallback, nc -> writeCallback, nc, nc);

    //
    // Open four channels.
    //

    iomixer -> addSlave(slave1Fd, 1);
    iomixer -> addSlave(slave2Fd, 2);
    iomixer -> addSlave(slave3Fd, 3);
    iomixer -> addSlave(slave4Fd, 4);

    //
    // Start listening on master IN.
    //

    iomixer -> start();

    //
    // Send four messages to server - each on different channel.
    //

    DBG_MSG("Writing...\n");

    write(slave1Fd[1], slave1Msg, strlen(slave1Msg));
    write(slave2Fd[1], slave2Msg, strlen(slave2Msg));
    write(slave3Fd[1], slave3Msg, strlen(slave3Msg));
    write(slave4Fd[1], slave4Msg, strlen(slave4Msg));

    //
    // Read four answers - each on different channel.
    //

    memset(slave1Msg, 0, sizeof(slave1Msg));
    memset(slave2Msg, 0, sizeof(slave2Msg));
    memset(slave3Msg, 0, sizeof(slave3Msg));
    memset(slave4Msg, 0, sizeof(slave4Msg));

    read(slave1Fd[0], slave1Msg, sizeof(slave1Msg));
    read(slave2Fd[0], slave2Msg, sizeof(slave2Msg));
    read(slave3Fd[0], slave3Msg, sizeof(slave3Msg));
    read(slave4Fd[0], slave4Msg, sizeof(slave4Msg));

    printf("Answer on slave #1 : [%s].\n", slave1Msg);
    printf("Answer on slave #2 : [%s].\n", slave2Msg);
    printf("Answer on slave #3 : [%s].\n", slave3Msg);
    printf("Answer on slave #4 : [%s].\n", slave4Msg);

    //
    // Close all slaves.
    //

    DBG_MSG("Closing...\n");

    close(slave1Fd[0]);
    close(slave2Fd[0]);
    close(slave3Fd[0]);
    close(slave4Fd[0]);

    close(slave1Fd[1]);
    close(slave2Fd[1]);
    close(slave3Fd[1]);
    close(slave4Fd[1]);

    //
    // Close our side of master socket.
    //
    // It means:
    //
    // - we don't want to read anymore
    //
    // but it does NOT mean:
    //
    // - we don't want to write anymore, because
    //   slaves can still contains some not flushed
    //   data to send to other side.
    //

    iomixer -> shutdown();

    //
    // Wait until all IO pending flushed inside IOMixer.
    //

    iomixer -> join();

    //
    // Release IOMixer object.
    //

    iomixer -> release();
  }

  //
  // Clean Up.
  //

  fail:

  if (nc)
  {
    nc -> release();
  }


  return 0;
}
