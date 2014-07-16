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
 * \file tasks.c
 *
 * Definition of the tasks executed on the autopilot
 */ 


#include "tasks.h"
#include "central_data.h"
#include "print_util.h"
#include "stabilisation.h"
#include "gps_ublox.h"
#include "navigation.h"
#include "led.h"
#include "imu.h"
#include "orca.h"
#include "delay.h"
#include "i2cxl_sonar.h"
#include "analog_monitor.h"
#include "lsm330dlc_driver.h"
#include "compass_hmc5883l.h"

NEW_TASK_SET(main_tasks, 10)

central_data_t *centralData;

/**
 * \brief	Function to call when the motors should be switched off
 */
void switch_off_motors(void);

task_set_t* tasks_get_main_taskset() 
{
	return &main_tasks;
}

void tasks_rc_user_channels(uint8_t *chanSwitch, int8_t *rc_check, int8_t *motor_state)
{
	
	remote_controller_get_channel_mode(chanSwitch);
	
	if ((remote_dsm2_rc_get_channel_neutral(RC_TRIM_P3) * RC_SCALEFACTOR) > 0.0f)
	{
		centralData->collision_avoidance = true;
	}
	else
	{
		centralData->collision_avoidance = false;
	}
	
	remote_controller_get_motor_state(motor_state);
	
	*rc_check = remote_dsm2_rc_check_receivers();
	}

void switch_off_motors(void)
{
	print_util_dbg_print("Switching off motors!\n");

	centralData->run_mode = MOTORS_OFF;
	centralData->mav_state = MAV_STATE_STANDBY;
	centralData->mav_mode = MAV_MODE_MANUAL_DISARMED;
	
	centralData->in_the_air = false;
}

void tasks_relevel_imu(void)
{
	uint32_t i,j;

	centralData->imu1.attitude.calibration_level = LEVELING;
	centralData->mav_state = MAV_STATE_CALIBRATING;
	centralData->mav_mode = MAV_MODE_PREFLIGHT;

	print_util_dbg_print("calibrating IMU...\n");

	for (j = 0;j < 3;j++)
	{
		centralData->imu1.attitude.raw_mag_mean[j] = (float)centralData->imu1.raw_channels[j + MAG_OFFSET];
	}

	for (i = 1000; i > 0; i--) 
	{
		tasks_run_imu_update(0);
		//mavlink_communication_update(&centralData->mavlink_communication);
		
		for (j = 0;j < 3;j++)
		{
			centralData->imu1.attitude.raw_mag_mean[j] = (1.0f - MAG_LPF) * centralData->imu1.attitude.raw_mag_mean[j] + MAG_LPF * ((float)centralData->imu1.raw_channels[j + MAG_OFFSET]);
		}

		delay_ms(5);
	}
	
	centralData->imu1.attitude.calibration_level = OFF;
	centralData->mav_state = MAV_STATE_STANDBY;
	centralData->mav_mode = MAV_MODE_MANUAL_DISARMED;
	
	print_util_dbg_print("IMU calibration done.\n");
}


task_return_t tasks_set_mav_mode_n_state(void* arg)
{
	uint8_t channelSwitches = 0;
	int8_t RC_check = 0;
	int8_t motor_switch = 0;
	
	float distFromHomeSqr;
	
	LED_Toggle(LED1);
	
	tasks_rc_user_channels(&channelSwitches,&RC_check, &motor_switch);
	
	switch(centralData->mav_state)
	{
		case MAV_STATE_BOOT:
			break;

		case MAV_STATE_CALIBRATING:
			break;

		case MAV_STATE_STANDBY:
			if (motor_switch == 1)
			{
				switch(channelSwitches)
				{
					case 0:
						print_util_dbg_print("Switching on the motors!\n");

						position_estimation_reset_home_altitude(&centralData->position_estimator);
						
						centralData->waypoint_set = false;
						centralData->run_mode = MOTORS_ON;
						centralData->mav_mode = MAV_MODE_MANUAL_ARMED;
						break;

					case 1:
						print_util_dbg_print("Switches not ready, both should be pushed!\n");
						break;

					case 2:
						print_util_dbg_print("Switches not ready, both should be pushed!\n");
						break;

					case 3:
						print_util_dbg_print("Switches not ready, both should be pushed!\n");
						break;
				}
			}
			if (centralData->run_mode == MOTORS_ON)
			{
				switch (channelSwitches)
				{
					case 0:
						centralData->mav_mode = MAV_MODE_MANUAL_ARMED;
						break;

					case 1:
						centralData->mav_mode = MAV_MODE_STABILIZE_ARMED;
						break;

					case 2:
						if (centralData->in_the_air)
						{
							centralData->mav_mode = MAV_MODE_GUIDED_ARMED;
							
							// Automatic take-off mode
							if (centralData->mav_mode_previous != MAV_MODE_GUIDED_ARMED)
							{
								centralData->automatic_take_off = true;
							}
						}
						break;

					case 3:
						if (centralData->in_the_air)
						{
							//centralData->mav_state = MAV_STATE_ACTIVE;
							centralData->mav_mode = MAV_MODE_AUTO_ARMED;
							
							// Automatic take-off mode
							if (centralData->mav_mode_previous != MAV_MODE_AUTO_ARMED)
							{
								centralData->automatic_take_off = true;
							}
						}
						break;
				}
				
				switch (centralData->mav_mode)
				{
					case MAV_MODE_MANUAL_ARMED:
						if (centralData->in_the_air)
						{
							centralData->mav_state = MAV_STATE_ACTIVE;
						}
						break;

					case MAV_MODE_STABILIZE_ARMED:
						if (centralData->in_the_air)
						{
							centralData->mav_state = MAV_STATE_ACTIVE;
						}
						break;

					case MAV_MODE_GUIDED_ARMED:
						// Automatic take-off mode
						if(centralData->automatic_take_off)
						{
							centralData->automatic_take_off = false;
							waypoint_handler_waypoint_take_off(&centralData->waypoint_handler);
						}
						
						distFromHomeSqr = SQR(centralData->position_estimator.localPosition.pos[X] - centralData->waypoint_hold_coordinates.pos[X]) + SQR(centralData->position_estimator.localPosition.pos[Y] - centralData->waypoint_hold_coordinates.pos[Y]) + SQR(centralData->position_estimator.localPosition.pos[Z] - centralData->waypoint_hold_coordinates.pos[Z]);
						if ((centralData->dist2wp_sqr <= 16.0f)&&(!centralData->automatic_take_off))
						{
							centralData->mav_state = MAV_STATE_ACTIVE;
							print_util_dbg_print("Automatic take-off finised, distFromHomeSqr (10x):");
							print_util_dbg_print_num(distFromHomeSqr * 10.0f,10);
							print_util_dbg_print(".\n");
						}
						break;

					case MAV_MODE_AUTO_ARMED:
						if(centralData->automatic_take_off)
						{
							centralData->automatic_take_off = false;
							waypoint_handler_waypoint_take_off(&centralData->waypoint_handler);
						}

						if (!centralData->waypoint_set)
						{
							waypoint_handler_waypoint_init(&centralData->waypoint_handler);
						}

						distFromHomeSqr = SQR(centralData->position_estimator.localPosition.pos[X] - centralData->waypoint_hold_coordinates.pos[X]) + SQR(centralData->position_estimator.localPosition.pos[Y] - centralData->waypoint_hold_coordinates.pos[Y]) + SQR(centralData->position_estimator.localPosition.pos[Z] - centralData->waypoint_hold_coordinates.pos[Z]);
						
						if ((centralData->dist2wp_sqr <= 16.0f)&&(!centralData->automatic_take_off))
						{
							centralData->mav_state = MAV_STATE_ACTIVE;
							print_util_dbg_print("Automatic take-off finised, distFromHomeSqr (10x):");
							print_util_dbg_print_num(distFromHomeSqr * 10.0f,10);
							print_util_dbg_print(".\n");
						}
						break;
				}
				switch (RC_check)
				{
					case 1:
						break;

					case -1:
						centralData->mav_state = MAV_STATE_CRITICAL;
						break;

					case -2:
						centralData->mav_state = MAV_STATE_CRITICAL;
						break;
				}
				if (remote_controller_get_thrust_from_remote() > -0.7f)
				{
					centralData->in_the_air = true;
				}
			}

			if (motor_switch == -1)
			{
				switch_off_motors();
			}
			
			break;

		case MAV_STATE_ACTIVE:
			switch(channelSwitches)
			{
				case 0:
					centralData->mav_mode= MAV_MODE_MANUAL_ARMED;
					break;

				case 1:
					centralData->mav_mode= MAV_MODE_STABILIZE_ARMED;
					break;

				case 2:
					centralData->mav_mode = MAV_MODE_GUIDED_ARMED;
					break;

				case 3:
					centralData->mav_mode = MAV_MODE_AUTO_ARMED;
					break;
			}
			
			switch (centralData->mav_mode)
			{
				case MAV_MODE_MANUAL_ARMED:
					break;

				case MAV_MODE_STABILIZE_ARMED:
					break;

				case MAV_MODE_GUIDED_ARMED:
					if (centralData->mav_mode_previous != MAV_MODE_GUIDED_ARMED)
					{
						waypoint_handler_waypoint_hold_init(&centralData->waypoint_handler,centralData->position_estimator.localPosition);
					}
					break;

				case MAV_MODE_AUTO_ARMED:
					if (centralData->mav_mode_previous != MAV_MODE_AUTO_ARMED)
					{
						centralData->auto_landing_enum = DESCENT_TO_SMALL_ALTITUDE;
						waypoint_handler_waypoint_hold_init(&centralData->waypoint_handler,centralData->position_estimator.localPosition);
					}

					if (!centralData->waypoint_set)
					{
						waypoint_handler_waypoint_init(&centralData->waypoint_handler);
					}

					waypoint_handler_waypoint_navigation_handler(&centralData->waypoint_handler);
					break;
			}
			
			if (motor_switch == -1)
			{
				switch_off_motors();
			}
		
			switch (RC_check)
			{
				case 1:
					break;

				case -1:
					centralData->mav_state = MAV_STATE_CRITICAL;
					break;

				case -2:
					centralData->mav_state = MAV_STATE_CRITICAL;
					break;
			}
			break;

		case MAV_STATE_CRITICAL:
			switch(channelSwitches)
			{
				case 0:
					centralData->mav_mode= MAV_MODE_MANUAL_ARMED;
					break;

				case 1:
					centralData->mav_mode= MAV_MODE_STABILIZE_ARMED;
					break;

				case 2:
					centralData->mav_mode = MAV_MODE_GUIDED_ARMED;
					break;

				case 3:
					centralData->mav_mode = MAV_MODE_AUTO_ARMED;
					break;
			}

			if (motor_switch == -1)
			{
				switch_off_motors();
			}
			
			switch (centralData->mav_mode)
			{
				case MAV_MODE_GUIDED_ARMED:
					// no break here

				case MAV_MODE_AUTO_ARMED:
					if (centralData->mav_state_previous != MAV_STATE_CRITICAL)
					{
						centralData->critical_behavior = CLIMB_TO_SAFE_ALT;
						centralData->critical_next_state = false;
					}
					
					waypoint_handler_waypoint_critical_handler(&centralData->waypoint_handler);
					break;
			}
			
			switch (RC_check)
			{
				case 1:  
					// !! only if receivers are back, switch into appropriate mode
					centralData->mav_state = MAV_STATE_ACTIVE;
					centralData->critical_behavior = CLIMB_TO_SAFE_ALT;
					centralData->critical_next_state = false;
					break;

				case -1:
					break;

				case -2:
					if (centralData->critical_landing)
					{
						centralData->mav_state = MAV_STATE_EMERGENCY;
					}

					break;
			}
			break;

		case MAV_STATE_EMERGENCY:
			centralData->mav_mode = MAV_MODE_MANUAL_DISARMED;
			switch (RC_check)
			{
				case 1:
					centralData->mav_state = MAV_STATE_STANDBY;
					break;

				case -1:
					break;

				case -2:
					break;
			}
			break;
	}
	
	centralData->mav_mode_previous = centralData->mav_mode;
	centralData->mav_state_previous = centralData->mav_state;
	
	if (centralData->simulation_mode_previous != centralData->simulation_mode)
	{
		simulation_switch_between_reality_n_simulation(&centralData->sim_model,centralData->servos);
			}
	centralData->simulation_mode_previous = centralData->simulation_mode;
	
	return TASK_RUN_SUCCESS;
}


void tasks_run_imu_update(void* arg) {
	if (centralData->simulation_mode == 1) 
	{
		simulation_update(&centralData->sim_model, 
					centralData->servos, 
					&(centralData->imu1), 
					&centralData->position_estimator);
		
		imu_update(	&centralData->imu1, 
					&centralData->pressure,
					&centralData->GPS_data );
		
		position_estimation_update(&centralData->position_estimator);
	} 
	else 
	{
		lsm330dlc_gyro_update(&(centralData->imu1.gyroData));
		lsm330dlc_acc_update(&(centralData->imu1.acceleroData));
		compass_hmc58831l_update(&(centralData->imu1.compassData));
		
		imu_get_raw_data(&(centralData->imu1));

		imu_update(	&(centralData->imu1), 
					&centralData->pressure, 
					&centralData->GPS_data);
		
		position_estimation_update(&centralData->position_estimator);
		
	}
}	


task_return_t tasks_run_stabilisation(void* arg) 
{
	tasks_run_imu_update(0);

	switch(centralData->mav_mode)
	{		
		case MAV_MODE_MANUAL_ARMED:
			centralData->controls = remote_controller_get_command_from_remote();
			centralData->controls.control_mode = ATTITUDE_COMMAND_MODE;
			centralData->controls.yaw_mode=YAW_RELATIVE;
			
			stabilisation_copter_cascade_stabilise(&centralData->stabilisation_copter, centralData->servos);
			break;

		case MAV_MODE_STABILIZE_ARMED:
			centralData->controls = remote_controller_get_command_from_remote();
			centralData->controls.control_mode = VELOCITY_COMMAND_MODE;
			centralData->controls.yaw_mode = YAW_RELATIVE;
			
			stabilisation_copter_get_velocity_vector_from_remote(centralData->controls.tvel,&centralData->stabilisation_copter);
			
			stabilisation_copter_cascade_stabilise(&centralData->stabilisation_copter, centralData->servos);
			
			break;

		case MAV_MODE_GUIDED_ARMED:
			centralData->controls = centralData->controls_nav;
			centralData->controls.control_mode = VELOCITY_COMMAND_MODE;
			
			if ((centralData->mav_state == MAV_STATE_CRITICAL) && (centralData->critical_behavior == FLY_TO_HOME_WP))
			{
				centralData->controls.yaw_mode = YAW_COORDINATED;
			}
			else
			{
				centralData->controls.yaw_mode = YAW_ABSOLUTE;
			}
			
			stabilisation_copter_cascade_stabilise(&centralData->stabilisation_copter, centralData->servos);
			
			break;

		case MAV_MODE_AUTO_ARMED:
			centralData->controls = centralData->controls_nav;
			centralData->controls.control_mode = VELOCITY_COMMAND_MODE;
			
			// if no waypoints are set, we do position hold therefore the yaw mode is absolute
			if (((centralData->waypoint_set&&(centralData->mav_state != MAV_STATE_STANDBY)))||((centralData->mav_state == MAV_STATE_CRITICAL)&&(centralData->critical_behavior == FLY_TO_HOME_WP)))
			{
				centralData->controls.yaw_mode = YAW_COORDINATED;
			}
			else
			{
				centralData->controls.yaw_mode = YAW_ABSOLUTE;
			}
			
			stabilisation_copter_cascade_stabilise(&centralData->stabilisation_copter, centralData->servos);
			break;
		
		case MAV_MODE_PREFLIGHT:
		case MAV_MODE_MANUAL_DISARMED:
		case MAV_MODE_STABILIZE_DISARMED:
		case MAV_MODE_GUIDED_DISARMED:
		case MAV_MODE_AUTO_DISARMED:
			centralData->run_mode = MOTORS_OFF;
			servo_pwm_failsafe(centralData->servos);
			break;
	}
	
	// !!! -- for safety, this should remain the only place where values are written to the servo outputs! --- !!!
	if (centralData->simulation_mode != 1) 
	{
		servo_pwm_set(centralData->servos);
	}
	
	return TASK_RUN_SUCCESS;
}

task_return_t tasks_run_gps_update(void* arg) 
{
	if (centralData->simulation_mode == 1) 
	{
		simulation_simulate_gps(&centralData->sim_model, &centralData->GPS_data);
	} 
	else 
	{
		gps_ublox_update(&centralData->GPS_data);
	}
	
	return TASK_RUN_SUCCESS;
}


task_return_t tasks_run_navigation_update(void* arg)
{	
	switch (centralData->mav_state)
	{
		case MAV_STATE_STANDBY:
			if (((centralData->mav_mode == MAV_MODE_GUIDED_ARMED)||(centralData->mav_mode == MAV_MODE_AUTO_ARMED)) && !centralData->automatic_take_off)
			{
				navigation_run(centralData->waypoint_hold_coordinates,&centralData->navigationData);
			}
			break;

		case MAV_STATE_ACTIVE:
			switch (centralData->mav_mode)
			{
				case MAV_MODE_AUTO_ARMED:
					if (centralData->waypoint_set)
					{
						navigation_run(centralData->waypoint_coordinates,&centralData->navigationData);
					}
					else
					{
						navigation_run(centralData->waypoint_hold_coordinates,&centralData->navigationData);
					}
					break;

				case MAV_MODE_GUIDED_ARMED:
					navigation_run(centralData->waypoint_hold_coordinates,&centralData->navigationData);
					break;
			}
			break;

		case MAV_STATE_CRITICAL:
			if ((centralData->mav_mode == MAV_MODE_GUIDED_ARMED)||(centralData->mav_mode == MAV_MODE_AUTO_ARMED))
			{
				navigation_run(centralData->waypoint_critical_coordinates,&centralData->navigationData);
			}
			break;
	}
	
	return TASK_RUN_SUCCESS;
}


task_return_t tasks_run_barometer_update(void* arg)
{
	central_data_t *central_data = central_data_get_pointer_to_struct();
	
	bmp085_get_pressure_data_slow(&(central_data->pressure));
	
	if (central_data->simulation_mode == 1) 
	{
		simulation_simulate_barometer(&centralData->sim_model, &(central_data->pressure));
	} 

	return TASK_RUN_SUCCESS;
}


task_return_t sonar_update(void* arg)
{
	central_data_t* central_data = central_data_get_pointer_to_struct();
	i2cxl_sonar_update(&central_data->i2cxl_sonar);
	
	return TASK_RUN_SUCCESS;
}

task_return_t adc_update(void* arg)
{
	central_data_t* central_data = central_data_get_pointer_to_struct();
	analog_monitor_update(&central_data->adc);
	
	return TASK_RUN_SUCCESS;
}

/**
 * \brief	Task to check if the time overpass the timer limit
 * 
 * \return	The status of execution of the task
 */
task_return_t control_waypoint_timeout (void* arg);

task_return_t control_waypoint_timeout (void* arg)
{
	waypoint_handler_control_time_out_waypoint_msg(&centralData->waypoint_handler);
	
	return TASK_RUN_SUCCESS;
}


void tasks_create_tasks() 
{
	scheduler_init(&main_tasks);
	
	centralData = central_data_get_pointer_to_struct();
	
	scheduler_register_task(&main_tasks, 0, 4000, RUN_REGULAR, &tasks_run_stabilisation, 0);
	
	scheduler_register_task(&main_tasks, 1, 15000, RUN_REGULAR, &tasks_run_barometer_update, 0);
	main_tasks.tasks[1].timing_mode = PERIODIC_RELATIVE;

	scheduler_register_task(&main_tasks, 2, 100000, RUN_REGULAR, &tasks_run_gps_update, 0);
	//scheduler_register_task(&main_tasks, , 100000, RUN_REGULAR, &radar_module_read, 0);

	scheduler_register_task(&main_tasks, 3, ORCA_TIME_STEP_MILLIS * 1000.0f, RUN_REGULAR, &tasks_run_navigation_update, 0);

	scheduler_register_task(&main_tasks, 4, 200000, RUN_REGULAR, &tasks_set_mav_mode_n_state, 0);
	
	scheduler_register_task(&main_tasks, 5, 4000, RUN_REGULAR, (task_function_t)&mavlink_communication_update, (task_argument_t)&centralData->mavlink_communication);
	
	// scheduler_register_task(&main_tasks, 6, 100000, RUN_REGULAR, &sonar_update, 0);
	scheduler_register_task(&main_tasks, 6, 100000, RUN_REGULAR, &adc_update, 0);
	
	scheduler_register_task(&main_tasks, 7, 10000, RUN_REGULAR, &control_waypoint_timeout, 0);
}