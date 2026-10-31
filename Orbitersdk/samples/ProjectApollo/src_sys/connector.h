/***************************************************************************
  This file is part of Project Apollo - NASSP
  Copyright 2004-2005 Jean-Luc Rocca-Serra, Mark Grant

  ORBITER vessel module: Connector class

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
  * CONNECTOR CLASS
  * Copyright (c) 2026 Cody Kerkove
  *
  * This file contains code adapted from the following NASSP file(s):
  *   - connector.h (Connector Class)
  * 
  * Changes: added multiplayer state synchronization
  *		     (2026-10)
  *
  * SPDX-License-Identifier: GPL-2.0-or-later
  */

#if !defined(_PA_CONNECTOR_H)
#define _PA_CONNECTOR_H

#include "VesselAPI.h"

#include <limits>

///
/// \ingroup Connectors
/// \brief Connector type.
///
enum ConnectorType
{
	NO_CONNECTION,				///< Dummy.
	CSM_IU_COMMAND,				///< Passes commands and data between CSM and IU.
	LV_IU_COMMAND,				///< Passes commands between launch vehicle and IU.
	CSM_LEM_DOCKING,			///< Docking connector between CSM and LEM.
	LEM_CSM_POWER,				///< Power connection from CSM to LEM.
	MFD_PANEL_INTERFACE,		///< Connector from an MFD to a panel.
	// [MP]
	MP_PANEL_INTERFACE,
	MP_DSKY_INTERFACE,
	// [/MP]
	LEM_CSM_ECS,				///< Tunnel connection from CSM to LEM.
	CHECKLIST_DATA_INTERFACE,	///< Data connector from checklist controller to vessel
	PAYLOAD_SLA_CONNECT,		///< Passes commands and data between payload and SIVb.
	CSM_PAYLOAD_COMMAND,		///< Docking connector between CSM and Payload
	SIVB_SI_COMMAND,			///< Docking connector between S-IVB and S-IB/S-II
	SII_SIC_COMMAND,			///< Docking connector between S-II and S-IC
	RADAR_RF_SIGNAL,			///< Radar connector betwen LM rendezvous radar amd CSM rendezvous radar transponder
	VHF_RNG,
};

#define VIRTUAL_CONNECTOR_PORT	(0xffff)		///< Port ID for 'virtual' connectors which don't physically exist.

//
// The message can pass various different parameters in this union. The receiver
// will determine which is correct based on the message type.
//
// \brief Message value for connector messages.
//
union ConnectorMessageValue
{
	int iValue;				///< Integer message value.
	double dValue;			///< Floating point message value.
	bool bValue;			///< Boolean message value.
	void *pValue;			///< Pointer message value.
	OBJHANDLE hValue;		///< Orbiter handle.
	VECTOR3 vValue;			///< Vector message value;
};

///
/// \ingroup Connectors
/// \brief Connector message to be passed through the system.
///
struct ConnectorMessage
{
	///
	/// \brief What kind of connector should this message go to?
	///
	ConnectorType destination;

	///
	/// \brief Connection-specific message type.
	///
	unsigned int messageType;

	///
	/// \brief Message value 1.
	///
	ConnectorMessageValue val1;

	///
	/// \brief Message value 2.
	///
	ConnectorMessageValue val2;

	///
	/// \brief Message value 3.
	///
	ConnectorMessageValue val3;

	///
	/// \brief Message value 4.
	///
	ConnectorMessageValue val4;
};

///
/// \ingroup Connectors
/// \brief Connector class. Specific connectors will be derived from this class.
///
class Connector {
public:
	///
	/// \brief Constructor.
	///
	Connector();

	///
	/// \brief Destructor. Disconnects the connector when called.
	///
	virtual ~Connector();

	///
	/// \brief Get the type of the connector.
	/// \return Connector type.
	///
	virtual ConnectorType GetType();

	///
	/// \brief Set the type of the connector.
	/// \param t Connector type.
	///
	void SetType(ConnectorType t) { type = t; };

	///
	/// \brief Connect to another connector.
	/// \param other Other end of the connection.
	/// \return True if the connector is the correct type and we connected.
	///
	virtual bool ConnectTo(Connector *other);

	///
	/// \brief Disconnect from the far end of the connection.
	///
	virtual void Disconnect();

	///
	/// \brief Has been disconnected from the far end of the connection.
	///
	virtual void Disconnected();

	///
	/// Send a message through the connector. Note that the receiver can update the value in the
	/// connector message, in order to return data to the caller.
	///
	/// \brief Send message.
	/// \param m Message to send.
	/// \return False if the connector isn't connected to anything or the message wasn't handled
	/// at the far end of the connection.
	///
	virtual bool SendMessage(ConnectorMessage &m);

	///
	/// Receieve a message through the connector. Note that the receiver can update the value in the
	/// connector message, in order to return data to the caller.
	///
	/// The default connector is output-only, and will return an error if the other end of the connection
	/// tries to send it data.
	///
	/// \brief Receive message.
	/// \param from The connector that sent the message.
	/// \param m Message received.
	/// \return False if we don't handle the message.
	///
	virtual bool ReceiveMessage(Connector *from, ConnectorMessage &m);

	///
	/// \brief Connector we're connected to, if any.
	///
	Connector *connectedTo;

protected:
	///
	/// \brief Type of connection.
	///
	ConnectorType type;
};

///
/// \ingroup Connectors
/// \brief Connector class for multiple connectors, typically used for docked ships.
///
class MultiConnector : public Connector
{
public:
	MultiConnector();
	~MultiConnector();

	///
	/// \brief Add another connection to this connector.
	/// \param other Connector to add.
	/// \return True if we could add the new connector.
	///
	virtual bool AddTo(Connector *other);

	bool ReceiveMessage(Connector *from, ConnectorMessage &m);
	void Disconnect();

#define N_MULTICONNECT_INPUTS 16

private:
	Connector *Inputs[N_MULTICONNECT_INPUTS];
};

class PanelSwitches;
class ChecklistController;

///
/// \ingroup Connectors
/// \brief Connector class for panel interface.
///
class PanelConnector : public Connector
{
public:

	///
	/// \ingroup Connectors
	/// \brief Message type to send from the CSM to the SIVb.
	///
	enum PanelConnectorMessageType
	{
		MFD_PANEL_FLASH_ITEM,					///< Turn flash on or off.
		MFD_PANEL_GET_ITEM_STATE,				///< Get the item's current state.
		MFD_PANEL_SET_ITEM_STATE,				///< Set the item's current state.
		MFD_PANEL_GET_FAILED_STATE,				///< Get the item's failed state.
		MFD_PANEL_CHECKLIST_AUTOCOMPLETE,		///< Checklist autocomplete.
		MFD_PANEL_GET_CHECKLIST_ITEM,			///< Get the an checklist item.
		MFD_PANEL_GET_CHECKLIST_LIST,			///< Get list of allowed checklists.
		MFD_PANEL_FAIL_ITEM,					///< Fail a checklist step.
		MFD_PANEL_COMPLETE_ITEM,				///< Complete a checklist step.
		MFD_PANEL_CHECKLIST_AUTOCOMPLETE_QUERY,	///< Find out the autocomplete state.
		MFD_PANEL_CHECKLIST_NAME,				///< Find out the name of the current checklist
		MFD_PANEL_RETRIEVE_CHECKLIST,			///< Get an entire, non-controlled, checklist
		MFD_PANEL_CHECKLIST_FLASHING,			///< Checklist item flashing.
		MFD_PANEL_CHECKLIST_FLASHING_QUERY,		///< Checklist item flashing.
		MFD_PANEL_GET_ITEM_FLASHING,			///< Get the item's current flashing.
		MFD_PANEL_GET_CHECKLIST_AUTOEXECUTE,	///< Get automatic checklist execution.
		MFD_PANEL_SET_CHECKLIST_AUTOEXECUTE,    ///< Set automatic checklist execution.
		MFD_PANEL_GOTO_CHECKLIST_ITEM,          ///< Go to checklist item
		MFD_PANEL_UNDO_CHECKLIST_ITEM           ///< Undo last item   
	};

	PanelConnector(PanelSwitches &p, ChecklistController &c);
	~PanelConnector();

	bool ReceiveMessage(Connector *from, ConnectorMessage &m);

private:
	PanelSwitches &panel;
	ChecklistController &checklist;
};

// [MP]
// Multiplayer Panel Connector
class Saturn;
class LEM;

class MP_PanelConnector : public Connector
{
public:

	///
	/// \ingroup Connectors
	/// \brief Message type to send from the CSM to the SIVb.
	///
	static enum MP_PanelConnectorMessageType
	{
		MP_PANEL_FLASH_ITEM,					///< Turn flash on or off.
		MP_PANEL_GET_ITEM_STATE,				///< Get the item's current state.
		MP_PANEL_SET_ITEM_STATE,				///< Set the item's current state.  
		MP_PANEL_GET_FAILED_STATE,
		MP_PANEL_SWITCH_TOGGLED,
		MP_PANEL_GUARD_TOGGLED,
		MP_PANEL_SEATS_TOGGLED,
		MP_PANEL_SET_SEATS,
		MP_PANEL_SET_IS_CLIENT,
		MP_PANEL_GET_MISSION_TIME,
		MP_PANEL_SET_MISSION_TIME,
		MP_PANEL_SYNC_MISSION_TIME,
		MP_PANEL_SET_FDAI,
		MP_PANEL_FDAI_CHANGED,
		MP_PANEL_SIVB_SEPARATED,
		MP_PANEL_RHC_CHANGED,
		MP_PANEL_SET_RHC,
		MP_PANEL_THC_CHANGED,
		MP_PANEL_SET_THC,
		MP_PANEL_RCS_SOUND_CHANGED,
		MP_PANEL_SET_RCS_SOUND,
		MP_PANEL_SET_THRUSTER,
		MP_PANEL_THRUSTER_CHANGED,
		MP_PANEL_LANDED,
		MP_PANEL_SET_LANDED
	};

	static enum MP_FdaiType
	{
		FDAI1_Target,
		FDAI1_Rates,
		FDAI1_Errors,
		FDAI2_Target,
		FDAI2_Rates,
		FDAI2_Errors
	};

	MP_PanelConnector(PanelSwitches& p, Saturn* pSat);
	MP_PanelConnector(PanelSwitches& p, LEM* pLem);
	~MP_PanelConnector();

	bool ReceiveMessage(Connector* from, ConnectorMessage& m);

	bool getIsClient();
	bool getSyncClient();
	void setSyncClient(bool sync);
	bool getSyncRhc();
	void setSyncRhc(bool sync);
	bool getSyncThc();
	void setSyncThc(bool sync);
	bool getSyncClientThrusters();
	void setSyncClientThrusters(bool sync);
	void setLandedState(bool state);
	bool getLandedState();

	bool MasterAlarmPressed = false;

	// EMS
	int EMSDvSetSwitch = 0;
	double EMSDvVoltage = 0.0;

	// Mission Time
	double GetSyncTime();
	double GetMissionTime();

	// FDAI
	VECTOR3 FDAI1;
	VECTOR3 dFDAI1;
	VECTOR3 FDAI1_last;
	
	VECTOR3 FDAI1_rates;
	VECTOR3 dFDAI1_rates;
	VECTOR3 FDAI1_rates_new;
	
	VECTOR3 FDAI1_errors;
	VECTOR3 dFDAI1_errors;
	VECTOR3 FDAI1_errors_new;
	
	double Fdai1Simt1;
	double Fdai1Simt2;
	

	VECTOR3 FDAI2;
	VECTOR3 dFDAI2;
	VECTOR3 FDAI2_last;

	VECTOR3 FDAI2_rates;
	VECTOR3 dFDAI2_rates;
	VECTOR3 FDAI2_rates_new;
	
	VECTOR3 FDAI2_errors;
	VECTOR3 dFDAI2_errors;
	VECTOR3 FDAI2_errors_new;

	double Fdai2Simt1;
	double Fdai2Simt2;

	double lastFdaiSync;

	// RHC/THC
	VECTOR3 rhc;
	VECTOR3 thc;

	// Sounds/Animations (Replaced by thrusters section) TODO: Remove this section and use thrusters instead
	bool rcsSound = false;
	bool rcsSustainSound = false;

	// Thrusters
	bool syncClientThrusters = false;
	bool SendThruster(const char* name, int index, double value);
	
	struct
	{
		double th_rcs_a[8];
		double th_rcs_b[8];
		double th_rcs_c[8];
		double th_rcs_d[8];
		double th_att_cm[12];
		double th_sps[1];
	} sat_thrusters;	// Saturn Thruster Levels

	struct
	{
		double th_hover[1];
		double th_dust[4];
		double th_rcs[16];
	} lem_thrusters;	// LEM Thruster Levels

private:
	PanelSwitches& panel;
	
	bool isClient = false;	// Is Multiplayer Client
	bool syncClient = false;	// Sync Client with Host
	bool syncRhc = false;	// Sync RHC
	bool syncThc = false;	// Sync THC
	bool landedState = false; // LEM Landed State

	Saturn* sat;
	LEM* lem;

	double mtOffset = 0.0; // Mission Timer offset from Simt
	double timeToSync = 999999999999.; // Delay simt to sync mission timer (Allow simt to sync with mjd before updating mission timer)
};

// Multiplayer DSKY Connector
class DSKY;

class MP_DskyConnector : public Connector
{
public:

	static enum DskyLightType
	{
		COMP_ACTY,
		UPLINK,
		NO_ATT,
		STBY,
		KB_REL,
		OPR_ERR,
		TEMP,
		GIM_LOCK,
		PROG,
		RESTART,
		TRACKER,
		VEL,
		ALT,
		PRIO_DISP,
		NO_DAP
	};
	
	///
	/// \ingroup Connectors
	/// \brief Message type to send from the CSM to the SIVb.
	///
	static enum MP_DskyConnectorMessageType
	{
		MP_DSKY_SET_HOSTING,
		MP_DSKY_SET_LIGHT,
		MP_DSKY_SET_NOUN,
		MP_DSKY_SET_VERB,
		MP_DSKY_SET_PROG,
		MP_DSKY_SET_R1,
		MP_DSKY_SET_R2,
		MP_DSKY_SET_R3,
		MP_DSKY_SET_VERB_FLASH,
		MP_DSKY_SET_NOUN_FLASH,
		MP_DSKY_LIGHT_CHANGED,
		MP_DSKY_NOUN_CHANGED,
		MP_DSKY_VERB_CHANGED,
		MP_DSKY_PROG_CHANGED,
		MP_DSKY_R1_CHANGED,
		MP_DSKY_R2_CHANGED,
		MP_DSKY_R3_CHANGED,
		MP_DSKY_VERB_FLASH_CHANGED,
		MP_DSKY_NOUN_FLASH_CHANGED
	};

	MP_DskyConnector(DSKY& d);
	~MP_DskyConnector();

	bool ReceiveMessage(Connector* from, ConnectorMessage& m);

	// Previous DSKY States
	bool CompActy;
	bool UplinkLight;
	bool NoAttLight;
	bool StbyLight;
	bool KbRelLight;
	bool OprErrLight;
	bool TempLight;
	bool GimbalLockLight;
	bool ProgLight;
	bool RestartLight;
	bool TrackerLight;
	bool VelLight;
	bool AltLight;
	bool PrioDispLight;
	bool NoDAPLight;

	char Prog[3];
	char Verb[3];
	char Noun[3];
	char R1[7];
	char R2[7];
	char R3[7];

	bool VerbFlashing;
	bool NounFlashing;

	bool isHosting();

private:
	DSKY& dsky;
	
	bool hosting;
	
};

/*class MP_SaturnConnector : public Connector
{
public:

	///
	/// \ingroup Connectors
	/// \brief Message type to send from the CSM to the SIVb.
	///
	static enum MP_SaturnConnectorMessageType
	{
		MP_PANEL_FLASH_ITEM,					///< Turn flash on or off.
		MP_PANEL_GET_ITEM_STATE,				///< Get the item's current state.
		MP_PANEL_SET_ITEM_STATE,				///< Set the item's current state.  
		MP_PANEL_GET_FAILED_STATE,
		MP_PANEL_SWITCH_TOGGLED,
		MP_PANEL_SET_IS_CLIENT,
		MP_PANEL_DSKY_CHANGED,
	};

	MP_PanelConnector(PanelSwitches& p);
	~MP_PanelConnector();

	bool ReceiveMessage(Connector* from, ConnectorMessage& m);

	bool getIsClient();

	bool MasterAlarmPressed = false;

private:
	PanelSwitches& panel;
	bool isClient = false;
};*/


///
/// ProjectApollo-specific vessel which allows us to get connectors to communicate
/// while docked.
/// \ingroup Connectors
///

class ProjectApolloConnectorVessel : public VESSEL4
{
public:

	struct ConnectorDefinition
	{
		int port;
		Connector *c;

		ConnectorDefinition() { port = 0; c = 0; };
	};

	///
	/// \brief Constructor.
	///
	ProjectApolloConnectorVessel(OBJHANDLE hObj, int fmodel);

	///
	/// \brief Destructor.
	///
	virtual ~ProjectApolloConnectorVessel();

	///
	/// Other vessels can call this function to get a connector to talk to when they
	/// are docked. They need to specify the docking port number so we can have multiple
	/// connectors of the same time on vessels with multiple ports; for example a space
	/// station may supply power to each docking port.
	///
	/// \brief Get a pointer to a connector of specified type.
	/// \param port Docking port number.
	/// \param t Connector type to look for.
	/// \return Pointer to connector if one is registered, NULL if none registered.
	///
	virtual Connector *GetConnector(int port, ConnectorType t);

	///
	/// This is a sanity-check. If the validation fails, then the vessel probably isn't really of this class!
	///
	/// \return True if this is a valid vessel.
	///
	bool ValidateVessel();

	///
	/// \brief Set up connectors on docking.
	///
	void DockConnectors(int port);

	///
	/// \brief Disconnect connectors on undocking.
	///
	void UndockConnectors(int port);

protected:

	///
	/// \brief Register a connector for use by other vessels.
	/// \param port Docking port number.
	/// \param c Pointer to a connector.
	/// \return True if registered, false if not (e.g. too many registered already).
	///
	bool RegisterConnector(int port, Connector *c);

#define PACV_N_VALIDATION	0x5a715a75

	///
	/// We store a known value here for validation. If a vessel is not of this class then it's unlikely to
	/// have the same value at this location and will fail to validate; unfortunately it may also crash if
	/// the vessel happens to have been allocated at the end of the heap!
	///
	/// \brief Validation value.
	///
	unsigned int ValidationValue;

#define PACV_N_CONNECTORS 16

	ConnectorDefinition ConnectorList[PACV_N_CONNECTORS];
};

///
/// \ingroup Connectors
///
/// This function tries to get a connector from the specified docking port on the
/// specified vessel. It tries to determine whether the connector is a Project
/// Apollo vessel and then tries to get the connector pointer if it is.
///
/// \brief Get a connector from a vessel.
/// \param v Vessel, which may or may not be one of ours.
/// \param port Docking port number.
/// \param t Connector type to look for.
/// \return Connector if found, or NULL if not.
///
extern Connector *GetVesselConnector(VESSEL *v, int port, ConnectorType t);

///
/// \ingroup Connectors
/// \brief Radar Messages
///
enum RFconnectorMessageType {
	CW_RADAR_SIGNAL, ///< Continuous Wave Radar Signal
	RR_XPDR_SIGNAL, ///< Radar Transponder Signal
	VHF_RNG_SIGNAL_CSM,
	VHF_RNG_SIGNAL_LM,
};

#endif // _PA_CONNECTOR_H
