#ifndef SENDO_SERVER_H
#define SENDO_SERVER_H

#include "dist/Executor.h"
#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>
using namespace std::chrono_literals;
using boost::asio::ip::tcp;

class Server {
    class Connection : public std::enable_shared_from_this<Connection> {
    	sendo::Executor executor;
        friend class Server;
        void ProcessCommand(const std::string& cmd) {
            if (cmd == "stop") {
                server_.Stop();
                return;
            }
            if (cmd == "") {
                Close();
                return;
            }
            std::thread t([this, self = shared_from_this(), cmd] {
				try {
					Write(executor.execute(cmd).query + "\r\n");
				}
				catch (sendo::SendoException* s){
					Write(s->what());
				}

                server_.io_service_.post([this, self] {
                    DoReadCmd();
                });
            });
            t.detach();
        }

        void DoReadCmd() {
            read_timer_.expires_from_now(server_.read_timeout_);
            read_timer_.async_wait([this](boost::system::error_code ec) {
                if (!ec) {
                    // Read timeout
                    Shutdown();
                }
            });
            boost::asio::async_read_until(socket_, buf_in_, '\r\n', [this, self = shared_from_this()](boost::system::error_code ec, std::size_t bytes_read) {
                read_timer_.cancel();
                if (!ec) {
                    const char* p = boost::asio::buffer_cast<const char*>(buf_in_.data());
                    std::string cmd(p, bytes_read - (bytes_read > 1 && p[bytes_read - 2] == '\r' ? 2 : 1));
                    buf_in_.consume(bytes_read);
                    ProcessCommand(cmd);
                }
                else {
                    Close();
                }
            });
        }

        void DoWrite() {
            active_buffer_ ^= 1; // switch buffers
            for (const auto& data : buffers_[active_buffer_]) {
                buffer_seq_.push_back(boost::asio::buffer(data));
            }
            write_timer_.expires_from_now(server_.write_timeout_);
            write_timer_.async_wait([this](boost::system::error_code ec) {
                if (!ec) {
                    // Write timeout
                    Shutdown();
                }
            });
            boost::asio::async_write(socket_, buffer_seq_, [this, self = shared_from_this()](const boost::system::error_code& ec, size_t bytes_transferred) {
                write_timer_.cancel();
                std::lock_guard<std::mutex> lock(buffers_mtx_);
                buffers_[active_buffer_].clear();
                buffer_seq_.clear();
                if (!ec) {
                    if (!buffers_[active_buffer_ ^ 1].empty()) // have more work
                        DoWrite();
                }
                else {
                    Close();
                }
            });
        }
        bool Writing() const { return !buffer_seq_.empty(); }

        Server& server_;
        boost::asio::streambuf buf_in_;
        std::mutex buffers_mtx_;
        std::vector<std::string> buffers_[2]; // a double buffer
        std::vector<boost::asio::const_buffer> buffer_seq_;
        int active_buffer_ = 0;
        bool closing_ = false;
        bool closed_ = false;
        boost::asio::deadline_timer read_timer_, write_timer_;
        tcp::socket socket_;

    public:
        Connection(Server& server) : server_(server), read_timer_(server.io_service_), write_timer_(server.io_service_), socket_(server.io_service_) {}

        void Start() {
            socket_.set_option(tcp::no_delay(true));
            DoReadCmd();
        }

        void Close() {
            closing_ = true;
            if (!Writing())
                Shutdown();
        }

        void Shutdown() {
            if (!closed_) {
                closing_ = closed_ = true;
                boost::system::error_code ec;
                socket_.shutdown(tcp::socket::shutdown_both, ec);
                socket_.close();
                server_.active_connections_.erase(shared_from_this());
            }
        }

        void Write(std::string&& data) {
            std::lock_guard<std::mutex> lock(buffers_mtx_);
            buffers_[active_buffer_ ^ 1].push_back(std::move(data)); // move input data to the inactive buffer
            if (!Writing())
                DoWrite();
        }

    };

    void DoAccept() {
        if (acceptor_.is_open()) {
            auto session = std::make_shared<Connection>(*this);
            acceptor_.async_accept(session->socket_, [this, session](boost::system::error_code ec) {
                if (!ec) {
                    active_connections_.insert(session);
                    session->Start();
                }
                DoAccept();
            });
        }
    }

    boost::asio::io_service io_service_;
    tcp::acceptor acceptor_;
    std::unordered_set<std::shared_ptr<Connection>> active_connections_;
    const boost::posix_time::time_duration read_timeout_ = boost::posix_time::seconds(30);
    const boost::posix_time::time_duration write_timeout_ = boost::posix_time::seconds(30);

public:
    Server(int port) : acceptor_(io_service_, tcp::endpoint(tcp::v6(), port), false) { }

    void run() {
        DoAccept();
        io_service_.run();
    }

    void Stop() {
        acceptor_.close();
        {
            std::vector<std::shared_ptr<Connection>> sessionsToClose;
            copy(active_connections_.begin(), active_connections_.end(), back_inserter(sessionsToClose));
            for (auto& s : sessionsToClose)
                s->Shutdown();
        }
        active_connections_.clear();
        io_service_.stop();
    }

};

#endif //SENDO_SERVER_H
