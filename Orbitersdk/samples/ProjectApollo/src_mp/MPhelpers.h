/*
* Modifications and additions for NASSP Multiplayer (unofficial fork):
* MULTIPLAYER DSKY CONNECTOR
* Copyright (c) 2026 Cody Kerkove
*
* SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef MP_HELPERS_H_
#define MP_HELPERS_H_

#include <OrbiterAPI.h>


class MP_Helpers
{
public:
	// HELPERS	
	static double Wrap2Pi(double angle)
	{
		while (angle >= 2.0 * PI)
			angle -= 2.0 * PI;

		while (angle < 0.0)
			angle += 2.0 * PI;

		return angle;
	}

	static VECTOR3 AngularDifference2Pi(const VECTOR3& target, const VECTOR3& current)
	{
		VECTOR3 diff = target - current;

		diff.x = Wrap2Pi(diff.x);
		diff.y = Wrap2Pi(diff.y);
		diff.z = Wrap2Pi(diff.z);

		// Convert to shortest direction
		if (diff.x > PI) diff.x -= 2.0 * PI;
		if (diff.y > PI) diff.y -= 2.0 * PI;
		if (diff.z > PI) diff.z -= 2.0 * PI;

		return diff;
	}

	static VECTOR3 AdvanceAngles2Pi(const VECTOR3& angle, const VECTOR3& delta)
	{
		return _V(
			Wrap2Pi(angle.x + delta.x),
			Wrap2Pi(angle.y + delta.y),
			Wrap2Pi(angle.z + delta.z)
		);
	}

	static void UpdateFdaiSample(const VECTOR3& values, VECTOR3& att, VECTOR3& last,
		VECTOR3& rate, double& t1, double& t2)
	{
		double now = oapiGetSimTime();
		double dt = now - t1;

		if (t1 <= 0.0) {
			t1 = now;                       // first sample: no rate yet, gate stays closed
		}
		else if (dt > 0.05) {               // sane interval
			rate = MP_Helpers::AngularDifference2Pi(values, last) / dt;  // wrapped, per-axis
			t2 = t1;
			t1 = now;
		}
		// else: duplicate in the same frame, so update the attitude but keep the old rate

		att = values;
		last = values;
	}

	VECTOR3 getRelAccel(VESSEL* ves)
	{
		// Calculates Acceleration relative to current reference body

		VESSELSTATUS vs;
		ves->GetStatus(vs);

		// Acceleration
		VECTOR3 fvec;
		VECTOR3 rfvecFromCtr;
		VECTOR3 rfvec;
		ves->GetForceVector(fvec);	// Get local force vector
		ves->Local2Rel(fvec, rfvecFromCtr);	// Convert Local force vector to relative force vector (Center of body)
		rfvec = rfvecFromCtr - vs.rpos;
		double mass = oapiGetMass(ves->GetHandle());

		// Get masses of all docked vessels
		UINT dockCount = ves->DockCount();

		for (UINT i = 0; i < dockCount; i++)
		{
			if (dockCount > 0)
			{
				DOCKHANDLE dHandle = ves->GetDockHandle(i);
				if (dHandle != NULL)
				{
					OBJHANDLE oHandle = ves->GetDockStatus(dHandle);
					if (oHandle != NULL)
					{
						VESSEL* dVes = oapiGetVesselInterface(oHandle);
						mass += dVes->GetMass();
					}
				}
			}
		}

		VECTOR3 acc = rfvec / mass;

		return acc;
	}
};

#endif