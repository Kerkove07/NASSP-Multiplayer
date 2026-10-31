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