
#ifndef __TRINITY_SOCIALMGR_H
#define __TRINITY_SOCIALMGR_H

#include "DatabaseEnvFwd.h"
#include "Common.h"
#include "ObjectGuid.h"
#include <map>

class SocialMgr;
class PlayerSocial;
class Player;
class WorldPacket;

enum FriendStatus
{
    FRIEND_STATUS_OFFLINE   = 0,
    FRIEND_STATUS_ONLINE    = 1,
    FRIEND_STATUS_AFK       = 2,
    FRIEND_STATUS_UNK3      = 3,
    FRIEND_STATUS_DND       = 4
};

enum SocialFlag
{
    SOCIAL_FLAG_FRIEND      = 0x01,
    SOCIAL_FLAG_IGNORED     = 0x02,
    SOCIAL_FLAG_MUTED       = 0x04                          // guessed
};

struct FriendInfo
{
    FriendStatus Status;
    uint32 Flags;
    uint32 Area;
    uint32 Level;
    uint32 Class;
    std::string Note;

    FriendInfo()
    {
        Status = FRIEND_STATUS_OFFLINE;
        Flags = 0;
        Area = 0;
        Level = 0;
        Class = 0;
        Note = "";
    }

    FriendInfo(uint32 flags, const std::string& note)
    {
        Status = FRIEND_STATUS_OFFLINE;
        Flags = flags;
        Area = 0;
        Level = 0;
        Class = 0;
        Note = note;
    }
};

typedef std::map<uint32, FriendInfo> PlayerSocialMap;
typedef std::map<uint32, PlayerSocial> SocialMap;

/// Results of friend related commands
enum FriendsResult
{
    FRIEND_DB_ERROR         = 0x00,
    FRIEND_LIST_FULL        = 0x01,
    FRIEND_ONLINE           = 0x02,
    FRIEND_OFFLINE          = 0x03,
    FRIEND_NOT_FOUND        = 0x04,
    FRIEND_REMOVED          = 0x05,
    FRIEND_ADDED_ONLINE     = 0x06,
    FRIEND_ADDED_OFFLINE    = 0x07,
    FRIEND_ALREADY          = 0x08,
    FRIEND_SELF             = 0x09,
    FRIEND_ENEMY            = 0x0A,
    FRIEND_IGNORE_FULL      = 0x0B,
    FRIEND_IGNORE_SELF      = 0x0C,
    FRIEND_IGNORE_NOT_FOUND = 0x0D,
    FRIEND_IGNORE_ALREADY   = 0x0E,
    FRIEND_IGNORE_ADDED     = 0x0F,
    FRIEND_IGNORE_REMOVED   = 0x10,
    FRIEND_IGNORE_AMBIGUOUS = 0x11,                         // That name is ambiguous, type more of the player's server name
    FRIEND_MUTE_FULL        = 0x12,
    FRIEND_MUTE_SELF        = 0x13,
    FRIEND_MUTE_NOT_FOUND   = 0x14,
    FRIEND_MUTE_ALREADY     = 0x15,
    FRIEND_MUTE_ADDED       = 0x16,
    FRIEND_MUTE_REMOVED     = 0x17,
    FRIEND_MUTE_AMBIGUOUS   = 0x18,                         // That name is ambiguous, type more of the player's server name
    FRIEND_UNK7             = 0x19,                         // no message at client
    FRIEND_UNKNOWN          = 0x1A                          // Unknown friend response from server
};

#define SOCIALMGR_FRIEND_LIMIT  50
#define SOCIALMGR_IGNORE_LIMIT  25

class TC_GAME_API PlayerSocial
{
    friend class SocialMgr;
    public:
        PlayerSocial();
        ~PlayerSocial();
        // adding/removing
#ifdef VOICECHAT
        bool AddToSocialList(ObjectGuid::LowType friend_guid, bool ignore, bool mute = false);
        void RemoveFromSocialList(ObjectGuid::LowType friend_guid, bool ignore, bool mute = false);
#else
        bool AddToSocialList(ObjectGuid::LowType friend_guid, bool ignore);
        void RemoveFromSocialList(ObjectGuid::LowType friend_guid, bool ignore);
#endif
        void SetFriendNote(ObjectGuid::LowType friend_guid, std::string note);
        // Packet send's
        void SendSocialList();
        // Misc
        bool HasFriend(ObjectGuid::LowType friend_guid);
        bool HasIgnore(ObjectGuid::LowType ignore_guid);
        ObjectGuid::LowType GetPlayerGUID() { return m_playerGUID; }
        void SetPlayerGUID(ObjectGuid::LowType guid) { m_playerGUID = guid; }
        uint32 GetNumberOfSocialsWithFlag(SocialFlag flag);
    private:
        PlayerSocialMap m_playerSocialMap;
        ObjectGuid::LowType m_playerGUID;
};

class TC_GAME_API SocialMgr
{
    private:
        SocialMgr();
        ~SocialMgr();
    public:
        static SocialMgr* instance()
        {
            static SocialMgr instance;
            return &instance;
        }
        // Misc
        void RemovePlayerSocial(ObjectGuid::LowType guid);
        void GetFriendInfo(Player *player, ObjectGuid::LowType friendGUID, FriendInfo &friendInfo);
        // Packet management
        void MakeFriendStatusPacket(FriendsResult result, ObjectGuid::LowType friend_guid, WorldPacket *data);
        void SendFriendStatus(Player *player, FriendsResult result, ObjectGuid::LowType friend_guid, bool broadcast);
        void BroadcastToFriendListers(Player *player, WorldPacket *packet);
        // Loading
        PlayerSocial* LoadFromDB(PreparedQueryResult result, ObjectGuid::LowType guid);
        PlayerSocial* GetDefault(ObjectGuid::LowType guid);
    private:
        SocialMap m_socialMap;
};

#define sSocialMgr SocialMgr::instance()
#endif

