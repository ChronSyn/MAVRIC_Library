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
 * \file offboard_tag_search.c
 *
 * \author MAV'RIC Team
 * \author Matthew Douglas
 *
 * \brief Offboard camera control
 *
 ******************************************************************************/


#include "sensing/offboard_tag_search.hpp"

extern "C"
{
#include "hal/common/time_keeper.hpp"
#include "util/print_util.hpp"
#include "util/constants.hpp"
#include "util/maths.h"
}



Offboard_Tag_Search::Offboard_Tag_Search(const INS& ins, const ahrs_t& ahrs, Mavlink_communication_T<10, 10, 10, 10>& mavlink_communication, offboard_tag_search_conf_t config):
    conf_(config),
    is_camera_running_(false),
    last_update_us_(time_keeper_get_us()),
    ins_(ins),
    ahrs_(ahrs),
    mavlink_communication_(mavlink_communication)
{
    // Set picture count to 0
    picture_count_ = 0;
    tag_count_ = 0;
    start_tag_msg_count_ = 0;

    // Set position at photos to 0 for all threads
    for (int i = 0; i < offboard_threads_; i++)
    {
        position_at_photo_[i][0] = 0.0f;
        position_at_photo_[i][1] = 0.0f;
        position_at_photo_[i][2] = 0.0f;

        ahrs_at_photo_[i].qe.v[0] = 0.0f;
        ahrs_at_photo_[i].qe.v[1] = 0.0f;
        ahrs_at_photo_[i].qe.v[2] = 0.0f;
        ahrs_at_photo_[i].qe.s = 0.0f;
        ahrs_at_photo_[i].angular_speed[0] = 0.0f;
        ahrs_at_photo_[i].angular_speed[1] = 0.0f;
        ahrs_at_photo_[i].angular_speed[2] = 0.0f;
        ahrs_at_photo_[i].linear_acc[0] = 0.0f;
        ahrs_at_photo_[i].linear_acc[1] = 0.0f;
        ahrs_at_photo_[i].linear_acc[2] = 0.0f;
        ahrs_at_photo_[i].internal_state = ahrs_state_t::AHRS_READY;
        ahrs_at_photo_[i].last_update_s = 0.0f;

        has_photo_been_taken_[i] = false;
    }
}


bool Offboard_Tag_Search::update(bool camera_state)
{
    bool success = true;

    // Switch camera on and off
    bool has_camera_state_changed = false;
    if (camera_state != is_camera_running_)
    {
        has_camera_state_changed = true;
    }
    is_camera_running_ = camera_state;

    // Send the message now if needed
    if (has_camera_state_changed)
    {
        // Update timing
        uint32_t t      = time_keeper_get_us();
        //dt_s_           = (float)(t - last_update_us_) / 1000000.0f;
        last_update_us_ = t;

        mavlink_message_t msg;
        offboard_tag_search_telemetry_send_start_stop(this, &(mavlink_communication().mavlink_stream()), &msg);
        mavlink_communication().mavlink_stream().send(&msg);
    }

    return success;
}


bool Offboard_Tag_Search::is_healthy() const
{
    // Check last detection time
    if ((time_keeper_get_us() - last_update_us()) > conf_.max_acc_time_since_last_detection_us)
    {
        return false;
    }
    else
    {
        return true;
    }
}

const int Offboard_Tag_Search::offboard_threads() const
{
    return offboard_threads_;
}

const local_position_t Offboard_Tag_Search::position_at_photo(int index) const
{
    return position_at_photo_[index];
}

const ahrs_t Offboard_Tag_Search::ahrs_at_photo(int index) const
{
    return ahrs_at_photo_[index];
}

void Offboard_Tag_Search::set_position_at_photo(int index)
{
    position_at_photo_[index][0] = ins_.position_lf()[0];
    position_at_photo_[index][1] = ins_.position_lf()[1];
    position_at_photo_[index][2] = ins_.position_lf()[2];

    ahrs_at_photo_[index].qe.v[0] = ahrs_.qe.v[0];
    ahrs_at_photo_[index].qe.v[1] = ahrs_.qe.v[1];
    ahrs_at_photo_[index].qe.v[2] = ahrs_.qe.v[2];
    ahrs_at_photo_[index].qe.s = ahrs_.qe.s;
    ahrs_at_photo_[index].angular_speed[0] = ahrs_.angular_speed[0];
    ahrs_at_photo_[index].angular_speed[1] = ahrs_.angular_speed[1];
    ahrs_at_photo_[index].angular_speed[2] = ahrs_.angular_speed[2];
    ahrs_at_photo_[index].linear_acc[0] = ahrs_.linear_acc[0];
    ahrs_at_photo_[index].linear_acc[1] = ahrs_.linear_acc[1];
    ahrs_at_photo_[index].linear_acc[2] = ahrs_.linear_acc[2];
    ahrs_at_photo_[index].internal_state = ahrs_.internal_state;
    ahrs_at_photo_[index].last_update_s = ahrs_.last_update_s;
}

const float& Offboard_Tag_Search::last_update_us(void) const
{
    return last_update_us_;
}

const bool& Offboard_Tag_Search::is_camera_running() const
{
    return is_camera_running_;
}

int Offboard_Tag_Search::camera_id() const
{
    return conf_.camera_id;
}

float  Offboard_Tag_Search::allowable_horizontal_tag_offset_sqr() const
{
    return conf_.allowable_horizontal_tag_offset_sqr;
}

float Offboard_Tag_Search::max_acc_drone_height_from_camera_mm() const
{
    return conf_.max_acc_drone_height_from_camera_mm;
}

float Offboard_Tag_Search::tag_search_timeout_us() const
{
    return conf_.tag_search_timeout_us;
}

void Offboard_Tag_Search::update_last_update_us()
{
    last_update_us_ = time_keeper_get_us();
}

int Offboard_Tag_Search::camera_x_resolution() const
{
    return conf_.camera_res_x;
}

int Offboard_Tag_Search::camera_y_resolution() const
{
    return conf_.camera_res_y;
}

float Offboard_Tag_Search::camera_rotation() const
{
    return conf_.camera_rotation;
}

float Offboard_Tag_Search::camera_x_fov() const
{
    return conf_.camera_fov_x;
}

float Offboard_Tag_Search::camera_y_fov() const
{
    return conf_.camera_fov_y;
}

std::array<float,2>& Offboard_Tag_Search::tag_location()
{
    return tag_location_;
}

const int16_t& Offboard_Tag_Search::picture_count() const
{
    return picture_count_;
}

const int16_t& Offboard_Tag_Search::tag_count() const
{
    return tag_count_;
}

const int16_t& Offboard_Tag_Search::start_tag_msg_count() const
{
    return start_tag_msg_count_;
}

void Offboard_Tag_Search::increment_picture_count()
{
    picture_count_++;
}

void Offboard_Tag_Search::increment_tag_count()
{
    tag_count_++;
}

void Offboard_Tag_Search::increment_start_tag_msg_count()
{
    start_tag_msg_count_++;
}

Offboard_Tag_Search::land_on_tag_behavior_t Offboard_Tag_Search::land_on_tag_behavior() const
{
    return land_on_tag_behavior_;
}

void Offboard_Tag_Search::land_on_tag_behavior(land_on_tag_behavior_t behavior)
{
    land_on_tag_behavior_ = behavior;
}

Mavlink_communication_T<10, 10, 10, 10>& Offboard_Tag_Search::mavlink_communication()
{
    return mavlink_communication_;
}

bool Offboard_Tag_Search::has_photo_been_taken(int index) const
{
    return has_photo_been_taken_[index];
}

void Offboard_Tag_Search::set_has_photo_been_taken(int index, bool state)
{
    has_photo_been_taken_[index] = state;
}

void Offboard_Tag_Search::set_is_camera_running(bool is_camera_running)
{
    is_camera_running_ = is_camera_running;
}
