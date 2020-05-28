/*******************************************************************************
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

/*      Title     : collision_check_thread.cpp
 *      Project   : moveit_jog_arm
 *      Created   : 1/11/2019
 *      Author    : Brian O'Neil, Andy Zelenak, Blake Anderson
 */

#include <std_msgs/Float64.h>

#include <moveit_jog_arm/collision_check_thread.h>
#include <moveit_jog_arm/boost_pool_allocation.h>

static const std::string LOGNAME = "collision_check_thread";
static const double MIN_RECOMMENDED_COLLISION_RATE = 10;

namespace moveit_jog_arm
{
// Constructor for the class that handles collision checking
CollisionCheckThread::CollisionCheckThread(
    ros::NodeHandle& nh, const moveit_jog_arm::JogArmParameters& parameters,
    const planning_scene_monitor::PlanningSceneMonitorPtr& planning_scene_monitor)
  : nh_(nh)
  , parameters_(parameters)
  , planning_scene_monitor_(planning_scene_monitor)
  , self_velocity_scale_coefficient_(-log(0.001) / parameters.self_collision_proximity_threshold)
  , scene_velocity_scale_coefficient_(-log(0.001) / parameters.scene_collision_proximity_threshold)
  , period_(1. / parameters_.collision_check_rate)
{
  // Init collision request
  collision_request_.group_name = parameters_.move_group_name;
  collision_request_.distance = true;  // enable distance-based collision checking

  if (parameters_.collision_check_rate < MIN_RECOMMENDED_COLLISION_RATE)
    ROS_WARN_STREAM_THROTTLE_NAMED(5, LOGNAME, "Collision check rate is low, increase it in yaml file if CPU allows");

  // subscribe to joints
  joint_state_sub_ = nh_.subscribe(parameters.joint_topic, 1, &CollisionCheckThread::jointStateCB, this);

  // Publish to internal namespace
  ros::NodeHandle internal_nh("~internal");
  collision_velocity_scale_pub_ = internal_nh.advertise<std_msgs::Float64>("collision_velocity_scale", 1);

  // Wait for incoming topics to appear
  ROS_DEBUG_NAMED(LOGNAME, "Waiting for JointState topic");
  ros::topic::waitForMessage<sensor_msgs::JointState>(parameters_.joint_topic);

  current_state_ = std::make_unique<moveit::core::RobotState>(getLockedPlanningSceneRO()->getCurrentState());
  acm_ = getLockedPlanningSceneRO()->getAllowedCollisionMatrix();
}

planning_scene_monitor::LockedPlanningSceneRO CollisionCheckThread::getLockedPlanningSceneRO() const
{
  return planning_scene_monitor::LockedPlanningSceneRO(planning_scene_monitor_);
}

void CollisionCheckThread::start()
{
  timer_ = nh_.createTimer(period_, &CollisionCheckThread::run, this);
}

void CollisionCheckThread::stop()
{
  timer_.stop();
}

void CollisionCheckThread::run(const ros::TimerEvent& timer_event)
{
  // Log last loop duration and warn if it was longer than period
  if (timer_event.profile.last_duration.toSec() < period_.toSec())
  {
    ROS_DEBUG_STREAM_THROTTLE_NAMED(10, LOGNAME, "last_duration: " << timer_event.profile.last_duration.toSec() << " ("
                                                                   << period_.toSec() << ")");
  }
  else
  {
    ROS_WARN_STREAM_THROTTLE_NAMED(1, LOGNAME, "last_duration: " << timer_event.profile.last_duration.toSec() << " > "
                                                                 << period_.toSec());
  }

  if (paused_)
  {
    return;
  }

  {
    // Copy the latest joint state
    const std::lock_guard<std::mutex> lock(CollisionCheckThread);
    for (std::size_t i = 0; i < latest_joint_state_->position.size(); ++i)
      current_state_->setJointPositions(latest_joint_state_->name[i], &latest_joint_state_->position[i]);
  }

  current_state_->updateCollisionBodyTransforms();
  collision_detected_ = false;

  // Do a thread-safe distance-based collision detection
  collision_result_.clear();
  getLockedPlanningSceneRO()->getCollisionEnv()->checkRobotCollision(collision_request_, collision_result_,
                                                                     *current_state_);
  scene_collision_distance_ = collision_result_.distance;
  collision_detected_ |= collision_result_.collision;

  collision_result_.clear();
  getLockedPlanningSceneRO()->getCollisionEnvUnpadded()->checkSelfCollision(collision_request_, collision_result_,
                                                                            *current_state_, acm_);
  self_collision_distance_ = collision_result_.distance;
  collision_detected_ |= collision_result_.collision;

  velocity_scale_ = 1;
  // If we're definitely in collision, stop immediately
  if (collision_detected_)
  {
    velocity_scale_ = 0;
  }

  // If we are far from a collision, velocity_scale should be 1.
  // If we are very close to a collision, velocity_scale should be ~zero.
  // When scene_collision_proximity_threshold is breached, start decelerating exponentially.
  if (scene_collision_distance_ < parameters_.scene_collision_proximity_threshold)
  {
    // velocity_scale = e ^ k * (collision_distance - threshold)
    // k = - ln(0.001) / collision_proximity_threshold
    // velocity_scale should equal one when collision_distance is at collision_proximity_threshold.
    // velocity_scale should equal 0.001 when collision_distance is at zero.
    velocity_scale_ =
        std::min(velocity_scale_, exp(scene_velocity_scale_coefficient_ *
                                      (scene_collision_distance_ - parameters_.scene_collision_proximity_threshold)));
  }

  if (self_collision_distance_ < parameters_.self_collision_proximity_threshold)
  {
    velocity_scale_ =
        std::min(velocity_scale_, exp(self_velocity_scale_coefficient_ *
                                      (self_collision_distance_ - parameters_.self_collision_proximity_threshold)));
  }

  // publish message
  {
    auto msg = make_shared_from_pool<std_msgs::Float64>();
    msg->data = velocity_scale_;
    collision_velocity_scale_pub_.publish(msg);
  }
}

void CollisionCheckThread::jointStateCB(const sensor_msgs::JointStateConstPtr& msg)
{
  const std::lock_guard<std::mutex> lock(joint_state_mutex_);
  latest_joint_state_ = msg;
}

void CollisionCheckThread::setPaused(bool paused)
{
  paused_ = paused;
}

}  // namespace moveit_jog_arm
