//
// PURPOSE: Echo server listening on 9995 port.
//

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>

#ifndef WIN32
# include <netinet/in.h>
# ifdef _XOPEN_SOURCE_EXTENDED
#  include <arpa/inet.h>
# endif
# include <sys/socket.h>
#endif

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>

static const int PORT = 9995;

static void listener_cb(struct evconnlistener *, evutil_socket_t, struct sockaddr *, int socklen, void *);
static void conn_writecb(struct bufferevent *, void *);
static void conn_eventcb(struct bufferevent *, short, void *);
static void signal_cb(evutil_socket_t, short, void *);
static void conn_readcb(struct bufferevent *bev, void *user_data);

//
// Entry point.
//

int main(int argc, char **argv)
{
  struct event_base *base;
  struct evconnlistener *listener;
  struct event *signal_event;

  struct sockaddr_in sin;
  
  #ifdef WIN32
  WSADATA wsa_data;
  WSAStartup(0x0201, &wsa_data);
  #endif

  base = event_base_new();
  
  if (!base) 
  {
    fprintf(stderr, "Could not initialize libevent!\n");
  
    return 1;
  }

  memset(&sin, 0, sizeof(sin));
  
  sin.sin_family = AF_INET;
  sin.sin_port = htons(PORT);

  listener = evconnlistener_new_bind(base, listener_cb, (void *) base,
                                         LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, 
                                             -1, (struct sockaddr *) &sin,
                                                 sizeof(sin));

  if (!listener) 
  {
    fprintf(stderr, "Could not create a listener!\n");
  
    return 1;
  }

  signal_event = evsignal_new(base, SIGINT, signal_cb, (void *)base);

  if (!signal_event || event_add(signal_event, NULL)<0) 
  {
    fprintf(stderr, "Could not create/add a signal event!\n");
  
    return 1;
  }

  event_base_dispatch(base);

  evconnlistener_free(listener);
  event_free(signal_event);
  event_base_free(base);

  printf("done\n");
  
  return 0;
}

//
// Callback called when new connection arrived.
//

static void listener_cb(struct evconnlistener *listener, evutil_socket_t fd,
                            struct sockaddr *sa, int socklen, void *user_data)
{
  struct event_base *base = user_data;
  
  struct bufferevent *bev;

  printf("-> listener_cb()...\n");
  
  printf("New connection.\n");
  
  bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
  
  if (!bev) 
  {
    fprintf(stderr, "Error constructing bufferevent!");
  
    event_base_loopbreak(base);
    
    return;
  }
  
  bufferevent_setcb(bev, conn_readcb, NULL, conn_eventcb, NULL);
  
  bufferevent_enable(bev, EV_WRITE);
  bufferevent_enable(bev, EV_READ);
  
  printf("<- listener_cb()...\n");
}

//
// Callback called when new data arrived.
//

static void conn_readcb(struct bufferevent *bev, void *user_data)
{
  printf("-> conn_readcb()...\n");

  struct evbuffer *input = bufferevent_get_input(bev);
  
  size_t len = evbuffer_get_length(input);
  
  unsigned char *buf = evbuffer_pullup(input, len);
  
  printf("Readed [%d][%.*s]\n", len, len, buf);
  
  bufferevent_write(bev, buf, len);
  
  evbuffer_drain(input, len);
    
  printf("-> conn_readcb()...\n");
}

//
// Callback called when connection closed.
//

static void conn_eventcb(struct bufferevent *bev, short events, void *user_data)
{
  printf("Connection closed.\n");

  bufferevent_free(bev);
}

//
// Callback called when SIGINT handled.
//

static void signal_cb(evutil_socket_t sig, short events, void *user_data)
{
  struct event_base *base = user_data;
  struct timeval delay    = {2, 0};

  printf("Caught an interrupt signal; exiting cleanly in two seconds.\n");

  event_base_loopexit(base, &delay);
}
