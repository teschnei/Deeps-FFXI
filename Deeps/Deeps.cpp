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

#include "Deeps.h"
#include "..\..\..\SDK\Depends\Common\Extension.h"

#include <time.h>
#include <windows.h>
#include <stdio.h>
#include <algorithm>

//do not delete this. Ashita will clean it up.
ExtensionInterfaceData* ModuleData;


bool Deeps::getPartyMemberFromID(uint32_t id, uint8_t* index)
{
	IParty* party = m_AshitaCore->GetDataModule()->GetParty();
	for(*index = 0; *index < 18; (*index)++)
	{
		if(party->Member[*index].ID == id)
			return true;
	}
	return false;
}

int __stdcall Deeps::Load(IAshitaCore* mAshitaCore, DWORD ExtensionID)
{
	m_AshitaCore = mAshitaCore;
	cmdParse = new CommandParser;

	m_Deeps = new party_deeps_t[18];
	IParty* party = m_AshitaCore->GetDataModule()->GetParty();
	for (int i = 0; i < 18; i++)
	{
		char Meters_String[256] = "";
		sprintf_s(Meters_String, 256, "_Meters[%d]", i);
		m_Meters[i] = m_AshitaCore->GetDirectxModule()->GetTextObjectManager()->CreateTextObject(Meters_String);
		m_Meters[i]->SetFont("Courier",8);
		m_Meters[i]->SetLocation((float)(m_AshitaCore->GetGameSettings()->ResolutionX - 300), (float)(m_AshitaCore->GetGameSettings()->ResolutionY - (100 + i*30)));
		m_Meters[i]->SetBGColor(255,0,0,0);
		m_Meters[i]->SetBGVisibility(true);
		m_Meters[i]->SetColor(255, 255, 255, 255);
		m_Meters[i]->SetBold(true);
		if (party->Member[i].ID == 0)
		{
			m_Meters[i]->SetVisibility(false);
		}
	}
	return 1;
}

void __stdcall Deeps::Unload()
{
	delete cmdParse;
	for (int i = 0; i < 18; i++)
	{
		char Meters_String[256] = "";
		sprintf_s(Meters_String, 256, "_Meters[%d]", i);
		m_AshitaCore->GetDirectxModule()->GetTextObjectManager()->DeleteTextObject(Meters_String);
	}
	delete m_Deeps;
}

ExtensionInterfaceData __stdcall Deeps::GetExtensionData()
{
	return *ModuleData;
}

bool __stdcall Deeps::HandleCommand(const char* szCommand, int iType)
{
	//ignore none typed commands
	/*if(iType == 0)
		return false;

	char szClean[1024];
	memset(szClean,0,1024);

	m_AshitaCore->GetDataModule()->ParseAutoTrans(szCommand,szClean,1024,false);
	
	cmdParse->InputCommand(szClean);
	std::string arg;
	cmdParse->GetFirstCommand(&arg);

	//Match the command.
	if(arg == "/deeps")
	{
		if(cmdParse->GetNextCommand(&arg));
		{
			if(arg == "lock")
			{
				locked = true;
				m_AshitaCore->GetDataModule()->AddChatLine(5,"Other character appearances: Locked");
				return true;
			}
			else if(arg == "unlock")
			{
				locked = false;
				m_AshitaCore->GetDataModule()->AddChatLine(5,"Other character appearance: Unlocked");
				return true;
			}
			else if(arg == "selflock")
			{
				selflock = true;
				m_AshitaCore->GetDataModule()->AddChatLine(5,"Own Appearance: Locked");
				return true;
			}
			else if(arg == "selfunlock")
			{
				selflock = false;
				m_AshitaCore->GetDataModule()->AddChatLine(5,"Own Appearance: Unlocked");
				return true;
			}
			else if(arg == "invislock")
			{
				invisflag = true;
				m_AshitaCore->GetDataModule()->AddChatLine(5,"Invisibility Flag: Locked");
				return true;
			}
			else if(arg == "invisunlock")
			{
				invisflag = false;
				m_AshitaCore->GetDataModule()->AddChatLine(5,"Invisibility Flag: Unlocked");
				return true;
			}
			else if(arg == "reset")
			{
				locked = false;
				selflock = false;
				invisflag = false;
				m_AshitaCore->GetDataModule()->AddChatLine(5,"All locks disabled");
				return true;
			}
		}
	}*/
	return false;
}

	
bool __stdcall Deeps::HandleIncomingPacket(unsigned int uiSize, void* pData)
{
	uint16_t packetType = RBUFW(pData,0) & 0x00FF;

	IResources* resources = m_AshitaCore->GetResources();

	if(packetType == 0x28)
	{
		IDataTools* dataTools = m_AshitaCore->GetDataModule()->GetDataTools();
		uint8_t actionNum = (uint8_t)(dataTools->unpackBitsBE((unsigned char*)pData, 182, 4));
		uint8_t targetNum = RBUFB(pData, 0x09);
		uint8_t actionType = (uint8_t)(dataTools->unpackBitsBE((unsigned char*)pData, 82, 4));
		uint16_t actionID = (uint16_t)(dataTools->unpackBitsBE((unsigned char*)pData, 86, 10));
		uint32_t userID = RBUFL(pData, 0x05);
		uint16_t startBit = 150;
		uint16_t damage = 0;
		if ((actionType >= 1 && actionType <= 4) || (actionType == 6) || (actionType == 11) || (actionType == 14))
		{
			if (targetNum == 1 && actionNum > 0) //single target spells, auto attack rounds
			{
				uint32_t targetID = (uint32_t)(dataTools->unpackBitsBE((unsigned char*)pData, 118, 32));
				for (int i=0; i<actionNum; i++)
				{
					damage += (uint16_t)(dataTools->unpackBitsBE((unsigned char*)pData, startBit + 63, 17));
			
					if ((dataTools->unpackBitsBE((unsigned char*)pData, startBit + 121, 2) & 0x1) && actionType != 6)
					{
						damage += (uint16_t)(dataTools->unpackBitsBE((unsigned char*)pData, startBit + 132, 16));
						startBit += 37;
					}
					startBit += 87;
				}
			} 
			else if (targetNum > 1 && actionNum > 0) //aoe
			{
				for (int i=0; i<targetNum; i++)
				{
					damage += (uint16_t)(dataTools->unpackBitsBE((unsigned char*)pData, startBit + 63, 17));
			
					if (dataTools->unpackBitsBE((unsigned char*)pData, startBit + 121, 2) & 0x1)
					{
						damage += (uint16_t)(dataTools->unpackBitsBE((unsigned char*)pData, startBit + 132, 16));
						startBit += 37;
					}
					startBit += 123;
				}
			}
			uint8_t index = 0;
			if (getPartyMemberFromID(userID, &index))
			{
				m_Deeps[index].total += damage;
			}
		}
	} 
	else if (packetType == 0x0E) 
	{
		uint32_t mobId = RBUFL(pData,(0x04));
		uint32_t claimId = RBUFL(pData,(0x2C));
		uint8_t hpp = RBUFB(pData,(0x1E));
		uint8_t animation = RBUFB(pData,(0x1F));
		uint8_t index = 0;

		if (claimId != 0 && getPartyMemberFromID(claimId, &index) && std::find(m_EntityList.begin(), m_EntityList.end(), mobId) == m_EntityList.end() && hpp > 0)
		{
			if( m_EntityList.size() == 0)
			{
				time_t currentTime = time(NULL);
				for( int i = 0; i < 18; ++i)
				{
					m_Deeps[i].lastactiontime = currentTime;
				}
			}
			m_EntityList.push_back(mobId);
		}
		else
		{
			std::vector<uint32_t>::iterator it = std::find(m_EntityList.begin(), m_EntityList.end(), mobId);
			if (it != m_EntityList.end())
			{
				if (hpp == 0 || animation == 0)
				{
					m_EntityList.erase(it);
				}
			}
		}
	}
	return false;
}

bool __stdcall Deeps::DxSetup(IDirect3DDevice8* mDevice)
{
	return true;
}

void __stdcall Deeps::DxRender()
{
	bool updateTime = false;
	if (m_Counter >= 30)
	{
		for(std::vector<uint32_t>::iterator it = m_EntityList.begin(); it != m_EntityList.end(); )
		{
			Entity* en = m_AshitaCore->GetDataModule()->GetEntityList()->Find(*it);
			if(en == NULL || en->Distance > 2500 || en->HPP == 0 )
			{
				it = m_EntityList.erase(it);
			}
			else
			{
				++it;
			}
		}
		if(m_EntityList.size() > 0)
		{
			updateTime = true;
		}
		m_Counter = 0;
	}
	for (int i = 0; i < 18; ++i)
	{
		if (updateTime)
		{
			time_t currentTime = time(NULL);
			if (currentTime > m_Deeps[i].lastactiontime)
			{
				m_Deeps[i].battletime += (uint32_t)difftime(currentTime, m_Deeps[i].lastactiontime);
			}
			m_Deeps[i].lastactiontime = currentTime;
		}
		char Meters_String[256] = "";
		sprintf_s(Meters_String, 256, "Damage: %llu\nDPS: %.1f", m_Deeps[i].total, m_Deeps[i].total / (float)m_Deeps[i].battletime);
		m_Meters[i]->SetText(Meters_String);
	}
	m_Counter++;
}

__declspec( dllexport ) void __stdcall CreateExtensionData( ExtensionInterfaceData* Data )
{
	ModuleData = Data;
	ModuleData->ExtensionVersion		=		1.00;
	ModuleData->InterfaceVersion		=		INTERFACEVERSION;
	ModuleData->RequiresValadation		=		false;
	ModuleData->AutoloadConfiguration	=		false;
	ModuleData->AutoloadLanguage		=		false;

	strncpy_s( ModuleData->Name, "Deeps", 256 );
	strncpy_s( ModuleData->Author, "kjLotus", 256 );
}

__declspec( dllexport ) IExtension* __stdcall CreateExtension( char* pszText )
{
	return (IExtension*)new Deeps();
}
