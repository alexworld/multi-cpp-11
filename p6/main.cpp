#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <fstream>
#include <cassert>
#include <queue>
#include <ctime>

using namespace boost::asio;

enum {CACHE_SIZE = 100000};

struct client
{
    boost::shared_ptr <ip::tcp::socket> socket;
    boost::shared_ptr <streambuf> buffer;
    client() {}
};

static io_service service;
static std::queue <client> clients;
static boost::mutex mutex;
static boost::condition_variable cond;
static std::map <std::string, std::pair <long long, std::vector <std::string> > > cache;
static boost::shared_mutex cache_mutex;

std::vector <std::string> check_file(std::string pref)
{
    std::vector <std::pair <int, std::string> > res;
    std::fstream data("data.txt", std::fstream::in);
    std::string s;
    int x;

    while (data >> s >> x) {
        if (s.size() >= pref.size() && pref == s.substr(0, pref.size()) && (res.size() < 10 || res.back().first < x)) {
            if (res.empty() || res.back().first >= x) {
                res.push_back(make_pair(x, s));
            }
            
            for (int i = 0; i < (int) res.size(); i++) {
                if (res[i].first < x) {
                    res.insert(res.begin() + i, make_pair(x, s));

                    if (res.size() > 10) {
                        res.pop_back();
                    }
                    break;
                }
            }
        }
    }
    std::vector <std::string> out;

    for (auto el: res) {
        out.push_back(el.second);
    }
    return out;
}


void on_write(client &c, const boost::system::error_code &error, size_t size)
{
    size = size;
    c = c;

    if (error) {
        return;
    }
    return;
}

void on_read(client &c, const boost::system::error_code &error, size_t size)
{
    size = size;
    std::istream is(c.buffer.get());
    std::string pref;
    is >> pref;

    std::vector <std::string> res;

    if (cache.count(pref) && (time(NULL) - cache[pref].first) < 10) {
        res = cache[pref].second;
    } else {
        res = check_file(pref);

        if (!cache.count(pref) && int(cache.size()) == CACHE_SIZE) {
            long long worst = cache.begin()->second.first;

            for (auto to: cache) {
                worst = std::min(worst, to.second.first);
            }

            for (auto to: cache) {
                if (to.second.first == worst) {
                    cache_mutex.lock_shared();
                    cache.erase(to.first);
                    cache_mutex.unlock_shared();
                    break;
                }
            }
        }

        cache_mutex.lock_shared();
        cache[pref] = make_pair(time(NULL), res);
        cache_mutex.unlock_shared();
    }

    std::string out;

    for (auto s: res) {
        out += s + "\n";
    }
    getline(is, pref);

    async_write(*c.socket, buffer(out), boost::bind(on_write, c, _1, _2));

    if (error) {
        boost::system::error_code now;
        c.socket->shutdown(ip::tcp::socket::shutdown_both, now);
        c.socket->close(now);
        std::cout << "connection terminated" << std::endl;
        return;
    }

    async_read_until(*c.socket, *c.buffer, "\n", boost::bind(on_read, c, _1, _2));
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

        async_read_until(*now.socket, *now.buffer, "\n", boost::bind(on_read, now, _1, _2));
    }
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

int main(void)
{
    boost::thread thr[10];

    for (int i = 0; i < 10; i++) {
        thr[i] = boost::thread(worker);
    }
    boost::shared_ptr <ip::tcp::acceptor> acceptor(new ip::tcp::acceptor(service, ip::tcp::endpoint(ip::tcp::v4(), 4000)));
    boost::shared_ptr <ip::tcp::socket> socket(new ip::tcp::socket(service));

    start_accept(acceptor, socket);
    service.run();
}
