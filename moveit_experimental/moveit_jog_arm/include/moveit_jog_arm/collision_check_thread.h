/*******************************************************************************
 * Title     : collision_check_thread.h
 * Project   : moveit_jog_arm
 * Created   : 1/11/2019
 * Author    : Brian O'Neil, Andy Zelenak, Blake Anderson
 *
 * BSD 3-Clause License
 *
 * Copyright (c) 2019, Los Alamos National Security, LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#pragma once

#include <mutex>

#include <moveit/robot_model_loader/robot_model_loader.h>
#include <moveit/planning_scene_monitor/planning_scene_monitor.h>
#include <sensor_msgs/JointState.h>

#include "jog_arm_data.h"
#include "low_pass_filter.h"

namespace moveit_jog_arm
{
class CollisionCheckThread
{
public:
  /** \brief Constructor
   *  \param parameters: common settings of jog_arm
   *  \param planning_scene_monitor: PSM should have scene monitor and state monitor
   *                                 already started when passed into this class
   */
  CollisionCheckThread(ros::NodeHandle& nh, const moveit_jog_arm::JogArmParameters& parameters,
                       const planning_scene_monitor::PlanningSceneMonitorPtr& planning_scene_monitor);

  /** \brief start and stop the Thread */
  void start();
  void stop();

  /** \brief Pause or unpause processing jog commands while keeping the threads alive */
  void setPaused(bool paused);

private:
  void run(const ros::TimerEvent& timer_event);
  planning_scene_monitor::LockedPlanningSceneRO getLockedPlanningSceneRO() const;
  void jointStateCB(const sensor_msgs::JointStateConstPtr& msg);

  // ROS node handle
  ros::NodeHandle nh_;

  // Parameters
  const moveit_jog_arm::JogArmParameters parameters_;

  // Pointer to the collision environment
  planning_scene_monitor::PlanningSceneMonitorPtr planning_scene_monitor_;

  // Robot state and collision matrix from planning scene
  std::unique_ptr<moveit::core::RobotState> current_state_;
  collision_detection::AllowedCollisionMatrix acm_;

  // Scale robot velocity according to collision proximity and user-defined thresholds.
  // I scaled exponentially (cubic power) so velocity drops off quickly after the threshold.
  // Proximity decreasing --> decelerate
  double velocity_scale_ = 1;
  double self_collision_distance_ = 0;
  double scene_collision_distance_ = 0;
  bool collision_detected_ = false;
  bool paused_ = false;

  const double self_velocity_scale_coefficient_;
  const double scene_velocity_scale_coefficient_;

  // collision request
  collision_detection::CollisionRequest collision_request_;
  collision_detection::CollisionResult collision_result_;

  // ROS
  ros::Timer timer_;
  ros::Duration period_;
  ros::Subscriber joint_state_sub_;
  ros::Publisher collision_velocity_scale_pub_;

  // Latest joint state, updated by ROS callback
  mutable std::mutex joint_state_mutex_;
  sensor_msgs::JointStateConstPtr latest_joint_state_;
};
}  // namespace moveit_jog_arm
