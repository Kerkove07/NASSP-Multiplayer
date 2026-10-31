/***************************************************************************
  This file is part of Project Apollo - NASSP
  Copyright 2003-2006

  Project Apollo MFD

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
  * MULTIPLAYER MFD
  * Copyright (c) 2026 Cody Kerkove
  *
  * This file contains code adapted from the following NASSP file(s):
  *   - ProjectApolloMFD.cpp (ProjectApolloMFD Class)
  * 
  * Changes: Used MFD code as a means for user interactions with multiplayer plugin
  *			 (2026-10)
  *
  * SPDX-License-Identifier: GPL-2.0-or-later
  */

#ifndef MULTIPLAYERMFD_H_
#define MULTIPLAYERMFD_H_

#include "MultiplayerMFDButtons.h"

class Saturn;
class LEM;
class Crawler;
class MCC;
class SIVB;
class Skylab;

#ifndef OAPI_MSGTYPE
typedef int OAPI_MSGTYPE;
#endif


class MultiplayerMFD : public MFD2
{
public:
	MultiplayerMFD(DWORD w, DWORD h, VESSEL* vessel);
	~MultiplayerMFD();

	char* ButtonLabel(int bt);
	int ButtonMenu(const MFDBUTTONMENU** menu) const;
	bool ConsumeButton(int bt, int event);
	bool ConsumeKeyBuffered(DWORD key);
	bool Update(oapi::Sketchpad* skp);

	// Pages
	void menuVoid();
	void menuSetMainPage();
	void menuSetSettingsPage();
	void menuSetChatPage();
	void menuSetConnectPage();

	void menuHost();
	void menuJoin();
	void menuDisconnect();
	void menuSetIP();
	void menuSetPort();

	void menuChat();
	void menuSync();


	static OAPI_MSGTYPE MsgProc(UINT msg, UINT mfd, WPARAM wparam, LPARAM lparam);

	OBJHANDLE AGCGravityRef(VESSEL* vessel) const;

protected:
	SURFHANDLE hLogo;
	DWORD width;
	DWORD height;
	
	// Vessel pointers
	VESSEL* our_vessel;
	//We can link to the Saturn OR the Crawler.  Only the Saturn has full funcionality.  The Crawler can ONLY read the mission time.
	Saturn* saturn;
	Crawler* crawler;
	LEM* lem;
	SIVB* sivb;
	MCC* mcc;
	Skylab* sl;
	bool isSaturnV;
	bool Supported; //Vessel is supported
	unsigned FailureSubpage;

	int screen;
	bool debug_frozen;

	static struct ScreenData {
		int screen;
	} screenData;

	const MultiplayerMFDButtons m_buttonPages;
};

#endif