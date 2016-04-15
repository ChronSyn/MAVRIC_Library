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
 * \file saccade_controller.c
 *
 * \author Darius Merk
 *
 * \brief   Saccade controller for indoors collision free navigation based on optic flow
 *
 ******************************************************************************/


#include "saccade_controller.hpp"
 #include "hal/common/time_keeper.hpp"

extern "C"
{
    #include "util/maths.h"
    #include "util/quick_trig.h"
}

#include <iostream>
#include <fstream>

static std::ofstream logfile;

//------------------------------------------------------------------------------
// PUBLIC FUNCTIONS IMPLEMENTATION
//------------------------------------------------------------------------------
Saccade_controller::Saccade_controller( Flow& flow_left,
                                        Flow& flow_right,
                                        const ahrs_t& ahrs,
                                        saccade_controller_conf_t config ):
  flow_left_(flow_left),
  flow_right_(flow_right),
  ahrs_(ahrs)
{
    // Init members
    gain_            = config.gain_;
    threshold_       = config.threshold_;
    goal_direction_  = config.goal_direction_;
    pitch_           = config.pitch_;

    last_saccade_ = 0.0f;

    saccade_state_ = SACCADE;

    attitude_command_.rpy[0]  = 0.0f;
    attitude_command_.rpy[1]  = 0.0f;
    attitude_command_.rpy[2]  = 0.0f;
    attitude_command_.quat    = coord_conventions_quaternion_from_rpy(attitude_command_.rpy);

    // 125 points along the 160 pixels of the camera, start at pixel number 17 finish at number 142 such that the total angle covered by the 125 points is 140.625 deg.
    // float angle_between_points = (140.625 / N_points);
    float angle_between_points = (2.*180. / 160.);

    // Init azimuth angles
    for (uint32_t i = 0; i < N_points; ++i)
    {
        // azimuth_[i]             = (-160.875 + i * angle_between_points) * (PI / 180.0f);
        azimuth_[i]             = (-173.25 + i * angle_between_points) * (PI / 180.0f);
        inv_sin_azimuth_[i]     = 1.0f/quick_trig_sin(azimuth_[i]);
        // azimuth_[i + N_points]  = (  19.125 + i * angle_between_points) * (PI / 180.0f);
        azimuth_[i + N_points]  = (  9. + i * angle_between_points) * (PI / 180.0f);
        inv_sin_azimuth_[i + N_points]  = 1.0f/quick_trig_sin(azimuth_[i + N_points]);

        cos_azimuth_[i] = quick_trig_cos(azimuth_[i]);
        cos_azimuth_[i + N_points] = quick_trig_cos(azimuth_[i + N_points]);
        sin_azimuth_[i] = quick_trig_sin(azimuth_[i]);
        sin_azimuth_[i + N_points] = quick_trig_sin(azimuth_[i + N_points]);
    }

    logfile.open("log.csv");
    // logfile << "movement_direction,posx,posy,posz,q0,q1,q2,q3,";
    logfile << "movement_direction,q0,q1,q2,q3,";
    for (uint32_t i = 0; i < 179; i++)
    {
        logfile << "of" << i << ",";
    }
    logfile << "of" << 179 << std::endl;
      /* code */
}

bool Saccade_controller::init(void)
{
    return true;
}

/*void Saccade_controller::set_direction(heading_angle)
{
  Voir comment définir ca
}*/


#include <stdlib.h>

bool Saccade_controller::update()
{
    // Random number generation for the noise, the value of the noise is between 0 and 0.5. A new number is generated at each time.
    // ATTENTION CHECK THAT THE NOISE IS RANDOM AND ISN'T 10 TIMES THE SAME IN 1S FOR EXAMPLE
    // float noise = 0.0f;

    // Calculate for both left and right the sum of the relative nearnesses
    // which are each given by RN = OF/sin(angle),
    for (uint32_t i = 0; i < N_points; ++i)
    {
        relative_nearness_[i]             = maths_f_abs(flow_left_.of.x[i]);// * inv_sin_azimuth_[i]);
        relative_nearness_[i + N_points]  = maths_f_abs(flow_right_.of.x[i]);// * inv_sin_azimuth_[i + N_points]);
    }


    // Calculate the comanv's x and y components, to then calculate can and NOD.
    float comanv_x = 0.0f;
    float comanv_y = 0.0f;
    for (uint32_t i = 0; i < 2*N_points; i++)
    {
        comanv_x += cos_azimuth_[i] * relative_nearness_[i];
        comanv_y += sin_azimuth_[i] * relative_nearness_[i];

    }


    // Intermediate variables :
    // can is the norm of the comanv vector,
    // nearest object direction gives the angle in radians to the azimuth_[i])


    // Sigmoid function for direction choice, it takes the can, a threshold and
    // a gain and describes how important it is for the drone to perform a saccade

    float weighted_function = 1.0f;

    // Calculation of the can and cad

    can_ = maths_fast_sqrt(comanv_x * comanv_x + comanv_y * comanv_y);

    float nearest_object_direction = 0.0f;
    if (comanv_x != 0.0f)
    {
        nearest_object_direction = atan2(comanv_y, comanv_x);
    }

    //float weighted_function = 1/(1 + pow(can_/threshold_ , - gain_));
    cad_ = nearest_object_direction + PI;


    aero_attitude_t current_rpy = coord_conventions_quat_to_aero(ahrs_.qe);
    float movement_direction  = 0.0f;
    float heading_error = 0.0f;
    float of_sum = 0.0f;

    switch (saccade_state_)
    {
        // This is the case where we are performing a saccade
        case SACCADE:
            heading_error = maths_f_abs( maths_calc_smaller_angle(attitude_command_.rpy[2] - current_rpy.rpy[2]) );
            if ( heading_error < 0.1)
            {
                attitude_command_.rpy[0]  = 0.0f;
                attitude_command_.rpy[1]  = pitch_;
                attitude_command_.quat    = coord_conventions_quaternion_from_rpy(attitude_command_.rpy);
                last_saccade_             = time_keeper_get_s();
                saccade_state_            = INTERSACCADE;
            }

            // printf("SACCADE\n");
        break;
        // In this case, we are now in intersaccadic phase
        case INTERSACCADE:
            // printf("INTERSACCADE\n");
            if ( (time_keeper_get_s() - last_saccade_) > 2.0f )
            {
                // Calculation of the movement direction (in radians)
                // movement_direction = weighted_function * cad_;//+ (1-weighted_function) * (goal_direction_ + noise);
                // for (uint32_t i = 45; i < 90; ++i)
                for (uint32_t i = 65; i < 75; ++i)
                {
                    of_sum +=  flow_left_.of.x[i];
                }
                // for (uint32_t i = 0; i < 45; ++i)
                for (uint32_t i = 15; i < 25; ++i)
                {
                    of_sum +=  flow_right_.of.x[i];
                }
                // if ( of_sum >= 0.0f )
                // {
                //     movement_direction = (5+rand()%5) * 0.05f * PI;
                // }
                // else
                // {
                //     movement_direction = - (5+rand()%5) * 0.05f * PI;
                // }
                movement_direction = - maths_sigmoid(20.0f * of_sum) * 0.5 * PI + (rand()%100-50)*0.001f;

                // log
                logfile << movement_direction << ",";
                // logfile << flow_left_.dynamic_model_.position_lf()[0] << "," << << flow_left_.dynamic_model_.position_lf()[1] << "," << flow_left_.dynamic_model_.position_lf()[2] << ",";
                logfile << ahrs_.qe.s << "," << ahrs_.qe.v[0] << "," << ahrs_.qe.v[1]<< "," << ahrs_.qe.v[2] << ",";
                for (uint32_t i = 0; i < 90; ++i)
                {
                  logfile << flow_left_.of.x[i] << ", ";
                }
                for (uint32_t i = 0; i < 89; ++i)
                {
                  logfile << flow_right_.of.x[i] << ", ";
                }
                logfile << flow_right_.of.x[89] << std::endl;

                // movement_direction = (float)(rand() % 360) * (PI / 180.0f);
                // printf("%f\n", movement_direction);
                attitude_command_.rpy[0]  = 0.0f;
                attitude_command_.rpy[1]  = 0.0f;
                attitude_command_.rpy[2]  += movement_direction;
                attitude_command_.quat    = coord_conventions_quaternion_from_rpy(attitude_command_.rpy);
                saccade_state_            = SACCADE;
            }
        break;
    }


    return true;
}
