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
  * MULTIPLAYER DSKY CONNECTOR
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

#ifndef MPDSKYCONNECTOR_H_
#define MPDSKYCONNECTOR_H_	

//#include "connector.h"

class VESSEL;

///
/// \ingroup Connectors
/// \brief MFD to panel connector class.
///
class MPDskyConnector : public Connector
{
public:
	MPDskyConnector();
	~MPDskyConnector();
	
	static struct MPDskyChange
	{
		char name[32];
		bool state;
		char cState[7];
	};

	static const char* MPDskyEStr(MP_DskyConnector::MP_DskyConnectorMessageType type, MP_DskyConnector::DskyLightType name);
	static MP_DskyConnector::DskyLightType MPDskyStrE(const char* type);

	std::vector<MPDskyChange> dskyChanges;

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


	bool SetState(const char* name, int state);
	bool SetState(const char* name, const char* state);

	bool SetIsHosting(bool bYN);

	///
	/// Disconnect from the current vessel, if connected.
	///
	void DisconnectVessel();

	bool ReceiveMessage(Connector* from, ConnectorMessage& m);

protected:
};

#endif // _PA_MPDskyConnector_H
