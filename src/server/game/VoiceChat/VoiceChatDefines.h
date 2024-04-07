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

#ifndef _VOICECHATDEFINES_H
#define _VOICECHATDEFINES_H

#include "Common.h"
//#include "Globals/SharedDefines.h"
#include "Entities/Object/ObjectGuid.h"

enum VoiceChatServerOpcodes
{
    VOICECHAT_NULL_ACTION = 0,
    VOICECHAT_CMSG_CREATE_CHANNEL = 1,
    VOICECHAT_SMSG_CHANNEL_CREATED = 2,
    VOICECHAT_CMSG_ADD_MEMBER = 3,
    VOICECHAT_CMSG_REMOVE_MEMBER = 4,
    VOICECHAT_CMSG_VOICE_MEMBER = 5,
    VOICECHAT_CMSG_DEVOICE_MEMBER = 6,
    VOICECHAT_CMSG_MUTE_MEMBER = 7,
    VOICECHAT_CMSG_UNMUTE_MEMBER = 8,
    VOICECHAT_CMSG_DELETE_CHANNEL = 9,
    VOICECHAT_CMSG_PING = 10,
    VOICECHAT_SMSG_PONG = 11,
    VOICECHAT_NUM_OPCODES = 12,
};

typedef VoiceChatServerOpcodes VoiceChatServerOpcodeClient;
typedef VoiceChatServerOpcodes VoiceChatServerOpcodeServer;

struct VoiceChatChannelRequest
{
    uint32 id;
    uint8 type;
    uint32 groupid;
    std::string channel_name;
    Team team;
};

enum VoiceChatMemberFlags
{
    VOICECHAT_MEMBER_FLAG_ENABLED     = 0x4,
    VOICECHAT_MEMBER_FLAG_FORCE_MUTED = 0x8,
    VOICECHAT_MEMBER_FLAG_MIC_MUTED   = 0x20,
    VOICECHAT_MEMBER_FLAG_VOICED      = 0x40,
};

enum VoiceChatChannelTypes
{
    VOICECHAT_CHANNEL_CUSTOM = 0,
    VOICECHAT_CHANNEL_BG     = 1,
    VOICECHAT_CHANNEL_GROUP  = 2,
    VOICECHAT_CHANNEL_RAID   = 3,
    VOICECHAT_CHANNEL_NONE   = 4,
};

enum VoiceChatState
{
    VOICECHAT_DISCONNECTED  = 0,
    VOICECHAT_NOT_CONNECTED = 1,
    VOICECHAT_CONNECTED     = 2,
    VOICECHAT_RECONNECTING  = 3,
};

struct VoiceChatMember
{
    explicit VoiceChatMember(ObjectGuid guid = ObjectGuid(), uint8 userId = 0, uint8 userFlags = VOICECHAT_MEMBER_FLAG_ENABLED)
    {
        m_guid = guid;
        user_id = userId;
        flags = userFlags;
        flags_unk = 0x80;
    }

    ObjectGuid m_guid;
    uint8 user_id;
    uint8 flags;
    uint8 flags_unk;

    inline bool HasFlag(uint8 flag) const { return (flags & flag) != 0; }
    void SetFlag(uint8 flag, bool state) { if (state) flags |= flag; else flags &= ~flag; }
    inline bool IsEnabled() const { return HasFlag(VOICECHAT_MEMBER_FLAG_ENABLED); }
    inline void SetEnabled(bool state) { SetFlag(VOICECHAT_MEMBER_FLAG_ENABLED, state); }
    inline bool IsVoiced() const { return HasFlag(VOICECHAT_MEMBER_FLAG_VOICED); }
    inline void SetVoiced(bool state) { SetFlag(VOICECHAT_MEMBER_FLAG_VOICED, state); }
    inline bool IsMuted() const { return HasFlag(VOICECHAT_MEMBER_FLAG_MIC_MUTED); }
    inline void SetMuted(bool state) { SetFlag(VOICECHAT_MEMBER_FLAG_MIC_MUTED, state); }
    inline bool IsForceMuted() const { return HasFlag(VOICECHAT_MEMBER_FLAG_FORCE_MUTED); }
    inline void SetForceMuted(bool state) { SetFlag(VOICECHAT_MEMBER_FLAG_FORCE_MUTED, state); }
};

class TC_GAME_API VoiceChatServerPacket : public ByteBuffer
{
public:
    // just container for later use
    VoiceChatServerPacket() : ByteBuffer(0), m_opcode(VOICECHAT_NULL_ACTION)
    {
    }

    explicit VoiceChatServerPacket(uint16 opcode, size_t res = 200) : ByteBuffer(res), m_opcode(opcode) { }

    VoiceChatServerPacket(VoiceChatServerPacket&& packet) : ByteBuffer(std::move(packet)), m_opcode(packet.m_opcode)
    {
    }

    VoiceChatServerPacket(VoiceChatServerPacket const& right) : ByteBuffer(right), m_opcode(right.m_opcode)
    {
    }

    VoiceChatServerPacket& operator=(VoiceChatServerPacket const& right)
    {
        if (this != &right)
        {
            m_opcode = right.m_opcode;
            ByteBuffer::operator=(right);
        }

        return *this;
    }

    VoiceChatServerPacket& operator=(VoiceChatServerPacket&& right)
    {
        if (this != &right)
        {
            m_opcode = right.m_opcode;
            ByteBuffer::operator=(std::move(right));
        }

        return *this;
    }

    VoiceChatServerPacket(uint16 opcode, MessageBuffer&& buffer) : ByteBuffer(std::move(buffer)), m_opcode(opcode) { }

    void Initialize(uint16 opcode, size_t newres = 200)
    {
        clear();
        _storage.reserve(newres);
        m_opcode = opcode;
    }

    uint16 GetOpcode() const { return m_opcode; }
    void SetOpcode(uint16 opcode) { m_opcode = opcode; }

protected:
    uint16 m_opcode;
};

//class VoiceChatServerPacket : public ByteBuffer
//{
//public:
//    // just container for later use
//    VoiceChatServerPacket() : ByteBuffer(0), m_opcode(VOICECHAT_NULL_ACTION)
//    {
//    }
//    explicit VoiceChatServerPacket(VoiceChatServerOpcodes opcode, size_t res = 200) : ByteBuffer(res), m_opcode(opcode) { }
//    // copy constructor
//    VoiceChatServerPacket(const VoiceChatServerPacket& packet) : ByteBuffer(packet), m_opcode(packet.m_opcode)
//    {
//    }
//
//    void Initialize(VoiceChatServerOpcodes opcode, size_t newres = 200)
//    {
//        clear();
//        reserve(newres);
//        m_opcode = opcode;
//    }
//
//    VoiceChatServerOpcodes GetOpcode() const { return m_opcode; }
//    void SetOpcode(VoiceChatServerOpcodes opcode) { m_opcode = opcode; }
//    
//protected:
//    VoiceChatServerOpcodes m_opcode;
//};

struct VoiceChatStatistics
{
    uint32 channels;
    uint32 active_users;
    uint32 totalVoiceChatEnabled;
    uint32 totalVoiceMicEnabled;
};

#endif
