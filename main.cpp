#include "Thread_Poll.h"
const int buffer_size = 1024;

void answer_on_read(bufferevent *buf_ev){
	char tmp[1024];
	string answer = " answer\n";
	struct evbuffer *buf_input = bufferevent_get_input( buf_ev );
	struct evbuffer *buf_output = bufferevent_get_output( buf_ev );

	//overhead
	int lol = bufferevent_write(buf_ev, (const void *)answer.c_str(), answer.size());
	//ECHO
	size_t how_much = bufferevent_read(buf_ev, tmp, buffer_size);
	lol = bufferevent_write(buf_ev, tmp, how_much);
}
/*read event for bufferevent */
static void echo_read_cb( struct bufferevent *buf_ev, void *poll )
{
	Thread_Poll* thread_poll=
			reinterpret_cast<Thread_Poll*>(poll);
	thread_poll->add_task(answer_on_read,buf_ev );
}

//if some bufferevent *buf_ev got event but not read/wtite
static void echo_event_cb( struct bufferevent *buf_ev, short events, void *arg )
{

	if( events & BEV_EVENT_ERROR )
		perror( " bufferevent ERROR" );
	if( events & (BEV_EVENT_EOF | BEV_EVENT_ERROR) ) {
		bufferevent_free(buf_ev);
	}
}

static void accept_error_cb( struct evconnlistener *listener, void *arg )
{
	struct event_base *base = evconnlistener_get_base( listener );
	int error = EVUTIL_SOCKET_ERROR();
	fprintf( stderr, "Ошибка %d (%s) в мониторе соединений. Завершение работы.\n",
			 error, evutil_socket_error_to_string( error ) );
	event_base_loopexit( base, NULL );
}


static void accept_connection_cb( struct evconnlistener *listener,
								  evutil_socket_t fd, struct sockaddr *addr, int sock_len,
								  void* poll )
{

	Thread_Poll* thread_poll=
			reinterpret_cast<Thread_Poll*>(poll);


	struct event_base *base = evconnlistener_get_base( listener );
	struct bufferevent *buf_ev = bufferevent_socket_new( base, fd, BEV_OPT_CLOSE_ON_FREE );
	//
	bufferevent_setcb( buf_ev, echo_read_cb, /*echo_write_cb*/ NULL, echo_event_cb, thread_poll );
	bufferevent_enable( buf_ev, (EV_READ | EV_WRITE) );
}

int main( int argc, char **argv )
{
	struct event_base *base;
	struct evconnlistener *listener;
	struct sockaddr_in sin;
	unordered_map<bufferevent*, bufferevent*> connections;
	try {
		Thread_Poll thread_poll;
		int port = 10000;
		if (argc > 1) port = atoi(argv[1]);
		if (port < 0 || port > 65535) {
			fprintf(stderr, "Incorect port.\n");
			return -1;
		}

		base = event_base_new();
		if (!base) {
			fprintf(stderr, "event_base error while creating\n");
			return -1;
		}
		memset(&sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;    /* работа с доменом IP-адресов */
		sin.sin_addr.s_addr = htonl(INADDR_ANY);  /* принимать запросы с любых адресов */
		sin.sin_port = htons(port);
		listener = evconnlistener_new_bind(base, accept_connection_cb, &thread_poll,
										   (LEV_OPT_CLOSE_ON_FREE),
										   -1, (struct sockaddr *) &sin, sizeof(sin));
		if (!listener) {
			perror("ERROR evconnlistener");
			return -1;
		}
		evconnlistener_set_error_cb(listener, accept_error_cb);
		event_base_dispatch(base);
	}
	catch(...){
		return 228;
	}
	return 0;
}