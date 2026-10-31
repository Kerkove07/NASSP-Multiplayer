/***************************************************************************
  This file is part of Project Apollo - NASSP
  Copyright 2004-2005 Jean-Luc Rocca-Serra, Mark Grant

  ORBITER vessel module: MFD Connector header file

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
* MULTIPLAYER VESSEL CONNECTOR
* Copyright (c) 2026 Cody Kerkove
*
* This file contains code adapted from the following NASSP file(s):
*   - MFDConnector.h (MFDConnector Class)
* 
* Changes: added multiplayer state synchronization
  *			 (2026-10)
*
* SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef MPCONNECTOR_H_
#define MPCONNECTOR_H_	

//#include "connector.h"

class VESSEL;

class Multiplayer;

///
/// \ingroup Connectors
/// \brief MFD to panel connector class.
///
class MPConnector : public Connector
{
public:
	MPConnector();
	MPConnector(Multiplayer* mp);
	~MPConnector();

	typedef struct {
		char* name;
		int state;
		bool guard;
		bool hold;
	} MPSwitchStatus;

	std::vector<MPSwitchStatus> switchStatuses;

	///
	/// Connect to a vessel. If already connected, this call will
	/// disconnect first.
	///
	/// \param v Vessel to attempt to connect to.
	/// \return True if we succeeded.
	///
	bool ConnectToVessel(VESSEL *v);
	
	///
	/// Do the actual connection.
	///
	/// \Param other, connector to connect to.
	/// \Return true if succeded.
	///
	bool ConnectTo(Connector *other);

	///
	/// Disconnect from the current vessel, if connected.
	///
	void DisconnectVessel();

	///
	/// Set a panel item flashing, or stop it flashing.
	///
	/// \param n Name of panel item.
	/// \param flash True to start flashing, false to stop flashing.
	/// \return True if the call succeeded.
	///
	bool SetFlashing(char *n, bool flash);

	///
	/// Get the state of a panel item.
	///
	/// \param n Name of panel item.
	/// \return Integer value if item found, -1 if not.
	///
	int GetState(char *n);

	///
	/// Set the state of a panel item.
	///
	/// \param n Name of panel item.
	/// \param value Value to set state to.
	/// \return True if call succeeded, false if failed.
	///
	bool SetState(char *n, int value, bool guard = false, bool hold = false);

	///
	/// Get the failure state of a panel item.
	///
	/// \param n Name of panel item.
	/// \return True if failed, false if not (or if the connector isn't connected).
	///
	bool GetFailed(char *n);

	bool SetIsClient(bool bYN);

	bool SyncMissionTime();

	bool SyncFDAI(int fdaiType, VECTOR3 fdai);

	bool SendRhc(VECTOR3 rhc);
	bool SendThc(VECTOR3 thc);
	bool SendRCSSound(bool rcsSound, bool rcsSustainSound);

	bool SendThruster(const char* name, int index, double value);
	bool SendLanded(bool state);

	bool ReceiveMessage(Connector* from, ConnectorMessage& m);

protected:
	Multiplayer* pMP;
};

#endif // _PA_MPConnector_H
