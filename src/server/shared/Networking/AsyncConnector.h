/*
* Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __ASYNCCONNECT_H_
#define __ASYNCCONNECT_H_

#include "IoContext.h"
#include "IpAddress.h"
#include "Log.h"
#include <boost/asio.hpp>
#include <functional>
#include <atomic>

using boost::asio::ip::tcp;

class AsyncConnector
{
public:
    typedef void(*ConnectCallback)(tcp::socket&& newSocket, uint32 threadIndex);

    AsyncConnector(Trinity::Asio::IoContext& ioContext, std::string const& connectIp, uint16 connectPort) :
        _endpoint(Trinity::Net::make_address(connectIp), connectPort),
        _socket(ioContext), _closed(false), _socketFactory(std::bind(&AsyncConnector::DefeaultSocketFactory, this))
    {
    }

    template<class T>
    void Connect();

    void Close()
    {
        if (_closed.exchange(true))
            return;

        //boost::system::error_code err;
        //_acceptor.close(err);
    }

    void SetSocketFactory(std::function<std::pair<tcp::socket*, uint32>()> func) { _socketFactory = func; }

private:
    std::pair<tcp::socket*, uint32> DefeaultSocketFactory() { return std::make_pair(&_socket, 0); }

    tcp::endpoint _endpoint;
    tcp::socket _socket;
    std::atomic<bool> _closed;
    std::function<std::pair<tcp::socket*, uint32>()> _socketFactory;
};

template<class T>
void AsyncConnector::Connect()
{
    _socket.async_connect(_endpoint, [this](boost::system::error_code error)
    {
        if (!error)
        {
            try
            {
                // this-> is required here to fix an segmentation fault in gcc 4.7.2 - reason is lambdas in a templated class
                std::make_shared<T>(std::move(this->_socket))->Start();
            }
            catch (boost::system::system_error const& err)
            {
                TC_LOG_INFO("network", "Failed to retrieve client's remote address %s", err.what());
            }
        }
    });
}

#endif /* __ASYNCCONNECT_H_ */
