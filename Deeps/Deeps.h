/**
* Copyright (c) 2011-2014 - Ashita Development Team
*
* Ashita is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Ashita is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Ashita.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __ASHITA_Deeps_H_INCLUDED__
#define __ASHITA_Deeps_H_INCLUDED__

#if defined (_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

enum REACTION
{
	REACTION_NONE = 0x00,
	REACTION_MISS = 0x01,
	REACTION_PARRY = 0x03,
	REACTION_BLOCK = 0x04,
	REACTION_HIT = 0x08,
	REACTION_EVADE = 0x09,
	REACTION_HIT2 = 0x10,
	REACTION_GUARD = 0x14
};

enum SPECEFFECT
{
	SPECEFFECT_NONE = 0x00,
	SPECEFFECT_BLOOD = 0x02,
	SPECEFFECT_HIT = 0x10,
	SPECEFFECT_RAISE = 0x11,
	SPECEFFECT_RECOIL = 0x20,
	SPECEFFECT_CRITICAL_HIT = 0x22
};

/**
 * @brief Required includes for an extension.
 */
#include "..\..\ADK\Ashita.h"
#include <map>
#include <functional>
#include <stdint.h>

struct damage_t
{
	uint64_t total;
	uint32_t max;
	uint32_t min;
	uint32_t count;

	damage_t()
	{
		total = 0;
		max = 0;
		min = 0;
		count = 0;
	}
	bool operator > (const damage_t& o) const
	{
		return (count > o.count);
	}
	uint32_t avg()
	{
		return count > 0 ? (total / count) : 0;
	}
};

struct source_t
{
	std::string name;
	std::map<const char*, damage_t> damage;

	source_t()
	{

	}

	uint64_t total() const
	{
		uint64_t tot = 0;
		for (auto d : damage)
		{
			tot += d.second.total;
		}
		return tot;
	}

	bool operator > (const source_t& o) const
	{
		return (total() > o.total());
	}
};

struct entitysources_t
{
	std::string name;
	std::map<uint32_t, source_t> sources;

	uint64_t total() const
	{
		int64_t total = 0;
		for (auto s : sources)
		{
			total += s.second.total();
		}
		return total;
	}
	bool operator == (const entitysources_t& o) const
	{
		return (total() == o.total());
	}

	bool operator > (const entitysources_t& o) const
	{
		return (total() > o.total());
	}
};

std::map<uint32_t, entitysources_t> entities;
std::map<IFontObject*, std::string> clickMap;

static const uint16_t validMessages[] = { 1, 2, 14, 15, 30, 31, 32, 33, 67, 69, 70, 77, 157, 158, 185, 187, 188, 189, 197, 227, 245, 248, 252, 264, 265,
										  274, 281, 282, 283, 317, 323, 324, 352, 353, 354, 355, 379, 413, 522, 535, 536, 576, 577 };

void g_onClick(int, void*, float, float);

/**
 * @brief Global copy of our plugin data.
 */
PluginData* g_PluginData = NULL;

/**
 * @brief Our Main Plugin Class
 *
 * @note    The main class of your plugin MUST use PluginBase as a base class. This is the
 *          internal base class that Ashita uses to communicate with your plugin!
 */
class Deeps : PluginBase
{
    /**
     * @brief Internal class variables.
     */
    IAshitaCore*        m_AshitaCore;
    DWORD               m_PluginId;
    IDirect3DDevice8*   m_Direct3DDevice;

private:
	source_t* getDamageSource(entitysources_t* entityInfo, uint8_t actionType, uint16_t actionID);
	void updateDamageSource(source_t* source, uint8_t reaction, uint8_t speceffect, uint32_t damage);
    void repairBars(IFontObject* deepsBase, uint8_t size);
	uint16_t getIndex(std::function<bool(IEntity*, int)>);
	uint32_t m_charInfo;
	std::string m_sourceInfo;
    uint8_t m_bars;

public:
    /**
     * @brief Constructor and deconstructor.
     */
	Deeps(void);
	virtual ~Deeps(void);

    /**
     * @brief GetPluginData implementation.
     */
    PluginData GetPluginData(void);

    /**
     * @brief PluginBase virtual overrides.
     */
    bool Initialize(IAshitaCore* ashitaCore, DWORD dwPluginId);
    void Release(void);
    bool HandleCommand(const char* pszCommand, int nCommandType);
    bool HandleNewChatLine(short sMode, char* pszChatLine);
    bool HandleIncomingPacket(unsigned int uiPacketId, unsigned int uiPacketSize, void* lpRawData);
    bool HandleOutgoingPacket(unsigned int uiPacketId, unsigned int uiPacketSize, void* lpRawData);
    bool Direct3DInitialize(IDirect3DDevice8* lpDevice);
    void Direct3DRelease(void);
    void Direct3DPreRender(void);
    void Direct3DRender(void);
    void onClick(int, IFontObject*, float, float);
};

// Global pointer to this

Deeps* g_Deeps = NULL;

/**
 * @brief Required Plugin Exports
 */
__declspec(dllexport) double     __stdcall GetInterfaceVersion(void);
__declspec(dllexport) void       __stdcall CreatePluginData(PluginData* lpBuffer);
__declspec(dllexport) IPlugin*   __stdcall CreatePlugin(char* pszUnused);

#endif // __ASHITA_Deeps_H_INCLUDED__
