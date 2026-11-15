/***************************************************************************
  This file is part of Project Apollo - NASSP
  Copyright 2026

  NASSP Multiplayer

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
* MULTIPLAYER MFD BUTTONS
* Copyright (c) 2026 Cody Kerkove
*
* This file contains code adapted from the following NASSP file(s):
*   - ProjectApolloMFDButtons.h (ProjectApolloMFDButtons Class)
* 
* Changes: Used MFD code as a means for user interactions with multiplayer plugin
*		   (2026-10)
*
* SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef MULTIPLAYERMFDBUTTONS_H_
#define MULTIPLAYERMFDBUTTONS_H_

#include "MFDButtonPage.hpp"

class MultiplayerMFD;

class MultiplayerMFDButtons : public EnjoLib::MFDGoodies::MFDButtonPage<MultiplayerMFD>
{
public:
	struct Pages {
		int None;
		int MAIN;
		int SETTINGS;
		int CHAT;
		int CONN;
	} page;

	MultiplayerMFDButtons();

	void SetButtonState(int state) { hideButtonState = state; }
protected:
	bool SearchForKeysInOtherPages() const;
private:
	int hideButtonState;
};
#endif