/***************************************************************************
  This file is part of Project Apollo - NASSP
  Copyright 2004-2005 Jean-Luc Rocca-Serra, Mark Grant

  ORBITER module: MFD Connector class

  Project Apollo is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Project Apollo is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Project Apollo; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  See http://nassp.sourceforge.net/license/ for more details.

  **************************************************************************/

  /*
  * Modifications and additions for NASSP Multiplayer (unofficial fork):
  * MULTIPLAYER DSKY CONNECTOR CLASS
  * Copyright (c) 2026 Cody Kerkove
  *
  * This file contains code adapted from the following NASSP file(s):
  *   - MFDConnector.cpp (MFDConnector Class)
  * 
  * Changes: added multiplayer state synchronization
  *			 (2026-10)
  *
  * SPDX-License-Identifier: GPL-2.0-or-later
  */

  // To force Orbitersdk.h to use <fstream> in any compiler version
#pragma include_alias( <fstream.h>, <fstream> )
#include "Orbitersdk.h"

#include "nasspdefs.h"

#include "connector.h"
#include "MPDskyConnector.h"

#include <stdio.h>
#include <string.h>
#include <vector>

//
// MFD to Panel interface connector.
//

MPDskyConnector::MPDskyConnector()

{
	SetType(MP_DSKY_INTERFACE);
	dskyChanges = std::vector<MPDskyChange>();
}

MPDskyConnector::~MPDskyConnector()

{
}
bool MPDskyConnector::ConnectTo(Connector *other)
{
	if (other)
	{
		if (other->GetType() != type)
		{
			return false;
		}
		other->connectedTo = this;
	}
	connectedTo = other;
	return true;
}

bool MPDskyConnector::SetState(const char* name, int state)
{
	ConnectorMessage cm;

	cm.destination = type;
	
	if (!strcmp(name, "fNoun"))
		cm.messageType = MP_DskyConnector::MP_DSKY_SET_NOUN_FLASH;

	else if (!strcmp(name, "fVerb"))
		cm.messageType = MP_DskyConnector::MP_DSKY_SET_VERB_FLASH;

	// Not a flashing state so must be light state
	else
	{
		cm.messageType = MP_DskyConnector::MP_DSKY_SET_LIGHT;
		cm.val1.iValue = (int)MPDskyStrE(name);
	}
	
	cm.val2.bValue = state;

	if (SendMessage(cm))
	{
		return cm.val1.bValue;
	}

	return false;
}

bool MPDskyConnector::SetState(const char* name, const char* state)
{
	std::string value = state;
	
	ConnectorMessage cm;

	cm.destination = type;

	if (!strcmp(name, "rNoun"))
		cm.messageType = MP_DskyConnector::MP_DSKY_SET_NOUN;

	else if (!strcmp(name, "rVerb"))
		cm.messageType = MP_DskyConnector::MP_DSKY_SET_VERB;

	else if (!strcmp(name, "rProg"))
		cm.messageType = MP_DskyConnector::MP_DSKY_SET_PROG;
	
	else if (!strcmp(name, "rR1"))
		cm.messageType = MP_DskyConnector::MP_DSKY_SET_R1;

	else if (!strcmp(name, "rR2"))
		cm.messageType = MP_DskyConnector::MP_DSKY_SET_R2;

	else if (!strcmp(name, "rR3"))
		cm.messageType = MP_DskyConnector::MP_DSKY_SET_R3;


	cm.val1.pValue = (char*)value.c_str();

	if (SendMessage(cm))
	{
		return cm.val1.bValue;
	}

	return false;
}

bool MPDskyConnector::SetIsHosting(bool bYN)
{
	ConnectorMessage cm;

	cm.destination = type;

	cm.messageType = MP_DskyConnector::MP_DSKY_SET_HOSTING;

	cm.val1.bValue = bYN;

	if (SendMessage(cm))
	{
		return cm.val1.bValue;
	}

	return false;
}

const char* MPDskyConnector::MPDskyEStr(MP_DskyConnector::MP_DskyConnectorMessageType type, MP_DskyConnector::DskyLightType name)
{
	// Return a stable pointer by using a thread-local static buffer.
	// This avoids returning a pointer to a temporary std::string.
	thread_local char buf[32];
	buf[0] = '\0';

	if (type == MP_DskyConnector::MP_DSKY_NOUN_CHANGED)
		strcpy(buf, "rNoun");

	else if (type == MP_DskyConnector::MP_DSKY_VERB_CHANGED)
		strcpy(buf, "rVerb");

	else if (type == MP_DskyConnector::MP_DSKY_PROG_CHANGED)
		strcpy(buf, "rProg");

	else if (type == MP_DskyConnector::MP_DSKY_R1_CHANGED)
		strcpy(buf, "rR1");

	else if (type == MP_DskyConnector::MP_DSKY_R2_CHANGED)
		strcpy(buf, "rR2");

	else if (type == MP_DskyConnector::MP_DSKY_R3_CHANGED)
		strcpy(buf, "rR3");

	else if (type == MP_DskyConnector::MP_DSKY_NOUN_FLASH_CHANGED)
		strcpy(buf, "fNoun");

	else if (type == MP_DskyConnector::MP_DSKY_VERB_FLASH_CHANGED)
		strcpy(buf, "fVerb");

	else if (type == MP_DskyConnector::MP_DSKY_LIGHT_CHANGED)
	{
		switch (name)
		{
		case MP_DskyConnector::COMP_ACTY:
			strcpy(buf, "CompA");
			break;
		case MP_DskyConnector::UPLINK:
			strcpy(buf, "Uplin");
			break;
		case MP_DskyConnector::NO_ATT:
			strcpy(buf, "NoAtt");
			break;
		case MP_DskyConnector::STBY:
			strcpy(buf, "StbyL");
			break;
		case MP_DskyConnector::KB_REL:
			strcpy(buf, "KbRel");
			break;
		case MP_DskyConnector::OPR_ERR:
			strcpy(buf, "OprEr");
			break;
		case MP_DskyConnector::TEMP:
			strcpy(buf, "TempL");
			break;
		case MP_DskyConnector::GIM_LOCK:
			strcpy(buf, "Gimba");
			break;
		case MP_DskyConnector::PROG:
			strcpy(buf, "ProgL");
			break;
		case MP_DskyConnector::RESTART:
			strcpy(buf, "Resta");
			break;
		case MP_DskyConnector::TRACKER:
			strcpy(buf, "Track");
			break;
		case MP_DskyConnector::VEL:
			strcpy(buf, "VelLi");
			break;
		case MP_DskyConnector::ALT:
			strcpy(buf, "AltLi");
			break;
		case MP_DskyConnector::PRIO_DISP:
			strcpy(buf, "PrioD");
			break;
		case MP_DskyConnector::NO_DAP:
			strcpy(buf, "NoDAP");
			break;
		}
	}

	return buf;
}

MP_DskyConnector::DskyLightType MPDskyConnector::MPDskyStrE(const char* type)
{
	if (!strcmp(type, "CompA"))
		return MP_DskyConnector::DskyLightType::COMP_ACTY;
	else if (!strcmp(type, "Uplin"))
		return MP_DskyConnector::DskyLightType::UPLINK;
	else if (!strcmp(type, "NoAtt"))
		return MP_DskyConnector::DskyLightType::NO_ATT;
	else if (!strcmp(type, "StbyL"))
		return MP_DskyConnector::DskyLightType::STBY;
	else if (!strcmp(type, "KbRel"))
		return MP_DskyConnector::DskyLightType::KB_REL;
	else if (!strcmp(type, "OprEr"))
		return MP_DskyConnector::DskyLightType::OPR_ERR;
	else if (!strcmp(type, "TempL"))
		return MP_DskyConnector::DskyLightType::TEMP;
	else if (!strcmp(type, "Gimba"))
		return MP_DskyConnector::DskyLightType::GIM_LOCK;
	else if (!strcmp(type, "ProgL"))
		return MP_DskyConnector::DskyLightType::PROG;
	else if (!strcmp(type, "Resta"))
		return MP_DskyConnector::DskyLightType::RESTART;
	else if (!strcmp(type, "Track"))
		return MP_DskyConnector::DskyLightType::TRACKER;
	else if (!strcmp(type, "VelLi"))
		return MP_DskyConnector::DskyLightType::VEL;
	else if (!strcmp(type, "AltLi"))
		return MP_DskyConnector::DskyLightType::ALT;
	else if (!strcmp(type, "PrioD"))
		return MP_DskyConnector::DskyLightType::PRIO_DISP;
	else if (!strcmp(type, "NoDAP"))
		return MP_DskyConnector::DskyLightType::NO_DAP;


	return MP_DskyConnector::DskyLightType();
}


bool MPDskyConnector::ConnectToVessel(VESSEL *v)

{
	//
	// Disconnect in case we're already connected.
	//
	Disconnect();

	//
	// See if we can find the appropriate connector on the vessel.
	//
	Connector *vessel = GetVesselConnector(v, VIRTUAL_CONNECTOR_PORT, type);

	//
	// Try to connect if we did.
	//
	if (vessel)
	{
		return ConnectTo(vessel);
	}

	return false;
}

void MPDskyConnector::DisconnectVessel()

{
	Disconnect();
}

bool MPDskyConnector::ReceiveMessage(Connector* from, ConnectorMessage& m)
{	
	//
	// Sanity check.
	//


	if (m.destination != type)
	{
		return false;
	}


	MP_DskyConnector::MP_DskyConnectorMessageType messageType;
	messageType = (MP_DskyConnector::MP_DskyConnectorMessageType)m.messageType;
	
	MPDskyChange change;
	
	strcpy(change.name, MPDskyConnector::MPDskyEStr(messageType, (MP_DskyConnector::DskyLightType)m.val1.iValue));

	//oapiDebugString(change.name);

	if (messageType == MP_DskyConnector::MP_DSKY_LIGHT_CHANGED ||
		messageType == MP_DskyConnector::MP_DSKY_NOUN_FLASH_CHANGED ||
		messageType == MP_DskyConnector::MP_DSKY_VERB_FLASH_CHANGED)
	{
		change.state = m.val2.bValue;
		strcpy(change.cState, "");
		dskyChanges.push_back(change);
		return true;
	}

	else if (messageType == MP_DskyConnector::MP_DSKY_NOUN_CHANGED ||
		messageType == MP_DskyConnector::MP_DSKY_VERB_CHANGED ||
		messageType == MP_DskyConnector::MP_DSKY_PROG_CHANGED ||
		messageType == MP_DskyConnector::MP_DSKY_R1_CHANGED ||
		messageType == MP_DskyConnector::MP_DSKY_R2_CHANGED ||
		messageType == MP_DskyConnector::MP_DSKY_R3_CHANGED)
	{
		change.state = false;
		strcpy(change.cState, (char*)m.val2.pValue);
		dskyChanges.push_back(change);
		return true;
	}

	return false;
}
