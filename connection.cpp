#include "headers.h"

using namespace std;

Connection::Connection ( int to_read_sock_fd,
                         int to_write_sock_fd )
{
    cout << "CREATING CONNECTION" << endl;

    this -> to_read_sock_fd = to_read_sock_fd;
    this -> to_write_sock_fd = to_write_sock_fd;

    buf = (char*) malloc (BUF_SIZE);
    empty_space = BUF_SIZE;

    recv_done = false;
    to_close = false;
}

Connection::~Connection()
{
    cout << "DELETING CONNECTION" << endl;

    close(to_read_sock_fd);
    close(to_write_sock_fd);

    free(buf);
}

void Connection::set_pair ( Connection * pair )
{
    this -> pair = pair;
}

int Connection::get_to_read_sock_fd() { return to_read_sock_fd ; }
int Connection::get_to_write_sock_fd() { return to_write_sock_fd ; }

bool Connection::have_empty_space() { return empty_space > 0 ; }
bool Connection::is_buffer_empty() { return empty_space == BUF_SIZE ; }

void Connection::do_receive()
{
    int received = recv ( to_read_sock_fd,
                          buf + BUF_SIZE - empty_space,
                          empty_space,
                          0 );
    switch (received)
    {
    case -1 :
        cout << "recv error." << endl;
        to_close = true;
        pair -> set_closing();
        break;
    case 0 :
        cout << "RECEIVING COMPLETED." << endl;
        to_close = true;
        pair -> set_closing();
        break;
    default:
        empty_space -= received;
    }
}

bool Connection::is_receiving_done() { return recv_done; }

void Connection::do_send()
{
    int sent = send ( to_write_sock_fd,
                      buf,
                      BUF_SIZE - empty_space,
                      0);
    switch (sent)
    {
    case -1 :
        cout << "send error." << endl;
        to_close = true;
        break;
    case 0 :
        cout << "SENDING COMPLETED."  << endl;
        to_close = true;
        pair -> set_closing();
        break;
    default:
        memmove ( buf, buf + sent, sent );
        empty_space += sent;
    }
}

void Connection::set_closing() { to_close = true ; }
bool Connection::is_closing() { return to_close ; }
