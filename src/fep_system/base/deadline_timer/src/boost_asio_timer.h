/**
 * Copyright 2023 CARIAD SE. 
 *
 * This Source Code Form is subject to the terms of the Mozilla
 * Public License, v. 2.0. If a copy of the MPL was not distributed
 * with this file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/chrono.hpp>

#include <thread>
#include <chrono>
#include <functional>

namespace fep3 {
class BoostAsioTimer {
public:
    BoostAsioTimer()
    {
        _work = std::make_unique<boost::asio::io_service::work>(_io_service);
        _asio_thread = std::thread([&]() { _io_service.run(); });
        _timer = std::make_shared<boost::asio::deadline_timer>(_io_service);
    }

    ~BoostAsioTimer()
    {
        _work.reset();
        if (!_io_service.stopped()) {
            _io_service.stop();
        }

        if (_asio_thread.joinable()) {
            _asio_thread.join();
        }
    }

    void expireAt(std::chrono::milliseconds timeout, std::function<void()> callback)
    {
        _timer->expires_from_now(boost::posix_time::milliseconds(timeout.count()));
        _timer->async_wait([callback](const boost::system::error_code& ec) -> void {
            //https://www.boost.org/doc/libs/1_66_0/doc/html/boost_asio/reference/basic_deadline_timer/async_wait.html
            // in case the timer is canceled we receive an error and we do not want to call
            // the callback in this case
            if (!ec) {
                callback();
            }
        });
    }

    void cancel()
    {
        _timer->cancel();
    }

    private:
    boost::asio::io_service _io_service;
    std::unique_ptr<boost::asio::io_service::work> _work;
    std::shared_ptr<boost::asio::deadline_timer> _timer;
    std::thread _asio_thread;
};

} // namespace fep3
