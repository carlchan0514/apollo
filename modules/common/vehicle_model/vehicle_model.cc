/******************************************************************************
 * Copyright 2019 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

#include "modules/common/vehicle_model/vehicle_model.h"
#include "modules/common/configs/config_gflags.h"
#include "modules/common/configs/vehicle_config_helper.h"
#include "modules/common/util/file.h"
#include "modules/common/vehicle_model/proto/vehicle_model.pb.h"
#include "modules/common/vehicle_state/proto/vehicle_state.pb.h"

namespace apollo {
namespace common {

bool VehicleModel::Predict(const double predicted_time_horizon,
                           const VehicleState& cur_vehicle_state,
                           VehicleState* predicted_vehicle_state) {
  VehicleModelConfig vehicle_model_config;

  const VehicleParam& vehicle_param =
      VehicleConfigHelper::GetConfig().vehicle_param();

  CHECK(apollo::common::util::GetProtoFromFile(
      FLAGS_vehicle_model_config_filename, &vehicle_model_config))
      << "Failed to load vehicle model config file "
      << FLAGS_vehicle_model_config_filename;

  if (vehicle_model_config.model_type() ==
      VehicleModelConfig::REAR_CENTERED_KINEMATIC_BICYCLE_MODEL) {
    // kinematic bicycle model centered at rear axis center by Euler forward
    // discretization
    double dt = vehicle_model_config.rc_kinematic_bicycle_model().dt();
    double wheel_base = vehicle_param.wheel_base();
    double cur_x = cur_vehicle_state.x();
    double cur_y = cur_vehicle_state.y();
    double cur_phi = cur_vehicle_state.heading();
    double cur_v = cur_vehicle_state.linear_velocity();
    double cur_steer = std::atan(cur_vehicle_state.kappa() * wheel_base);
    double cur_a = cur_vehicle_state.linear_acceleration();
    double next_x = cur_x;
    double next_y = cur_y;
    double next_phi = cur_phi;
    double next_v = cur_v;

    for (double i = 0.0; i <= predicted_time_horizon; i += dt) {
      next_x =
          cur_x + dt * (cur_v + 0.5 * dt * cur_a) *
                      std::cos(cur_phi + 0.5 * dt * cur_v *
                                             std::tan(cur_steer) / wheel_base);
      next_y =
          cur_y + dt * (cur_v + 0.5 * dt * cur_a) *
                      std::sin(cur_phi + 0.5 * dt * cur_v *
                                             std::tan(cur_steer) / wheel_base);
      next_phi = cur_phi + dt * (cur_v + 0.5 * dt * cur_a) *
                               std::tan(cur_steer) / wheel_base;
      next_v = cur_v + dt * cur_a;
      cur_x = next_x;
      cur_y = next_y;
      cur_phi = next_phi;
      cur_v = next_v;
    }

    predicted_vehicle_state->set_x(next_x);
    predicted_vehicle_state->set_y(next_y);
    predicted_vehicle_state->set_z(0.0);
    predicted_vehicle_state->set_heading(next_phi);
    predicted_vehicle_state->set_kappa(cur_vehicle_state.kappa());
    predicted_vehicle_state->set_linear_velocity(next_v);
    predicted_vehicle_state->set_linear_acceleration(
        cur_vehicle_state.linear_acceleration());
    return true;
  } else {
    ADEBUG << "model not implemented or not supported";
    return false;
  }
}

}  // namespace common
}  // namespace apollo