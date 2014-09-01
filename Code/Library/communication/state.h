/**
 * \page The MAV'RIC License
 *
 * The MAV'RIC Framework
 *
 * Copyright © 2011-2014
 *
 * Laboratory of Intelligent Systems, EPFL
 */
 

/**
 * \file state.h
 *
 *  Place where the state structure is defined
 */


#ifndef STATE_H_
#define STATE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "scheduler.h"
#include "mavlink_communication.h"
#include "analog_monitor.h"
#include <stdbool.h>
#include "mav_modes.h"

/**
 * \brief	The critical behavior enum
 */
typedef enum
{
	CLIMB_TO_SAFE_ALT,											///< First critical behavior
	FLY_TO_HOME_WP,												///< Second critical behavior, comes after CLIMB_TO_SAFE_ALT
	CRITICAL_LAND												///< Third critical behavior, comes after FLY_TO_HOME_WP
} critical_behavior_enum;

/**
 * \brief	The auto-landing enum
 */
typedef enum
{
	DESCENT_TO_SMALL_ALTITUDE,							///< First auto landing behavior
	DESCENT_TO_GND										///< Second auto landing behavior, comes after DESCENT_TO_SMAL_ALTITUDE
} auto_landing_behavior_t;

typedef enum MAV_MODE_FLAG mav_flag_t;

/**
 * \brief The state structure
 */
typedef struct  
{
	mav_mode_t mav_mode;								///< The value of the MAV mode 
	mav_state_t mav_state;								///< The value of the MAV state
		
	mav_mode_t mav_mode_previous;						///< The value of the MAV mode at previous time step
		
	int32_t simulation_mode;							///< The value of the simulation_mode (0: real, 1: simulation)
	
	uint8_t autopilot_type;								///< The type of the autopilot (MAV_TYPE enum in common.h)
	uint8_t autopilot_name;								///< The name of the autopilot (MAV_AUTOPILOT enum in common.h)
	
	uint16_t sensor_present;							///< The type of sensors that are present on the autopilot (Value of 0: not present. Value of 1: present. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control)
	uint16_t sensor_enabled;							///< The sensors enabled on the autopilot (Value of 0: not enabled. Value of 1: enabled. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control)
	uint16_t sensor_health;								///< The health of sensors present on the autopilot (Value of 0: not enabled. Value of 1: enabled. Indices: 0: 3D gyro, 1: 3D acc, 2: 3D mag, 3: absolute pressure, 4: differential pressure, 5: GPS, 6: optical flow, 7: computer vision position, 8: laser based position, 9: external ground-truth (Vicon or Leica). Controllers: 10: 3D angular rate control 11: attitude stabilization, 12: yaw position, 13: z/altitude control, 14: x/y position control, 15: motor outputs / control)

	bool nav_plan_active;								///< Flag to tell that a flight plan (min 1 waypoint) is active
	bool in_the_air;									///< Flag to tell whether the vehicle is airborne or not
	bool collision_avoidance;							///< Flag to tell whether the collision avoidance is active or not
	bool reset_position;
	
	uint32_t remote_active;								///< Flag to tell whether the remote is active or not
	
	const analog_monitor_t* analog_monitor;				///< The pointer to the analog monitor structure
	const mavlink_stream_t* mavlink_stream;				///< Pointer to the mavlin kstream structure
} state_t;


/**
 * \brief						Initialise the state of the MAV
 *
 * \param	state		The pointer to the state structure
 * \param	state_config		The pointer to the state configuration structure
 * \param   mavlink_stream		The pointer to the mavlink stream structure
 * \param	message_handler		The pointer to the message handler
 */
void state_init(state_t *state, state_t* state_config, const analog_monitor_t* analog_monitor, const mavlink_stream_t* mavlink_stream, mavlink_message_handler_t *message_handler);

/**
 * \brief	Task to send the mavlink heartbeat message
 * 
 * \param	state		The pointer to the state structure
 *
 * \return	The status of execution of the task
 */
task_return_t state_send_heartbeat(const state_t* state);


/**
 * \brief	Task to send the mavlink system status message, project specific message!
 * 
 * \param	state		The pointer to the state structure
 *
 * \return	The status of execution of the task
 */
task_return_t state_send_status(const state_t* state);


#ifdef __cplusplus
}
#endif

#endif //STATE_H_