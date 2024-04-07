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

#include "Common.h"
#include "VoiceChatMgr.h"
#include "VoiceChatChannel.h"
#include "ChannelMgr.h"
#include "SocialMgr.h"
#include "World.h"
#include "Server/WorldSession.h"
#include "Language.h"
#include "CharacterCache.h"

void WorldSession::HandleVoiceSessionEnableOpcode(WorldPacket & recv_data)
{
    TC_LOG_DEBUG("voicechat", "WORLD: Received CMSG_VOICE_SESSION_ENABLE");

    if (!sVoiceChatMgr->CanSeeVoiceChat())
        return;

    // comes from in game voice chat settings
    // is sent during login or when changing settings
    uint8 voiceEnabled, micEnabled;
    recv_data >> voiceEnabled;
    recv_data >> micEnabled;

    if(!voiceEnabled)
    {
        if (_player)
        {
            sVoiceChatMgr->RemoveFromVoiceChatChannels(_player->GetGUID());
            SetCurrentVoiceChannelId(0);
        }
    }
    else
    {
        // send available voice channels
        if (_player && _player->IsInWorld() && !m_voiceEnabled)
        {
            // enable it here to allow joining channels
            m_voiceEnabled = voiceEnabled;
            m_micEnabled = micEnabled;
            sVoiceChatMgr->JoinAvailableVoiceChatChannels(this);
        }
    }

    if (!micEnabled)
    {
        if (_player)
        {
            if (GetCurrentVoiceChannelId())
            {
                VoiceChatChannel* current_channel = sVoiceChatMgr->GetVoiceChatChannel(GetCurrentVoiceChannelId());
                if (current_channel)
                    current_channel->MuteMember(_player->GetGUID());
            }
        }
    }
    else
    {
        if (_player)
        {
            if (GetCurrentVoiceChannelId())
            {
                VoiceChatChannel* current_channel = sVoiceChatMgr->GetVoiceChatChannel(GetCurrentVoiceChannelId());
                if (current_channel)
                    current_channel->UnmuteMember(_player->GetGUID());
            }
        }
    }

    m_micEnabled = micEnabled;
    m_voiceEnabled = voiceEnabled;
}

void WorldSession::HandleSetActiveVoiceChannel(WorldPacket & recv_data)
{
    TC_LOG_DEBUG("voicechat", "WORLD: Received CMSG_SET_ACTIVE_VOICE_CHANNEL");

    if (!sVoiceChatMgr->CanUseVoiceChat())
        return;

    if (!_player || !_player->IsInWorld())
        return;

    uint32 type;
    std::string name;
    recv_data >> type;

    // leave current voice channel if player selects different one
    VoiceChatChannel* current_channel = nullptr;
    if (GetCurrentVoiceChannelId())
        current_channel = sVoiceChatMgr->GetVoiceChatChannel(GetCurrentVoiceChannelId());

    switch (type)
    {
        case VOICECHAT_CHANNEL_CUSTOM:
        {
            recv_data >> name;
            // custom channel
            if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
            {
                Channel* chan = cMgr->GetChannel(name);
                if (!chan || !chan->IsOn(_player->GetGUID()) || chan->IsBanned(_player->GetGUID()) || !chan->IsVoiceEnabled())
                    return;
            }
            else
                return;

            if (VoiceChatChannel* v_channel = sVoiceChatMgr->GetCustomVoiceChatChannel(name, (Team)_player->GetTeam()))
            {
                if (current_channel)
                {
                    // if same channel, just update roster
                    if (v_channel == current_channel)
                    {
                        v_channel->SendVoiceRosterUpdate();
                        return;
                    }
                    else
                        current_channel->DevoiceMember(_player->GetGUID());
                }

                v_channel->AddVoiceChatMember(_player->GetGUID());
                if (v_channel->IsOn(_player->GetGUID()))
                {
                    // change speaker icon from grey to color
                    v_channel->VoiceMember(_player->GetGUID());
                    // allow to speak depending on settings
                    if (IsMicEnabled())
                        v_channel->UnmuteMember(_player->GetGUID());
                    else
                        v_channel->MuteMember(_player->GetGUID());

                    SetCurrentVoiceChannelId(v_channel->GetChannelId());
                }
            }

            break;
        }
        case VOICECHAT_CHANNEL_GROUP:
        case VOICECHAT_CHANNEL_RAID:
        {
            // group
            Group* grp = _player->GetGroup();
            if (grp && grp->isBGGroup())
                grp = _player->GetOriginalGroup();

            if (grp)
            {
                VoiceChatChannel* v_channel = nullptr;
                if (grp->isRaidGroup())
                    v_channel = sVoiceChatMgr->GetRaidVoiceChatChannel(grp->GetLowGUID());
                else
                    v_channel = sVoiceChatMgr->GetGroupVoiceChatChannel(grp->GetLowGUID());

                if (v_channel)
                {
                    if (current_channel)
                    {
                        // if same channel, just update roster
                        if (v_channel == current_channel)
                        {
                            v_channel->SendVoiceRosterUpdate();
                            return;
                        }
                        else
                            current_channel->DevoiceMember(_player->GetGUID());
                    }

                    v_channel->AddVoiceChatMember(_player->GetGUID());
                    if (v_channel->IsOn(_player->GetGUID()))
                    {
                        // change speaker icon from grey to color
                        v_channel->VoiceMember(_player->GetGUID());
                        // allow to speak depending on settings
                        if (IsMicEnabled())
                            v_channel->UnmuteMember(_player->GetGUID());
                        else
                            v_channel->MuteMember(_player->GetGUID());

                        SetCurrentVoiceChannelId(v_channel->GetChannelId());
                    }
                }
            }

            break;
        }
        case VOICECHAT_CHANNEL_BG:
        {
            // bg
            if (_player->InBattleground())
            {
                VoiceChatChannel* v_channel = sVoiceChatMgr->GetBattlegroundVoiceChatChannel(_player->GetBattlegroundId(), (Team)_player->GetBGTeam());
                if (v_channel)
                {
                    if (current_channel)
                    {
                        // if same channel, just update roster
                        if (v_channel == current_channel)
                        {
                            v_channel->SendVoiceRosterUpdate();
                            return;
                        }
                        else
                            current_channel->DevoiceMember(_player->GetGUID());
                    }

                    v_channel->AddVoiceChatMember(_player->GetGUID());
                    if (v_channel->IsOn(_player->GetGUID()))
                    {
                        // change speaker icon from grey to color
                        v_channel->VoiceMember(_player->GetGUID());
                        // allow to speak depending on settings
                        if (IsMicEnabled())
                            v_channel->UnmuteMember(_player->GetGUID());
                        else
                            v_channel->MuteMember(_player->GetGUID());

                        SetCurrentVoiceChannelId(v_channel->GetChannelId());
                    }
                }
            }

            break;
        }
        case VOICECHAT_CHANNEL_NONE:
        {
            // leave current channel
            if (current_channel)
            {
                current_channel->DevoiceMember(_player->GetGUID());
            }
            SetCurrentVoiceChannelId(0);

            break;
        }
        default:
            break;
    }
}

void WorldSession::HandleChannelVoiceOnOpcode(WorldPacket& recv_data)
{
    TC_LOG_DEBUG("voicechat", "WORLD: Received CMSG_CHANNEL_VOICE_ON");

    if (!sVoiceChatMgr->CanUseVoiceChat())
        return;

    std::string name;
    recv_data >> name;

    if (!_player)
        return;

    // custom channel
    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
    {
        Channel* chn = cMgr->GetChannel(name, _player);
        if (!chn)
            return;

        if (chn->IsLFG() || chn->IsConstant())
        {
            TC_LOG_ERROR("voicechat", "VoiceChat: Channel is LFG or constant, can't use voice!");
            return;
        }

        // already enabled
        if (chn->IsVoiceEnabled())
            return;

        chn->ToggleVoice(_player->GetGUID());

        sVoiceChatMgr->CreateCustomVoiceChatChannel(chn->GetName(), (Team)_player->GetTeam());
    }
}

void WorldSession::HandleChannelVoiceOffOpcode(WorldPacket& recv_data)
{
    TC_LOG_DEBUG("voicechat", "WORLD: Received CMSG_CHANNEL_VOICE_OFF");

    // todo check if possible to send with chat commands
}

void WorldSession::HandleAddVoiceIgnoreOpcode(WorldPacket& recvData)
{
    //TC_LOG_DEBUG("network", "WORLD: Received CMSG_ADD_IGNORE");

    std::string ignoreName = GetTrinityString(LANG_FRIEND_IGNORE_UNKNOWN);

    recvData >> ignoreName;

    if (!normalizePlayerName(ignoreName))
        return;

    /*TC_LOG_DEBUG("network", "WORLD: %s asked to Ignore: '%s'",
        GetPlayer()->GetName().c_str(), ignoreName.c_str());*/

    FriendsResult ignoreResult;

    ignoreResult = FRIEND_MUTE_NOT_FOUND;

    ObjectGuid::LowType ignoreGuid = sCharacterCache->GetCharacterGuidByName(ignoreName);
    if (ignoreGuid)
    {
        if (ignoreGuid == GetPlayer()->GetGUID())              //not add yourself
            ignoreResult = FRIEND_MUTE_SELF;
        else if (GetPlayer()->GetSocial()->HasIgnore(ignoreGuid))
            ignoreResult = FRIEND_MUTE_ALREADY;
        else
        {
            ignoreResult = FRIEND_MUTE_ADDED;

            // ignore list full
            if (!GetPlayer()->GetSocial()->AddToSocialList(ignoreGuid, true))
                ignoreResult = FRIEND_MUTE_FULL;
        }
    }

    sSocialMgr->SendFriendStatus(GetPlayer(), ignoreResult, ignoreGuid, false);
}

void WorldSession::HandleDelVoiceIgnoreOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "WORLD: Received CMSG_DEL_IGNORE");

    ObjectGuid ignoreGUID;

    recvData >> ignoreGUID;

    _player->GetSocial()->RemoveFromSocialList(ignoreGUID.GetCounter(), true);

    sSocialMgr->SendFriendStatus(GetPlayer(), FRIEND_IGNORE_REMOVED, ignoreGUID.GetCounter(), false);
}

void WorldSession::HandleSilenceInParty(WorldPacket& recvData)
{
    TC_LOG_DEBUG("voicechat", "WORLD: Received CMSG_PARTY_SILENCE");

    if (!sVoiceChatMgr->CanUseVoiceChat())
        return;

    ObjectGuid ignoreGuid;
    recvData >> ignoreGuid;

    if (!_player)
        return;

    Group* grp = _player->GetGroup();
    if (!grp)
        return;

    if (!grp->IsMember(ignoreGuid))
        return;

    if (!grp->IsLeader(GetPlayer()->GetGUID()) && !grp->IsAssistant(GetPlayer()->GetGUID()))
        return;

    VoiceChatChannel* v_channel = nullptr;
    if (!grp->isBGGroup())
    {
        if (grp->isRaidGroup())
            v_channel = sVoiceChatMgr->GetRaidVoiceChatChannel(grp->GetLowGUID());
        else
            v_channel = sVoiceChatMgr->GetGroupVoiceChatChannel(grp->GetLowGUID());
    }
    else if (_player->InBattleground())
        v_channel = sVoiceChatMgr->GetBattlegroundVoiceChatChannel(_player->GetBattlegroundId(), (Team)_player->GetBGTeam());

    if (!v_channel)
        return;

    v_channel->ForceMuteMember(ignoreGuid);
}

void WorldSession::HandleUnsilenceInParty(WorldPacket& recvData)
{
    TC_LOG_DEBUG("voicechat", "WORLD: Received CMSG_PARTY_UNSILENCE");

    if (!sVoiceChatMgr->CanUseVoiceChat())
        return;

    ObjectGuid ignoreGuid;
    recvData >> ignoreGuid;

    if (!_player)
        return;

    Group* grp = _player->GetGroup();
    if (!grp)
        return;

    if (!grp->IsMember(ignoreGuid))
        return;

    if (!grp->IsLeader(GetPlayer()->GetGUID()) && !grp->IsAssistant(GetPlayer()->GetGUID()))
        return;

    VoiceChatChannel* v_channel = nullptr;
    if (!grp->isBGGroup())
    {
        if (grp->isRaidGroup())
            v_channel = sVoiceChatMgr->GetRaidVoiceChatChannel(grp->GetLowGUID());
        else
            v_channel = sVoiceChatMgr->GetGroupVoiceChatChannel(grp->GetLowGUID());
    }
    else if (_player->InBattleground())
        v_channel = sVoiceChatMgr->GetBattlegroundVoiceChatChannel(_player->GetBattlegroundId(), (Team)_player->GetBGTeam());

    if (!v_channel)
        return;

    v_channel->ForceUnmuteMember(ignoreGuid);
}

void WorldSession::HandleSilenceInChannel(WorldPacket& recvData)
{
    TC_LOG_DEBUG("voicechat", "WORLD: Received CMSG_CHANNEL_SILENCE");

    if (!sVoiceChatMgr->CanUseVoiceChat())
        return;

    if (!_player)
        return;

    std::string channelName, playerName;
    recvData >> channelName >> playerName;

    Player* plr = ObjectAccessor::FindConnectedPlayerByName(playerName);
    if (!plr)
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
    {
        Channel* chan = cMgr->GetChannel(channelName);
        if (chan && chan->IsVoiceEnabled())
        {
            if (VoiceChatChannel* v_channel = sVoiceChatMgr->GetCustomVoiceChatChannel(channelName, (Team)_player->GetTeam()))
            {
                if (v_channel->IsOn(plr->GetGUID()))
                {
                    v_channel->ForceMuteMember(plr->GetGUID());
                    chan->SetMicMute(plr->GetGUID(), true);
                }
            }
        }
    }
}

void WorldSession::HandleUnsilenceInChannel(WorldPacket& recvData)
{
    TC_LOG_DEBUG("voicechat", "WORLD: Received CMSG_CHANNEL_UNSILENCE");

    if (!sVoiceChatMgr->CanUseVoiceChat())
        return;

    if (!_player)
        return;

    std::string channelName, playerName;
    recvData >> channelName >> playerName;

    Player* plr = ObjectAccessor::FindConnectedPlayerByName(playerName);
    if (!plr)
        return;

    if (ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
    {
        Channel* chan = cMgr->GetChannel(channelName);
        if (chan && chan->IsVoiceEnabled())
        {
            if (VoiceChatChannel* v_channel = sVoiceChatMgr->GetCustomVoiceChatChannel(channelName, (Team)_player->GetTeam()))
            {
                if (v_channel->IsOn(plr->GetGUID()))
                {
                    v_channel->ForceUnmuteMember(plr->GetGUID());
                    chan->SetMicMute(plr->GetGUID(), false);
                }
            }
        }
    }
}
