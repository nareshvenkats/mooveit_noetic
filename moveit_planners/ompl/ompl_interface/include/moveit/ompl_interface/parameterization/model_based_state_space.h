/*********************************************************************
 * Software License Agreement (BSD License)
 *
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
 *   * Neither the name of Willow Garage nor the names of its
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

/* Author: Ioan Sucan */

#ifndef MOVEIT_OMPL_INTERFACE_PARAMETERIZATION_MODEL_BASED_STATE_SPACE_
#define MOVEIT_OMPL_INTERFACE_PARAMETERIZATION_MODEL_BASED_STATE_SPACE_

#include <ompl/base/spaces/RealVectorStateSpace.h>
#include <moveit/robot_model/robot_model.h>
#include <moveit/robot_state/robot_state.h>
#include <moveit/kinematic_constraints/kinematic_constraint.h>
#include <moveit/constraint_samplers/constraint_sampler.h>
#include "../detail/same_shared_ptr.hpp"

namespace ompl_interface
{
namespace ob = ompl::base;

typedef boost::function<bool(const ob::State* from, const ob::State* to, const double t, ob::State* state)>
    InterpolationFunction;
typedef boost::function<double(const ob::State* state1, const ob::State* state2)> DistanceFunction;

struct ModelBasedStateSpaceSpecification
{
  ModelBasedStateSpaceSpecification(const robot_model::RobotModelConstPtr& robot_model,
                                    const robot_model::JointModelGroup* jmg)
    : robot_model_(robot_model), joint_model_group_(jmg)
  {
  }

  ModelBasedStateSpaceSpecification(const robot_model::RobotModelConstPtr& robot_model, const std::string& group_name)
    : robot_model_(robot_model), joint_model_group_(robot_model_->getJointModelGroup(group_name))
  {
    if (!joint_model_group_)
      throw std::runtime_error("Group '" + group_name + "'  was not found");
  }

  robot_model::RobotModelConstPtr robot_model_;
  const robot_model::JointModelGroup* joint_model_group_;
  robot_model::JointBoundsVector joint_bounds_;
};

class ModelBasedStateSpace : public ob::RealVectorStateSpace
{
public:
  class StateType : public ob::RealVectorStateSpace::StateType
  {
  public:
    enum
    {
      VALIDITY_KNOWN = 1,
      GOAL_DISTANCE_KNOWN = 2,
      VALIDITY_TRUE = 4,
      IS_START_STATE = 8,
      IS_GOAL_STATE = 16
    };

    StateType() : ob::RealVectorStateSpace::StateType(), tag(-1), flags(0), distance(0.0)
    {
    }

    void markValid(double d)
    {
      distance = d;
      flags |= GOAL_DISTANCE_KNOWN;
      markValid();
    }

    void markValid()
    {
      flags |= (VALIDITY_KNOWN | VALIDITY_TRUE);
    }

    void markInvalid(double d)
    {
      distance = d;
      flags |= GOAL_DISTANCE_KNOWN;
      markInvalid();
    }

    void markInvalid()
    {
      flags &= ~VALIDITY_TRUE;
      flags |= VALIDITY_KNOWN;
    }

    bool isValidityKnown() const
    {
      return flags & VALIDITY_KNOWN;
    }

    void clearKnownInformation()
    {
      flags = 0;
    }

    bool isMarkedValid() const
    {
      return flags & VALIDITY_TRUE;
    }

    bool isGoalDistanceKnown() const
    {
      return flags & GOAL_DISTANCE_KNOWN;
    }

    bool isStartState() const
    {
      return flags & IS_START_STATE;
    }

    bool isGoalState() const
    {
      return flags & IS_GOAL_STATE;
    }

    bool isInputState() const
    {
      return flags & (IS_START_STATE | IS_GOAL_STATE);
    }

    void markStartState()
    {
      flags |= IS_START_STATE;
    }

    void markGoalState()
    {
      flags |= IS_GOAL_STATE;
    }

    int tag;
    int flags;
    double distance;
  };

  ModelBasedStateSpace(const ModelBasedStateSpaceSpecification& spec);
  virtual ~ModelBasedStateSpace();

  void setInterpolationFunction(const InterpolationFunction& fun)
  {
    interpolation_function_ = fun;
  }

  void setDistanceFunction(const DistanceFunction& fun)
  {
    distance_function_ = fun;
  }

  virtual ob::State* allocState() const;
  virtual void freeState(ob::State* state) const;
  virtual unsigned int getDimension() const;
  virtual void enforceBounds(ob::State* state) const;
  virtual bool satisfiesBounds(const ob::State* state) const;

  virtual void copyState(ob::State* destination, const ob::State* source) const;
  virtual void interpolate(const ob::State* from, const ob::State* to, const double t, ob::State* state) const;
  virtual double distance(const ob::State* state1, const ob::State* state2) const;
  virtual bool equalStates(const ob::State* state1, const ob::State* state2) const;
  virtual double getMaximumExtent() const;
  virtual double getMeasure() const;

  virtual unsigned int getSerializationLength() const;
  virtual void serialize(void* serialization, const ob::State* state) const;
  virtual void deserialize(ob::State* state, const void* serialization) const;
  virtual double* getValueAddressAtIndex(ob::State* state, const unsigned int index) const;

  virtual ob::StateSamplerPtr allocDefaultStateSampler() const;

  virtual const std::string& getParameterizationType() const = 0;

  const robot_model::RobotModelConstPtr& getRobotModel() const
  {
    return spec_.robot_model_;
  }

  const robot_model::JointModelGroup* getJointModelGroup() const
  {
    return spec_.joint_model_group_;
  }

  const std::string& getJointModelGroupName() const
  {
    return getJointModelGroup()->getName();
  }

  const ModelBasedStateSpaceSpecification& getSpecification() const
  {
    return spec_;
  }

  virtual void printState(const ob::State* state, std::ostream& out) const;
  virtual void printSettings(std::ostream& out) const;

  /// Set the planning volume for the possible SE2 and/or SE3 components of the state space
  virtual void setPlanningVolume(double minX, double maxX, double minY, double maxY, double minZ, double maxZ);

  const robot_model::JointBoundsVector& getJointsBounds() const
  {
    return spec_.joint_bounds_;
  }

  /// Copy the data from an OMPL state to a set of joint states.
  // The joint states \b must be specified in the same order as the joint models in the constructor
  virtual void copyToRobotState(robot_state::RobotState& rstate, const ob::State* state) const;

  /// Copy the data from a set of joint states to an OMPL state.
  //  The joint states \b must be specified in the same order as the joint models in the constructor
  virtual void copyToOMPLState(ob::State* state, const robot_state::RobotState& rstate) const;

  /**
   * \brief Copy a single joint's values (which might have multiple variables) from a MoveIt! robot_state to an OMPL
   * state.
   * \param state - output OMPL state with single joint modified
   * \param robot_state - input MoveIt! state to get the joint value from
   * \param joint_model - the joint to copy values of
   * \param ompl_state_joint_index - the index of the joint in the ompl state (passed in for efficiency, you should
   * cache this index)
   *        e.g. ompl_state_joint_index = joint_model_group_->getVariableGroupIndex("virtual_joint");
   */
  virtual void copyJointToOMPLState(ob::State* state, const robot_state::RobotState& robot_state,
                                    const moveit::core::JointModel* joint_model, int ompl_state_joint_index) const;

  double getTagSnapToSegment() const;
  void setTagSnapToSegment(double snap);

protected:
  ModelBasedStateSpaceSpecification spec_;
  std::vector<robot_model::JointModel::Bounds> joint_bounds_storage_;
  std::vector<const robot_model::JointModel*> joint_model_vector_;
  unsigned int variable_count_;
  size_t state_values_size_;

  InterpolationFunction interpolation_function_;
  DistanceFunction distance_function_;

  double tag_snap_to_segment_;
  double tag_snap_to_segment_complement_;
};

typedef same_shared_ptr<ModelBasedStateSpace, ob::StateSpacePtr>::type ModelBasedStateSpacePtr;
}

#endif
