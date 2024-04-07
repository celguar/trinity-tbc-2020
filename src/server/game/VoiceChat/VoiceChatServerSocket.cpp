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

#include "VoiceChatServerSocket.h"
#include "VoiceChatMgr.h"
#include <boost/asio/ip/tcp.hpp>

#if defined( __GNUC__ )
#pragma pack(1)
#else
#pragma pack(push,1)
#endif

struct VoiceChatServerPktHeader
{
    /**
    * size is the length of the payload _plus_ the length of the opcode
    */
    VoiceChatServerPktHeader(uint16 size, uint16 cmd)
    {
        ASSERT(size < std::numeric_limits<uint16>::max());
        header[3] = 0xFF & (size >> 8);
        header[2] = 0xFF & size;

        header[0] = 0xFF & cmd;
        header[1] = 0xFF & (cmd >> 8);
    }

    inline uint8 getHeaderLength() const
    {
        return 2 + 2;
    }

    uint8 header[4];
};

#if defined( __GNUC__ )
#pragma pack()
#else
#pragma pack(pop)
#endif

using boost::asio::ip::tcp;

VoiceChatServerSocket::VoiceChatServerSocket(tcp::socket&& socket)
    : Socket(std::move(socket)), _sendBufferSize(4096)
{
    _headerBuffer.Resize(sizeof(VoiceChatFromServerPktHeader));
}

VoiceChatServerSocket::~VoiceChatServerSocket()
{
}

void VoiceChatServerSocket::Start()
{
    sVoiceChatMgr->RequestNewSocket(this->shared_from_this().get());
    AsyncRead();
}

bool VoiceChatServerSocket::Update()
{
    VoiceChatServerPacket* queued;
    MessageBuffer buffer(_sendBufferSize);
    while (_bufferQueue.Dequeue(queued))
    {
        VoiceChatServerPktHeader header(queued->size()/* + 2*/, queued->GetOpcode());

        if (buffer.GetRemainingSpace() < queued->size() + header.getHeaderLength())
        {
            QueuePacket(std::move(buffer));
            buffer.Resize(_sendBufferSize);
        }

        if (buffer.GetRemainingSpace() >= queued->size() + header.getHeaderLength())
        {
            buffer.Write(header.header, header.getHeaderLength());
            if (!queued->empty())
                buffer.Write(queued->contents(), queued->size());
        }
        else    // single packet larger than 4096 bytes
        {
            MessageBuffer packetBuffer(queued->size() + header.getHeaderLength());
            packetBuffer.Write(header.header, header.getHeaderLength());
            if (!queued->empty())
                packetBuffer.Write(queued->contents(), queued->size());

            QueuePacket(std::move(packetBuffer));
        }

        delete queued;
    }

    if (buffer.GetActiveSize() > 0)
        QueuePacket(std::move(buffer));

    if (!VoiceSocket::Update())
        return false;

    _queryProcessor.ProcessReadyQueries();

    return true;
}

void VoiceChatServerSocket::OnClose()
{
    //sVoiceChatMgr->SocketDisconnected();
}

void VoiceChatServerSocket::ReadHandler()
{
    if (!IsOpen())
        return;

    MessageBuffer& packet = GetReadBuffer();
    while (packet.GetActiveSize() > 0)
    {
        if (_headerBuffer.GetRemainingSpace() > 0)
        {
            // need to receive the header
            std::size_t readHeaderSize = std::min(packet.GetActiveSize(), _headerBuffer.GetRemainingSpace());
            _headerBuffer.Write(packet.GetReadPointer(), readHeaderSize);
            packet.ReadCompleted(readHeaderSize);

            if (_headerBuffer.GetRemainingSpace() > 0)
            {
                // Couldn't receive the whole header this time.
                ASSERT(packet.GetActiveSize() == 0);
                break;
            }

            // We just received nice new header
            if (!ReadHeaderHandler())
            {
                CloseSocket();
                return;
            }
        }

        // We have full read header, now check the data payload
        if (_packetBuffer.GetRemainingSpace() > 0)
        {
            // need more data in the payload
            std::size_t readDataSize = std::min(packet.GetActiveSize(), _packetBuffer.GetRemainingSpace());
            _packetBuffer.Write(packet.GetReadPointer(), readDataSize);
            packet.ReadCompleted(readDataSize);

            if (_packetBuffer.GetRemainingSpace() > 0)
            {
                // Couldn't receive the whole data this time.
                ASSERT(packet.GetActiveSize() == 0);
                break;
            }
        }

        // just received fresh new payload
        ReadDataHandlerResult result = ReadDataHandler();
        _headerBuffer.Reset();
        if (result != ReadDataHandlerResult::Ok)
        {
            if (result != ReadDataHandlerResult::WaitingForQuery)
                CloseSocket();

            return;
        }
    }

    AsyncRead();
}

bool VoiceChatServerSocket::ReadHeaderHandler()
{
    ASSERT(_headerBuffer.GetActiveSize() == sizeof(VoiceChatFromServerPktHeader));

    VoiceChatFromServerPktHeader* header = reinterpret_cast<VoiceChatFromServerPktHeader*>(_headerBuffer.GetReadPointer());

    // not needed for voice server
    //EndianConvertReverse(header->size);
    //EndianConvert(header->cmd);

    if (!header->IsValidSize() || !header->IsValidOpcode())
    {
        TC_LOG_ERROR("voicechat", "VoiceChatServerSocket::ReadHeaderHandler(): voicechat server %s sent malformed packet (size: %hu, cmd: %u)",
            GetRemoteIpAddress().to_string().c_str(), header->size, header->cmd);
        return false;
    }

    // not needed for voice server
    //header->size -= sizeof(header->cmd);
    _packetBuffer.Resize(header->size);
    return true;
}

VoiceChatServerSocket::ReadDataHandlerResult VoiceChatServerSocket::ReadDataHandler()
{
    VoiceChatFromServerPktHeader* header = reinterpret_cast<VoiceChatFromServerPktHeader*>(_headerBuffer.GetReadPointer());
    VoiceChatServerOpcodeServer opcode = static_cast<VoiceChatServerOpcodeServer>(header->cmd);
    std::unique_ptr<VoiceChatServerPacket> packet = std::make_unique<VoiceChatServerPacket>(opcode, std::move(_packetBuffer));

    if (!sVoiceChatMgr->CanUseVoiceChat())
    {
        TC_LOG_ERROR("voicechat", "ReadDataHandler: voicechat server sent opcode = %u, but can't use voice chat", uint32(opcode));
        CloseSocket();
        return ReadDataHandlerResult::Error;
    }

    // Copy the packet to the heap before enqueuing
    sVoiceChatMgr->QueuePacket(std::move(packet));
    return ReadDataHandlerResult::Ok;
}

void VoiceChatServerSocket::SendPacket(VoiceChatServerPacket const& packet)
{
    if (!IsOpen())
        return;

    _bufferQueue.Enqueue(new VoiceChatServerPacket(packet));
}
