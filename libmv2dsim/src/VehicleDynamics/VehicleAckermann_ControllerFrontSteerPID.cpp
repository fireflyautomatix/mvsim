/*+-------------------------------------------------------------------------+
  |                       MultiVehicle 2D simulator (libmv2dsim)            |
  |                                                                         |
  | Copyright (C) 2014  Jose Luis Blanco Claraco (University of Almeria)    |
  | Distributed under GNU General Public License version 3                  |
  |   See <http://www.gnu.org/licenses/>                                    |
  +-------------------------------------------------------------------------+  */

#include <mv2dsim/VehicleDynamics/VehicleAckermann.h>
#include "xml_utils.h"

using namespace mv2dsim;
using namespace std;

DynamicsAckermann::ControllerFrontSteerPID::ControllerFrontSteerPID(DynamicsAckermann &veh) :
	ControllerBase(veh),
	setpoint_lin_speed(0),
	setpoint_steer_ang(0),
	KP(100),
	KI(0),
	KD(0),
	I_MAX(10),
	max_torque(100.0),
	m_twist_control(veh)
{
	// Get distance between wheels:
	m_r2f_L = m_veh.m_wheels_info[WHEEL_FL].x - m_veh.m_wheels_info[WHEEL_RL].x;
	ASSERT_(m_r2f_L>0.0)
}

// See base class docs
void DynamicsAckermann::ControllerFrontSteerPID::control_step(
	const DynamicsAckermann::TControllerInput &ci,
	DynamicsAckermann::TControllerOutput &co)
{
	// Equivalent v/w velocities:
	const double v=setpoint_lin_speed;
	double w;
	if (setpoint_steer_ang==0.0) {
          w=0.0;
	}
	else {
		// ang = atan(r2f_L/R)  ->  R= r2f_L / tan(ang)
		// R = v/w              ->   w=v/R
		const double R = m_r2f_L / tan(setpoint_steer_ang);
		w = v/R;
	}

	// Let the twist controller do the calculations:
	m_twist_control.setpoint_lin_speed = v;
	m_twist_control.setpoint_ang_speed = w;

	m_twist_control.KP = KP;
	m_twist_control.KI = KI;
	m_twist_control.KD = KD;
	m_twist_control.I_MAX = I_MAX;
	m_twist_control.max_torque = max_torque;

	m_twist_control.control_step(ci,co);
	co.steer_ang = setpoint_steer_ang; // Mainly for the case of v=0
}

void DynamicsAckermann::ControllerFrontSteerPID::load_config(const rapidxml::xml_node<char>&node )
{
	std::map<std::string,TParamEntry> params;
	params["KP"] = TParamEntry("%lf", &KP);
	params["KI"] = TParamEntry("%lf", &KI);
	params["KD"] = TParamEntry("%lf", &KD);
	params["I_MAX"] = TParamEntry("%lf", &I_MAX);
	params["max_torque"] = TParamEntry("%lf", &max_torque);


	// Initial speed.
	params["V"] = TParamEntry("%lf", &this->setpoint_lin_speed);
	params["STEER_ANG"] = TParamEntry("%lf_deg", &this->setpoint_steer_ang);

	parse_xmlnode_children_as_param(node,params);
}

void DynamicsAckermann::ControllerFrontSteerPID::teleop_interface(const TeleopInput &in, TeleopOutput &out)
{
	switch (in.keycode)
	{
	case 'w':  setpoint_lin_speed+= 0.1;break;
	case 's':  setpoint_lin_speed-= 0.1; break;
	case 'a':  setpoint_steer_ang += 1.0*M_PI/180.0; mrpt::utils::keep_min(setpoint_steer_ang, m_veh.getMaxSteeringAngle()); break;
	case 'd':  setpoint_steer_ang -= 1.0*M_PI/180.0; mrpt::utils::keep_max(setpoint_steer_ang, -m_veh.getMaxSteeringAngle()); break;
	case ' ':  setpoint_lin_speed=.0; break;
	};
	out.append_gui_lines+="[Controller="+ string(class_name()) +"] Teleop keys: w/s=incr/decr lin speed. a/d=left/right steering. spacebar=stop.\n";
	out.append_gui_lines+=mrpt::format("setpoint: v=%.03f steer=%.03f deg\n", setpoint_lin_speed, setpoint_steer_ang*180.0/M_PI);
}