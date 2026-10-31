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
  *   - ProjectApolloMFD.cpp
  * 
  * Changes: Used MFD code as a means for user interactions with multiplayer plugin
  *			 (2026-10)
  *
  * SPDX-License-Identifier: GPL-2.0-or-later
  */

  // To force Orbitersdk.h to use <fstream> in any compiler version
#pragma include_alias( <fstream.h>, <fstream> )

#include "math.h"
#include "windows.h"
#include "nassputils.h"

#include "MPResource.h"
#include "MultiplayerMFD.h"
#include "Multiplayer.h"

#include "../src_skylab/skylab.h"

#include <queue>

using namespace nassp;

#define LINE0	.10
#define LINE1	.14
#define LINE2	.18
#define LINE3	.22
#define LINE4	.26
#define LINE5	.30
#define LINE6	.34
#define LINE7	.38
#define LINE8	.42
#define LINE9	.46
#define LINE10	.50
#define LINE11  .54
#define LINE12  .58
#define LINE13  .62
#define LINE14  .66
#define LINE15  .70
#define LINE16  .74
#define LINE17  .78
#define LINE18  .82
#define LINE19  .86
#define NLINES  20
#define HLINE   .04




// ==============================================================
// Global variables

static HINSTANCE g_MFDhDLL;
static int g_MFDmode; // identifier for new MFD mode

Multiplayer* g_Multiplayer = nullptr;

static struct MultiplayerMFDData {  // global data storage
	int prog;

	// MFD Lines
	char cIP[512];
	char cPort[512];
	char cStatus[512];
	char cDataSent[512];
	char cChat[512];
	char cChat2[512];
	char cChat3[512];
	char cRecvd[512];
} g_Data;

static char debugStringBuffer[100];

void MultiplayerMFDopcDLLInit(HINSTANCE hDLL)
{
	static char* name = "Project Apollo Multiplayer";      // MFD mode name
	MFDMODESPECEX spec;
	spec.name = name;
	spec.key = OAPI_KEY_A;					   // MFD mode selection key is obsolete
	spec.context = NULL;
	spec.msgproc = MultiplayerMFD::MsgProc;  // MFD mode callback function

	// Register the new MFD mode with Orbiter
	g_MFDmode = oapiRegisterMFDMode(spec);
	g_MFDhDLL = hDLL;

	// Init Multiplayer
	g_Multiplayer = &Multiplayer::GetInstance();
}

void MultiplayerMFDopcDLLExit(HINSTANCE hDLL)
{
	oapiUnregisterMFDMode(g_MFDmode);
}

void MultiplayerMFDopcDLLPreStep(double simt, double simdt, double mjd)
{
	g_Multiplayer->Update(simt, simdt, mjd);
}

void MultiplayerMFDopcDLLPostStep(double simt, double simdt, double mjd)
{
	g_Multiplayer->UpdatePost(simt, simdt, mjd);
}


// ==============================================================
// MFD class implementation

// Constructor
MultiplayerMFD::MultiplayerMFD(DWORD w, DWORD h, VESSEL* vessel) : MFD2(w, h, vessel)
{
	// Init MFD
	width = w;
	height = h;
	HBITMAP hBmpLogo = LoadBitmap(g_MFDhDLL, MAKEINTRESOURCE(IDB_LOGO));
	hLogo = oapiCreateSurface(hBmpLogo);
	screen = 0;
	debug_frozen = false;

	// Check if Vessel is Supported
	Supported = false;
	
	if (utils::IsVessel(vessel, utils::Saturn))
	{
		Supported = true;
	}

	else if (utils::IsVessel(vessel, utils::LEM))
	{
		Supported = true;
	}


	// Register Callbacks
	g_Multiplayer->RegisterChatMessageCllbk([this](const char* message) {
		sprintf(g_Data.cChat, "%s", message);
		});

}

MultiplayerMFD::~MultiplayerMFD()
{
	oapiDestroySurface(hLogo);
}

char* MultiplayerMFD::ButtonLabel(int bt)
{
	// The labels for the buttons used by our MFD mode
	//If we are working with an unsupported vehicle, we don't want to return any button labels.
	if (!Supported) {
		return 0;
	}

	return m_buttonPages.ButtonLabel(bt);
}

int MultiplayerMFD::ButtonMenu(const MFDBUTTONMENU** menu) const
{
	// The menu descriptions for the buttons used by our MFD mode
	// We don't want to display a menu if we are in an unsupported vessel.
	if (!Supported) {
		menu = 0;
		return 0;
	}

	return m_buttonPages.ButtonMenu(menu);
}

bool MultiplayerMFD::ConsumeButton(int bt, int event)
{
	return m_buttonPages.ConsumeButton(this, bt, event);
}

bool MultiplayerMFD::ConsumeKeyBuffered(DWORD key)
{
	//We don't want to accept keyboard commands from the wrong vessels.
	if (!Supported)
		return false;

	return m_buttonPages.ConsumeKeyBuffered(this, key);
}

bool MultiplayerMFD::Update(oapi::Sketchpad* skp)
{
	char buffer[100];

	SURFHANDLE s = skp->GetSurface();
	RECT src{ 0, 0, 255, 255 };
	RECT dst{ 1, 1, (LONG)width - 2, (LONG)height - 2 };
	oapiBlt(s, hLogo, &dst, &src);

	// Draws the MFD title
	Title(skp, "Project Apollo Multiplayer");

	skp->SetFont(GetDefaultFont(0));
	skp->SetBackgroundMode(oapi::Sketchpad::BK_TRANSPARENT);
	skp->SetTextAlign(oapi::Sketchpad::CENTER);

	skp->SetTextColor(RGB(0, 255, 0));
	skp->Text((int)(width * 0.5), (int)(height * 0.1), "Connection Status", 17);
	sprintf(buffer, g_Multiplayer->getStatusString());
	skp->SetTextColor(RGB(0, 200, 0));
	skp->Text((int)(width * 0.5), (int)(height * 0.15), buffer, strlen(buffer));

	skp->SetPen(GetDefaultPen(0));
	skp->MoveTo((int)(width * 0.05), (int)(height * 0.20));
	skp->LineTo((int)(width * 0.95), (int)(height * 0.20));

	skp->SetTextColor(RGB(0, 255, 0));
	skp->Text((int)(width * 0.25), (int)(height * 0.25), "IP:", 3);
	skp->SetTextColor(RGB(0, 200, 0));
	sprintf(buffer, g_Multiplayer->getIP());
	skp->Text((int)(width * 0.5), (int)(height * 0.25), buffer, strlen(buffer));

	skp->SetTextColor(RGB(0, 255, 0));
	skp->Text((int)(width * 0.25), (int)(height * 0.3), "Port:", 5);
	skp->SetTextColor(RGB(0, 200, 0));
	sprintf(buffer, g_Multiplayer->getPORT());
	skp->Text((int)(width * 0.5), (int)(height * 0.3), buffer, strlen(buffer));

	skp->SetTextColor(RGB(255, 0, 255));
	sprintf(buffer, "Message:  %s", g_Data.cChat);
	skp->Text((int)(width * 0.5), (int)(height * LINE6), buffer, strlen(buffer));

	skp->SetTextColor(RGB(255, 0, 255));
	sprintf(buffer, g_Data.cChat2);
	skp->Text((int)(width * 0.5), (int)(height * LINE7), buffer, strlen(buffer));

	skp->SetTextColor(RGB(255, 0, 255));
	sprintf(buffer, "Data Sent:  %.2f MB", g_Multiplayer->getSentMb());
	skp->Text((int)(width * 0.5), (int)(height * LINE8), buffer, strlen(buffer));

	skp->SetTextColor(RGB(255, 0, 0));
	sprintf(buffer, g_Multiplayer->getVessel()->GetClassName());
	skp->Text((int)(width * 0.5), (int)(height * LINE9), buffer, strlen(buffer));

	return true;
}

OBJHANDLE MultiplayerMFD::AGCGravityRef(VESSEL* vessel) const
{
	//Returns either the object handle to Earth or Moon, depending on the current sphere of influence as defined by the AGC constant for the lunar SOI radius
	OBJHANDLE gravref;
	VECTOR3 rsph;

	gravref = oapiGetGbodyByName("Moon");
	vessel->GetRelativePos(gravref, rsph);
	if (length(rsph) > 64373760.0)
	{
		gravref = oapiGetGbodyByName("Earth");
	}
	return gravref;
}

void MultiplayerMFD::menuVoid() {}

void MultiplayerMFD::menuSetMainPage()
{
	screen = m_buttonPages.page.None;
	m_buttonPages.SelectPage(this, screen);
}

void MultiplayerMFD::menuSetSettingsPage()
{
	screen = m_buttonPages.page.SETTINGS;
	m_buttonPages.SelectPage(this, screen);
}

void MultiplayerMFD::menuSetChatPage()
{
	screen = m_buttonPages.page.CHAT;
	m_buttonPages.SelectPage(this, screen);
}

void MultiplayerMFD::menuSetConnectPage()
{
	screen = m_buttonPages.page.CONN;
	m_buttonPages.SelectPage(this, screen);
}

void MultiplayerMFD::menuHost()
{
	g_Multiplayer->Host();
}

void MultiplayerMFD::menuJoin()
{
	g_Multiplayer->Join();
}

void MultiplayerMFD::menuDisconnect()
{
	g_Multiplayer->Disconnect();
}

void MultiplayerMFD::menuSetIP()
{
	bool ipInput(void* id, char* str, void* data);
	oapiOpenInputBox("IP:", ipInput, 0, 20, (void*)g_Multiplayer);
}

void MultiplayerMFD::menuSetPort()
{
	bool portInput(void* id, char* str, void* data);
	oapiOpenInputBox("Port:", portInput, 0, 20, (void*)g_Multiplayer);
}

void MultiplayerMFD::menuChat()
{
	bool sayInput(void* id, char* str, void* data);
	oapiOpenInputBox("Say:", sayInput, 0, 20, (void*)g_Multiplayer);
}

void MultiplayerMFD::menuSync()
{
	if (g_Multiplayer->isHost())
	{
		g_Multiplayer->SendMJD();
	}
	else
	{
		g_Multiplayer->SendTimeSyncReq();
	}
}


OAPI_MSGTYPE MultiplayerMFD::MsgProc(UINT msg, UINT mfd, WPARAM wparam, LPARAM lparam)
{
	switch (msg) {
	case OAPI_MSG_MFD_OPENED:
		// Our new MFD mode has been selected, so we create the MFD and
		// return a pointer to it.
		return (OAPI_MSGTYPE)(new MultiplayerMFD(LOWORD(wparam), HIWORD(wparam), (VESSEL*)lparam));
	}
	return 0;
}

MultiplayerMFD::ScreenData MultiplayerMFD::screenData = { 0 };

DLLCLBK char* pacMFDGetDebugString()
{
	return debugStringBuffer;
}


// TODO - Make these static methods in Multiplayer
bool sayInput(void* id, char* str, void* data)
{
	return ((Multiplayer*)data)->SendChat(str);
}

bool ipInput(void* id, char* str, void* data)
{
	return ((Multiplayer*)data)->SetIP(str);
}

bool portInput(void* id, char* str, void* data)
{
	return ((Multiplayer*)data)->SetPORT(str);
}
