#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <cassert>
#include <queue>
#include <syslog.h>

#include "http.h"

using namespace boost::asio;

std::string root_dir;

struct client
{
    boost::shared_ptr <ip::tcp::socket> socket;
    boost::shared_ptr <streambuf> buffer;
    std::string message;
    client() {}
};

static io_service service;
static std::queue <client> clients;
static boost::mutex mutex;
static boost::condition_variable cond;

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
        syslog(1, "Connection terminated\n");
        return;
    }

    async_read_until(*c.socket, *c.buffer, "\r\n", boost::bind(on_read, c, _1, _2));
}

void worker()
{
    while (true) {
        boost::unique_lock <boost::mutex> lock(mutex);

        while (clients.empty()) {
            cond.wait(lock);
        }
        client now = clients.front();
        clients.pop();
        mutex.unlock();

        async_read_until(*now.socket, *now.buffer, "\r\n", boost::bind(on_read, now, _1, _2));
    }
}

void start_accept(boost::shared_ptr <ip::tcp::acceptor> acceptor, boost::shared_ptr <ip::tcp::socket> socket);

void handle_accept(boost::shared_ptr <ip::tcp::acceptor> acceptor, boost::shared_ptr <ip::tcp::socket> socket, const boost::system::error_code &error)
{
    if (error) {
        return;
    }
    syslog(1, "Accepted connection\n");
    client now;
    now.socket = socket;
    now.buffer = boost::shared_ptr <streambuf> (new streambuf);

    mutex.lock();
    clients.push(now);
    cond.notify_one();
    mutex.unlock();

    boost::shared_ptr <ip::tcp::socket> newsocket(new ip::tcp::socket(service));
    start_accept(acceptor, newsocket);
}

void start_accept(boost::shared_ptr <ip::tcp::acceptor> acceptor, boost::shared_ptr <ip::tcp::socket> socket)
{
    acceptor->async_accept(*socket, boost::bind(handle_accept, acceptor, socket, _1));
}

int main(int argc, char **argv)
{
    extern char *optarg;
    int port = 3100, numw = 1;
    root_dir = "./content/";
    int opt;
    
    while ((opt = getopt(argc, argv, "d:p:w:")) != -1) {
        switch (opt) {
            case 'd':
                root_dir = optarg;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 'w':
                numw = atoi(optarg);
                break;
        }
    }
    boost::thread thr[numw];

    for (int i = 0; i < numw; i++) {
        thr[i] = boost::thread(worker);
    }
    boost::shared_ptr <ip::tcp::acceptor> acceptor(new ip::tcp::acceptor(service, ip::tcp::endpoint(ip::tcp::v4(), (unsigned short) port)));
    boost::shared_ptr <ip::tcp::socket> socket(new ip::tcp::socket(service));

    start_accept(acceptor, socket);
    service.run();
}
