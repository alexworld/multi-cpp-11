#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include <cassert>

#include "http.h"

using namespace boost::asio;

struct client
{
    boost::shared_ptr <ip::tcp::socket> socket;
    boost::shared_ptr <streambuf> buffer;
    std::string message;
    client() {}
};

static std::vector <client> clients;
static io_service service;

void on_write(client &c, const boost::system::error_code &error, size_t size)
{
    boost::system::error_code now;
    c.socket->shutdown(ip::tcp::socket::shutdown_both, now);
    c.socket->close(now);
    size = size;

    if (error) {
        return;
    }
    return;
}

void on_read(client &c, const boost::system::error_code &error, size_t size)
{
    size = size;
    std::istream is(c.buffer.get());
    is >> std::noskipws;
    char ch;

    while (is >> ch) {
        c.message += ch;
    }
    std::string result;

    while (process_request(c.message, result)) {
        async_write(*c.socket, buffer(result), boost::bind(on_write, c, _1, _2));
    }

    if (error) {
        boost::system::error_code now;
        c.socket->shutdown(ip::tcp::socket::shutdown_both, now);
        c.socket->close(now);
        std::cout << "connection terminated" << std::endl;
        return;
    }

    async_read_until(*c.socket, *c.buffer, "\r\n", boost::bind(on_read, c, _1, _2));
}

void start_accept(boost::shared_ptr <ip::tcp::acceptor> acceptor, boost::shared_ptr <ip::tcp::socket> socket);

void handle_accept(boost::shared_ptr <ip::tcp::acceptor> acceptor, boost::shared_ptr <ip::tcp::socket> socket, const boost::system::error_code &error)
{
    if (error) {
        return;
    }
    std::cout << "accepted connection" << std::endl;
    client now;
    now.socket = socket;
    now.buffer = boost::shared_ptr <streambuf> (new streambuf);
    clients.push_back(now);

    async_read_until(*now.socket, *now.buffer, "\r\n", boost::bind(on_read, now, _1, _2));

    boost::shared_ptr <ip::tcp::socket> newsocket(new ip::tcp::socket(service));
    start_accept(acceptor, newsocket);
}

void start_accept(boost::shared_ptr <ip::tcp::acceptor> acceptor, boost::shared_ptr <ip::tcp::socket> socket)
{
    acceptor->async_accept(*socket, boost::bind(handle_accept, acceptor, socket, _1));
}

int main(void)
{
    boost::shared_ptr <ip::tcp::acceptor> acceptor(new ip::tcp::acceptor(service, ip::tcp::endpoint(ip::tcp::v4(), 3100)));
    boost::shared_ptr <ip::tcp::socket> socket(new ip::tcp::socket(service));

    start_accept(acceptor, socket);
    service.run();
}
