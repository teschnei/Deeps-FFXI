/*
===========================================================================

  Copyright (c) 2012-2013 Tyler Schneider aka 'kjLotus'

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

#include "..\..\..\SDK\Depends\Common\Extension.h"
#include "..\..\..\SDK\Depends\Include\d3d8.h"
#include "..\..\..\SDK\Depends\Include\d3dx8tex.h"
#include "CommandParser.h"
#include <stdint.h>

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

struct party_deeps_t {
	uint64_t total;
	uint32_t battletime;
	time_t lastactiontime;

	party_deeps_t()
	{
		total = 0;
		battletime = 1;
		lastactiontime = 0;
	}
};

class Deeps : public ExtensionBase
{
private:
	IAshitaCore* m_AshitaCore;
	uint8_t m_Counter;
	CommandParser* cmdParse;
	ITextObject* m_Meters[18];
	party_deeps_t* m_Deeps;
	bool getPartyMemberFromID(uint32_t id, uint8_t* index);
	std::vector<uint32_t> m_EntityList;
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
