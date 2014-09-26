/*******************************************************************************
 * Copyright (c) 2009-2014, MAV'RIC Development Team
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, 
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, 
 * this list of conditions and the following disclaimer in the documentation 
 * and/or other materials provided with the distribution.
 * 
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

/*******************************************************************************
 * \file navigation.c
 * 
 * \author MAV'RIC Team
 * \author Nicolas Dousse
 *   
 * \brief Waypoint navigation controller
 *
 ******************************************************************************/


#include "navigation.h"
#include "conf_platform.h"
#include "print_util.h"
#include "time_keeper.h"

#define KP_YAW 0.2f

//------------------------------------------------------------------------------
// PRIVATE FUNCTIONS DECLARATION
//------------------------------------------------------------------------------

/**
 * \brief	Initialise the position hold mode
 *
 * \param	waypoint_handler		The pointer to the waypoint handler structure
 * \param	local_pos				The position where the position will be held
 */
static void navigation_waypoint_hold_init(mavlink_waypoint_handler_t* waypoint_handler, local_coordinates_t local_pos);

/**
 * \brief	Sets the automatic takeoff waypoint
 *
 * \param	waypoint_handler		The pointer to the waypoint handler structure
 */
static void navigation_waypoint_take_off_init(mavlink_waypoint_handler_t* waypoint_handler);

/**
 * \brief					Computes the relative position and distance to the given way point
 *
 * \param	waypoint_pos		Local coordinates of the waypoint
 * \param	rel_pos			Array to store the relative 3D position of the waypoint
 * \param	local_pos		The 3D array of the actual position
 *
 * \return					Distance to waypoint squared
 */
static float navigation_set_rel_pos_n_dist2wp(float waypoint_pos[], float rel_pos[], const float local_pos[3]);

/**
 * \brief					Sets the Robot speed to reach waypoint
 *
 * \param	rel_pos			Relative position of the waypoint
 * \param	navigation	The structure of navigation data
 */
static void navigation_set_speed_command(float rel_pos[], navigation_t* navigation);

/**
 * \brief						Navigates the robot towards waypoint waypoint_input in 3D velocity command mode
 *
 * \param	waypoint_input		Destination waypoint in local coordinate system
 * \param	navigation			The navigation structure
 */
static void navigation_run(local_coordinates_t waypoint_input, navigation_t* navigation);

/**
 * \brief	Sets auto-takeoff procedure from a mavlink command message MAV_CMD_NAV_TAKEOFF
 *
 * \param	navigation			The pointer to the navigation structure
 * \param	packet				The pointer to the structure of the mavlink command message long
 */
static void navigation_set_auto_takeoff(navigation_t *navigation, mavlink_command_long_t* packet);

/**
 * \brief	Drives the auto landing procedure from the MAV_CMD_NAV_LAND message long
 *
 * \param	navigation			The pointer to the navigation structure
 * \param	packet					The pointer to the structure of the mavlink command message long
 */
static void navigation_set_auto_landing(navigation_t* navigation, mavlink_command_long_t* packet);

/**
 * \brief	Check if the nav mode is equal to the state mav mode
 *
 * \param	navigation			The pointer to the navigation structure
 *
 * \return	True if the flag STABILISE, GUIDED and ARMED are equal, false otherwise
 */
static bool navigation_mode_change(navigation_t* navigation);

/**
 * \brief	Drives the automatic takeoff procedure
 *
 * \param	navigation		The pointer to the navigation structure in central_data
 */
static void navigation_waypoint_take_off_handler(navigation_t* navigation);

/**
 * \brief	Drives the hold position procedure
 *
 * \param	navigation		The pointer to the navigation structure in central_data
 */
static void navigation_hold_position_handler(navigation_t* navigation);

/**
 * \brief	Drives the GPS navigation procedure
 *
 * \param	navigation		The pointer to the navigation structure in central_data
 */
static void navigation_waypoint_navigation_handler(navigation_t* navigation);

/**
 * \brief	Drives the critical navigation behavior
 *
 * \param	navigation		The pointer to the navigation structure in central_data
 */
static void navigation_critical_handler(navigation_t* navigation);

/**
 * \brief	Drives the auto-landing navigation behavior
 *
 * \param	navigation		The pointer to the navigation structure in central_data
 */
static void navigation_auto_landing_handler(navigation_t* navigation);

//------------------------------------------------------------------------------
// PRIVATE FUNCTIONS IMPLEMENTATION
//------------------------------------------------------------------------------

static void navigation_waypoint_hold_init(mavlink_waypoint_handler_t* waypoint_handler, local_coordinates_t local_pos)
{
	waypoint_handler->hold_waypoint_set = true;
	
	waypoint_handler->waypoint_hold_coordinates = local_pos;
	
	//waypoint_handler->waypoint_hold_coordinates.heading = coord_conventions_get_yaw(waypoint_handler->ahrs->qe);
	//waypoint_handler->waypoint_hold_coordinates.heading = local_pos.heading;
	
	print_util_dbg_print("Position hold at: (");
	print_util_dbg_print_num(waypoint_handler->waypoint_hold_coordinates.pos[X],10);
	print_util_dbg_print(", ");
	print_util_dbg_print_num(waypoint_handler->waypoint_hold_coordinates.pos[Y],10);
	print_util_dbg_print(", ");
	print_util_dbg_print_num(waypoint_handler->waypoint_hold_coordinates.pos[Z],10);
	print_util_dbg_print(", ");
	print_util_dbg_print_num((int32_t)(waypoint_handler->waypoint_hold_coordinates.heading*180.0f/3.14f),10);
	print_util_dbg_print(")\r\n");
	
}

static void navigation_waypoint_take_off_init(mavlink_waypoint_handler_t* waypoint_handler)
{
	print_util_dbg_print("Automatic take-off, will hold position at: (");
	print_util_dbg_print_num(waypoint_handler->position_estimator->local_position.pos[X],10);
	print_util_dbg_print(", ");
	print_util_dbg_print_num(waypoint_handler->position_estimator->local_position.pos[Y],10);
	print_util_dbg_print(", ");
	print_util_dbg_print_num(-10.0f,10);
	print_util_dbg_print("), with heading of: ");
	print_util_dbg_print_num((int32_t)(waypoint_handler->position_estimator->local_position.heading*180.0f/3.14f),10);
	print_util_dbg_print("\r\n");

	waypoint_handler->waypoint_hold_coordinates = waypoint_handler->position_estimator->local_position;
	waypoint_handler->waypoint_hold_coordinates.pos[Z] = -10.0f;
	
	aero_attitude_t aero_attitude;
	aero_attitude=coord_conventions_quat_to_aero(waypoint_handler->ahrs->qe);
	waypoint_handler->waypoint_hold_coordinates.heading = aero_attitude.rpy[2];
	
	waypoint_handler->dist2wp_sqr = 100.0f; // same position, 10m above => dist_sqr = 100.0f
	
	waypoint_handler->hold_waypoint_set = true;
}

static float navigation_set_rel_pos_n_dist2wp(float waypoint_pos[], float rel_pos[], const float local_pos[3])
{
	float dist2wp_sqr;
	
	rel_pos[X] = (float)(waypoint_pos[X] - local_pos[X]);
	rel_pos[Y] = (float)(waypoint_pos[Y] - local_pos[Y]);
	rel_pos[Z] = (float)(waypoint_pos[Z] - local_pos[Z]);
	
	dist2wp_sqr = vectors_norm_sqr(rel_pos);
	
	return dist2wp_sqr;
}


static void navigation_set_speed_command(float rel_pos[], navigation_t* navigation)
{
	float  norm_rel_dist, v_desired;
	quat_t qtmp1, qtmp2;
	
	float dir_desired_bf[3];
	// dir_desired[3],
	
	float rel_heading;
	
	norm_rel_dist = sqrt(navigation->waypoint_handler->dist2wp_sqr);
	
	if (norm_rel_dist < 0.0005f)
	{
		norm_rel_dist += 0.0005f;
	}
	
	// calculate dir_desired in local frame
	// vel = qe-1 * rel_pos * qe
	qtmp1 = quaternions_create_from_vector(rel_pos);
	qtmp2 = quaternions_global_to_local(*navigation->qe,qtmp1);
	dir_desired_bf[0] = qtmp2.v[0]; dir_desired_bf[1] = qtmp2.v[1]; dir_desired_bf[2] = qtmp2.v[2];
	
	dir_desired_bf[2] = rel_pos[2];
	
	if (((navigation->state->mav_mode.GUIDED == GUIDED_ON)&&(navigation->state->mav_mode.AUTO == AUTO_OFF))||((maths_f_abs(rel_pos[X])<=1.0f)&&(maths_f_abs(rel_pos[Y])<=1.0f))||((maths_f_abs(rel_pos[X])<=5.0f)&&(maths_f_abs(rel_pos[Y])<=5.0f)&&(maths_f_abs(rel_pos[Z])>=3.0f)))
	{
		rel_heading = 0.0f;
	}
	else
	{
		rel_heading = maths_calc_smaller_angle(atan2(rel_pos[Y],rel_pos[X]) - navigation->position_estimator->local_position.heading);
	}
	
	navigation->dist2vel_controller.clip_max = navigation->cruise_speed;
	v_desired = pid_control_update_dt(&navigation->dist2vel_controller, (maths_center_window_2(4.0f * rel_heading) * norm_rel_dist), navigation->dt);
	
	if (v_desired *  maths_f_abs(dir_desired_bf[Z]) > navigation->max_climb_rate * norm_rel_dist ) {
		v_desired = navigation->max_climb_rate * norm_rel_dist /maths_f_abs(dir_desired_bf[Z]);
	}
	
	dir_desired_bf[X] = v_desired * dir_desired_bf[X] / norm_rel_dist;
	dir_desired_bf[Y] = v_desired * dir_desired_bf[Y] / norm_rel_dist;
	dir_desired_bf[Z] = v_desired * dir_desired_bf[Z] / norm_rel_dist;
	
	/*
	loop_count = loop_count++ %50;
	if (loop_count == 0)
	{
		mavlink_msg_named_value_float_send(MAVLINK_COMM_0,time_keeper_get_millis(),"v_desired",v_desired*100);
		mavlink_msg_named_value_float_send(MAVLINK_COMM_0,time_keeper_get_millis(),"act_vel",vector_norm(navigation->position_estimator->vel_bf)*100);
		print_util_dbg_print("Desired_vel_Bf(x100): (");
		print_util_dbg_print_num(dir_desired_bf[X] * 100,10);
		print_util_dbg_print_num(dir_desired_bf[Y] * 100,10);
		print_util_dbg_print_num(dir_desired_bf[Z] * 100,10);
		print_util_dbg_print("). \n");
		print_util_dbg_print("Actual_vel_bf(x100): (");
		print_util_dbg_print_num(navigation->position_estimator->vel_bf[X] * 100,10);
		print_util_dbg_print_num(navigation->position_estimator->vel_bf[Y] * 100,10);
		print_util_dbg_print_num(navigation->position_estimator->vel_bf[Z] * 100,10);
		print_util_dbg_print("). \n");
		print_util_dbg_print("Actual_pos(x100): (");
		print_util_dbg_print_num(navigation->position_estimator->local_position.pos[X] * 100,10);
		print_util_dbg_print_num(navigation->position_estimator->local_position.pos[Y] * 100,10);
		print_util_dbg_print_num(navigation->position_estimator->local_position.pos[Z] * 100,10);
		print_util_dbg_print("). \n");
	}
	*/

	navigation->controls_nav->tvel[X] = dir_desired_bf[X];
	navigation->controls_nav->tvel[Y] = dir_desired_bf[Y];
	navigation->controls_nav->tvel[Z] = dir_desired_bf[Z];		
	navigation->controls_nav->rpy[YAW] = KP_YAW * rel_heading;
}

static void navigation_run(local_coordinates_t waypoint_input, navigation_t* navigation)
{
	float rel_pos[3];
	
	// Control in translational speed of the platform
	navigation->waypoint_handler->dist2wp_sqr = navigation_set_rel_pos_n_dist2wp(waypoint_input.pos,
																					rel_pos,
																					navigation->position_estimator->local_position.pos);
	navigation_set_speed_command(rel_pos, navigation);
	
	navigation->controls_nav->theading=waypoint_input.heading;
}

static void navigation_set_auto_takeoff(navigation_t *navigation, mavlink_command_long_t* packet)
{
	mavlink_message_t msg;
	
	MAV_RESULT result;
	
	if (!navigation->state->in_the_air)
	{
		print_util_dbg_print("Starting automatic take-off from button\n");
		navigation->auto_takeoff = true;

		result = MAV_RESULT_ACCEPTED;
	}
	else
	{
		result = MAV_RESULT_DENIED;
	}
	
	mavlink_msg_command_ack_pack( 	navigation->mavlink_stream->sysid,
									navigation->mavlink_stream->compid,
									&msg,
									MAV_CMD_NAV_TAKEOFF,
									result);
	
	mavlink_stream_send(navigation->mavlink_stream, &msg);
}

static void navigation_set_auto_landing(navigation_t* navigation, mavlink_command_long_t* packet)
{
	mavlink_message_t msg;
	
	MAV_RESULT result;

	if (navigation->state->in_the_air)
	{
		result = MAV_RESULT_ACCEPTED;
		navigation->auto_landing_behavior = DESCENT_TO_SMALL_ALTITUDE;
		
		navigation->auto_landing = true;
		print_util_dbg_print("Auto-landing procedure initialised.\r\n");
	}
	else
	{
		result = MAV_RESULT_DENIED;
	}

	mavlink_msg_command_ack_pack( 	navigation->mavlink_stream->sysid,
									navigation->mavlink_stream->compid,
									&msg,
									MAV_CMD_NAV_LAND,
									result);
	mavlink_stream_send(navigation->mavlink_stream, &msg);
}

static bool navigation_mode_change(navigation_t* navigation)
{
	mav_mode_t mode_local = navigation->state->mav_mode;
	mav_mode_t mode_nav = navigation->mode;
	
	bool result = false;
	
	if ((mode_local.STABILISE == mode_nav.STABILISE)&&(mode_local.GUIDED == mode_nav.GUIDED)&&(mode_local.AUTO == mode_nav.AUTO))
	{
		result = true;
	}
	
	return result;
}

static void navigation_waypoint_take_off_handler(navigation_t* navigation)
{
	if (!navigation->waypoint_handler->hold_waypoint_set)
	{
		navigation_waypoint_take_off_init(navigation->waypoint_handler);
	}
	if (!navigation->state->nav_plan_active)
	{
		waypoint_handler_nav_plan_init(navigation->waypoint_handler);
	}
	
	//if (navigation->mode == navigation->state->mav_mode.byte)
	if (navigation_mode_change(navigation))
	{
		if (navigation->waypoint_handler->dist2wp_sqr <= 16.0f)
		{
			//state_machine->state->mav_state = MAV_STATE_ACTIVE;
			navigation->state->in_the_air = true;
			navigation->auto_takeoff = false;
			print_util_dbg_print("Automatic take-off finised, dist2wp_sqr (10x):");
			print_util_dbg_print_num(navigation->waypoint_handler->dist2wp_sqr * 10.0f,10);
			print_util_dbg_print(".\r\n");
		}
	}
}


static void navigation_hold_position_handler(navigation_t* navigation)
{
	//if (navigation->mode != navigation->state->mav_mode.byte)
	if (!navigation_mode_change(navigation))
	{
		navigation->waypoint_handler->hold_waypoint_set = false;
	}
	
	if (!navigation->state->nav_plan_active)
	{
		waypoint_handler_nav_plan_init(navigation->waypoint_handler);
	}
	
	if (!navigation->waypoint_handler->hold_waypoint_set)
	{
		navigation_waypoint_hold_init(navigation->waypoint_handler, navigation->waypoint_handler->position_estimator->local_position);
	}
}

static void navigation_waypoint_navigation_handler(navigation_t* navigation)
{
	//if (navigation->mode != navigation->state->mav_mode.byte)
	if (!navigation_mode_change(navigation))
	{
		navigation->waypoint_handler->hold_waypoint_set = false;
	}
	
	if (navigation->state->nav_plan_active)
	{
		uint8_t i;
		float rel_pos[3];
		
		for (i=0;i<3;i++)
		{
			rel_pos[i] = navigation->waypoint_handler->waypoint_coordinates.pos[i]-navigation->waypoint_handler->position_estimator->local_position.pos[i];
		}
		navigation->waypoint_handler->dist2wp_sqr = vectors_norm_sqr(rel_pos);
		
		if (navigation->waypoint_handler->dist2wp_sqr < (navigation->waypoint_handler->current_waypoint.param2*navigation->waypoint_handler->current_waypoint.param2))
		{
			print_util_dbg_print("Waypoint Nr");
			print_util_dbg_print_num(navigation->waypoint_handler->current_waypoint_count,10);
			print_util_dbg_print(" reached, distance:");
			print_util_dbg_print_num(sqrt(navigation->waypoint_handler->dist2wp_sqr),10);
			print_util_dbg_print(" less than :");
			print_util_dbg_print_num(navigation->waypoint_handler->current_waypoint.param2,10);
			print_util_dbg_print(".\r\n");
			
			mavlink_message_t msg;
			mavlink_msg_mission_item_reached_pack( 	navigation->mavlink_stream->sysid,
													navigation->mavlink_stream->compid,
													&msg,
													navigation->waypoint_handler->current_waypoint_count);
			mavlink_stream_send(navigation->mavlink_stream, &msg);
			
			navigation->waypoint_handler->waypoint_list[navigation->waypoint_handler->current_waypoint_count].current = 0;
			if((navigation->waypoint_handler->current_waypoint.autocontinue == 1)&&(navigation->waypoint_handler->number_of_waypoints>1))
			{
				print_util_dbg_print("Autocontinue towards waypoint Nr");
				
				if (navigation->waypoint_handler->current_waypoint_count == (navigation->waypoint_handler->number_of_waypoints-1))
				{
					navigation->waypoint_handler->current_waypoint_count = 0;
				}
				else
				{
					navigation->waypoint_handler->current_waypoint_count++;
				}
				print_util_dbg_print_num(navigation->waypoint_handler->current_waypoint_count,10);
				print_util_dbg_print("\r\n");
				navigation->waypoint_handler->waypoint_list[navigation->waypoint_handler->current_waypoint_count].current = 1;
				navigation->waypoint_handler->current_waypoint = navigation->waypoint_handler->waypoint_list[navigation->waypoint_handler->current_waypoint_count];
				navigation->waypoint_handler->waypoint_coordinates = waypoint_handler_set_waypoint_from_frame(navigation->waypoint_handler, navigation->waypoint_handler->position_estimator->local_position.origin);
				
				mavlink_message_t msg;
				mavlink_msg_mission_current_pack( 	navigation->mavlink_stream->sysid,
													navigation->mavlink_stream->compid,
													&msg,
													navigation->waypoint_handler->current_waypoint_count);
				mavlink_stream_send(navigation->mavlink_stream, &msg);
				
			}
			else
			{
				navigation->state->nav_plan_active = false;
				print_util_dbg_print("Stop\r\n");
				
				navigation_waypoint_hold_init(navigation->waypoint_handler, navigation->waypoint_handler->waypoint_coordinates);
			}
		}
	}
	else
	{
		if (!navigation->waypoint_handler->hold_waypoint_set)
		{
			navigation_waypoint_hold_init(navigation->waypoint_handler, navigation->waypoint_handler->position_estimator->local_position);
		}
		waypoint_handler_nav_plan_init(navigation->waypoint_handler);
	}
}

static void navigation_critical_handler(navigation_t* navigation)
{
	float rel_pos[3];
	uint8_t i;
	
	if (navigation->state->mav_state == MAV_STATE_STANDBY)
	{
		navigation->critical_behavior = CLIMB_TO_SAFE_ALT;
	}
	
	if (!(navigation->critical_next_state))
	{
		navigation->critical_next_state = true;
		
		aero_attitude_t aero_attitude;
		aero_attitude=coord_conventions_quat_to_aero(*navigation->qe);
		navigation->waypoint_handler->waypoint_critical_coordinates.heading = aero_attitude.rpy[2];
		
		switch (navigation->critical_behavior)
		{
			case CLIMB_TO_SAFE_ALT:
				navigation->waypoint_handler->waypoint_critical_coordinates.pos[X] = navigation->position_estimator->local_position.pos[X];
				navigation->waypoint_handler->waypoint_critical_coordinates.pos[Y] = navigation->position_estimator->local_position.pos[Y];
				navigation->waypoint_handler->waypoint_critical_coordinates.pos[Z] = -30.0f;
				break;
			
			case FLY_TO_HOME_WP:
				navigation->waypoint_handler->waypoint_critical_coordinates.pos[X] = 0.0f;
				navigation->waypoint_handler->waypoint_critical_coordinates.pos[Y] = 0.0f;
				navigation->waypoint_handler->waypoint_critical_coordinates.pos[Z] = -30.0f;
				break;
			
			case CRITICAL_LAND:
				navigation->waypoint_handler->waypoint_critical_coordinates.pos[X] = 0.0f;
				navigation->waypoint_handler->waypoint_critical_coordinates.pos[Y] = 0.0f;
				navigation->waypoint_handler->waypoint_critical_coordinates.pos[Z] = 0.0f;
				break;
		}
		
		for (i=0;i<3;i++)
		{
			rel_pos[i] = navigation->waypoint_handler->waypoint_critical_coordinates.pos[i] - navigation->position_estimator->local_position.pos[i];
		}
		navigation->waypoint_handler->dist2wp_sqr = vectors_norm_sqr(rel_pos);
	}
	
	if (navigation->waypoint_handler->dist2wp_sqr < 3.0f)
	{
		navigation->critical_next_state = false;
		switch (navigation->critical_behavior)
		{
			case CLIMB_TO_SAFE_ALT:
				print_util_dbg_print("Critical State! Flying to home waypoint.\r\n");
				navigation->critical_behavior = FLY_TO_HOME_WP;
				break;
			
			case FLY_TO_HOME_WP:
				print_util_dbg_print("Critical State! Performing critical landing.\r\n");
				navigation->critical_behavior = CRITICAL_LAND;
				break;
			
			case CRITICAL_LAND:
				print_util_dbg_print("Critical State! Landed, switching off motors, Emergency mode.\r\n");
				navigation->critical_landing = true;
				break;
		}
	}
}

static void navigation_auto_landing_handler(navigation_t* navigation)
{
	float rel_pos[3];
	uint8_t i;
	
	local_coordinates_t local_position;
	
	switch(navigation->auto_landing_behavior)
	{
		case DESCENT_TO_SMALL_ALTITUDE:
			local_position = navigation->position_estimator->local_position;
			local_position.pos[Z] = -2.0f;
		
			navigation_waypoint_hold_init(navigation->waypoint_handler, local_position);
			break;
			
		case DESCENT_TO_GND:
			local_position = navigation->position_estimator->local_position;
			local_position.pos[Z] = 0.0f;
			
			navigation_waypoint_hold_init(navigation->waypoint_handler, local_position);
			break;
	}
	
	for (i=0;i<3;i++)
	{
		rel_pos[i] = navigation->waypoint_handler->waypoint_critical_coordinates.pos[i] - navigation->position_estimator->local_position.pos[i];
	}
	
	navigation->waypoint_handler->dist2wp_sqr = vectors_norm_sqr(rel_pos);
	
	if (navigation->waypoint_handler->dist2wp_sqr < 0.5f)
	{
		switch(navigation->auto_landing_behavior)
		{
			case DESCENT_TO_SMALL_ALTITUDE:
				print_util_dbg_print("Automatic-landing: descent_to_GND\r\n");
				navigation->auto_landing_behavior = DESCENT_TO_GND;
				break;
			case DESCENT_TO_GND:
				break;
		}
	}
}

//------------------------------------------------------------------------------
// PUBLIC FUNCTIONS IMPLEMENTATION
//------------------------------------------------------------------------------

void navigation_init(navigation_t* navigation, control_command_t* controls_nav, const quat_t* qe, mavlink_waypoint_handler_t* waypoint_handler, const position_estimator_t* position_estimator, state_t* state, const control_command_t* control_joystick, const remote_t* remote, mavlink_communication_t* mavlink_communication)
{
	
	navigation->controls_nav = controls_nav;
	navigation->qe = qe;
	navigation->waypoint_handler = waypoint_handler;
	navigation->position_estimator = position_estimator;
	navigation->state = state;
	navigation->mavlink_stream = &mavlink_communication->mavlink_stream;
	navigation->control_joystick = control_joystick;
	navigation->remote = remote;
	
	navigation->controls_nav->rpy[ROLL] = 0.0f;
	navigation->controls_nav->rpy[PITCH] = 0.0f;
	navigation->controls_nav->rpy[YAW] = 0.0f;
	navigation->controls_nav->tvel[X] = 0.0f;
	navigation->controls_nav->tvel[Y] = 0.0f;
	navigation->controls_nav->tvel[Z] = 0.0f;
	navigation->controls_nav->theading = 0.0f;
	navigation->controls_nav->thrust = -1.0f;
	navigation->controls_nav->control_mode = VELOCITY_COMMAND_MODE;
	navigation->controls_nav->yaw_mode = YAW_ABSOLUTE;
	
	static pid_controller_t nav_default_controller =
	{
		.p_gain = 0.2f,
		.clip_min = 0.0f,
		.clip_max = 3.0f,
		.integrator={
			.pregain = 0.5f,
			.postgain = 0.0f,
			.accumulator = 0.0f,
			.maths_clip = 0.65f,
			.leakiness = 0.0f
		},
		.differentiator={
			.gain = 0.4f,
			.previous = 0.0f,
			.LPF = 0.5f,
			.maths_clip = 0.65f
		},
		.output = 0.0f,
		.error = 0.0f,
		.last_update = 0.0f,
		.dt = 1,
		.soft_zone_width = 0.0f
	};
	
	navigation->dist2vel_controller = nav_default_controller;
	
	navigation->mode.byte = state->mav_mode.byte;
	
	navigation->auto_takeoff = false;
	navigation->auto_landing = false;
	
	navigation->critical_behavior = CLIMB_TO_SAFE_ALT;
	navigation->auto_landing_behavior = DESCENT_TO_SMALL_ALTITUDE;
	
	navigation->critical_landing = false;
	
	navigation->controls_nav->mavlink_stream = &mavlink_communication->mavlink_stream;
	
	navigation->dist2vel_gain = 0.7f;
	navigation->cruise_speed = 3.0f;
	navigation->max_climb_rate = 1.0f;
	
	navigation->soft_zone_size = 0.0f;
	
	navigation->loop_count = 0;
	
	navigation->last_update = time_keeper_get_time_ticks();
	navigation->dt = 0.004;
	
	// Add callbacks for waypoint handler commands requests
	mavlink_message_handler_cmd_callback_t callbackcmd;
	
	callbackcmd.command_id = MAV_CMD_NAV_TAKEOFF; // 22
	callbackcmd.sysid_filter = MAVLINK_BASE_STATION_ID;
	callbackcmd.compid_filter = MAV_COMP_ID_ALL;
	callbackcmd.compid_target = MAV_COMP_ID_ALL; // 0
	callbackcmd.function = (mavlink_cmd_callback_function_t)	&navigation_set_auto_takeoff;
	callbackcmd.module_struct =									navigation;
	mavlink_message_handler_add_cmd_callback(&mavlink_communication->message_handler, &callbackcmd);
	
	callbackcmd.command_id = MAV_CMD_NAV_LAND; // 21
	callbackcmd.sysid_filter = MAVLINK_BASE_STATION_ID;
	callbackcmd.compid_filter = MAV_COMP_ID_ALL;
	callbackcmd.compid_target = MAV_COMP_ID_ALL; // 0
	callbackcmd.function = (mavlink_cmd_callback_function_t)	&navigation_set_auto_landing;
	callbackcmd.module_struct =									waypoint_handler;
	mavlink_message_handler_add_cmd_callback(&mavlink_communication->message_handler, &callbackcmd);
	
	print_util_dbg_print("Navigation initialized.\r\n");
}

task_return_t navigation_update(navigation_t* navigation)
{
	mav_mode_t mode_local = navigation->state->mav_mode;
	
	uint32_t t = time_keeper_get_time_ticks();
	
	navigation->dt = time_keeper_ticks_to_seconds(t - navigation->last_update);
	navigation->last_update = t;
	
	float thrust;
	
	switch (navigation->state->mav_state)
	{
		case MAV_STATE_ACTIVE:
			if (navigation->state->in_the_air)
			{
				if(mode_local.AUTO == AUTO_ON)
				{
					navigation_waypoint_navigation_handler(navigation);
						
					if (navigation->state->nav_plan_active)
					{
						navigation_run(navigation->waypoint_handler->waypoint_coordinates,navigation);
					}
					else
					{
						navigation_run(navigation->waypoint_handler->waypoint_hold_coordinates,navigation);
					}
				}
				else if(mode_local.GUIDED == GUIDED_ON)
				{
					navigation_hold_position_handler(navigation);
						
					navigation_run(navigation->waypoint_handler->waypoint_hold_coordinates,navigation);
					break;
				}
			}
			else
			{
				if (navigation->state->remote_active == 1)
				{
					thrust = remote_get_throttle(navigation->remote);
				}
				else
				{
					thrust = navigation->control_joystick->thrust;
				}
				
				if (thrust > -0.7f)
				{
					if ((mode_local.GUIDED == GUIDED_ON)||(mode_local.AUTO == AUTO_ON))
					{
						if (!navigation->auto_takeoff)
						{
							navigation->waypoint_handler->hold_waypoint_set = false;
						}
						navigation->auto_takeoff = true;
					}
					else
					{
						navigation->state->in_the_air = true;
					}
				}
				
				if ((mode_local.GUIDED == GUIDED_ON)||(mode_local.AUTO == AUTO_ON))
				{
					if (navigation->auto_takeoff)
					{
						navigation_waypoint_take_off_handler(navigation);
						
						navigation_run(navigation->waypoint_handler->waypoint_hold_coordinates,navigation);
					}
					else
					if (navigation->auto_landing)
					{
						navigation_auto_landing_handler(navigation);
						
						navigation_run(navigation->waypoint_handler->waypoint_hold_coordinates,navigation);
					}
				}
			}
			break;

		case MAV_STATE_CRITICAL:
			// In MAV_MODE_VELOCITY_CONTROL, MAV_MODE_POSITION_HOLD and MAV_MODE_GPS_NAVIGATION
			if (mode_local.STABILISE == STABILISE_ON)
			{
				navigation_critical_handler(navigation);
				navigation_run(navigation->waypoint_handler->waypoint_critical_coordinates,navigation);
			}
			break;
			
		default:
			break;
	}
	
	navigation->mode = mode_local;
	
	return TASK_RUN_SUCCESS;
}
