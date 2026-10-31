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
  * MULTIPLAYER VESSEL CONNECTOR CLASS
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
#include "MPconnector.h"

#include "Multiplayer.h"

#include <stdio.h>
#include <string.h>
#include <vector>

//
// MFD to Panel interface connector.
//

MPConnector::MPConnector()
{
	SetType(MP_PANEL_INTERFACE);
	pMP = nullptr;
}

MPConnector::MPConnector(Multiplayer* mp)

{
	SetType(MP_PANEL_INTERFACE);
	pMP = mp;
}

MPConnector::~MPConnector()

{
}
bool MPConnector::ConnectTo(Connector *other)
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
bool MPConnector::ConnectToVessel(VESSEL *v)

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

void MPConnector::DisconnectVessel()

{
	Disconnect();
}

bool MPConnector::SetFlashing(char *n, bool flash)

{
	ConnectorMessage cm;

	cm.destination = type;
	cm.messageType = MP_PanelConnector::MP_PANEL_FLASH_ITEM;
	cm.val1.pValue = n;
	cm.val2.bValue = flash;

	if (SendMessage(cm))
	{
		return cm.val1.bValue;
	}

	return false;
}

int MPConnector::GetState(char *n)

{
	ConnectorMessage cm;

	cm.destination = type;
	cm.messageType = MP_PanelConnector::MP_PANEL_GET_ITEM_STATE;
	cm.val1.pValue = n;

	if (SendMessage(cm))
	{
		return cm.val1.iValue;
	}

	return (-1);
}

bool MPConnector::SetState(char *n, int value, bool guard, bool hold)

{
	ConnectorMessage cm;

	cm.destination = type;
	cm.messageType = MP_PanelConnector::MP_PANEL_SET_ITEM_STATE;
	cm.val1.pValue = n;
	cm.val2.iValue = value;
	cm.val3.bValue = guard;
	cm.val4.bValue = hold;

	if (SendMessage(cm))
	{
		return cm.val1.bValue;
	}

	return false;
}

bool MPConnector::GetFailed(char *n)

{
	ConnectorMessage cm;

	cm.destination = type;
	cm.messageType = MP_PanelConnector::MP_PANEL_GET_FAILED_STATE;
	cm.val1.pValue = n;

	if (SendMessage(cm))
	{
		return cm.val1.bValue;
	}

	return false;
}

bool MPConnector::SetIsClient(bool bYN)
{
	ConnectorMessage cm;

	cm.destination = type;
	cm.messageType = MP_PanelConnector::MP_PANEL_SET_IS_CLIENT;
	cm.val1.bValue = bYN;

	if (SendMessage(cm))
	{
		return cm.val1.bValue;
	}

	return false;
}

bool MPConnector::SyncMissionTime()
{
	ConnectorMessage cm;

	cm.destination = type;
	cm.messageType = MP_PanelConnector::MP_PANEL_SYNC_MISSION_TIME;

	if (SendMessage(cm))
	{
		return cm.val1.bValue;
	}

	return false;
}

bool MPConnector::SyncFDAI(int fdaiType, VECTOR3 fdai)
{
	ConnectorMessage cm;

	cm.destination = type;
	cm.messageType = MP_PanelConnector::MP_PANEL_SET_FDAI;

	cm.val1.iValue = fdaiType;
	cm.val2.dValue = fdai.x;
	cm.val3.dValue = fdai.y;
	cm.val4.dValue = fdai.z;
	
	if (SendMessage(cm))
	{
		return cm.val1.bValue;
	}

	return false;
}

bool MPConnector::SendRhc(VECTOR3 rhc)
{
	ConnectorMessage cm;

	cm.destination = type;
	cm.messageType = MP_PanelConnector::MP_PANEL_SET_RHC;
	cm.val1.iValue = rhc.x;
	cm.val2.iValue = rhc.y;
	cm.val3.iValue = rhc.z;

	if (SendMessage(cm))
	{
		return cm.val1.bValue;
	}

	return false;
}

bool MPConnector::SendThc(VECTOR3 thc)
{
	ConnectorMessage cm;

	cm.destination = type;
	cm.messageType = MP_PanelConnector::MP_PANEL_SET_THC;
	cm.val1.iValue = thc.x;
	cm.val2.iValue = thc.y;
	cm.val3.iValue = thc.z;

	if (SendMessage(cm))
	{
		return cm.val1.bValue;
	}

	return false;
}

bool MPConnector::SendRCSSound(bool rcsSound, bool rcsSustainSound)
{
	ConnectorMessage cm;

	cm.destination = type;
	cm.messageType = MP_PanelConnector::MP_PANEL_SET_RCS_SOUND;
	cm.val1.bValue = rcsSound;
	cm.val2.bValue = rcsSustainSound;

	if (SendMessage(cm))
	{
		return cm.val1.bValue;
	}

	return false;
}

bool MPConnector::SendThruster(const char* name, int index, double value)
{
	ConnectorMessage cm;
	cm.destination = type;
	cm.messageType = MP_PanelConnector::MP_PANEL_SET_THRUSTER;
	cm.val1.pValue = (char*)name;
	cm.val2.iValue = index;
	cm.val3.dValue = value;

	if (SendMessage(cm))
	{
		return cm.val1.bValue;
	}
	
	return false;
}

bool MPConnector::SendLanded(bool state)
{
	ConnectorMessage cm;
	cm.destination = type;
	cm.messageType = MP_PanelConnector::MP_PANEL_SET_LANDED;
	cm.val1.bValue = state;

	if (SendMessage(cm))
	{
		return cm.val1.bValue;
	}

	return false;
}

bool MPConnector::ReceiveMessage(Connector* from, ConnectorMessage& m)
{
	//
	// Sanity check.
	//

	if (m.destination != type)
	{
		return false;
	}

	MP_PanelConnector::MP_PanelConnectorMessageType messageType;

	messageType = (MP_PanelConnector::MP_PanelConnectorMessageType)m.messageType;

	switch (messageType)
	{
		case MP_PanelConnector::MP_PANEL_SWITCH_TOGGLED:
			MPConnector::MPSwitchStatus sw_status;
			sw_status.name = (char*)m.val1.pValue;
			sw_status.state = m.val2.iValue;
			sw_status.guard = m.val3.bValue;
			sw_status.hold = m.val4.bValue;
			switchStatuses.push_back(sw_status);
			break;

		case MP_PanelConnector::MP_PANEL_FDAI_CHANGED:		
			VECTOR3 fdai;		
			fdai.x = m.val2.dValue;
			fdai.y = m.val3.dValue;
			fdai.z = m.val4.dValue;


			if (pMP)
			{
				pMP->SendFDAI(m.val1.iValue, fdai);
			}
			break;

		case MP_PanelConnector::MP_PANEL_SIVB_SEPARATED:
			if (pMP)
			{
				pMP->SetSIVBSeparated(m.val1.bValue);
			}
			break;

		case MP_PanelConnector::MP_PANEL_RHC_CHANGED:
			if (pMP)
			{
				double x = m.val1.iValue;
				double y = m.val2.iValue;
				double z = m.val3.iValue;

				pMP->SendRhc({ x, y, z });
			}
			break;

		case MP_PanelConnector::MP_PANEL_THC_CHANGED:
			if (pMP)
			{
				double x = m.val1.iValue;
				double y = m.val2.iValue;
				double z = m.val3.iValue;

				pMP->SendThc({ x, y, z });
			}
			break;

		case MP_PanelConnector::MP_PANEL_THRUSTER_CHANGED:
			if (pMP)
			{
				pMP->SendThruster((char*)m.val1.pValue, m.val2.iValue, m.val3.dValue);

			}
			break;

		case MP_PanelConnector::MP_PANEL_LANDED:
			if (pMP)
			{
				pMP->SendLanded(m.val1.bValue);
			}
			break;

		/*case MP_PanelConnector::MP_PANEL_RCS_SOUND_CHANGED:
			if (pMP)
			{
				pMP->SendRCSSound(m.val1.bValue, m.val2.bValue);
			}
			break;*/


	}
	
	return true;
}
