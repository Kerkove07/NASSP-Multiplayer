/*
* Modifications and additions for NASSP Multiplayer (unofficial fork):
* MULTIPLAYER MFD BUTTONS
* Copyright (c) 2026 Cody Kerkove
*
* This file contains code adapted from the following NASSP file(s):
*   - ProjectApolloMFDButtons.cpp (ProjectApolloMFDButtons Class)
* 
* Changes: Used MFD code as a means for user interactions with multiplayer plugin
*		   (2026-10)
*
* SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "MultiplayerMFD.h"
#include "MultiplayerMFDButtons.h"

MultiplayerMFDButtons::MultiplayerMFDButtons()
{
	static const MFDBUTTONMENU mnuNone[12] = {
		{ "Connect", 0, 'M' },
		{ "Settings", 0, 'S' },
		{ "Chat", 0, 'C' },
		{ 0,0,0 },
		{ 0,0,0 },
		{ 0,0,0 },
		{ 0,0,0 },
		{ 0,0,0 },
		{ 0,0,0 },
		{ 0,0,0 },
		{ 0,0,0 },
		{ 0,0,0 }
	};

	page.None = RegisterPage(mnuNone, sizeof(mnuNone) / sizeof(MFDBUTTONMENU));

	RegisterFunction("CONN", OAPI_KEY_M, &MultiplayerMFD::menuSetConnectPage);
	RegisterFunction("CONFIG", OAPI_KEY_S, &MultiplayerMFD::menuSetSettingsPage);
	RegisterFunction("CHAT", OAPI_KEY_C, &MultiplayerMFD::menuSetChatPage);
	RegisterFunction("", OAPI_KEY_H, &MultiplayerMFD::menuVoid);
	RegisterFunction("", OAPI_KEY_F, &MultiplayerMFD::menuVoid);
	RegisterFunction("", OAPI_KEY_B, &MultiplayerMFD::menuVoid);
	
	RegisterFunction("", OAPI_KEY_T, &MultiplayerMFD::menuVoid);
	RegisterFunction("", OAPI_KEY_L, &MultiplayerMFD::menuVoid);
	RegisterFunction("", OAPI_KEY_A, &MultiplayerMFD::menuVoid);
	RegisterFunction("", OAPI_KEY_D, &MultiplayerMFD::menuVoid);
	RegisterFunction("", OAPI_KEY_I, &MultiplayerMFD::menuVoid);
	RegisterFunction("", OAPI_KEY_E, &MultiplayerMFD::menuVoid);

	static const MFDBUTTONMENU mnuConnect[12] = {
		{ "Host", 0, 'H' },
		{ "Join", 0, 'J'},
		{ "Discnect", 0, 'D'},
		{ "Set IP", 0, 'I' },
		{ "Set Port", 0, 'P'},
		{ 0, 0, 0 },
		{ "Say", 0, 'C'},
		{ "Sync Sim Time", 0, 'S'},
		{ 0, 0, 0 },
		{ 0, 0, 0 },
		{ 0, 0, 0 },
		{ "Back", 0, 'B' },
	};
	
	page.CONN = RegisterPage(mnuConnect, sizeof(mnuConnect) / sizeof(MFDBUTTONMENU));

	RegisterFunction("HOST", OAPI_KEY_H, &MultiplayerMFD::menuHost);
	RegisterFunction("JOIN", OAPI_KEY_J, &MultiplayerMFD::menuJoin);
	RegisterFunction("DISC", OAPI_KEY_D, &MultiplayerMFD::menuDisconnect);
	RegisterFunction("SAY", OAPI_KEY_C, &MultiplayerMFD::menuChat);
	RegisterFunction("SYNC", OAPI_KEY_S, &MultiplayerMFD::menuSync);
	RegisterFunction("IP", OAPI_KEY_I, &MultiplayerMFD::menuSetIP);
	RegisterFunction("PORT", OAPI_KEY_P, &MultiplayerMFD::menuSetPort);
	RegisterFunction("BACK", OAPI_KEY_B, &MultiplayerMFD::menuSetMainPage);
	
	static const MFDBUTTONMENU mnuSettings[12] = {
		{ 0, 0, 0},
		{ 0, 0, 0 },
		{ 0, 0, 0 },
		{ 0, 0, 0 },
		{ 0, 0, 0 },
		{ 0, 0, 0 },
		{ 0, 0, 0 },
		{ 0, 0, 0 },
		{ 0, 0, 0 },
		{ 0, 0, 0 },
		{ 0, 0, 0 },
		{ "Back", 0, 'B' },
	};

	page.SETTINGS = RegisterPage(mnuSettings, sizeof(mnuSettings) / sizeof(MFDBUTTONMENU));

	RegisterFunction("BACK", OAPI_KEY_B, &MultiplayerMFD::menuSetMainPage);

	static const MFDBUTTONMENU mnuChat[5] = {
	{ "Back", 0, 'B' },
	};

	page.CHAT = RegisterPage(mnuChat, sizeof(mnuChat) / sizeof(MFDBUTTONMENU));

	RegisterFunction("BACK", OAPI_KEY_B, &MultiplayerMFD::menuSetMainPage);

}

bool MultiplayerMFDButtons::SearchForKeysInOtherPages() const
{
	return false;
}
