/*
 * This file is part of the CMaNGOS Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _VOICECHATSERVERSSOCKET_H
#define _VOICECHATSERVERSSOCKET_H

#include "Socket.h"
#include "VoiceChatDefines.h"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <chrono>
#include <mutex>
#include <shared_mutex>

using boost::asio::ip::tcp;
class VoiceChatServerPacket;

#pragma pack(push, 1)

struct VoiceChatFromServerPktHeader
{
    uint16 cmd;
    uint16 size;

    bool IsValidSize() const { return size >= 2 && size < 10240; }
    bool IsValidOpcode() const { return cmd < VOICECHAT_NUM_OPCODES; }
};

#pragma pack(pop)

class TC_GAME_API VoiceChatServerSocket : public Socket<VoiceChatServerSocket>
{
    typedef Socket<VoiceChatServerSocket> VoiceSocket;

public:
    VoiceChatServerSocket(tcp::socket&& socket);
    ~VoiceChatServerSocket();

    VoiceChatServerSocket(VoiceChatServerSocket const& right) = delete;
    VoiceChatServerSocket& operator=(VoiceChatServerSocket const& right) = delete;

    void Start() override;
    bool Update() override;

    void SendPacket(VoiceChatServerPacket const& packet);

    void SetSendBufferSize(std::size_t sendBufferSize) { _sendBufferSize = sendBufferSize; }

protected:
    void OnClose() override;
    void ReadHandler() override;
    bool ReadHeaderHandler();

    enum class ReadDataHandlerResult
    {
        Ok = 0,
        Error = 1,
        WaitingForQuery = 2
    };

    ReadDataHandlerResult ReadDataHandler();

private:

    MessageBuffer _headerBuffer;
    MessageBuffer _packetBuffer;
    MPSCQueue<VoiceChatServerPacket> _bufferQueue;
    std::size_t _sendBufferSize;

    QueryCallbackProcessor _queryProcessor;
};

#endif
