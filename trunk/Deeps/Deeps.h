/*
===========================================================================

  Copyright (c) 2012-2013 'kjLotus'

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see http://www.gnu.org/licenses/

  This file is part of Ashita-Deeps extension code.

===========================================================================
*/

#ifndef _DEEPS
#define _DEEPS

#include "..\..\..\SDK\Depends\Common\Extension.h"
#include "..\..\..\SDK\Depends\Include\d3d8.h"
#include "..\..\..\SDK\Depends\Include\d3dx8tex.h"
#include "CommandParser.h"
#include <stdint.h>
#include <map>
#include <set>
#include <array>

#define RBUFP(p,pos) (((uint8_t*)(p)) + (pos))
#define RBUFB(p,pos) (*(uint8_t*)RBUFP((p),(pos)))
#define RBUFW(p,pos) (*(uint16_t*)RBUFP((p),(pos)))
#define RBUFL(p,pos) (*(uint32_t*)RBUFP((p),(pos)))
#define RBUFF(p,pos) (*(float*)RBUFP((p),(pos)))

#define WBUFP(p,pos) (((uint8_t*)(p)) + (pos))
#define WBUFB(p,pos) (*(uint8_t*)WBUFP((p),(pos)))
#define WBUFW(p,pos) (*(uint16_t*)WBUFP((p),(pos)))
#define WBUFL(p,pos) (*(uint32_t*)WBUFP((p),(pos)))
#define WBUFF(p,pos) (*(float*)WBUFP((p),(pos)))

#define OUTPUT_FILE "deeps.log"
#define MAX_PATH 260

enum REACTION
{
        REACTION_NONE                   = 0x00,
        REACTION_MISS                   = 0x01,
        REACTION_PARRY                  = 0x03,
        REACTION_BLOCK                  = 0x04,
        REACTION_HIT                    = 0x08,
        REACTION_EVADE                  = 0x09,
        REACTION_GUARD                  = 0x14
};

enum SPECEFFECT
{
        SPECEFFECT_NONE                 = 0x00,
        SPECEFFECT_BLOOD                = 0x02,
        SPECEFFECT_HIT                  = 0x10,
        SPECEFFECT_RAISE                = 0x11,
        SPECEFFECT_RECOIL               = 0x20,
        SPECEFFECT_CRITICAL_HIT			= 0x22
};

static uint16_t validMessages[] = {1, 2, 14, 15, 30, 31, 32, 33, 67, 69, 70, 77, 157, 158, 185, 187, 188, 189, 197, 227, 245, 248, 252, 264, 265, 
		274, 281, 282, 283, 317, 323, 324, 352, 353, 354, 355, 379, 413, 522, 535, 536, 576, 577};

struct party_deeps_t
{

    uint32_t id;
	uint64_t total;
	uint32_t battletime;
	time_t lastactiontime;

	party_deeps_t()
	{
        id = 0;
		total = 0;
		battletime = 1;
		lastactiontime = 0;
	}
};

struct damage_t
{
	uint64_t total;
	uint32_t min;
	uint32_t max;
	uint32_t count;

	damage_t()
	{
        total = 0;
		min = 0;
		max = 0;
		count = 0;
	}
};

struct damage_type_t
{
	std::string name;
	damage_t hit;
	damage_t miss;
	damage_t crit;
	damage_t block;
	damage_t guard;
	damage_t parry;
	damage_t evade;
};

struct cmp
{
	bool operator()(const std::pair<uint32_t*, damage_type_t*> a, const std::pair<uint32_t*, damage_type_t*> b)
	{
		return (a.second->hit.total + a.second->crit.total + a.second->block.total + a.second->guard.total) < 
			(b.second->hit.total + b.second->crit.total + b.second->block.total + b.second->guard.total);
	}
};

typedef std::map<uint32_t, damage_type_t> damagesourcemap;

struct entity_info_t
{
	std::string name;
	damagesourcemap sources;
	std::set<std::pair<uint32_t*, damage_type_t*>, cmp> sorter;
	//party_deeps_t dps;
};

typedef std::map<uint32_t, entity_info_t> entityinfomap;

class Deeps : public ExtensionBase
{
private:
	bool getPartyMemberFromID(uint32_t id, uint8_t* index);
	damage_type_t* getDamageSource(entity_info_t* entityInfo, uint8_t actionType, uint16_t actionID);
	void updateDamageSource(damage_type_t* source, uint8_t reaction, uint8_t speceffect, uint32_t damage);

	IAshitaCore* m_AshitaCore;
	uint8_t m_Counter;
	CommandParser* cmdParse;
	ITextObject* m_Meters[18];
	party_deeps_t* m_Deeps;
	std::vector<uint32_t> m_EntityList;
	entityinfomap damageInfo;
	uint16_t* validMessages;
public:
	Deeps()
	{}
	~Deeps()
	{}
	int __stdcall Load(IAshitaCore* mAshitaCore, DWORD ExtensionID);

	void __stdcall Unload();

	ExtensionInterfaceData __stdcall GetExtensionData();

	bool __stdcall HandleCommand(const char* szCommand, int iType);
	
	bool __stdcall HandleIncomingPacket(unsigned int uiSize, void* pData);

	bool __stdcall DxSetup(IDirect3DDevice8* mDevice);

	void __stdcall DxRender();
};

//Exports
__declspec( dllexport ) void __stdcall CreateExtensionData( ExtensionInterfaceData* lpExtensionData );
__declspec( dllexport ) IExtension* __stdcall CreateExtension( char* pszText );


#endif