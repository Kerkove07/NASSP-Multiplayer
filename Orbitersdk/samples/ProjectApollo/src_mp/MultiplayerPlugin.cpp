/***************************************************************************
  This file is part of Project Apollo - NASSP
  Copyright 2004-2008

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
  * MULTIPLAYER PLUGIN
  * Copyright (c) 2026 Cody Kerkove
  *
  * This file contains code adapted from the following NASSP file(s):
  *   - ProjectApolloPlugin.cpp
  * 
  * Changes: Used ProjectApolloPlugin as a reference
  *		     (2026-10)
  *
  * SPDX-License-Identifier: GPL-2.0-or-later
  */

#define STRICT
#define ORBITER_MODULE

// To force Orbitersdk.h to use <fstream> in any compiler version
#pragma include_alias( <fstream.h>, <fstream> )
#include "Orbitersdk.h"

#include "math.h"
#include "windows.h"
#include "saturn.h"
#include "LEM.h"
#include "Crawler.h"
#include "papi.h"
#include <stdio.h>

#include "MultiplayerMFD.h"
#include "Multiplayer.h"
#include "MultiplayerPlugin.h"

DLLCLBK void opcDLLInit(HINSTANCE hDLL)
{
	//Multiplayer_opcDLLInit(hDLL);
	MultiplayerMFDopcDLLInit(hDLL);
}

DLLCLBK void opcDLLExit(HINSTANCE hDLL)
{
	//Multiplayer_opcDLLExit(hDLL);
	MultiplayerMFDopcDLLExit(hDLL);
}

DLLCLBK void opcPreStep(double simt, double simdt, double mjd)
{
	MultiplayerMFDopcDLLPreStep(simt, simdt, mjd);
}

DLLCLBK void opcPostStep(double simt, double simdt, double mjd)
{
	MultiplayerMFDopcDLLPostStep(simt, simdt, mjd);
}

DLLCLBK void opcFocusChanged(OBJHANDLE hGainsFocus, OBJHANDLE hLosesFocus)
{
	MPopcDllFocusChanged(hGainsFocus, hLosesFocus);
}
