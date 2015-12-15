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
 * \file pitot_tube_analog_telemetry.h
 * 
 * \author MAV'RIC Team
 * \author Matthew Douglas
 *   
 * \brief Analog pitot tube sensor telemetry functions
 *
 ******************************************************************************/


#ifndef PITOT_TUBE_ANALOG_TELEMETRY_H_
#define PITOT_TUBE_ANALOG_TELEMETRY_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "pitot_tube_analog.h"
#include "mavlink_stream.h"
#include "mavlink_message_handler.h"



/**
 * \brief							Initialize the servo mix
 * 
 * \param	pitot_tube_analog		The pointer to the pitot_tube_analog structure
 * \param	mavlink_handler			The pointer to the MAVLink message handler
 *
 * \return	True if the init succeed, false otherwise
 */
bool pitot_tube_analog_telemetry_init(pitot_tube_analog_t* pitot_tube_analog, mavlink_message_handler_t *mavlink_handler);

/**
 * \brief 	Send pitot tube and useful data for debuging.
 *
 * \param	pitot_tube_analog	Pointer to the analog pitot tube sensor data structure
 * \param	mavlink_stream		Pointer to mavlink stream structure
 * \param	msg					Pointer to the message structure
 */
void pitot_tube_analog_telemetry_send(pitot_tube_analog_t* pitot_tube_analog, const mavlink_stream_t* mavlink_stream, mavlink_message_t* msg);


#ifdef __cplusplus
}
#endif

#endif