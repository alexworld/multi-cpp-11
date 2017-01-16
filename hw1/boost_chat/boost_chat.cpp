#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <iostream>

using namespace boost::asio;

struct client
{
    boost::shared_ptr <ip::tcp::socket> socket;
    boost::shared_ptr <streambuf> buffer;
    client() {}
};

static std::vector <client> clients;
static char greeting[] = "Welcome\n";
static io_service service;
static ip::tcp::endpoint endpoint(ip::tcp::v4(), 3100);
static ip::tcp::acceptor acceptor(service, endpoint);

void on_write(const boost::system::error_code &error, size_t size)
{
    return;
}

void on_read(client &c, const boost::system::error_code &error, size_t size)
{
    if (error) {
        boost::system::error_code now;
        c.socket->shutdown(ip::tcp::socket::shutdown_both, now);
        c.socket->close(now);
        std::cout << "connection terminated" << std::endl;
        return;
    }
    std::istream is(c.buffer.get());
    std::string message;
    std::getline(is, message);

    std::cout << message << std::endl;

    for (client &to: clients) {
        async_write(*to.socket, buffer(message + "\n"), on_write);
    }
    async_read_until(*c.socket, *c.buffer, '\n', boost::bind(on_read, c, _1, _2));
}

void start_accept(boost::shared_ptr <ip::tcp::socket> socket);

void handle_accept(boost::shared_ptr <ip::tcp::socket> socket, const boost::system::error_code &error)
{
    if (error) {
        return;
    }
    std::cout << "accepted connection" << std::endl;
    client now;
    now.socket = socket;
    now.buffer = boost::shared_ptr <streambuf> (new streambuf);
    clients.push_back(now);

    async_write(*now.socket, buffer(greeting, sizeof(greeting)), on_write);
    async_read_until(*now.socket, *now.buffer, '\n', boost::bind(on_read, now, _1, _2));

    boost::shared_ptr <ip::tcp::socket> newsocket(new ip::tcp::socket(service));
    start_accept(newsocket);
}

void start_accept(boost::shared_ptr <ip::tcp::socket> socket)
{
    acceptor.async_accept(*socket, boost::bind(handle_accept, socket, _1));
}

int main(void)
{
    boost::shared_ptr <ip::tcp::socket> socket(new ip::tcp::socket(service));
    start_accept(socket);
    service.run();
}
