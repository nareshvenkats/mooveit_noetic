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

#ifndef MOVEIT_OMPL_INTERFACE_PLANNING_CONTEXT_MANAGER_
#define MOVEIT_OMPL_INTERFACE_PLANNING_CONTEXT_MANAGER_

#include <moveit/ompl_interface/model_based_planning_context.h>
#include <moveit/ompl_interface/parameterization/model_based_state_space_factory.h>
#include <moveit/constraint_samplers/constraint_sampler_manager.h>
#include <moveit/macros/class_forward.h>

#include <vector>
#include <string>
#include <map>

namespace ompl_interface
{
class PlanningContextManager
{
public:
  PlanningContextManager(const robot_model::RobotModelConstPtr& kmodel,
                         const constraint_samplers::ConstraintSamplerManagerPtr& csm);

  /** @brief Specify configurations for the planners.
      @param pconfig Configurations for the different planners */
  void setPlannerConfigurations(const planning_interface::PlannerConfigurationMap& pconfig);

  /** @brief Return the previously set planner configurations */
  const planning_interface::PlannerConfigurationMap& getPlannerConfigurations() const
  {
    return planner_configs_;
  }

  const robot_model::RobotModelConstPtr& getRobotModel() const
  {
    return kmodel_;
  }

  // ModelBasedPlanningContextPtr getLastPlanningContext() const;

  ModelBasedPlanningContextPtr getPlanningContext(const planning_scene::PlanningSceneConstPtr& planning_scene,
                                                  const planning_interface::MotionPlanRequest& req,
                                                  moveit_msgs::MoveItErrorCodes& error_code) const;

  void registerStateSpaceFactory(const ModelBasedStateSpaceFactoryPtr& factory)
  {
    state_space_factories_[factory->getType()] = factory;
  }

  const std::map<std::string, ModelBasedStateSpaceFactoryPtr>& getRegisteredStateSpaceFactories() const
  {
    return state_space_factories_;
  }

protected:
  typedef boost::function<const ModelBasedStateSpaceFactoryPtr&(const std::string&)> StateSpaceFactoryTypeSelector;

  void registerDefaultStateSpaces();

  /** \brief This is the function that constructs new planning contexts if no previous ones exist that are suitable */
  ModelBasedPlanningContextPtr getPlanningContext(const planning_interface::PlannerConfigurationSettings& config,
                                                  const StateSpaceFactoryTypeSelector& factory_selector,
                                                  const moveit_msgs::MotionPlanRequest& req) const;

  const ModelBasedStateSpaceFactoryPtr& getStateSpaceFactory1(const std::string& group_name,
                                                              const std::string& factory_type) const;
  const ModelBasedStateSpaceFactoryPtr& getStateSpaceFactory2(const std::string& group_name,
                                                              const moveit_msgs::MotionPlanRequest& req) const;

  /** \brief The kinematic model for which motion plans are computed */
  robot_model::RobotModelConstPtr kmodel_;

  constraint_samplers::ConstraintSamplerManagerPtr constraint_sampler_manager_;

  std::map<std::string, ModelBasedStateSpaceFactoryPtr> state_space_factories_;

  /** \brief All the existing planning configurations. The name
      of the configuration is the key of the map. This name can
      be of the form "group_name[config_name]" if there are
      particular configurations specified for a group, or of the
      form "group_name" if default settings are to be used. */
  planning_interface::PlannerConfigurationMap planner_configs_;

private:
  // MOVEIT_CLASS_FORWARD(LastPlanningContext);
  // LastPlanningContextPtr last_planning_context_;

  // MOVEIT_CLASS_FORWARD(CachedContexts);
  // CachedContextsPtr cached_contexts_;
};
}

#endif
