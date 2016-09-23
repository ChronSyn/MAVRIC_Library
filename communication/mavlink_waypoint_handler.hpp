/*******************************************************************************
 * Copyright (c) 2009-2016, MAV'RIC Development Team
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
 * \file mavlink_waypoint_handler.hpp
 *
 * \author MAV'RIC Team
 * \author Nicolas Dousse
 * \author Matthew Douglas
 *
 * \brief The MAVLink waypoint handler
 *
 ******************************************************************************/


#ifndef MAVLINK_WAYPOINT_HANDLER__
#define MAVLINK_WAYPOINT_HANDLER__

#include "communication/mavlink_message_handler.hpp"
#include "communication/mavlink_stream.hpp"
#include "communication/mavlink_message_handler.hpp"
#include "mission/navigation.hpp"
#include "mission/waypoint.hpp"

#define MAX_WAYPOINTS 10        ///< The maximal size of the waypoint list

/*
 * N.B.: Reference Frames and MAV_CMD_NAV are defined in "maveric.h"
 */

class Mavlink_waypoint_handler
{
public:

    struct conf_t
    {
        ;
    };


    /**
     * \brief   Initialize the waypoint handler
     *
     * \param   ins                         The reference to the Inertial Navigation System
     * \param   navigation                  The reference to the navigation structure
     * \param   message_handler             The reference to the message handler
     * \param   mavlink_stream              The reference to the MAVLink stream structure
     * \param   mission_handler_registry    The reference to the mission handler registry
     * \param   config                      The config structure (optional)
     *
     * \return  True if the init succeed, false otherwise
     */
    Mavlink_waypoint_handler(   const INS& ins,
                                const Navigation& navigation,
                                Mavlink_message_handler& message_handler,
                                const Mavlink_stream& mavlink_stream,
                                Mission_handler_registry& mission_handler_registry,
                                conf_t config = default_config());

    bool init();

    /**
     * \brief   Returns the number of waypoints
     *
     * \return  number of waypoints
     */
    inline uint16_t waypoint_count() const {return waypoint_count_;};

    /**
     * \brief   Returns the time that the waypoint list was received
     *
     * \return  Waypoint list reception time
     */
    inline uint64_t waypoint_received_time_ms() const { return waypoint_received_time_ms_; };

    /**
     * \brief   Default configuration
     *
     * \return  Config structure
     */
    static inline conf_t default_config();

    /**
     * \brief Gets the current waypoint
     *
     * \details If there is no waypoints in the list, creates a hold position
     *          waypoint as the first index in the list and returns it
     *
     * \return current waypoint
     */
    const Waypoint& current_waypoint() const;

    /**
     * \brief Gets the next waypoint if available
     *
     * \details If there is no waypoints in the list, creates a hold position
     *          waypoint as the first index in the list and returns it
     *
     * \return next waypoint
     */
    const Waypoint& next_waypoint() const;

    /**
     * \brief   Returns a waypoint from the list from a specific index
     *
     * \details If there is no waypoints in the list, creates a hold position
     *          waypoint as the first index in the list and returns it
     *
     * \return  waypoint_list_[i]
     */
    const Waypoint& waypoint_from_index(int i) const;

    /**
     * \brief   Returns the home waypoint
     *
     * \return  Home
     */
    const Waypoint& home_waypoint() const;

    /**
     * \brief   Sets the next waypoint as the current one. Should be called when
     * the current waypoint has been reached.
     */
    void advance_to_next_waypoint();

    /**
     * \brief   Gets the current waypoint index
     *
     * \return  Current waypoint index
     */
    uint16_t current_waypoint_index() const;

    /**
     * \brief   Sets the current waypoint index if possible
     *
     * \param   index   The new waypoint index
     *
     * \return  Success
     */
    bool set_current_waypoint_index(int index);

protected:
    Waypoint waypoint_list_[MAX_WAYPOINTS];                     ///< The array of all waypoints (max MAX_WAYPOINTS)
    uint16_t waypoint_count_;                                   ///< The total number of waypoints
    uint16_t current_waypoint_index_;                           ///< The current waypoint index

    Waypoint home_waypoint_;                                    ///< The home waypoint

    const Mavlink_stream& mavlink_stream_;                      ///< The reference to MAVLink stream object
    const INS& ins_;                                            ///< The pointer to the position estimation structure
    const Navigation& navigation_;                                    ///< The reference to the navigation object
    Mavlink_message_handler& message_handler_;                  ///< The reference to the mavlink message handler
    Mission_handler_registry& mission_handler_registry_;        ///< The reference to the mission handler registry
private:

    uint64_t waypoint_received_time_ms_;                        ///< The time that the waypoint list was received

    uint16_t requested_waypoint_count_;                         ///< The number of waypoints requested from the GCS

    uint32_t timeout_max_waypoint_;                             ///< The max waiting time for communication

    conf_t config_;


    /************************************************
     *      static member functions (callbacks)     *
     ************************************************/

    /**
     * \brief   Sets the home position in the local frame
     *
     * \param   waypoint_handler    The pointer to the object of the waypoint handler
     * \param   packet              The pointer to the structure of the MAVLink command message long
     *
     * \return  The MAV_RESULT of the command
     */
    static mav_result_t set_home(Mavlink_waypoint_handler* waypoint_handler, mavlink_command_long_t* packet);

    /**
     * \brief   Sends the number of onboard waypoint to MAVLink when asked by ground station
     *
     * \param   waypoint_handler        The pointer to the waypoint handler structure
     * \param   sysid                   The system ID
     * \param   msg                     The pointer to the received MAVLink message structure asking the send count
     */
    static void request_list_callback(Mavlink_waypoint_handler* waypoint_handler, uint32_t sysid, mavlink_message_t* msg);

    /**
     * \brief   Sends a given waypoint via a MAVLink message
     *
     * \param   waypoint_handler        The pointer to the waypoint handler structure
     * \param   sysid                   The system ID
     * \param   msg                     The pointer to the received MAVLink message structure asking for a waypoint
     */
    static void mission_request_callback(Mavlink_waypoint_handler* waypoint_handler, uint32_t sysid, mavlink_message_t* msg);

    /**
     * \brief   Receives a acknowledge message from MAVLink
     *
     * \param   waypoint_handler        The pointer to the waypoint handler structure
     * \param   sysid                   The system ID
     * \param   msg                     The received MAVLink message structure
     */
    static void mission_ack_callback(Mavlink_waypoint_handler* waypoint_handler, uint32_t sysid, mavlink_message_t* msg);

    /**
     * \brief   Receives the number of waypoints that the ground station is sending
     *
     * \param   waypoint_handler        The pointer to the waypoint handler structure
     * \param   sysid                   The system ID
     * \param   msg                     The received MAVLink message structure with the total number of waypoint
     */
    static void mission_count_callback(Mavlink_waypoint_handler* waypoint_handler, uint32_t sysid, mavlink_message_t* msg);

    /**
     * \brief   Receives a given waypoint and stores it in the local structure
     *
     * \param   waypoint_handler        The pointer to the waypoint handler structure
     * \param   sysid                   The system ID
     * \param   msg                     The received MAVLink message structure with the waypoint
     */
    static void mission_item_callback(Mavlink_waypoint_handler* waypoint_handler, uint32_t sysid, mavlink_message_t* msg);

    /**
     * \brief   Clears the waypoint list
     *
     * \param   waypoint_handler        The pointer to the waypoint handler structure
     * \param   sysid                   The system ID
     * \param   msg                     The received MAVLink message structure with the clear command
     */
    static void mission_clear_all_callback(Mavlink_waypoint_handler* waypoint_handler, uint32_t sysid, mavlink_message_t* msg);

    /**
     * \brief   Initialize a home waypoint at (0,0,0) at start up
     *
     * \details Is called by the constructor
     *
     */
    void init_homing_waypoint();
};


Mavlink_waypoint_handler::conf_t Mavlink_waypoint_handler::default_config()
{
    conf_t conf                                                = {};

    return conf;
};







#endif // MAVLINK_WAYPOINT_HANDLER__
