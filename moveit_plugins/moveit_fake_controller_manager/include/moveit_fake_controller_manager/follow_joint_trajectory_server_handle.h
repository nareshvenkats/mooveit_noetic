/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2013, Unbounded Robotics Inc.
 *  Copyright (c) 2012, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the Willow Garage nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

/* Author: Michael Ferguson, Ioan Sucan, E. Gil Jones */

#ifndef MOVEIT_PLUGINS_FOLLOW_TRAJECTORY_SERVER_HANDLE
#define MOVEIT_PLUGINS_FOLLOW_TRAJECTORY_SERVER_HANDLE

#include <actionlib/server/simple_action_server.h>
#include <control_msgs/FollowJointTrajectoryAction.h>

namespace moveit_fake_controller_manager
{
/*
 * This is generally used for arms, but could also be used for multi-dof hands,
 *   or anything using a control_mgs/FollowJointTrajectoryAction.
 */
class FollowJointTrajectoryServerHandle
{
public:
  actionlib::SimpleActionServer<control_msgs::FollowJointTrajectoryAction> follow_joint_trajectory_server;

  FollowJointTrajectoryServerHandle(const ros::NodeHandle node_handle, const std::string& name,
                                    const std::vector<std::string>& joints)
    : follow_joint_trajectory_server(node_handle, name, false)
  {
    follow_joint_trajectory_server.start();
  }

  ~FollowJointTrajectoryServerHandle()
  {
    follow_joint_trajectory_server.shutdown();
  }

// virtual bool sendTrajectory(const moveit_msgs::RobotTrajectory& trajectory);
#if 0  
  {
    ROS_ERROR("sendTrajectory on FollowJointTrajectoryServerHandle");
  }
#endif
#if 0
  virtual bool sendTrajectory(const moveit_msgs::RobotTrajectory& trajectory)
  {
    ROS_DEBUG_STREAM_NAMED("FollowJointTrajectoryServer", "new trajectory to " << name_);

    if (!controller_action_client_)
      return false;

    if (!trajectory.multi_dof_joint_trajectory.points.empty())
    {
      ROS_WARN_NAMED("FollowJointTrajectoryServer", "%s cannot execute multi-dof trajectories.", name_.c_str());
    }

    if (done_)
      ROS_DEBUG_STREAM_NAMED("FollowJointTrajectoryServer", "sending trajectory to " << name_);
    else
      ROS_DEBUG_STREAM_NAMED("FollowJointTrajectoryServer",
                             "sending continuation for the currently executed trajectory to " << name_);

    control_msgs::FollowJointTrajectoryGoal goal;
    goal.trajectory = trajectory.joint_trajectory;
    controller_action_client_->sendGoal(
        goal, boost::bind(&FollowJointTrajectoryServerHandle::controllerDoneCallback, this, _1, _2),
        boost::bind(&FollowJointTrajectoryServerHandle::controllerActiveCallback, this),
        boost::bind(&FollowJointTrajectoryServerHandle::controllerFeedbackCallback, this, _1));
    done_ = false;
    last_exec_ = moveit_controller_manager::ExecutionStatus::RUNNING;
    return true;
  }

protected:
  void controllerDoneCallback(const actionlib::SimpleClientGoalState& state,
                              const control_msgs::FollowJointTrajectoryResultConstPtr& result)
  {
    // Output custom error message for FollowJointTrajectoryResult if necessary
    if (result)
    {
      switch (result->error_code)
      {
        case control_msgs::FollowJointTrajectoryResult::INVALID_GOAL:
          ROS_WARN_STREAM("Server " << name_ << " failed with error code INVALID_GOAL");
          break;
        case control_msgs::FollowJointTrajectoryResult::INVALID_JOINTS:
          ROS_WARN_STREAM("Server " << name_ << " failed with error code INVALID_JOINTS");
          break;
        case control_msgs::FollowJointTrajectoryResult::OLD_HEADER_TIMESTAMP:
          ROS_WARN_STREAM("Server " << name_ << " failed with error code OLD_HEADER_TIMESTAMP");
          break;
        case control_msgs::FollowJointTrajectoryResult::PATH_TOLERANCE_VIOLATED:
          ROS_WARN_STREAM("Server " << name_ << " failed with error code PATH_TOLERANCE_VIOLATED");
          break;
        case control_msgs::FollowJointTrajectoryResult::GOAL_TOLERANCE_VIOLATED:
          ROS_WARN_STREAM("Server " << name_ << " failed with error code GOAL_TOLERANCE_VIOLATED");
          break;
      }
    }
    else
    {
      ROS_WARN_STREAM("Server " << name_ << ": no result returned");
    }

    finishServerExecution(state);
  }

  void controllerActiveCallback()
  {
    ROS_DEBUG_STREAM_NAMED("FollowJointTrajectoryServer", name_ << " started execution");
  }

  void controllerFeedbackCallback(const control_msgs::FollowJointTrajectoryFeedbackConstPtr& feedback)
  {
  }
#endif
};

MOVEIT_CLASS_FORWARD(FollowJointTrajectoryServerHandle);

}  // end namespace moveit_simple_controller_manager

#endif  // MOVEIT_PLUGINS_FOLLOW_TRAJECTORY_SERVER_HANDLE
