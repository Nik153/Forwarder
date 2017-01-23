#ifndef CONNECTION_H
#define CONNECTION_H


class Connection
{
public:
    Connection ( int to_read_sock_fd,
                 int to_write_sock_fd );
    ~Connection();

    void set_pair(Connection * pair);

    int get_to_read_sock_fd();
    int get_to_write_sock_fd();

    bool have_empty_space();
    bool is_buffer_empty();

    void do_receive();
    void do_send();

    bool is_receiving_done();

    void set_closing();
    bool is_closing();

private:
    int to_read_sock_fd;
    int to_write_sock_fd;

    int empty_space;
    char * buf;

    bool recv_done;
    bool to_close;

    Connection * pair;
};

#endif
