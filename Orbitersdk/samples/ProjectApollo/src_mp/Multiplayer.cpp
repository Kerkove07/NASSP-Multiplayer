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
* MULTIPLAYER DSKY CONNECTOR
* Copyright (c) 2026 Cody Kerkove
*
* SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "OrbiterSDK.h"

#include "math.h"
#include "windows.h"
#include "saturn.h"
#include "LEM.h"
#include "Crawler.h"
#include "../src_skylab/skylab.h"

#include "MPconnector.h"
#include "Multiplayer.h"
#include "MultiplayerPlugin.h"

void MPopcDllFocusChanged(OBJHANDLE hGainsFocus, OBJHANDLE hLosesFocus)
{
	Multiplayer& mp = Multiplayer::GetInstance();
	mp.UpdateVessel(oapiGetVesselInterface(hGainsFocus));
}

void Multiplayer::UpdateVessel(VESSEL* newVessel)
{
	vessel = newVessel;
	VESSEL* currentVessel = NULL;
	
	if (!stricmp(vessel->GetClassName(), "ProjectApollo\\Saturn5") ||
		!stricmp(vessel->GetClassName(), "ProjectApollo/Saturn5") ||
		!stricmp(vessel->GetClassName(), "ProjectApollo\\Saturn1b") ||
		!stricmp(vessel->GetClassName(), "ProjectApollo/Saturn1b")) {
		saturn = (Saturn*)vessel;
		currentVessel = saturn;
	}
	else if (!stricmp(vessel->GetClassName(), "ProjectApollo\\Crawler") ||
		!stricmp(vessel->GetClassName(), "ProjectApollo/Crawler")) {
		crawler = (Crawler*)vessel;
		currentVessel = crawler;
	}
	else if (!stricmp(vessel->GetClassName(), "ProjectApollo\\LEM") ||
			!stricmp(vessel->GetClassName(), "ProjectApollo/LEM") ||
			!stricmp(vessel->GetClassName(),"ProjectApollo\\Sat5LMDSC")  ||
			!stricmp(vessel->GetClassName(),"ProjectApollo/Sat5LMDSC"))
	{
		lem = (LEM*)vessel;
		currentVessel = lem;
	}

	if (saturn)
	{
		conn.ConnectToVessel(saturn);
		csmDskyConn.ConnectToVessel(saturn);
	}

	if (lem)
	{
		lemConn.ConnectToVessel(lem);
		lemDskyConn.ConnectToVessel(lem);
	}
	
	return;
}

VESSEL* Multiplayer::getVessel()
{
	return vessel;
}

void Multiplayer::SyncStateVector(const Multiplayer::StateVector& sv)
{
	OBJHANDLE hVes = oapiGetVesselByName(const_cast<char*>(sv.name.c_str()));

	if (!hVes)
	{
		LogClient("Invalid Vessel Handle on SyncStateVector");
		return;
	}

	VESSEL* ves = oapiGetVesselInterface(hVes);

	if (!ves)
	{
		LogClient("Invalid Vessel Interface on SyncStateVector");
		return;
	}


	int match = -1;

	for (size_t i = 0; i < lastStateVectors.size(); i++)
	{
		if (lastStateVectors[i].name == sv.name)
		{
			match = static_cast<int>(i);
			break;
		}
	}

	if (match == -1)
	{
		return;
	}

	const StateVector& old = lastStateVectors[match];

	double dt = sv.mjd - old.mjd;

	// If the client Sim Time gets ahead of the Host, clear state vector info and resysnc clocks.
	if (dt <= 0.0)
	{
		LogClient("CLIENT LEADING: RESYNC");
		lastStateVectors.clear();
		SendTimeSyncReq();
		return;
	}

	if (dt > 0.0 && ves)
	{		
		// Predict where the remote vessel should be now.
		double targetDt = oapiGetSimTime() - sv.mjd;

		VECTOR3 hostAccel = (sv.vel - old.vel) / dt;

		VECTOR3 targetPos =
			sv.pos +
			sv.vel * targetDt +
			hostAccel * (0.5 * targetDt * targetDt);

		VECTOR3 targetVel =
			sv.vel + hostAccel * targetDt;

		//std::string logstr = "Target (" + sv.name + "," + sv.rbody + ") = " + to_string(targetPos.x) + ":" + to_string(targetPos.y) + " : " + to_string(targetPos.z) + to_string(sv.vel.x) + " : " + to_string(sv.vel.y) + " : " + to_string(sv.vel.z);
		//LogClient(logstr.c_str());

		VESSELSTATUS vs;
		ves->GetStatus(vs);

		char buf[128];
		oapiGetObjectName(vs.rbody, buf, 128);

		VECTOR3 currentPos;
		VECTOR3 currentVel;

		ves->GetRelativePos(vs.rbody, currentPos);
		ves->GetRelativeVel(vs.rbody, currentVel);

		std::string namestr = ves->GetName();

		//logstr = "Current (" + namestr + "," + buf + ") = " + to_string(currentPos.x) + ":" + to_string(currentPos.y) + " : " + to_string(currentPos.z) + ":" + to_string(currentVel.x) + " : " + to_string(currentVel.y) + " : " + to_string(currentVel.z);
		//LogClient(logstr.c_str());

		// Position error.
		VECTOR3 posError = targetPos - currentPos;
		VECTOR3 velError = targetVel - currentVel;

		// Host acceleration feed-forward + PD correction
		const double positionGain = 2.0;
		const double velocityGain = 3.0;

		// Calc acceleration
		VECTOR3 accel =
			hostAccel +
			posError * positionGain +
			velError * velocityGain;


		/* Clamp acceleration so a bad packet can't blow things up
		const double maxAccel = 10.0;
		double accelMagnitude = length(accel);
		if (accelMagnitude > maxAccel)
		{
			accel *= maxAccel / accelMagnitude;
		}*/

		// AddForce takes LOCAL vessel coordinates, so rotate global -> local
		VECTOR3 force = accel * ves->GetMass();
		MATRIX3 R;
		ves->GetRotationMatrix(R);            // local -> global
		VECTOR3 localForce = tmul(R, force);  // transpose = global -> local

		ves->AddForce(localForce, _V(0, 0, 0));
	}
}

/*
void Multiplayer::ApplyThrustVectors()
{
	for (size_t i = 0; i < thrustVectors.size(); i++)
	{
		const ThrustVector& tv = thrustVectors[i];
		OBJHANDLE hVes = oapiGetVesselByName(const_cast<char*>(tv.name.c_str()));
		if (!hVes)
		{
			LogClient("Invalid Vessel Handle on ApplyThrustVector");
			return;
		}
		VESSEL* ves = oapiGetVesselInterface(hVes);
		if (!ves)
		{
			LogClient("Invalid Vessel Interface on ApplyThrustVector");
			return;
		}

		ves->AddForce(tv.tvec, _V(0, 0, 0));
	}
}
*/

Multiplayer::Multiplayer()
{	
	ZMQData.context = nullptr;
	ZMQData.socket_implicit = nullptr;
	ZMQData.socket_explicit = nullptr;
	ZMQData.status = ZeroMQStatus::DISCONNECTED;
	ZMQData.useLAN = true;
	recvBuffer[0] = '\0';

	saturn = NULL;
	lem = NULL;
	crawler = NULL;
	planet = NULL;
	vessel = NULL;
	skylab = NULL;

	address = "localhost";
	port = 5555;

	conn = MPConnector(this);
	lemConn = MPConnector(this);
}	

Multiplayer& Multiplayer::GetInstance()
{
	static Multiplayer instance;
	return instance;
}

const char* Multiplayer::Host()
{
	if (ZMQData.status == ZeroMQStatus::HOST)
	{
		return "Already Hosting";
	}

	// Build the address string
	char iAddr[128];
	sprintf(iAddr, "tcp://%s:%d", "0.0.0.0", port);

	char eAddr[128];
	sprintf(eAddr, "tcp://%s:%d", "0.0.0.0", port + 1);

	// Init Context
	ZMQData.context = new zmq::context_t(1);
	
	// Init Implicit socket (Server Pub/Sub)
	ZMQData.socket_implicit = new zmq::socket_t(*ZMQData.context, zmq::socket_type::pub);
	
	// Init Explicit socket (Client Req/Rep)
	ZMQData.socket_explicit = new zmq::socket_t(*ZMQData.context, zmq::socket_type::rep);

	// Bind sockets (LAN Only)
	if (ZMQData.useLAN)
	{
		ZMQData.socket_implicit->bind(iAddr);
		ZMQData.socket_explicit->bind(eAddr);
	}

	else
	{
		sprintf(iAddr, "tcp://%s:%d", address.c_str(), port);
		sprintf(eAddr, "tcp://%s:%d", address.c_str(), port + 2);

		ZMQData.socket_implicit->connect(iAddr);
		ZMQData.socket_explicit->connect(eAddr);
	}
	
	oapiDebugString("Listening for connections...");
	
	ZMQData.status = ZeroMQStatus::HOST;

	conn.SetIsClient(false);
	csmDskyConn.SetIsHosting(true);
	lemConn.SetIsClient(false);
	lemDskyConn.SetIsHosting(true);

	mpLogFileHost = oapiOpenFile("MultiplayerHost.log", FILE_OUT);

	return "Hosting Session";
}

const char* Multiplayer::Join()
{
	if (ZMQData.status == ZeroMQStatus::CLIENT)
	{
		return "Already Joined";
	}

	// Build the address string
	char iAddr[128];
	sprintf(iAddr, "tcp://%s:%d", address.c_str(), port);

	char eAddr[128];
	sprintf(eAddr, "tcp://%s:%d", address.c_str(), port + 1);

	if (!ZMQData.useLAN)
	{
		sprintf(iAddr, "tcp://%s:%d", address.c_str(), port + 1);
		sprintf(eAddr, "tcp://%s:%d", address.c_str(), port + 3);
	}

	// Init Context
	ZMQData.context = new zmq::context_t(1);

	// Init Implicit socket (Server Pub/Sub)
	oapiDebugString("Connecting to host...");
	ZMQData.socket_implicit = new zmq::socket_t(*ZMQData.context, zmq::socket_type::sub);

	// Init Explicit socket (Client Req/Rep)
	ZMQData.socket_explicit = new zmq::socket_t(*ZMQData.context, zmq::socket_type::req);

	// Bind sockets
	ZMQData.socket_implicit->connect(iAddr);
	ZMQData.socket_explicit->connect(eAddr);

	// Subscribe to all messages
	ZMQData.socket_implicit->set(zmq::sockopt::subscribe, "");
	
	ZMQData.status = ZeroMQStatus::CLIENT;

	conn.SetIsClient(true);
	lemConn.SetIsClient(true);

	TimeOfSyncRequest = oapiGetSimTime() + 1.0;

	mpLogFileClient = oapiOpenFile("MultiplayerClient.log", FILE_OUT);

	return "Joined Session";
}

void Multiplayer::ToggleLAN()
{
	if (!ZMQData.useLAN)
	{
		ZMQData.useLAN = true;
	}
	else
	{
		ZMQData.useLAN = false;
	}
}

void Multiplayer::Update(double simt, double simdt, double mjd)
{
	// CHECK VESSELS
	if (!saturn || !lem)
		{
		for (DWORD i = 0; i < oapiGetVesselCount(); i++)
		{
			VESSEL* v = oapiGetVesselInterface(oapiGetVesselByIndex(i));
			if (v)
			{
				UpdateVessel(v);
			}		
		}
	}
	
	// MULTIPLAYER
	if (isHost())
	{
		while (true)
		{
			// RECEIVE MESSAGES
			bool received = ReceiveExplicit();
			/*if (received && (strcmp(lastSent, recvBuffer) != 0))
			{
				HandleASCIIMessage();
			}*/

			if (!received) break; 
			
			HandleASCIIMessage();
		}

		
		// SYNC STATE VECTORS
		if (simt - lastSendSVSimt >= sendSVInterval * oapiGetTimeAcceleration())
		{
			// Send State Vector for all vessels)
			for (DWORD i = 0; i < oapiGetVesselCount(); i++)
			{
				VESSEL* v = oapiGetVesselInterface(oapiGetVesselByIndex(i));

				if (v)
				{
					SendStateVector(v->GetName());
				}
			}
			lastSendSVSimt = simt;
		}

		// SYNC TIME ACCELERATION
		double TimeAccel = oapiGetTimeAcceleration();
		if (TimeAccel != lastTimeAccel)
		{
			SendTimeAccel(TimeAccel);
			lastTimeAccel = TimeAccel;
			SendMJD();
		}
		
		// SYNC EMS DV DISPLAY
		if (oapiGetSimTime() >= lastSendEMSSimt + 0.2)
		{
			if (emsDvs.size())
			{
				SendSwitch("SATURN", "EMSDvDisplay", emsDvs.front(), 0, 0);
				sendEMS = true;
				lastSendEMSSimt = oapiGetSimTime();
				emsDvs.clear();
			}
		}

		// SYNC LEM RADAR DISPLAY
		if (oapiGetSimTime() >= lastSendRadarSimt + 0.2)
		{
			if (radars.size())
			{
				SendRadar(radars.front().x, radars.front().y, radars.front().z);
				sendRadar = true;
				lastSendRadarSimt = oapiGetSimTime();
				radars.clear();
			}
		}

		// SYNC LEM CROSSPOINTER DISPLAY
		if (oapiGetSimTime() >= lastSendCrossPointerSimt + 0.2)
		{
			if (crossPointers.size())
			{
				SendCrossPointer(crossPointers.front().left, crossPointers.front().right);
				sendCrossPointer = true;
				lastSendCrossPointerSimt = oapiGetSimTime();
				crossPointers.clear();
			}
		}
	}

	if (isClient())
	{
		// CLEINT RECEIVE MESSAGES
		while (true)
		{
			bool received = ReceiveBroadcast();
			/*if (received && (strcmp(lastSent, recvBuffer) != 0))
			{
				HandleASCIIMessage();
			}*/

			if (!received) break;
			
			HandleASCIIMessage();
		}
		while (true)
		{
			bool received = ReceiveExplicit();
			/*if (received && (strcmp(lastSent, recvBuffer) != 0))
			{
				// Do something with the received message (ACK)
			}*/
			if (!received) break;
		}

		// TIME SYNC REQUEST
		if (oapiGetSimTime() >= TimeOfSyncRequest)
		{
			SendTimeSyncReq();
			TimeOfSyncRequest = 1e100;
		}

		// APPLY STATE VECTORS
		for (size_t i = 0; i < stateVectors.size(); i++)
		{
			SyncStateVector(stateVectors.at(i));
		}
	}
	
	// Check for switch state changes (SATURN)
	if (conn.switchStatuses.size() > 0)
	{
		if (ZMQData.status != DISCONNECTED /* && simt - lastSendSwitchSimt >= sendSwitchInterval*/)
		{
			SendSwitch("SATURN", conn.switchStatuses.at(0).name,
				conn.switchStatuses.at(0).state,
				conn.switchStatuses.at(0).guard,
				conn.switchStatuses.at(0).hold);

			lastSendSwitchSimt = simt;

			conn.switchStatuses.erase(conn.switchStatuses.begin());
		}

		if (ZMQData.status == DISCONNECTED)
		{
			conn.switchStatuses.erase(conn.switchStatuses.begin());
		}
	}

	// Check for switch state changes (LEM)
	if (lemConn.switchStatuses.size() > 0)
	{
		if (ZMQData.status != DISCONNECTED /* && simt - lastSendSwitchSimt >= sendSwitchInterval*/)
		{
			SendSwitch("LEM", lemConn.switchStatuses.at(0).name,
				lemConn.switchStatuses.at(0).state,
				lemConn.switchStatuses.at(0).guard,
				lemConn.switchStatuses.at(0).hold);

			lastSendSwitchSimt = simt;

			lemConn.switchStatuses.erase(lemConn.switchStatuses.begin());
		}

		if (ZMQData.status == DISCONNECTED)
		{
			lemConn.switchStatuses.erase(lemConn.switchStatuses.begin());
		}
	}

	// Check for DSKY state changes
	if (csmDskyConn.dskyChanges.size() > 0)
	{
		MPDskyConnector::MPDskyChange csmChange = csmDskyConn.dskyChanges.at(0);
		
		if (oapiGetTimeAcceleration() <= 1.0)
		{
			if (!strcmp(csmChange.cState, ""))
				SendDskyLight("CSM", csmChange.name, csmChange.state);
			else
				SendDskyReg("CSM", csmChange.name, csmChange.cState);
		}
		csmDskyConn.dskyChanges.erase(csmDskyConn.dskyChanges.begin());
	}

	if (lemDskyConn.dskyChanges.size() > 0)
	{
		MPDskyConnector::MPDskyChange lemChange = lemDskyConn.dskyChanges.at(0);

		if (oapiGetTimeAcceleration() <= 1.0)
		{
			if (!strcmp(lemChange.cState, ""))
				SendDskyLight("LEM", lemChange.name, lemChange.state);
			else
				SendDskyReg("LEM", lemChange.name, lemChange.cState);
		}
		lemDskyConn.dskyChanges.erase(lemDskyConn.dskyChanges.begin());
	}


}

void Multiplayer::UpdatePost(double simt, double simdt, double mjd)
{
	/*if (isClient())
	{
		for (size_t i = stateVectors.size(); i > 0;)
		{
			i--;
			SyncStateVector(stateVectors.at(i));
		}

		stateVectors.clear();
	}*/
}

void Multiplayer::Broadcast(const char* message)
{
	if (ZMQData.status == ZeroMQStatus::HOST && ZMQData.socket_implicit != nullptr) 
	{
		zmq::message_t zmqMessage(message, strlen(message));
		try {
			ZMQData.socket_implicit->send(zmqMessage, zmq::send_flags::dontwait);
		}
		catch (const zmq::error_t& e) {
			if (e.num() != EAGAIN)
			{
				char buf[100];
				sprintf(buf, "ZeroMQ send error: %s", e.what());
				oapiDebugString(buf);
			}
		}
	}
}

bool Multiplayer::ReceiveBroadcast()
{
	if (ZMQData.status == ZeroMQStatus::CLIENT && ZMQData.socket_implicit != nullptr)
	{
		try {
			zmq::message_t zmqMessage;

			// Receive published message from host
			zmq::recv_result_t result = ZMQData.socket_implicit->recv(zmqMessage, zmq::recv_flags::dontwait);

			// Checks for valid message and non-empty message
			if (!result.has_value()) return false;
			if (zmqMessage.size() == 0) return false;

			// Overflow prevention
			const size_t bufferSize = sizeof(recvBuffer);
			size_t copySize = zmqMessage.size();
			if (copySize >= bufferSize)
			{
				copySize = bufferSize - 1; // Leave space for null terminator
			}

			// Return Valid Message as cstring
			memcpy(recvBuffer, zmqMessage.data(), copySize);
			recvBuffer[copySize] = '\0'; // Ensure null termination
			return true;
		}
		catch (const zmq::error_t& e) {
			if (e.num() != EAGAIN)
			{
				char buf[100];
				sprintf(buf, "ZeroMQ receive error: %s", e.what());
				oapiDebugString(buf);
			}
		}
	}
	return false;
}

bool Multiplayer::SendExplicit(const char* message)
{
	if (ZMQData.socket_explicit != nullptr)
	{
		zmq::message_t zmqMessage(message, strlen(message));
		try {
			ZMQData.socket_explicit->send(zmqMessage, zmq::send_flags::dontwait);
		}
		catch (const zmq::error_t& e) {
			if (e.num() != EAGAIN)
			{
				char buf[100];
				sprintf(buf, "ZeroMQ send error: %s", e.what());
				oapiDebugString(buf);
			}
		}
	}

	strcpy(lastSent, message);
	return false;
}

bool Multiplayer::ReceiveExplicit()
{
	if (ZMQData.status == ZeroMQStatus::CLIENT && ZMQData.socket_explicit != nullptr)
	{
		zmq::message_t zmqMessage;
		std::string buffer;

		try {
			// Receive published message from host
			zmq::recv_result_t result = ZMQData.socket_explicit->recv(zmqMessage, zmq::recv_flags::dontwait);
			
			// Checks for valid message and non-empty message
			if (!result.has_value()) return false;
			if (zmqMessage.size() == 0) return false;
			
			// Overflow prevention
			const size_t bufferSize = sizeof(recvBuffer);
			size_t copySize = zmqMessage.size();
			if (copySize >= bufferSize)
			{
				copySize = bufferSize - 1; // Leave space for null terminator
			}

			// Return Valid Message as cstring
			memcpy(recvBuffer, zmqMessage.data(), copySize);
			recvBuffer[copySize] = '\0'; // Ensure null termination
			return true;
		}
		catch (const zmq::error_t& e) {
			if (e.num() != EAGAIN)
			{
				char buf[100];
				sprintf(buf, "ZeroMQ receive error: %s", e.what());
				//oapiDebugString(buf);
			}
		}
	}

	if (ZMQData.status == ZeroMQStatus::HOST && ZMQData.socket_explicit != nullptr)
	{
		zmq::message_t zmqMessage;
		std::string buffer;

		try {
			// Receive published message from host
			zmq::recv_result_t result = ZMQData.socket_explicit->recv(zmqMessage, zmq::recv_flags::dontwait);
			// Checks for valid message and non-empty message
			if (!result.has_value()) return false;
			if (zmqMessage.size() == 0) return false;
			// Return Valid Message as cstring
			sprintf(recvBuffer, "%.*s", static_cast<int>(zmqMessage.size()), (char*)zmqMessage.data());
			SendExplicit("[ACK]");
			return true;
		}
		catch (const zmq::error_t& e) {
			if (e.num() != EAGAIN)
			{
				char buf[100];
				sprintf(buf, "ZeroMQ receive error: %s", e.what());
				oapiDebugString(buf);
			}
		}
	}
	
	return false;
}

void Multiplayer::HandleASCIIMessage()
{
	std::string input = recvBuffer;
	
	// Tokenize the message
	std::vector<std::string> tokens;
	size_t start = 0;
	size_t end = input.find_first_of("[]");

	while (end != std::string::npos) {
		if (end > start) 
		{
			tokens.push_back(input.substr(start, end - start));
		}

		start = end + 1;
		end = input.find_first_of("[]", start);
	}

	if (start < input.length()) {
		tokens.push_back(input.substr(start));
	}

	if (tokens.size() < 2) {
		//oapiDebugString("Invalid message format");
		return;
	}

	const std::string& command = tokens.at(0);
	const std::string& payload = tokens.at(1);

	if (command == "CHAT")
		HandleChatMessage(tokens.at(1).c_str());

	else if (command == "SW")
	{
		std::string logMessage = "<Recv>: " + input;
		if (isHost())
			LogHost(logMessage.c_str());
		if (isClient())
			LogClient(logMessage.c_str());

		if (strcmp(lastSent, recvBuffer))
			HandleSwitchMessage(tokens.at(1).c_str());
	}

	else if (command == "DL")
		HandleDskyLightMessage(tokens.at(1).c_str());

	else if (command == "DR")
		HandleDskyRegMessage(tokens.at(1).c_str());

	else if (command == "MJD")
	{
		HandleMJDMessage(tokens.at(1).c_str());
	}

	else if (command == "TREQ")
		SendMJD();

	else if (command == "SEAT")
		HandleSeatMessage(tokens.at(1).c_str());

	else if (command == "FDAI")
		HandleFdaiMessage(tokens.at(1).c_str());

	else if (command == "SV")
	{
		std::string logMessage = "<Recv>: " + input;
		if (isHost())
			LogHost(logMessage.c_str());
		if (isClient())
			LogClient(logMessage.c_str());
		//oapiWriteLog("Received state vector message");
		if (SIVBSeparated)
			HandleStateVectorMessage(tokens.at(1).c_str());
	}

	else if (command == "TA")
		HandleTimeAccelMessage(tokens.at(1).c_str());

	else if (command == "RHC")
		HandleRhcMessage(tokens.at(1).c_str());
	else if (command == "THC")
		HandleThcMessage(tokens.at(1).c_str());
	else if (command == "ACA")
		HandleAcaMessage(tokens.at(1).c_str());
	else if (command == "TTCA")
		HandleTtcaMessage(tokens.at(1).c_str());
	else if (command == "RCS")
		HandleRCSSoundMessage(tokens.at(1).c_str());
	else if (command == "TH")
	{
		std::string logMessage = "<Recv>: " + input;
		if (isHost())
			LogHost(logMessage.c_str());
		if (isClient())
			LogClient(logMessage.c_str());
		HandleThrusterMessage(tokens.at(1).c_str());
	}
	else if (command == "LAND")
	{
		std::string logMessage = "<Recv>: " + input;
		if (isHost())
			LogHost(logMessage.c_str());
		if (isClient())
			LogClient(logMessage.c_str());
		//HandleLandedMessage(tokens.at(1).c_str());
	}
	else if (command == "RAD")
	{
		std::string logMessage = "<Recv>: " + input;
		if (isClient())
			LogClient(logMessage.c_str());
		HandleRadarMessage(tokens.at(1).c_str());
	}
	else if (command == "XP")
	{
		std::string logMessage = "<Recv>: " + input;
		if (isClient())
			LogClient(logMessage.c_str());
		HandleCrossPointerMessage(tokens.at(1).c_str());
	}
		
	else
	{
		oapiDebugString("Unknown command received");
	}
}

void Multiplayer::HandleChatMessage(const char* message)
{
	onChatMessage(message);
}

void Multiplayer::HandleSwitchMessage(const char* message)
{
	std::string input = message;
	
	// Tokenize the switch command
	std::vector<std::string> tokens;
	size_t start = 0;
	size_t end = input.find_first_of(",");

	while (end != std::string::npos) {
		if (end > start)
		{
			tokens.push_back(input.substr(start, end - start));
		}

		start = end + 1;
		end = input.find_first_of(",", start);
	}

	if (start < input.length()) {
		tokens.push_back(input.substr(start));
	}

	if (tokens.size() < 5) {
		oapiDebugString("Invalid switch message format");
		return;
	}

	const std::string vesselName = tokens.at(0);
	const std::string name = tokens.at(1);
	const std::string state = tokens.at(2);
	const std::string guard = tokens.at(3);
	const std::string hold = tokens.at(4);

	// DEBOUNCE
	// CLIENT SIDE
	if (ZMQData.status == ZeroMQStatus::CLIENT)
	{
		if (lastSwitch == name && lastSwitchState == std::stoi(state)) 
		{
			return;
		}
			
	}

	// HOST SIDE
	if (ZMQData.status == ZeroMQStatus::HOST)
	{
		if (lastSwitch == name && lastSwitchState == std::stoi(state))
		{
			return;
		}

		SendSwitch(vesselName.c_str(), name.c_str(), std::stoi(state.c_str()), std::stoi(guard.c_str()), std::stoi(hold.c_str()));
	}

	if (vesselName == "SATURN")
	{
		conn.SetState((char*)name.c_str(), std::stoi(state.c_str()), std::stoi(guard.c_str()), std::stoi(hold.c_str()));
	}
	else if (vesselName == "LEM")
	{
		lemConn.SetState((char*)name.c_str(), std::stoi(state.c_str()), std::stoi(guard.c_str()), std::stoi(hold.c_str()));
	}
}

void Multiplayer::HandleDskyLightMessage(const char* message)
{
	std::string input = message;

	// Tokenize the switch command
	std::vector<std::string> tokens;
	size_t start = 0;
	size_t end = input.find_first_of(",");

	while (end != std::string::npos) {
		if (end > start)
		{
			tokens.push_back(input.substr(start, end - start));
		}

		start = end + 1;
		end = input.find_first_of(",", start);
	}

	if (start < input.length()) {
		tokens.push_back(input.substr(start));
	}

	if (tokens.size() < 3) {
		oapiDebugString("Invalid switch message format");
		return;
	}

	const std::string& dsky = tokens.at(0);
	const std::string& name = tokens.at(1);
	const std::string& state = tokens.at(2);

	if (dsky == "CSM")
	{
		csmDskyConn.SetState((char*)name.c_str(), std::stoi(state));
	}
	else if (dsky == "LEM")
	{
		lemDskyConn.SetState((char*)name.c_str(), std::stoi(state));
	}
}

void Multiplayer::HandleDskyRegMessage(const char* message)
{
	std::string input = message;

	// Tokenize the switch command
	std::vector<std::string> tokens;
	size_t start = 0;
	size_t end = input.find_first_of(",");

	while (end != std::string::npos) {
		if (end > start)
		{
			tokens.push_back(input.substr(start, end - start));
		}

		start = end + 1;
		end = input.find_first_of(",", start);
	}

	if (start < input.length()) {
		tokens.push_back(input.substr(start));
	}

	if (tokens.size() < 3) {
		oapiDebugString("Invalid switch message format");
		return;
	}

	const std::string& dsky = tokens.at(0);
	const std::string& name = tokens.at(1);
	const std::string& state = tokens.at(2);

	if (dsky == "CSM")
	{
		csmDskyConn.SetState((char*)name.c_str(), (char*)state.c_str());
	}
	else if (dsky == "LEM")
	{
		lemDskyConn.SetState((char*)name.c_str(), (char*)state.c_str());
	}
}

void Multiplayer::HandleMJDMessage(const char* message)
{
	double mjdNew = stod(message);
	oapiSetSimMJD(mjdNew);
	conn.SyncMissionTime();
	lemConn.SyncMissionTime();
}

void Multiplayer::HandleSeatMessage(const char* message)
{
	bool state = stoi(message);
	//conn.SetVCState(state);
}

void Multiplayer::HandleFdaiMessage(const char* message)
{
	std::string input = message;

	// Tokenize the switch command
	std::vector<std::string> tokens;
	size_t start = 0;
	size_t end = input.find_first_of(",");

	while (end != std::string::npos) {
		if (end > start)
		{
			tokens.push_back(input.substr(start, end - start));
		}

		start = end + 1;
		end = input.find_first_of(",", start);
	}

	if (start < input.length()) {
		tokens.push_back(input.substr(start));
	}

	if (tokens.size() < 3) {
		oapiDebugString("Invalid FDAI message format");
		return;
	}
	 
	int type = stoi(tokens.at(0));
	VECTOR3 fdai = { stod(tokens.at(1)), stod(tokens.at(2)), stod(tokens.at(3)) };

	// CSM FDAI
	if (type < 6)
		conn.SyncFDAI(type, fdai);
	// LEM FDAI
	else
		lemConn.SyncFDAI(type, fdai);
}

void Multiplayer::HandleStateVectorMessage(const char* message)
{
	std::string input = message;

	// Tokenize the switch command
	std::vector<std::string> tokens;
	size_t start = 0;
	size_t end = input.find_first_of(",");

	while (end != std::string::npos) {
		if (end > start)
		{
			tokens.push_back(input.substr(start, end - start));
		}

		start = end + 1;
		end = input.find_first_of(",", start);
	}

	if (start < input.length()) {
		tokens.push_back(input.substr(start));
	}

	if (tokens.size() < 18) {
		oapiDebugString("Invalid SV message format");
		return;
	}

	StateVector sv;
	
	sv.name = tokens.at(0);
	sv.rbody = tokens.at(1);
	sv.pos = { stod(tokens.at(2)), stod(tokens.at(3)), stod(tokens.at(4)) };
	sv.vel = { stod(tokens.at(5)), stod(tokens.at(6)), stod(tokens.at(7)) };
	sv.apos = { stod(tokens.at(8)), stod(tokens.at(9)), stod(tokens.at(10)) };
	sv.avel = { stod(tokens.at(11)), stod(tokens.at(12)), stod(tokens.at(13)) };
	sv.tvec = { stod(tokens.at(14)), stod(tokens.at(15)), stod(tokens.at(16)) };
	sv.mjd = stod(tokens.at(17));

	// Remove whitespace from rbody
	if (!sv.rbody.empty() && sv.rbody.front() == ' ')
		sv.rbody.erase(0, 1);

	// Apply orientation and angular veocity
	OBJHANDLE hVes = oapiGetVesselByName(const_cast<char*>(sv.name.c_str()));
	VESSEL* ves = oapiGetVesselInterface(hVes);

	ves->SetGlobalOrientation(sv.apos);
	ves->SetAngularVel(sv.avel);

	// Erase the last state vector from the list
	for (size_t i = 0; i < lastStateVectors.size(); i++)
	{
		StateVector& lsv = lastStateVectors.at(i);
		if (lsv.name == sv.name)
		{
			lastStateVectors.erase(lastStateVectors.begin() + i);
		}
	}

	// Add state vector to the list
	for (size_t i = 0; i < stateVectors.size(); i++)
	{
		if (stateVectors[i].name == sv.name)
		{
			lastStateVectors.push_back(stateVectors.at(i));
			stateVectors.erase(stateVectors.begin() + i);
		}
	}

	stateVectors.push_back(sv);
}

void Multiplayer::HandleTimeAccelMessage(const char* message)
{
	double timeAccel = stod(message);
	oapiSetTimeAcceleration(timeAccel);
	//oapiDebugString("Time Acceleration Changed");
}

void Multiplayer::HandleRhcMessage(const char* message)
{
	std::string input = message;

	// Tokenize the switch command
	std::vector<std::string> tokens;
	size_t start = 0;
	size_t end = input.find_first_of(",");

	while (end != std::string::npos) {
		if (end > start)
		{
			tokens.push_back(input.substr(start, end - start));
		}

		start = end + 1;
		end = input.find_first_of(",", start);
	}

	if (start < input.length()) {
		tokens.push_back(input.substr(start));
	}

	if (tokens.size() < 3) {
		oapiDebugString("Invalid RHC message format");
		return;
	}

	VECTOR3 rhc = { stod(tokens.at(0)), stod(tokens.at(1)), stod(tokens.at(2)) };

	conn.SendRhc(rhc);
}

void Multiplayer::HandleThcMessage(const char* message)   
{
	std::string input = message;

	// Tokenize the switch command
	std::vector<std::string> tokens;
	size_t start = 0;
	size_t end = input.find_first_of(",");

	while (end != std::string::npos) {
		if (end > start)
		{
			tokens.push_back(input.substr(start, end - start));
		}

		start = end + 1;
		end = input.find_first_of(",", start);
	}

	if (start < input.length()) {
		tokens.push_back(input.substr(start));
	}

	if (tokens.size() < 3) {
		oapiDebugString("Invalid THC message format");
		return;
	}

	VECTOR3 thc = { stod(tokens.at(0)), stod(tokens.at(1)), stod(tokens.at(2)) };

	conn.SendThc(thc);
}

void Multiplayer::HandleAcaMessage(const char* message)
{
	std::string input = message;

	// Tokenize the switch command
	std::vector<std::string> tokens;
	size_t start = 0;
	size_t end = input.find_first_of(",");

	while (end != std::string::npos) {
		if (end > start)
		{
			tokens.push_back(input.substr(start, end - start));
		}

		start = end + 1;
		end = input.find_first_of(",", start);
	}

	if (start < input.length()) {
		tokens.push_back(input.substr(start));
	}

	if (tokens.size() < 3) {
		oapiDebugString("Invalid ACA message format");
		return;
	}

	VECTOR3 aca = { stod(tokens.at(0)), stod(tokens.at(1)), stod(tokens.at(2)) };

	lemConn.SendAca(aca);
}

void Multiplayer::HandleTtcaMessage(const char* message)
{
	std::string input = message;

	// Tokenize the switch command
	std::vector<std::string> tokens;
	size_t start = 0;
	size_t end = input.find_first_of(",");

	while (end != std::string::npos) {
		if (end > start)
		{
			tokens.push_back(input.substr(start, end - start));
		}

		start = end + 1;
		end = input.find_first_of(",", start);
	}

	if (start < input.length()) {
		tokens.push_back(input.substr(start));
	}

	if (tokens.size() < 3) {
		oapiDebugString("Invalid TTCA message format");
		return;
	}

	VECTOR3 ttca = { stod(tokens.at(0)), stod(tokens.at(1)), stod(tokens.at(2)) };

	lemConn.SendTtca(ttca);
}

void Multiplayer::HandleRCSSoundMessage(const char* message)
{
	std::string input = message;

	// Tokenize the switch command
	std::vector<std::string> tokens;
	size_t start = 0;
	size_t end = input.find_first_of(",");

	while (end != std::string::npos) {
		if (end > start)
		{
			tokens.push_back(input.substr(start, end - start));
		}

		start = end + 1;
		end = input.find_first_of(",", start);
	}

	if (start < input.length()) {
		tokens.push_back(input.substr(start));
	}

	if (tokens.size() < 2) {
		oapiDebugString("Invalid RCS sound message format");
		return;
	}

	bool sound = stoi(tokens.at(0));
	bool sustain = stoi(tokens.at(1));

	conn.SendRCSSound(sound, sustain);
}

void Multiplayer::HandleThrusterMessage(const char* message)
{
	std::string input = message;

	// Tokenize the switch command
	std::vector<std::string> tokens;
	size_t start = 0;
	size_t end = input.find_first_of(",");

	while (end != std::string::npos) {
		if (end > start)
		{
			tokens.push_back(input.substr(start, end - start));
		}

		start = end + 1;
		end = input.find_first_of(",", start);
	}

	if (start < input.length()) {
		tokens.push_back(input.substr(start));
	}

	if (tokens.size() < 3) {
		oapiDebugString("Invalid Thruster message format");
		return;
	}

	std::string name = tokens.at(0);
	int index = stoi(tokens.at(1));
	double value = stod(tokens.at(2));

	if (name == "th_rcs" || name == "th_dust" || name == "th_hover")
		lemConn.SendThruster(name.c_str(), index, value);
	else
		conn.SendThruster(name.c_str(), index, value);
}

void Multiplayer::HandleLandedMessage(const char* message)
{
	std::string input = message;

	// Tokenize the switch command
	std::vector<std::string> tokens;
	size_t start = 0;
	size_t end = input.find_first_of(",");

	while (end != std::string::npos) {
		if (end > start)
		{
			tokens.push_back(input.substr(start, end - start));
		}

		start = end + 1;
		end = input.find_first_of(",", start);
	}

	if (start < input.length()) {
		tokens.push_back(input.substr(start));
	}

	if (tokens.size() < 1) {
		oapiDebugString("Invalid LEM Landed message format");
		return;
	}

	bool state = stoi(tokens.at(0));

	lemConn.SendLanded(state);
}

void Multiplayer::HandleRadarMessage(const char* message)
{
	std::string input = message;

	// Tokenize the switch command
	std::vector<std::string> tokens;
	size_t start = 0;
	size_t end = input.find_first_of(",");

	while (end != std::string::npos) {
		if (end > start)
		{
			tokens.push_back(input.substr(start, end - start));
		}

		start = end + 1;
		end = input.find_first_of(",", start);
	}

	if (start < input.length()) {
		tokens.push_back(input.substr(start));
	}

	if (tokens.size() < 3) {
		oapiDebugString("Invalid LEM radar message format");
		return;
	}

	ConnectorMessage cm;
	cm.destination = MP_PANEL_INTERFACE;
	cm.messageType = MP_PanelConnector::MP_PANEL_SET_RADAR;
	cm.val1.dValue = stod(tokens.at(0));
	cm.val2.dValue = stod(tokens.at(1));
	cm.val3.dValue = stod(tokens.at(2));
	lemConn.SendMessage(cm);
}

void Multiplayer::HandleCrossPointerMessage(const char* message)
{
	std::string input = message;

	// Tokenize the switch command
	std::vector<std::string> tokens;
	size_t start = 0;
	size_t end = input.find_first_of(",");

	while (end != std::string::npos) {
		if (end > start)
		{
			tokens.push_back(input.substr(start, end - start));
		}

		start = end + 1;
		end = input.find_first_of(",", start);
	}

	if (start < input.length()) {
		tokens.push_back(input.substr(start));
	}

	if (tokens.size() < 6) {
		oapiDebugString("Invalid LEM cross pointer message format");
		return;
	}

	VECTOR3 left = { stod(tokens.at(0)), stod(tokens.at(1)), stod(tokens.at(2)) };
	VECTOR3 right = { stod(tokens.at(3)), stod(tokens.at(4)), stod(tokens.at(5)) };

	ConnectorMessage cm;
	cm.destination = MP_PANEL_INTERFACE;
	cm.messageType = MP_PanelConnector::MP_PANEL_SET_CROSSPOINTER;
	cm.val1.vValue = left;
	cm.val2.vValue = right;
	lemConn.SendMessage(cm);
}

void Multiplayer::Disconnect()
{
	conn.SetIsClient(false);
	
	if (ZMQData.socket_implicit != nullptr) {
		ZMQData.socket_implicit->close();
		delete ZMQData.socket_implicit;
		ZMQData.socket_implicit = nullptr;
	}

	if (ZMQData.socket_explicit != nullptr) {
		ZMQData.socket_explicit->close();
		delete ZMQData.socket_explicit;
		ZMQData.socket_explicit = nullptr;
	}

	if (ZMQData.context != nullptr) {
		ZMQData.context->close();
		delete ZMQData.context;
		ZMQData.context = nullptr;
	}
	
	ZMQData.status = ZeroMQStatus::DISCONNECTED;

	oapiDebugString("Disconnected");
}

Multiplayer::~Multiplayer()
{
	Disconnect();
	oapiCloseFile(mpLogFileClient, FILE_OUT);
	oapiCloseFile(mpLogFileHost, FILE_OUT);
}

void Multiplayer::LogClient(const char* message)
{
	std::string buf = "[CLIENT " + std::to_string(oapiGetSimTime()) + "] ";
	buf += message;

	if (mpLogFileClient) {
		oapiWriteLine(mpLogFileClient, const_cast<char*>(buf.c_str()));
	}
}

void Multiplayer::LogHost(const char* message)
{
	std::string buf = "[HOST " + std::to_string(oapiGetSimTime()) + "] ";
	buf += message;

	if (mpLogFileHost) {
		oapiWriteLine(mpLogFileHost, const_cast<char*>(buf.c_str()));
	}
}

bool Multiplayer::isHost()
{
	if (ZMQData.status == ZeroMQStatus::HOST) {
		return true;
	}
	
	return false;
}

bool Multiplayer::isClient()
{
	if (ZMQData.status == ZeroMQStatus::CLIENT) {
		return true;
	}
	
	return false;
}

char* Multiplayer::getStatusString()
{
	std::string statusStr;
	
	if (ZMQData.status == ZeroMQStatus::HOST) {
		statusStr = "Hosting";
	}
	else if (ZMQData.status == ZeroMQStatus::CLIENT) {
		statusStr = "Client";
	}
	else if (ZMQData.status == ZeroMQStatus::DISCONNECTED) {
		statusStr = "Disconnected";
	}
	else
		statusStr = "Idle";

	if (ZMQData.useLAN) {
		statusStr += " (LAN)";
	}
	else {
		statusStr += " (WAN)";
	}

	return const_cast<char*>(statusStr.c_str());
}

bool Multiplayer::SetIP(const char* cIP)
{
	address = cIP;
	return true;
}

bool Multiplayer::SetPORT(const char* cPort)
{
	port = stoi(cPort);
	return true;
}

const char* Multiplayer::getIP()
{
	char cIp[32];
	strcpy(cIp, address.c_str());
	return cIp;
}

const char* Multiplayer::getPORT()
{
	char cPort[32];
	strcpy(cPort, std::to_string(port).c_str());
	return cPort;
}

bool Multiplayer::SendChat(const char* message)
{
	if (ZMQData.status == ZeroMQStatus::CLIENT)
	{ 
		char buf[512];
		sprintf(buf, "[CHAT]Chat(Server): %s", message);
		SendExplicit(buf);
		sentBytes += strlen(buf);
	}

	else if (ZMQData.status == ZeroMQStatus::HOST)
	{
		char buf[512];
		sprintf(buf,"[CHAT]Chat(Server): %s", message);
		Broadcast(buf);
		sentBytes += strlen(buf);
	}

	return true;
}

bool Multiplayer::SendSwitch(const char* vessel, const char* name, int state, bool guard, bool hold)
{
	lastSwitch = name;
	lastSwitchState = state;

	// Control the sending of EMSDvDisplay messages to avoid flooding the network with too many messages
	if (!strcmp(name, "EMSDvDisplay"))
	{
		if (!sendEMS)
		{
			emsDvs.push_back(state);
			return true;
		}

		else
			sendEMS = false;
	}

	// hold off on sending any state vectors for a few seconds to allow the SIVB to separate and stabilize
	if (!strcmp(name, "CsmLvSepSwitch") && state == 1)
	{	
		if (!SIVBSepTriggerrd)
		{
			SIVBSepTime = oapiGetSimTime();
		}
		SIVBSepTriggerrd = true;
	}

	
	if (ZMQData.status == ZeroMQStatus::CLIENT)
	{
		char buf[512];
		sprintf(buf, "[SW]%s,%s,%d, %d, %d", vessel, name, state, guard, 1);
		SendExplicit(buf);
		sentBytes += strlen(buf);
		std::string logMsg = "<SEND>: " + std::string(buf);
		LogClient(logMsg.c_str());
		return true;
	}
	else if (ZMQData.status == ZeroMQStatus::HOST)
	{
		char buf[512];
		sprintf(buf, "[SW]%s,%s,%d, %d, %d", vessel, name, state, guard, 1);
		Broadcast(buf);
		sentBytes += strlen(buf);
		std::string logMsg = "<SEND>: " + std::string(buf);
		LogHost(logMsg.c_str());
		return true;
	}

	
	return false;
}

bool Multiplayer::SendDskyLight(const char* dsky, const char* name, bool state)
{
	if ((SIVBSepTriggerrd && oapiGetSimTime() < SIVBSepTime + SIVBSepDuration)) return false;

	if (ZMQData.status == ZeroMQStatus::HOST)
	{
		char buf[512];
		sprintf(buf, "[DL]%s,%s,%d", dsky, name, state);
		Broadcast(buf);
		sentBytes += strlen(buf);
		std::string logMsg = "<SEND>: " + std::string(buf);
		LogHost(logMsg.c_str());
		return true;
	}
	
	return false;
}

bool Multiplayer::SendDskyReg(const char* dsky,const char* name, const char* state)
{
	if ((SIVBSepTriggerrd && oapiGetSimTime() < SIVBSepTime + SIVBSepDuration)) return false;

	if (ZMQData.status == ZeroMQStatus::HOST)
	{
		char buf[512];
		sprintf(buf, "[DR]%s,%s,%s", dsky, name, state);
		Broadcast(buf);
		sentBytes += strlen(buf);
		std::string logMsg = "<SEND>: " + std::string(buf);
		LogHost(logMsg.c_str());
		return true;
	}
	
	return false;
}

bool Multiplayer::SendMJD()
{
	if (ZMQData.status == ZeroMQStatus::HOST)
	{
		char buf[512];
		sprintf(buf, "[MJD]%.6f", oapiGetSimMJD());
		Broadcast(buf);
		sentBytes += strlen(buf);
		std::string logMsg = "<SEND>: " + std::string(buf);
		LogHost(logMsg.c_str());
		return true;
	}
	return false;
}

bool Multiplayer::SendTimeSyncReq()
{
	if (ZMQData.status == ZeroMQStatus::CLIENT)
	{
		char buf[512];
		sprintf(buf, "[TREQ] ");
		SendExplicit(buf);
		sentBytes += strlen(buf);
		std::string logMsg = "<SEND>: " + std::string(buf);
		LogHost(logMsg.c_str());
		return true;
	}
	return false;
}

bool Multiplayer::SendToggleSeats(bool bYN)
{
	if (ZMQData.status == ZeroMQStatus::CLIENT)
	{
		char buf[512];
		sprintf(buf, "[SEAT]%d", bYN);
		SendExplicit(buf);
		sentBytes += strlen(buf);
		std::string logMsg = "<SEND>: " + std::string(buf);
		LogHost(logMsg.c_str());
		return true;
	}
	else if (ZMQData.status == ZeroMQStatus::HOST)
	{
		char buf[512];
		sprintf(buf, "[SEAT]%d", bYN);
		Broadcast(buf);
		sentBytes += strlen(buf);
		std::string logMsg = "<SEND>: " + std::string(buf);
		LogHost(logMsg.c_str());
		return true;
	}
	return false;
}

bool Multiplayer::SendFDAI(int type, VECTOR3 fdai)
{
	if ((SIVBSepTriggerrd && oapiGetSimTime() < SIVBSepTime + SIVBSepDuration)) return false;
	
	if (ZMQData.status == ZeroMQStatus::HOST)
	{
		char buf[512];
		sprintf(buf, "[FDAI]%d, %f, %f, %f", type, fdai.x, fdai.y, fdai.z);
		Broadcast(buf);
		sentBytes += strlen(buf);
		std::string logMsg = "<SEND>: " + std::string(buf);
		LogHost(logMsg.c_str());
		return true;
	}
	return false;
}

bool Multiplayer::SendStateVector(const char* craft)
{
	if ((SIVBSepTriggerrd && oapiGetSimTime() < SIVBSepTime + SIVBSepDuration)) return false;
	
	if (!strcmp(craft, "Floodlight") || !strcmp(craft, "MCC")) return false;
	
	StateVector sv;

	VESSEL* ves = oapiGetVesselInterface(oapiGetVesselByName(const_cast<char*>(craft)));
	
	if (!ves) return false;

	VESSELSTATUS vs;
	ves->GetStatus(vs);

	sv.name = craft;
	char buf[512];
	oapiGetObjectName(vs.rbody, buf, 128);
	sv.rbody = buf;
	oapiGetRelativePos(ves->GetHandle(), vs.rbody, &sv.pos);
	oapiGetRelativeVel(ves->GetHandle(), vs.rbody, &sv.vel);
	ves->GetGlobalOrientation(sv.apos);
	ves->GetAngularVel(sv.avel);
	ves->GetThrustVector(sv.tvec);
	sv.mjd = oapiGetSimTime();

	sprintf(buf, "[SV]%s, %s, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f", sv.name.c_str(), sv.rbody.c_str(), sv.pos.x, sv.pos.y, sv.pos.z, sv.vel.x, sv.vel.y, sv.vel.z, sv.apos.x, sv.apos.y, sv.apos.z, sv.avel.x, sv.avel.y, sv.avel.z, sv.tvec.x, sv.tvec.y, sv.tvec.z, sv.mjd);
	Broadcast(buf);
	sentBytes += strlen(buf);
	std::string logMsg = "<SEND>: " + std::string(buf);
	LogHost(logMsg.c_str());
	return true;
}

bool Multiplayer::SendTimeAccel(double accel)
{
		char buf[512];
		sprintf(buf, "[TA]%f", accel);
		
		if (ZMQData.status == ZeroMQStatus::HOST)
			Broadcast(buf);
		else if (ZMQData.status == ZeroMQStatus::CLIENT)
			SendExplicit(buf);
		sentBytes += strlen(buf);
		std::string logMsg = "<SEND>: " + std::string(buf);
		LogHost(logMsg.c_str());
		return true;

	return false;
}

bool Multiplayer::SendRhc(VECTOR3 rhc)
{
	char buf[512];
	if (ZMQData.status == ZeroMQStatus::CLIENT)
	{
		sprintf(buf, "[RHC]%f,%f,%f", rhc.x, rhc.y, rhc.z);
		SendExplicit(buf);
		sentBytes += strlen(buf);
		std::string logMsg = "<SEND>: " + std::string(buf);
		LogClient(logMsg.c_str());
		return true;
	}
	return false;
}

bool Multiplayer::SendThc(VECTOR3 thc)
{
	char buf[512];
	if (ZMQData.status == ZeroMQStatus::CLIENT)
	{
		sprintf(buf, "[THC]%f,%f,%f", thc.x, thc.y, thc.z);
		SendExplicit(buf);
		sentBytes += strlen(buf);
		std::string logMsg = "<SEND>: " + std::string(buf);
		LogClient(logMsg.c_str());
		return true;
	}
	return false;
}

bool Multiplayer::SendAca(VECTOR3 aca)
{
	char buf[512];
	if (ZMQData.status == ZeroMQStatus::CLIENT)
	{
		sprintf(buf, "[ACA]%f,%f,%f", aca.x, aca.y, aca.z);
		SendExplicit(buf);
		sentBytes += strlen(buf);
		std::string logMsg = "<SEND>: " + std::string(buf);
		LogClient(logMsg.c_str());
		return true;
	}
	return false;
}

bool Multiplayer::SendTtca(VECTOR3 ttca)
{
	char buf[512];
	if (ZMQData.status == ZeroMQStatus::CLIENT)
	{
		sprintf(buf, "[TTCA]%f,%f,%f", ttca.x, ttca.y, ttca.z);
		SendExplicit(buf);
		sentBytes += strlen(buf);
		std::string logMsg = "<SEND>: " + std::string(buf);
		LogClient(logMsg.c_str());
		return true;
	}
	return false;
}

bool Multiplayer::SendRCSSound(bool sound, bool sustain)
{
	char buf[512];
	if (ZMQData.status == ZeroMQStatus::HOST)
	{
		sprintf(buf, "[RCS]%d,%d", sound ? 1 : 0, sustain ? 1 : 0);
		Broadcast(buf);
		sentBytes += strlen(buf);
		std::string logMsg = "<SEND>: " + std::string(buf);
		LogHost(logMsg.c_str());
		return true;
	}
	return false;
}

bool Multiplayer::SendThruster(const char* name, int index, double value)
{
	char buf[512];
	if (ZMQData.status == ZeroMQStatus::HOST)
	{
		sprintf(buf, "[TH]%s,%d,%f", name, index, value);
		Broadcast(buf);
		sentBytes += strlen(buf);
		std::string logMsg = "<SEND>: " + std::string(buf);
		LogHost(logMsg.c_str());
		return true;
	}
	return false;
}

bool Multiplayer::SendLanded(bool state)
{
	char buf[512];
	if (ZMQData.status == ZeroMQStatus::HOST)
	{
		sprintf(buf, "[LAND]%b", state);
		Broadcast(buf);
		sentBytes += strlen(buf);
		std::string logMsg = "<SEND>: " + std::string(buf);
		LogHost(logMsg.c_str());
		return true;
	}
	return false;
}

bool Multiplayer::SendRadar(double range, double rate, double light)
{
	if (!sendRadar)
	{
		radars.push_back({range, rate, light});
		return true;
	}
	
	sendRadar = false;

	char buf[512];
	if (ZMQData.status == ZeroMQStatus::HOST)
	{
		sprintf(buf, "[RAD]%f,%f,%f", range, rate, light);
		Broadcast(buf);
		sentBytes += strlen(buf);
		std::string logMsg = "<SEND>: " + std::string(buf);
		LogHost(logMsg.c_str());
		return true;
	}
	return false;
}

bool Multiplayer::SendCrossPointer(VECTOR3 left, VECTOR3 right)
{
	if (!sendCrossPointer)
	{
		crossPointers.push_back({ left, right });
		return true;
	}

	sendRadar = false;

	char buf[512];
	if (ZMQData.status == ZeroMQStatus::HOST)
	{
		sprintf(buf, "[XP]%f,%f,%f,%f,%f,%f", left.x, left.y, left.z, right.x, right.y, right.z);
		Broadcast(buf);
		sentBytes += strlen(buf);
		std::string logMsg = "<SEND>: " + std::string(buf);
		LogHost(logMsg.c_str());
		return true;
	}
	return false;
}
