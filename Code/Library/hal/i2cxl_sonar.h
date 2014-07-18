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
* \file i2cxl_sonar.h
*
* This file is the driver for the sonar module using i2C communication protocol
*/


#ifndef I2CXL_SONAR_H_
#define I2CXL_SONAR_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include "scheduler.h"
#include <stdint.h>

/**
 * \brief structure of the i2cxl_sonar module
*/
typedef struct 
{
	uint8_t i2c_address;		///< address of the sonar module
	uint16_t distance_cm;		///< measured distance in centimeters
	float distance_m;			///< measured distance in meters
} i2cxl_sonar_t;

/**
 * \brief Initializes the I2CXL sonar data struct and the i2c bus
 * 
 * \param i2c_sonar pointer to the i2c_sonar Data structure
 */
void i2cxl_sonar_init(i2cxl_sonar_t* i2c_sonar);

/**
 * \brief Reads last value from sensor and start new recording
 * \details This function should be called at a frequency lower than 10Hz
 * 
 * \param i2c_sonar Data struct
 */
void i2cxl_sonar_update(i2cxl_sonar_t* i2c_sonar);


/**
 * \brief	Task to send the mavlink sonar message
 * 
 * \param i2c_sonar Data struct
 *
 * \return	The status of execution of the task
 */
task_return_t i2cxl_send_sonar(i2cxl_sonar_t* i2cxl_sonar);

#ifdef __cplusplus
	}
#endif

#endif /* I2CXL_SONAR_H */