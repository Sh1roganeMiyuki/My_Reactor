#pragma once
#include <sys/epoll.h>
#include <unistd.h>
#include <vector>
#include <functional>

#include "EventLoop.h"
using ReadCallback = std::function<void()>;
using WriteCallback = std::function<void()>;
using ErrorCallback = std::function<void()>;
class Epoller;
class EventLoop;
class Channel{
public:
    Channel(int fd, EventLoop *loop);
    ~Channel();
    void handle_event();
    void enableReading();
    void update();
    int get_fd() const { return fd_; };
    uint32_t get_events() const { return events_; }
    void set_revents(uint32_t revents) { revents_ = revents; }

    void disableAll() { events_ = 0; update(); }
    void remove();

    void setReadCallback(const ReadCallback &cb) { read_callback_ = cb; }
    void setWriteCallback(const WriteCallback &cb) { write_callback_ = cb; }
    void setErrorCallback(const ErrorCallback &cb) { error_callback_ = cb; }

private:
    int fd_;
    uint32_t events_;
    uint32_t revents_;
    EventLoop *loop_;
    bool is_in_loop_;

    ReadCallback read_callback_;
    WriteCallback write_callback_;
    ErrorCallback error_callback_;
};