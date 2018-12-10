/*

# Money For Kills #

### Description ###
------------------------------------------------------------------------------------------------------------------
- Pays players bounty money for kills of players and creatures.
- Bounty and other amounts can be changed in the config file.
- Bounty can be paid to only the player with killing blow or all players.
- Bounty can be paid to players that are near or far away from the group.
- Dungeon boss kills are announced to the party.
- World boss kills are announced to the world.
- Player suicides are announced to the world.


### Data ###
------------------------------------------------------------------------------------------------------------------
- Type: Server/Player
- Script: MoneyForKills
- Config: Yes
- SQL: No


### Version ###
------------------------------------------------------------------------------------------------------------------
- v2018.12.09 - Updated config
- v2018.12.01 - Added Low Level MOB bounty option, Prevent low PVP payouts
- v2017.09.22 - Added PVPCorpseLoot as a config option
- v2017.09.02 - Added distance check, Fixed group payment
- v2017.08.31 - Added boss kills
- v2017.08.24


### Credits ###
------------------------------------------------------------------------------------------------------------------
#### A module for AzerothCore by StygianTheBest ([stygianthebest.github.io](http://stygianthebest.github.io)) ####

###### Additional Credits include:
- [Blizzard Entertainment](http://blizzard.com)
- [TrinityCore](https://github.com/TrinityCore/TrinityCore/blob/3.3.5/THANKS)
- [SunwellCore](http://www.azerothcore.org/pages/sunwell.pl/)
- [AzerothCore](https://github.com/AzerothCore/azerothcore-wotlk/graphs/contributors)
- [AzerothCore Discord](https://discord.gg/gkt4y2x)
- [EMUDevs](https://youtube.com/user/EmuDevs)
- [AC-Web](http://ac-web.org/)
- [ModCraft.io](http://modcraft.io/)
- [OwnedCore](http://ownedcore.com/)
- [OregonCore](https://wiki.oregon-core.net/)
- [Wowhead.com](http://wowhead.com)
- [AoWoW](https://wotlk.evowow.com/)


### License ###
------------------------------------------------------------------------------------------------------------------
- This code and content is released under the [GNU AGPL v3](https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3).

*/

#include "Config.h"
#include "Group.h"

bool MFKEnable = true;
bool MFKAnnounceModule = true;
bool MFKKillingBlowOnly = false;
bool MFKMoneyForNothing = false;
bool MFKLowLevelBounty = false;
uint32 MFKPVPCorpseLootPercent = 5;
uint32 MFKKillMultiplier = 100;
uint32 MFKPVPMultiplier = 200;
uint32 MFKDungeonBossMultiplier = 10;
uint32 MFKWorldBossMultiplier = 15;

class MFKConfig : public WorldScript
{
public:
    MFKConfig() : WorldScript("MFKConfig") { }

    void OnBeforeConfigLoad(bool reload) override
    {
        if (!reload) {
            std::string conf_path = _CONF_DIR;
            std::string cfg_file = conf_path + "/mod_moneyforkills.conf";
#ifdef WIN32
            cfg_file = "mod_moneyforkills.conf";
#endif
            std::string cfg_def_file = cfg_file + ".dist";
            sConfigMgr->LoadMore(cfg_def_file.c_str());
            sConfigMgr->LoadMore(cfg_file.c_str());
        }
        // Load Configuration Settings
        SetInitialWorldSettings();
    }

    // Load Configuration Settings
    void SetInitialWorldSettings()
    {
        MFKEnable = sConfigMgr->GetBoolDefault("MFK.Enable", true);
        MFKAnnounceModule = sConfigMgr->GetBoolDefault("MFK.Announce", true);
        MFKKillingBlowOnly = sConfigMgr->GetBoolDefault("MFK.KillingBlowOnly", false);
        MFKMoneyForNothing = sConfigMgr->GetBoolDefault("MFK.MoneyForNothing", false);
        MFKLowLevelBounty = sConfigMgr->GetBoolDefault("MFK.LowLevelBounty", false);
        MFKPVPCorpseLootPercent = sConfigMgr->GetIntDefault("MFK.PVP.CorpseLootPercent", 5);
        MFKKillMultiplier = sConfigMgr->GetIntDefault("MFK.Kill.Multiplier", 100);
        MFKPVPMultiplier = sConfigMgr->GetIntDefault("MFK.PVP.Multiplier", 200);
        MFKDungeonBossMultiplier = sConfigMgr->GetIntDefault("MFK.DungeonBoss.Multiplier", 25);
        MFKWorldBossMultiplier = sConfigMgr->GetIntDefault("MFK.WorldBoss.Multiplier", 100);
    }
};

class MFKAnnounce : public PlayerScript
{

public:
    MFKAnnounce() : PlayerScript("MFKAnnounce") {}

    void OnLogin(Player* player)
    {
        // Announce Module
        if (MFKEnable)
        {
            if (MFKAnnounceModule)
            {
                ChatHandler(player->GetSession()).SendSysMessage("This server is running the |cff4CFF00MoneyForKills |rmodule.");
            }
        }
    }
};

class MoneyForKills : public PlayerScript
{
public:
    MoneyForKills() : PlayerScript("MoneyForKills") { }

    // Player Kill Reward
    void OnPVPKill(Player* player, Player* victim)
    {
        // If enabled...
        if (MFKEnable)
        {
            // If enabled...
            if (MFKPVPMultiplier > 0)
            {
                std::ostringstream ss;

                // No reward for killing yourself
                if (player->GetGUID() == victim->GetGUID())
                {
                    // Inform the world
                    ss << "|cff676767[ |cffFFFF00World |cff676767]|r:|cff4CFF00 " << player->GetName() << " met an untimely demise!";
                    sWorld->SendServerMessage(SERVER_MSG_STRING, ss.str().c_str());
                    return;
                }

                // Don't let low level players game the system
                if (player->getLevel() < 30)
                {
                    ss << "|cff676767[ |cffFFFF00World |cff676767]|r:|cff4CFF00 " << player->GetName() << " was slaughtered mercilessly!";
                    sWorld->SendServerMessage(SERVER_MSG_STRING, ss.str().c_str());
                    return;
                }

                // Calculate the amount of gold to give to the victor
                const uint32 VictimLevel = victim->getLevel();
                const int VictimLoot = (victim->GetMoney() * MFKPVPCorpseLootPercent) / 100;    // Configured % of victim's loot
                const int BountyAmount = ((VictimLevel * MFKPVPMultiplier) / 3);                // Modifier

                // Rifle the victim's corpse for loot
                if (victim->GetMoney() >= 10000)
                {
                    // Player loots 5% of the victim's gold
                    player->ModifyMoney(VictimLoot);
                    victim->ModifyMoney(-VictimLoot);

                    // Inform the player of the corpse loot
                    Notify(player, victim, NULL, "Loot", NULL, VictimLoot);

                    // Pay the player the additional PVP bounty
                    player->ModifyMoney(BountyAmount);
                }
                else
                {
                    // Pay the player the additional PVP bounty
                    player->ModifyMoney(BountyAmount);
                }

                // Inform the player of the bounty amount
                Notify(player, victim, NULL, "PVP", BountyAmount, VictimLoot);

                //return;
            }
        }
    }

    // Creature Kill Reward
    void OnCreatureKill(Player* player, Creature* killed)
    {
        std::ostringstream ss;

        // No reward for killing yourself
        if (player->GetGUID() == killed->GetGUID())
        {
            // Inform the world
            ss << "|cff676767[ |cffFFFF00World |cff676767]|r:|cff4CFF00 " << player->GetName() << " met an untimely demise!";
            sWorld->SendServerMessage(SERVER_MSG_STRING, ss.str().c_str());
            return;
        }

        // If enabled...
        if (MFKEnable)
        {
            // Get the creature level
            const uint32 CreatureLevel = killed->getLevel();
            const uint32 CharacterLevel = player->getLevel();

            // What did the player kill?
            if (killed->IsDungeonBoss() || killed->isWorldBoss())
            {
                uint32 BossMultiplier;

                // Dungeon Boss or World Boss multiplier?
                if (killed->IsDungeonBoss())
                {
                    BossMultiplier = MFKDungeonBossMultiplier;
                }
                else
                {
                    BossMultiplier = MFKWorldBossMultiplier;
                }

                // If enabled...
                if (BossMultiplier > 0)
                {
                    // Reward based on creature level
                    const int BountyAmount = ((CreatureLevel * BossMultiplier) * 100);

                    if (killed->IsDungeonBoss())
                    {
                        // Pay the bounty amount
                        CreatureBounty(player, killed, "DungeonBoss", BountyAmount);
                    }
                    else
                    {
                        // Pay the bounty amount
                        CreatureBounty(player, killed, "WorldBoss", BountyAmount);
                    }
                }
            }
            else
            {
                // If enabled...
                if (MFKKillMultiplier > 0)
                {
                    // Reward based on creature level
                    int BountyAmount = ((CreatureLevel * MFKKillMultiplier) / 3);

                    // Is the character higher level than the mob?
                    if (CharacterLevel > CreatureLevel)
                    {
                        // If Low Level Bounty not enabled zero the bounty
                        if (!MFKLowLevelBounty)
                        {
                            BountyAmount = 0;
                        }
                    }

                    // Pay the bounty amount
                    CreatureBounty(player, killed, "MOB", BountyAmount);
                }
            }
        }
    }

    // Pay Creature Bounty
    void CreatureBounty(Player* player, Creature* killed, string KillType, int bounty)
    {
        Group* group = player->GetGroup();
        Group::MemberSlotList const &members = group->GetMemberSlots();

        if (MFKEnable)
        {
            // If we actually have a bounty..
            if (bounty >= 1)
            {
                // Determine who receives the bounty
                if (!group || MFKKillingBlowOnly == 1)
                {
                    // Pay a specific player bounty amount
                    player->ModifyMoney(bounty);

                    // Inform the player of the bounty amount
                    Notify(player, NULL, killed, KillType, bounty, NULL);
                }
                else
                {
                    // Pay the group (OnCreatureKill only rewards the player that got the killing blow)
                    for (Group::MemberSlotList::const_iterator itr = members.begin(); itr != members.end(); ++itr)
                    {
                        Group::MemberSlot const &slot = *itr;
                        Player* playerInGroup = ObjectAccessor::FindPlayer((*itr).guid);

                        // Pay each player in the group
                        if (playerInGroup && playerInGroup->GetSession())
                        {
                            // Money for nothing and the kills for free..
                            if (MFKMoneyForNothing == 1)
                            {
                                // Pay the bounty
                                playerInGroup->ModifyMoney(bounty);

                                // Inform the player of the bounty amount
                                Notify(playerInGroup, NULL, killed, KillType, bounty, NULL);
                            }
                            else
                            {
                                // Only pay players that are in reward distance	
                                if (playerInGroup->IsAtGroupRewardDistance(killed))
                                {
                                    // Pay the bounty
                                    playerInGroup->ModifyMoney(bounty);

                                    // Inform the player of the bounty amount
                                    Notify(playerInGroup, NULL, killed, KillType, bounty, NULL);
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                //return;
            }
        }
    }

    // Payment/Kill Notification
    void Notify(Player* player, Player* victim, Creature* killed, string KillType, int bounty, int loot)
    {
        std::ostringstream ss;
        std::ostringstream sv;
        int result[3];

        if (MFKEnable)
        {
            // Determine type of kill
            if (KillType == "Loot")
            {
                const int copper = loot % 100;
                loot = (loot - copper) / 100;
                const int silver = loot % 100;
                const int gold = (loot - silver) / 100;
                result[0] = copper;
                result[1] = silver;
                result[2] = gold;
            }
            else
            {
                const int copper = bounty % 100;
                bounty = (bounty - copper) / 100;
                const int silver = bounty % 100;
                const int gold = (bounty - silver) / 100;
                result[0] = copper;
                result[1] = silver;
                result[2] = gold;
            }

            // Payment notification
            if (KillType == "Loot")
            {
                ss << "You loot ";
                sv << player->GetName() << " rifles through your corpse and takes ";
            }
            else if (KillType == "PVP")
            {
                ss << "|cff676767[ |cffFFFF00World |cff676767]|r:|cff4CFF00 " << player->GetName() << " |cffFF0000has slain " << victim->GetName() << " earning a bounty of ";
            }
            else
            {
                ss << "You receive a bounty of ";
            }

            // Figure out the money (todo: find a better way to handle the different strings)
            if (result[2] > 0)
            {
                ss << result[2] << " gold";
                sv << result[2] << " gold";
            }
            if (result[1] > 0)
            {
                if (result[2] > 0)
                {
                    ss << " " << result[1] << " silver";
                    sv << " " << result[1] << " silver";
                }
                else
                {
                    ss << result[1] << " silver";
                    sv << result[1] << " silver";

                }
            }
            if (result[0] > 0)
            {
                if (result[1] > 0)
                {
                    ss << " " << result[0] << " copper";
                    sv << " " << result[0] << " copper";
                }
                else
                {
                    ss << result[0] << " copper";
                    sv << result[0] << " copper";
                }
            }

            // Type of kill
            if (KillType == "Loot")
            {
                ss << " from the corpse.";
                sv << ".";
            }
            else if (KillType == "PVP")
            {
                ss << ".";
                sv << ".";
            }
            else
            {
                ss << " for the kill.";
            }

            // If it's a boss kill..
            if (KillType == "WorldBoss")
            {
                // Inform the world of the kill
                std::ostringstream sw;
                sw << "|cffFF0000[cffFFFF00World |cffFF0000]|r:|cff4CFF00 " << player->GetName() << "'s|r group triumphed victoriously over |CFF18BE00[" << killed->GetName() << "]|r !";
                sWorld->SendServerMessage(SERVER_MSG_STRING, sw.str().c_str());

                // Inform the player of the bounty
                ChatHandler(player->GetSession()).SendSysMessage(ss.str().c_str());
            }
            else if (KillType == "Loot")
            {
                // Inform the player of the corpse loot
                ChatHandler(player->GetSession()).SendSysMessage(ss.str().c_str());

                // Inform the victim of the corpse loot
                ChatHandler(victim->GetSession()).SendSysMessage(sv.str().c_str());
            }
            else if (KillType == "PVP")
            {
                // Inform the world of the kill
                sWorld->SendServerMessage(SERVER_MSG_STRING, ss.str().c_str());
            }
            else
            {
                if (KillType == "DungeonBoss")
                {
                    // Inform the player of the Dungeon Boss kill
                    std::ostringstream sb;
                    sb << "|cffFF8000Your party has defeated |cffFF0000" << killed->GetName() << "|cffFF8000.";
                    ChatHandler(player->GetSession()).SendSysMessage(sb.str().c_str());
                }

                // Inform the player of the bounty
                ChatHandler(player->GetSession()).SendSysMessage(ss.str().c_str());
            }
        }
    }
};

void AddMoneyForKillsScripts()
{
    new MFKConfig();
    new MFKAnnounce();
    new MoneyForKills();
}
