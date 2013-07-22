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

#include "Deeps.h"
#include "..\..\..\SDK\Depends\Common\Extension.h"

#include <time.h>
#include <windows.h>
#include <stdio.h>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>

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

damage_type_t* Deeps::getDamageSource(entity_info_t* entityInfo, uint8_t actionType, uint16_t actionID)
{
	uint32_t key = (actionID << 8) + actionType;
	std::map<uint32_t, damage_type_t>::iterator sourcesIt = entityInfo->sources.find(key);

	damage_type_t* source;

	if (sourcesIt != entityInfo->sources.end())
	{
		source = &sourcesIt->second;
	}
	else
	{
        damage_type_t newsource;

        sourcesIt = entityInfo->sources.insert(std::make_pair(key, newsource)).first;
		//entityInfo.sorter.insert(std::make_pair(&(sourcesIt->first), &(sourcesIt->second)));

        source = &sourcesIt->second;
		if (actionType == 1) { source->name.append("Auto Attack");}
		else if (actionType == 2) { source->name.append("Ranged Attack");}
		else if (actionType == 3 || actionType == 11)
		{
			//TODO: Real WS Name (Ashita Resources)
			char name[20] = "";
			sprintf_s(name, 20, "WS: %d", actionID);
			source->name.append(name);
		}
		else if (actionType == 4) { source->name.append(m_AshitaCore->GetResources()->GetSpellByID(actionID)->Name); }
		else if (actionType == 6 || actionType == 14 || actionType == 15) { source->name.append(m_AshitaCore->GetResources()->GetAbilityByID(actionID)->Name); }

	}
	return source;
}

void Deeps::updateDamageSource(damage_type_t* source, uint8_t reaction, uint8_t speceffect, uint32_t damage)
{
	if (reaction == REACTION_MISS)
	{
		source->miss.total += damage;
		source->miss.count++;
		source->miss.min = (damage < source->miss.min ? damage : source->miss.min);
		source->miss.max = (damage > source->miss.min ? damage : source->miss.min);
	}
	else if (reaction == REACTION_PARRY)
	{
		source->parry.total += damage;
		source->parry.count++;
		source->parry.min = (damage < source->parry.min ? damage : source->parry.min);
		source->parry.max = (damage > source->parry.min ? damage : source->parry.min);
	}
	else if (reaction == REACTION_BLOCK)
	{
		source->block.total += damage;
		source->block.count++;
		source->block.min = (damage < source->block.min ? damage : source->block.min);
		source->block.max = (damage > source->block.min ? damage : source->block.min);
	}
	else if (reaction == REACTION_HIT)
	{
		if (speceffect == SPECEFFECT_CRITICAL_HIT)
		{
			source->crit.total += damage;
			source->crit.count++;
			source->crit.min = (damage < source->crit.min ? damage : source->crit.min);
			source->crit.max = (damage > source->crit.min ? damage : source->crit.min);
		}
		else
		{
			source->hit.total += damage;
			source->hit.count++;
			source->hit.min = (damage < source->hit.min ? damage : source->hit.min);
			source->hit.max = (damage > source->hit.min ? damage : source->hit.min);
		}
	}
	else if (reaction == REACTION_GUARD)
	{
		source->guard.total += damage;
		source->guard.count++;
		source->guard.min = (damage < source->guard.min ? damage : source->guard.min);
		source->guard.max = (damage > source->guard.min ? damage : source->guard.min);
	}
	else if (reaction == REACTION_EVADE)
	{
		source->evade.total += damage;
		source->evade.count++;
		source->evade.min = (damage < source->evade.min ? damage : source->evade.min);
		source->evade.max = (damage > source->evade.min ? damage : source->evade.min);
	}
}

bool validMessage(uint16_t messageID)
{
	for (int i = 0; i < ARRAYSIZE(validMessages); i++)
	{
		if (validMessages[i] == messageID)
			return true;
	}
	return false;
}

void printSourceLine(damage_t* source, std::ofstream& logFile, char* header)
{
    if (source->count != 0)
    {
        logFile << std::setw(7) << header << std::setw(12) << source->total << std::setw(12) << source->total / source->count
                << std::setw(12) << source->min << std::setw(12) << source->max << std::setw(12) << source->count << std::endl;
    }
}

int __stdcall Deeps::Load(IAshitaCore* mAshitaCore, DWORD ExtensionID)
{
	m_AshitaCore = mAshitaCore;
	cmdParse = new CommandParser;

	m_Deeps = new party_deeps_t[18];

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
	if(iType == 0)
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
		if(cmdParse->GetNextCommand(&arg))
		{
			if(arg == "reset")
			{
				memset(m_Deeps, 0, sizeof(m_Deeps));
				m_AshitaCore->GetDataModule()->AddChatLine(5,"Deeps: Meters reset.");
				return true;
			}
			else if(arg == "flush")
			{
				std::ofstream logFile;
                char logPath[MAX_PATH];
                sprintf_s(logPath, MAX_PATH, "%S\\Extensions\\%s", m_AshitaCore->AshitaInstallPath, OUTPUT_FILE);
				logFile.open(logPath, std::ios::out | std::ios::trunc);

				if (logFile.is_open())
				{
					entityinfomap::iterator entityIt = damageInfo.begin();

					while (entityIt != damageInfo.end() && logFile.good())
					{
						logFile << entityIt->second.name << "\n\n";

						uint64_t totalDamage = 0;
						damagesourcemap sources = entityIt->second.sources;
						damagesourcemap::iterator sourcesIt = sources.begin();
						while (sourcesIt != sources.end())
						{
							totalDamage += sourcesIt->second.block.total;
							totalDamage += sourcesIt->second.hit.total;
							totalDamage += sourcesIt->second.crit.total;
							totalDamage += sourcesIt->second.guard.total;
							sourcesIt++;
						}

						sourcesIt = sources.begin();
						while (sourcesIt != sources.end() && logFile.good())
						{
							uint64_t sourceDamage = 0;
							damage_type_t source = sourcesIt->second;

							sourceDamage += source.block.total;
							sourceDamage += source.hit.total;
							sourceDamage += source.crit.total;
							sourceDamage += source.guard.total;

							logFile << source.name << " - " << sourceDamage << " - " << (sourceDamage*100)/(totalDamage == 0 ? 1 : totalDamage) << "%\n";
							//logFile.width(12);
							logFile << std::setw(19) << "Total" << std::setw(12) << "Avg" << std::setw(12) << "Min" << std::setw(12) << "Max" << std::setw(12) << "Count" << std::endl;
                            printSourceLine(&source.hit, logFile, "Hit");
                            printSourceLine(&source.crit, logFile, "Crit");
                            printSourceLine(&source.miss, logFile, "Miss");
                            printSourceLine(&source.evade, logFile, "Evade");
                            printSourceLine(&source.parry, logFile, "Parry");
                            printSourceLine(&source.block, logFile, "Block");
                            printSourceLine(&source.guard, logFile, "Guard");
							logFile << std::endl;
							sourcesIt++;
						}
						logFile << std::endl;
					    entityIt++;
					}
					logFile.close();
				}
				else
				{
					m_AshitaCore->GetDataModule()->AddChatLine(5, "Deeps: Failed to open log file.");
				}
			}
		}
	}
	return false;
}
	
bool __stdcall Deeps::HandleIncomingPacket(unsigned int uiSize, void* pData)
{
	uint16_t packetType = RBUFW(pData,0) & 0x00FF;

	IResources* resources = m_AshitaCore->GetResources();

	if(packetType == 0x28) //action
	{
		IDataTools* dataTools = m_AshitaCore->GetDataModule()->GetDataTools();
		uint8_t actionNum = (uint8_t)(dataTools->unpackBitsBE((unsigned char*)pData, 182, 4));
		uint8_t targetNum = RBUFB(pData, 0x09);
		uint8_t actionType = (uint8_t)(dataTools->unpackBitsBE((unsigned char*)pData, 82, 4));
		//uint8_t reaction = 0;
		//uint8_t speceffect = 0;
		uint16_t actionID = (uint16_t)(dataTools->unpackBitsBE((unsigned char*)pData, 86, 10));
		uint32_t userID = RBUFL(pData, 0x05);
		uint16_t startBit = 150;
		uint16_t damage = 0;

		std::map<uint32_t, entity_info_t>::iterator it = damageInfo.find(userID);
		entity_info_t* entityInfo;
		
		if ( it != damageInfo.end() )
		{
			entityInfo = &it->second;
		}
		else
		{
            entity_info_t newentityInfo;
			newentityInfo.name = m_AshitaCore->GetDataModule()->GetEntityList()->Find(userID)->Name;
            entityInfo = &damageInfo.insert(std::make_pair(userID, newentityInfo)).first->second;
		}

		if ((actionType >= 1 && actionType <= 4) || (actionType == 6) || (actionType == 11) || (actionType == 14) || (actionType == 15))
		{
			if (targetNum == 1 && actionNum > 0) //single target spells, auto attack rounds
			{
				damage_type_t* source = getDamageSource(entityInfo, actionType, actionID);
				uint16_t messageID = (uint16_t)(dataTools->unpackBitsBE((unsigned char*)pData, startBit + 80, 10));

				uint32_t addEffectDamage = 0;
				uint8_t addEffectCount = 0;
				uint16_t addMessageID = 0;
				for (int i=0; i<actionNum; i++)
				{
					uint32_t mainDamage = (uint16_t)(dataTools->unpackBitsBE((unsigned char*)pData, startBit + 63, 17));
					uint8_t reaction = (uint8_t)(dataTools->unpackBitsBE((unsigned char*)pData, startBit + 36, 5));
					uint8_t speceffect = (uint8_t)(dataTools->unpackBitsBE((unsigned char*)pData, startBit + 53, 9));
			
					bool valid = validMessage(messageID);
					if (valid)
						updateDamageSource(source, reaction, speceffect, mainDamage);

					if ((dataTools->unpackBitsBE((unsigned char*)pData, startBit + 121, 2) & 0x1) && actionType != 6)
					{
						addMessageID = (uint16_t)(dataTools->unpackBitsBE((unsigned char*)pData, startBit + 149, 10));
						if (addMessageID == 163 || addMessageID == 229 || (addMessageID >= 288 && addMessageID <= 302))
						{
							addEffectDamage += (uint16_t)(dataTools->unpackBitsBE((unsigned char*)pData, startBit + 132, 16));
							addEffectCount++;
						}
						else
						{
							addMessageID = 0;
						}
						startBit += 37;
					}
					if (valid)
						damage += mainDamage;
					startBit += 87;
				}
				
				if (addMessageID != 0)
				{
					uint32_t key = 0;
					if (addMessageID == 163 || addMessageID == 229)
						key = 1 << 8;
					else
						key = 2 << 8;
					damagesourcemap::iterator sourcesIt = entityInfo->sources.find(key);

					damage_type_t* source;

					if (sourcesIt != entityInfo->sources.end())
					{
						source = &sourcesIt->second;
					}
					else
					{
                        damage_type_t newsource;
						if (key == 1 << 8) { newsource.name.append("Additional Effect");}
						else { newsource.name.append("Skillchain");}

						sourcesIt = entityInfo->sources.insert(std::make_pair(key, newsource)).first;
                        source = &sourcesIt->second;
						//entityInfo.sorter.insert(std::make_pair(&(sourcesIt->first), &(sourcesIt->second)));
					}
					source->hit.count += addEffectCount;
					source->hit.total += addEffectDamage;
					source->hit.min = (addEffectDamage < source->hit.min ? addEffectDamage : source->hit.min);
					source->hit.max = (addEffectDamage > source->hit.min ? addEffectDamage : source->hit.min);

					damage += addEffectDamage;
				}
			} 
			else if (targetNum > 1 && actionNum > 0) //aoe
			{
				
				damage_type_t* source = getDamageSource(entityInfo, actionType, actionID);

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
	else if (packetType == 0x0E) //entity_update
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
    else if (packetType == 0xC8) //party_define
    {
        //need to reassign everything

        bool accounted[18];
        memset(accounted, 0, sizeof(accounted));

        for (int i = 0; i < 17; i++)
        {
            uint32_t id = RBUFL(pData, 12*i + (0x08));
            uint8_t partyflags = RBUFW(pData, 12*i + (0x0E));
            if (id != 0)
            {
                if (partyflags & 0x01)
                {
                    //first alliance party
                    for (int j = 6; j < 12; j++)
                    {
                        if(m_Deeps[j].id == id)
                        {
                            if (j != i)
                            {
                                party_deeps_t temp = m_Deeps[j];
                                m_Deeps[j] = m_Deeps[i];
                                m_Deeps[i] = temp;
                            }
                            accounted[i] = true;
                        }
                    }
                }
                else if (partyflags & 0x02)
                {
                    //second alliance party
                    for (int j = 12; j < 18; j++)
                    {
                        if(m_Deeps[j].id == id)
                        {
                            if (j != i)
                            {
                                party_deeps_t temp = m_Deeps[j];
                                m_Deeps[j] = m_Deeps[i];
                                m_Deeps[i] = temp;
                            }
                            accounted[i] = true;
                        }
                    }
                }
                else
                {
                    //players party
                    for (int j = 0; j < 6; j++)
                    {
                        if(m_Deeps[j].id == id)
                        {
                            if (j != i)
                            {
                                party_deeps_t temp = m_Deeps[j];
                                m_Deeps[j] = m_Deeps[i];
                                m_Deeps[i] = temp;
                            }
                            accounted[i] = true;
                        }
                    }
                }
            }
        }
        for (int i = 0; i < 17; i++)
        {
            if (accounted[i] == false && m_Deeps[i].id != 0)
            {
                m_Deeps[i].reset();
            }
        }
    }
	return false;
}

bool __stdcall Deeps::DxSetup(IDirect3DDevice8* mDevice)
{
	IParty* party = m_AshitaCore->GetDataModule()->GetParty();
	for (int i = 0; i < 18; i++)
	{
		char Meters_String[256] = "";
		sprintf_s(Meters_String, 256, "_Meters[%d]", i);
		m_Meters[i] = m_AshitaCore->GetDirectxModule()->GetTextObjectManager()->CreateTextObject(Meters_String);
		m_Meters[i]->SetFont("Courier",10);
		m_Meters[i]->SetBGColor(255,0,0,0);
		m_Meters[i]->SetBGVisibility(false);
		m_Meters[i]->SetColor(255, 255, 255, 255);
		m_Meters[i]->SetBold(false);
        m_Deeps[i].id = party->Member[i].ID;
		if (m_Deeps[i].id == 0)
		{
			m_Meters[i]->SetVisibility(false);
		}
        if ( i < 6 )
        {
            auto membercount = party->Member[0].Alliance->Party0Count;
		    m_Meters[i]->SetLocation((float)(m_AshitaCore->GetGameSettings()->ResolutionX - 310), (float)(m_AshitaCore->GetGameSettings()->ResolutionY - (30 + (membercount - 1 - i)*22)));
        }
        else
        {
            m_Meters[i]->SetLocation((float)(m_AshitaCore->GetGameSettings()->ResolutionX - 310), (float)(m_AshitaCore->GetGameSettings()->ResolutionY - (50 + i*25)));
        }
	}
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
        if(m_AshitaCore->GetDataModule()->GetParty()->Member[i].Active)
        {
		    char Meters_String[256] = "";
		    sprintf_s(Meters_String, 256, "DMG: %7llu DPS: %4.1f", m_Deeps[i].total, m_Deeps[i].total / (float)m_Deeps[i].battletime);
		    m_Meters[i]->SetText(Meters_String);
            m_Meters[i]->SetVisibility(true);
        }
        else
        {
            m_Meters[i]->SetVisibility(false);
            int j = i;
            while (j != 5 && j != 11 && j != 17 && m_Deeps[i+1].id != 0)
            {
                m_Deeps[j] = m_Deeps[j+1];
                j++;
            }
            m_Deeps[j].reset();
        }
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
