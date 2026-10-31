/*
* Modifications and additions for NASSP Multiplayer (unofficial fork):
* MULTIPLAYER DSKY CONNECTOR
* Copyright (c) 2026 Cody Kerkove
*
* SPDX-License-Identifier: GPL-2.0-or-later
*/


#ifndef MULTIPLAYER_H_
#define MULTIPLAYER_H_

#include <zmq.hpp>
#include <functional>

#include "connector.h"
#include "MPconnector.h"
#include "MPDskyConnector.h"

#include <map>


class Saturn;
class LEM;
class Skylab;
class Crawler;


class Multiplayer
{
public:
	using Callback = std::function<void(const char*)>;

	static Multiplayer& GetInstance();
	
	static struct StateVector
	{
		std::string name;
		std::string rbody;
		VECTOR3 pos;
		VECTOR3 vel;
		VECTOR3 apos;
		VECTOR3 avel;
		double mjd;
	};

	// Initialization
	const char* Host();
	const char* Join();

	// Update
	void Update(double simt, double simdt, double mjd);
	void UpdatePost(double simt, double simdt, double mjd);

	// Messaging
	void Broadcast(const char* message);
	bool ReceiveBroadcast();
	
	bool SendExplicit(const char* message);
	bool ReceiveExplicit();
	
	const char* getReceivedMessage() { return recvBuffer; }
	double getSentMb() { return sentBytes / 1024.0 / 1024.0; }
	
	// Message Handling
	void HandleASCIIMessage();
	void HandleChatMessage(const char* message);
	void HandleSwitchMessage(const char* message);
	void HandleDskyLightMessage(const char* message);
	void HandleDskyRegMessage(const char* message);
	void HandleMJDMessage(const char* message);
	void HandleSeatMessage(const char* message);
	void HandleFdaiMessage(const char* message);
	void HandleStateVectorMessage(const char* message);
	void HandleTimeAccelMessage(const char* message);
	void HandleRhcMessage(const char* message);
	void HandleThcMessage(const char* message);
	void HandleRCSSoundMessage(const char* message);
	void HandleThrusterMessage(const char* message);
	void HandleLandedMessage(const char* message);

	// Deinitialization
	void Disconnect();

	// Status
	bool isHost();
	bool isClient();
	char* getStatusString();

	// Host/Port
	bool SetIP(const char* cIP);
	bool SetPORT(const char* cPort);
	const char* getIP();
	const char* getPORT();

	// Senders
	bool SendChat(const char* message);
	bool SendSwitch(const char* vessel, const char* name, int state, bool guard, bool hold);
	bool SendDskyLight(const char* name, bool state);
	bool SendDskyReg(const char* name, const char* state);
	bool SendMJD();
	bool SendTimeSyncReq();
	bool SendToggleSeats(bool bYN);
	bool SendFDAI(int type, VECTOR3 fdai);
	bool SendStateVector(const char* craft);
	bool SendTimeAccel(double accel);
	bool SendRhc(VECTOR3 rhc);
	bool SendThc(VECTOR3 thc);
	bool SendRCSSound(bool sound, bool sustain);
	bool SendThruster(const char* name, int index, double value);
	bool SendLanded(bool state);

	// MFD Callback Registration
	void RegisterChatMessageCllbk(Callback callback) { onChatMessage = callback; }

	// Vessel
	void UpdateVessel(VESSEL* newVessel);
	VESSEL* getVessel();

	void SyncStateVector(const Multiplayer::StateVector& sv);
	void SetSIVBSeparated(bool separated) { SIVBSeparated = separated; }

	
private:
	Multiplayer();// Private constructor
	~Multiplayer();

	static enum ZeroMQStatus {
		DISCONNECTED,
		HOST,
		CLIENT
	};
	
	struct ZeroMQData {
		zmq::context_t* context;
		
		zmq::socket_t* socket_implicit;	// Implicit socket (Server Pub/Sub)
		zmq::socket_t* socket_explicit;	// Explicit socket (Client Req/Rep)

		Multiplayer::ZeroMQStatus status;
	} ZMQData;

	std::string address;
	int port;

	// ZeroMQ Data
	char recvBuffer[512];
	char lastSent[512];
	long sentBytes = 0;
	
	double TimeOfSyncRequest = 1e100;
	
	// Switch Sync
	double lastSendSwitchSimt;
	double sendSwitchInterval = 0.2;

	// Time Acceleration
	double lastTimeAccel = 1.0;

	// EMS Sync
	double lastEMSDv = 0.0;
	double lastSendEMSSimt = 0.0;
	bool sendEMS = false;

	// SIVB Separation
	bool SIVBSepTriggerrd = false;
	double SIVBSepTime = 0.0;
	double SIVBSepDuration = 5.0;
	bool SIVBSeparated = false;

	// Switch Debounce
	std::string lastSwitch;
	int lastSwitchState;

	// MFD Callbacks
	Callback onChatMessage;

	// Connector
	MPConnector conn;
	MPDskyConnector csmDskyConn;
	MPConnector lemConn;
	MPDskyConnector lemDskyConn;

	// VESSEL
	Saturn* saturn;
	LEM* lem;
	Skylab* skylab;
	Crawler* crawler;
	OBJHANDLE planet;
	VESSEL* vessel;

	// STATE VECTOR SYNC
	std::vector<StateVector> lastStateVectors;
	std::vector<StateVector> stateVectors;
	double lastSendSVSimt;
	double sendSVInterval = 0.2;

	// LOGGIN'
	FILEHANDLE mpLogFileClient;
	FILEHANDLE mpLogFileHost;
	void  LogClient(const char* message);
	void  LogHost(const char* message);
};

#endif
