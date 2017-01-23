#include "headers.h"

using namespace std;

void exit_handler(int sig);
void exit_with(string mess);
void exit_with(char * mess);
void exit_with(const char * mess);
void init_server_socket();
void start_main_loop();

int server_socket_fd;
vector < Connection * > connections;

int port_to_bind;
int port_to_connect;
char * host_to_connect;

int main(int argc, char ** argv)
{

    cout << "Starting port forwarder!" << endl;

    signal(SIGINT, exit_handler);
    signal(SIGKILL, exit_handler);
    signal(SIGTERM, exit_handler);

    if (argc != 4)
        exit_with("Usage: ./a.out port_to_listen port_to_connect host_to_connect");

    cout << "Parsing args." << endl;

    port_to_bind = atoi(argv[1]);
    port_to_connect = atoi(argv[2]);
    host_to_connect = argv[3];

    cout << "Initing server socket." << endl;

    init_server_socket();

    cout << "Starting main loop." << endl;

    start_main_loop();

    return 0;
}

void start_main_loop()
{
    void handle_new_connection();

    fd_set to_read_desc;
    fd_set to_write_desc;

    int max_fd;

    for(;;)
    {
//        cout << "Initing descriptors sets." << endl;

        FD_ZERO ( &to_read_desc );
        FD_ZERO ( &to_write_desc );

        FD_SET ( server_socket_fd, &to_read_desc );
        max_fd = server_socket_fd;

        for ( auto connection : connections )
        {
            if ( ! connection -> is_receiving_done() && connection -> have_empty_space() )
            {
                FD_SET ( connection -> get_to_read_sock_fd(), &to_read_desc );
                max_fd = max( max_fd, connection -> get_to_read_sock_fd() );
            }
            if ( ! connection -> is_buffer_empty() )
            {
                FD_SET ( connection -> get_to_write_sock_fd(), &to_write_desc );
                max_fd = max( max_fd, connection -> get_to_write_sock_fd() );
            }
        }

//        cout << "Starting select." << endl;

        int select_result = select ( max_fd + 1, &to_read_desc, &to_write_desc, NULL, 0);

        if ( select_result == 0 )
        {
            cout << "Select returned 0." << endl;
            continue;
        }
        if ( select_result < 0 )
            exit_with ( "select error." );

//        cout << "Select done well." << endl;

        if ( FD_ISSET ( server_socket_fd,  &to_read_desc ) )
            handle_new_connection();

//        cout << "Working with opened connections." << endl;
        for ( auto connection : connections)
        {
            if ( ! connection -> is_closing() && FD_ISSET ( connection -> get_to_read_sock_fd(), &to_read_desc ) )
                connection -> do_receive();
            if ( ! connection -> is_closing() && FD_ISSET ( connection -> get_to_write_sock_fd(), &to_write_desc ) )
                connection -> do_send();
        }

        for ( auto it = connections.begin(); it != connections.end(); )
        {
            if ( (*it) -> is_closing() )
            {
                delete * it;
                it = connections.erase(it);
            }
            else
                ++it;
        }

    }
}

void init_server_socket()
{
    server_socket_fd = socket(PF_INET, SOCK_STREAM, 0);

    if(server_socket_fd < 0)
        exit_with ( "socket error." );

    int enable = 1;
    if ( setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) )
        exit_with (  "setsockopt error." );

    struct sockaddr_in server_sockaddr;
    server_sockaddr.sin_family = PF_INET;
    server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_sockaddr.sin_port = htons( port_to_bind );

    int res = bind(server_socket_fd, (struct sockaddr * ) &server_sockaddr, sizeof(server_sockaddr));
    if(res < 0)
        exit_with("bind error.");

    res = listen(server_socket_fd, 1024);
    if(res)
        exit_with("listen error.");
}

void handle_new_connection()
{
    cout << "Accepting new connection." << endl;

// Initing "from" socket"
    struct sockaddr_in client_sockaddr_in;
    int size_of_address = sizeof ( sockaddr_in );
    int client_to_read_socket = accept ( server_socket_fd, ( struct sockaddr * ) &client_sockaddr_in, ( socklen_t * ) &size_of_address );

    if ( client_to_read_socket < 0 )
    {
        cout << "accept error." << endl;
        cout << "CANT HANDLE CLIENT" << endl;
        return;
    }

// Initing "to" socket"

    struct hostent * host_info = gethostbyname ( host_to_connect );

    if ( ! host_info )
    {
        cout << "gethostbyname error." << endl;
        cout << "CANT HANDLE CLIENT" << endl;
        close ( client_to_read_socket );
        return;
    }

    int client_to_write_socket = socket(PF_INET, SOCK_STREAM, 0);

    if ( client_to_write_socket < 0 )
    {
        cout << "socket error." << endl;
        cout << "CANT HANDLE CLIENT" << endl;
        close ( client_to_read_socket );
        return;
    }

    struct sockaddr_in dest_addr;

    dest_addr.sin_family = PF_INET;
    dest_addr.sin_port = htons ( port_to_connect);
    memcpy ( &dest_addr.sin_addr, host_info -> h_addr, host_info -> h_length );

    if (-1 == connect ( client_to_write_socket, ( struct sockaddr * ) &dest_addr, sizeof ( dest_addr ) ) )
    {
        cout << "connect error." << endl;
        cout << "CANT HANDLE CLIENT" << endl;
        close ( client_to_read_socket );
        close ( client_to_write_socket );
        return;
    }

    Connection * connection_to = new Connection(client_to_read_socket, client_to_write_socket);
    connections.push_back(connection_to);

    Connection * connection_from = new Connection(client_to_write_socket, client_to_read_socket);
    connections.push_back(connection_from);

    connection_to -> set_pair ( connection_from );
    connection_from -> set_pair ( connection_to );
}

void exit_with(char * mess)
{
    cout << mess << endl;
    exit_handler(153);
}

void exit_with(const char * mess)
{
    perror ( mess );
    cout << endl;
    exit_handler(153);
}

void exit_with(string mess)
{
    cout << mess << endl;
    exit_handler(153);
}

void exit_handler (int sig)
{
    cout << endl << "Exiting with code: " << sig << endl;

    for ( auto it = connections.begin(); it != connections.end() ; )
    {
        delete * it;
        it = connections.erase(it);
    }

    close(server_socket_fd);

    exit(EXIT_SUCCESS);
}
