/*
<--------------------------------------------------------------------------->
- Developer(s): WiiZZy
- Complete: 100%
- ScriptName: 'World chat'
- Comment: Fully tested
<--------------------------------------------------------------------------->
*/

#include "Channel.h"
#include "Chat.h"
#include "Common.h"
#include "Config.h"
#include "Log.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "World.h"
#include "WorldSession.h"
#include <unordered_map>
#include "SocialMgr.h"
#include <iostream>
#include <string>
#include <chrono>

using namespace std;

#if AC_COMPILER == AC_COMPILER_GNU
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

/* VERSION */
float ver = 2.0f;

/* Colors */
std::string WORLD_CHAT_ALLIANCE_BLUE = "|cff3399FF";
std::string WORLD_CHAT_HORDE_RED     = "|cffCC0000";
std::string WORLD_CHAT_WHITE         = "|cffFFFFFF";
std::string WORLD_CHAT_GREEN         = "|cff00CC00";
std::string WORLD_CHAT_RED           = "|cffFF0000";
std::string WORLD_CHAT_BLUE          = "|cff6666FF";
std::string WORLD_CHAT_BLACK         = "|cff000000";
std::string WORLD_CHAT_GREY          = "|cff808080";

/* Class Colors */
std::string world_chat_ClassColor[11] = {
    "|cffC79C6E", // WARRIOR
    "|cffF58CBA", // PALADIN
    "|cffABD473", // HUNTER
    "|cffFFF569", // ROGUE
    "|cffFFFFFF", // PRIEST
    "|cffC41F3B", // DEATHKNIGHT
    "|cff0070DE", // SHAMAN
    "|cff40C7EB", // MAGE
    "|cff8787ED", // WARLOCK
    "",           // ADDED IN MOP FOR MONK - NOT USED
    "|cffFF7D0A", // DRUID
};

/* Ranks */
std::string world_chat_GM_RANKS[4] = {
    "Player",
    "MOD",
    "GM",
    "ADMIN",
};

/* BLIZZARD CHAT ICON FOR GM'S */
std::string world_chat_GMIcon = "|TINTERFACE/CHATFRAME/UI-CHATICON-BLIZZ:13:13:0:-1|t";
std::string star1icon = "|TINTERFACE/ICONS/Achievement_PVP_P_01:13:13:0:-1|t";
std::string star2icon = "|TINTERFACE/ICONS/Achievement_PVP_P_02:13:13:0:-1|t";
std::string star3icon = "|TINTERFACE/ICONS/Achievement_PVP_P_03:13:13:0:-1|t";
std::string star4icon = "|TINTERFACE/ICONS/Achievement_PVP_P_04:13:13:0:-1|t";
std::string star5icon = "|TINTERFACE/ICONS/Achievement_PVP_P_05:13:13:0:-1|t";

/* COLORED TEXT FOR CURRENT FACTION || NOT FOR GMS */
std::string world_chat_TeamIcon[2] = {"|cff3399FF联盟|r", "|cffCC0000部落|r"};

/* Config Variables */
struct WCConfig
{
    bool        Enabled;
    std::string ChannelName;
    bool        LoginState;
    bool        CrossFaction;
    bool        Announce;
};

WCConfig WC_Config;

class WorldChat_Config : public WorldScript
{
public:
    WorldChat_Config() : WorldScript("WorldChat_Config") {};
    void OnBeforeConfigLoad(bool reload) override
    {
        if (!reload) {
            WC_Config.Enabled = sConfigMgr->GetOption<bool>("World_Chat.Enable", true);
            WC_Config.ChannelName = sConfigMgr->GetOption<std::string>("World_Chat.ChannelName", "World");
            WC_Config.LoginState = sConfigMgr->GetOption<bool>("World_Chat.OnLogin.State", true);
            WC_Config.CrossFaction = sConfigMgr->GetOption<bool>("World_Chat.CrossFactions", true);
            WC_Config.Announce = sConfigMgr->GetOption<bool>("World_Chat.Announce", true);
        }
    }
};

/* STRUCTURE FOR WorldChat map */
struct ChatElements
{
    uint8  chat     = (WC_Config.LoginState) ? 1 : 0; // CHAT DISABLED BY DEFAULT
    double cooldown = 0;

    //增加星团长数据
    uint8  rank     = 0;
    double  rankupdatemark = 0;
};

/* UNORDERED MAP FOR STORING IF CHAT IS ENABLED OR DISABLED */
std::unordered_map<uint32, ChatElements> WorldChat;

void SendWorldMessage(Player* sender, std::string msg, int team) {
	
	double timenow = getMSTime();
	
    if (!WC_Config.Enabled)
    {
        ChatHandler(sender->GetSession()).PSendSysMessage(Acore::StringFormat("[世界] {}世界频道已关闭.|r", WORLD_CHAT_RED));
        return;
    }

    if (!sender->CanSpeak())
    {
        ChatHandler(sender->GetSession()).PSendSysMessage(Acore::StringFormat("[世界] {}禁言玩家禁止使用世界频道!|r", WORLD_CHAT_RED));
        return;
    }

    if (!WorldChat[sender->GetGUID().GetCounter()].chat)
    {
        ChatHandler(sender->GetSession()).PSendSysMessage(Acore::StringFormat("[世界] {}世界频道已关闭. (使用.sj on重新打开)|r", WORLD_CHAT_RED));
        return;
    }

    if (WorldChat[sender->GetGUID().GetCounter()].cooldown < (timenow - 15000) || sender->isGMChat() || sender->IsGameMaster())
    {
        WorldChat[sender->GetGUID().GetCounter()].cooldown = timenow;
    }
    else
    {
        ChatHandler(sender->GetSession()).PSendSysMessage("[世界] {}世界频道说话太快,请稍等 %d秒再试,请勿刷屏|r", WORLD_CHAT_RED.c_str(), (int) (WorldChat[sender->GetGUID().GetCounter()].cooldown + 15000 - timenow) / 1000);
        return;
    }

    //屏蔽系统
    string msgfiter(msg);

    if (msgfiter.find("完成") < msgfiter.length())
    {
        ChatHandler(sender->GetSession()).PSendSysMessage("[世界] {}世界频道禁止使用完成二字(会导致玩家任务插件错误通报).|r", WORLD_CHAT_RED.c_str());
        return;
    }
    //屏蔽系统

    //修复链接后黄字
    string flag = "|r";
    string::size_type position = 0;
    uint8 rflag = 0;
    while ((position = msgfiter.find(flag, position)) != string::npos)
    {
        if (position + 2 < msgfiter.length())
        {
            msgfiter = msgfiter.insert(position + 2, WORLD_CHAT_WHITE.c_str());
            rflag = 1;
        }
        if (position + 2 >= msgfiter.length())
            rflag = 2;
        position++;
    }
    if (rflag == 1)
        msgfiter = msgfiter.insert(msgfiter.length(), flag);
    msg = msgfiter.c_str();
    //修复链接后黄字


    //记录系统,需要添加 Appender.SJCHAT=2,5,9,./chat/SJ.log   以及 Logger.chat.sj=5,Console SJCHAT
    char messagelog[1024];
    if (sender->GetGuild())
    {
        snprintf(messagelog, 1024, "[%s][%s]:%s", sender->GetGuildName().c_str(), sender->GetName().c_str(), msg.c_str());
    }
    else
    {
        snprintf(messagelog, 1024, "[无公会][%s]:%s", sender->GetName().c_str(), msg.c_str());
    }
    LOG_INFO("chat.sj", "{}", messagelog);
    //记录系统

    //初始化星团长
    if (WorldChat[sender->GetGUID().GetCounter()].rankupdatemark < (timenow -30000) || WorldChat[sender->GetGUID().GetCounter()].rankupdatemark == 0 )
    {
        WorldChat[sender->GetGUID().GetCounter()].rankupdatemark = timenow;
        QueryResult result = CharacterDatabase.Query("SELECT `rank` FROM `_star` WHERE id = '{}'", sender->GetSession()->GetAccountId());
        if (result)
        {
            WorldChat[sender->GetGUID().GetCounter()].rank = (*result)[0].Get<uint8>();
        }
    }

    std::string message;

    SessionMap sessions = sWorld->GetAllSessions();

    for (SessionMap::iterator itr = sessions.begin(); itr != sessions.end(); ++itr)
    {
        if (!itr->second)
        {
            continue;
        }

        if (!itr->second->GetPlayer())
        {
            continue;
        }

        if (!itr->second->GetPlayer()->IsInWorld())
        {
            continue;
        }

        Player* target = itr->second->GetPlayer();
        uint64  guid2  = target->GetGUID().GetCounter();

        if (WorldChat[guid2].chat == 1 && (team == -1 || target->GetTeamId() == team))
        {
            if (!target->GetSocial()->HasIgnore(sender->GetGUID()))//增加屏蔽检测
            {
                if (WC_Config.CrossFaction || (sender->GetTeamId() == target->GetTeamId()) || target->GetSession()->GetSecurity())
                {
                    if (sender->isGMChat())
                    {
                        message = Acore::StringFormat("[{}][{}|Hplayer:{}|h{}|h|r]: {}{}|r", ((sender->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_DEVELOPER)) ? (world_chat_ClassColor[5] + "DEV|r") : world_chat_GMIcon), world_chat_ClassColor[sender->getClass() - 1], sender->GetName(), sender->GetName(), WORLD_CHAT_WHITE, msg);
                        WorldChat[sender->GetGUID().GetCounter()].cooldown = 0;
                    }
                    else
                    {
                        //检测是否星团长
                        uint8 rank = WorldChat[sender->GetGUID().GetCounter()].rank;
                        if (rank && rank > 0)
                        {
                            if (rank == 1)
                            {
                                message = Acore::StringFormat("[{}星团长][{}|Hplayer:{}|h{}|h|r]: {}{}|r", star1icon, world_chat_ClassColor[sender->getClass() - 1], sender->GetName(), sender->GetName(), WORLD_CHAT_WHITE, msg);
                                //WorldChat[sender->GetGUID().GetCounter()].cooldown = 0;
                            }
                            if (rank == 2)
                            {
                                message = Acore::StringFormat("[{}星团长][{}|Hplayer:{}|h{}|h|r]: {}{}|r", star2icon, world_chat_ClassColor[sender->getClass() - 1], sender->GetName(), sender->GetName(), WORLD_CHAT_WHITE, msg);
                                //WorldChat[sender->GetGUID().GetCounter()].cooldown = 0;
                            }
                            if (rank == 3)
                            {
                                message = Acore::StringFormat("[{}星团长][{}|Hplayer:{}|h{}|h|r]: {}{}|r", star3icon, world_chat_ClassColor[sender->getClass() - 1], sender->GetName(), sender->GetName(), WORLD_CHAT_WHITE, msg);
                                WorldChat[sender->GetGUID().GetCounter()].cooldown = 0;
                            }
                            if (rank == 4)
                            {
                                message = Acore::StringFormat("[{}星团长][{}|Hplayer:{}|h{}|h|r]: {}{}|r", star4icon, world_chat_ClassColor[sender->getClass() - 1], sender->GetName(), sender->GetName(), WORLD_CHAT_WHITE, msg);
                                WorldChat[sender->GetGUID().GetCounter()].cooldown = 0;
                            }
                            if (rank == 5)
                            {
                                message = Acore::StringFormat("[{}星团长][{}|Hplayer:{}|h{}|h|r]: {}{}|r", star5icon, world_chat_ClassColor[sender->getClass() - 1], sender->GetName(), sender->GetName(), WORLD_CHAT_WHITE, msg);
                                WorldChat[sender->GetGUID().GetCounter()].cooldown = 0;
                            }
                        }
                        else
                        {
                            message = Acore::StringFormat("[{}][{}|Hplayer:{}|h{}|h|r]: {}{}|r", world_chat_TeamIcon[sender->GetTeamId()], world_chat_ClassColor[sender->getClass() - 1], sender->GetName(), sender->GetName(), WORLD_CHAT_WHITE, msg);
                        }
                    }
                    ChatHandler(target->GetSession()).PSendSysMessage(Acore::StringFormat("{}", message));
                }
            }
        }
    }
}

using namespace Acore::ChatCommands;

class World_Chat : public CommandScript
{
public:
    World_Chat() : CommandScript("World_Chat") {}

    static bool HandleWorldChatCommand(ChatHandler* pChat, const char* msg)
    {

        if (!*msg)
        {
            return false;
        }

        SendWorldMessage(pChat->GetSession()->GetPlayer(), msg, -1);

        return true;
    }

    static bool HandleWorldChatHordeCommand(ChatHandler* pChat, const char* msg)
    {

        if (!*msg)
        {
            return false;
        }

        SendWorldMessage(pChat->GetSession()->GetPlayer(), msg, TEAM_HORDE);

        return true;
    }

    static bool HandleWorldChatAllianceCommand(ChatHandler* pChat, const char* msg)
    {

        if (!*msg)
        {
            return false;
        }

        SendWorldMessage(pChat->GetSession()->GetPlayer(), msg, TEAM_ALLIANCE);

        return true;
    }

    static bool HandleWorldChatOnCommand(ChatHandler* handler, const char* /*msg*/)
    {
        Player* player = handler->GetSession()->GetPlayer();
        uint64  guid   = player->GetGUID().GetCounter();

        if (!WC_Config.Enabled) {
            ChatHandler(player->GetSession()).PSendSysMessage(Acore::StringFormat("[世界] {}World Chat System is disabled.|r", WORLD_CHAT_RED));
            return true;
        }

        if (WorldChat[guid].chat) {
            ChatHandler(player->GetSession()).PSendSysMessage(Acore::StringFormat("[世界] {}World Chat is already visible.|r", WORLD_CHAT_RED));
            return true;
        }

        WorldChat[guid].chat = 1;

        ChatHandler(player->GetSession()).PSendSysMessage(Acore::StringFormat("[世界] {}World Chat is now visible.|r", WORLD_CHAT_GREEN));

        return true;
    };

    static bool HandleWorldChatOffCommand(ChatHandler* handler, const char* /*msg*/)
    {
        Player* player = handler->GetSession()->GetPlayer();
        uint64  guid   = player->GetGUID().GetCounter();

        if (!sConfigMgr->GetOption<bool>("World_Chat.Enable", true))
        {
            ChatHandler(player->GetSession()).PSendSysMessage(Acore::StringFormat("[世界] {}World Chat System is disabled.|r", WORLD_CHAT_RED));
            return true;
        }

        if (!WorldChat[guid].chat)
        {
            ChatHandler(player->GetSession()).PSendSysMessage(Acore::StringFormat("[世界] {}World Chat is already hidden.|r", WORLD_CHAT_RED));
            return true;
        }

        WorldChat[guid].chat = 0;

        ChatHandler(player->GetSession()).PSendSysMessage(Acore::StringFormat("[世界] {}World Chat is now hidden.|r", WORLD_CHAT_GREEN));

        return true;
    };

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable wcCommandTable =
        {
            { "on",      HandleWorldChatOnCommand, SEC_PLAYER, Console::No },
            { "off",     HandleWorldChatOffCommand, SEC_PLAYER, Console::No },
            { "",        HandleWorldChatCommand, SEC_PLAYER, Console::No },
        };

        static ChatCommandTable commandTable =
        {
            { "sj", wcCommandTable },
            { "chath", HandleWorldChatHordeCommand, SEC_MODERATOR, Console::Yes},
            { "chata", HandleWorldChatAllianceCommand, SEC_MODERATOR, Console::Yes},
        };

        return commandTable;

    }
};

class WorldChat_Announce : public PlayerScript
{
public:
    WorldChat_Announce() : PlayerScript("WorldChat_Announce") {}

    void OnLogin(Player* player)
    {
        // Announce Module
        if (WC_Config.Enabled && WC_Config.Announce)
        {
            ChatHandler(player->GetSession()).SendSysMessage(("This server is running the |cff4CFF00WorldChat |rmodule. Use .chat" + ((WC_Config.ChannelName != "") ? " or use /join " + WC_Config.ChannelName : "") + " to communicate"+ ((!WC_Config.CrossFaction) ? " with your faction." : ".")));
        }
    }

    void OnChat(Player* player, uint32 /*type*/, uint32 lang, std::string& msg, Channel* channel)
    {
        if (WC_Config.ChannelName != "" && lang != LANG_ADDON && !strcmp(channel->GetName().c_str(), WC_Config.ChannelName.c_str()))
        {
            SendWorldMessage(player, msg, -1);
            msg = -1;
        }
    }
};

void AddSC_WorldChatScripts()
{
    new WorldChat_Announce();
    new WorldChat_Config();
    new World_Chat();
}
