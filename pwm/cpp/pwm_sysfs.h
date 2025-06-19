// Copyright (c) 2025 by T3 Foundation. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//     https://docs.t3gemstone.org/en/license
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#ifndef PWM_SYSFS_H
#define PWM_SYSFS_H

#include <cstdint>
#include <string_view>

class PwmSysfs
{
  public:
    PwmSysfs(uint8_t chip_no, uint8_t channel_no, uint32_t period_ns, uint32_t duty_cycle_ns);
    ~PwmSysfs();

    PwmSysfs(const PwmSysfs&) = delete;
    PwmSysfs& operator=(const PwmSysfs&) = delete;

    int initialize();
    int is_initialized();
    int set_enable(std::string_view value);

  private:
    std::string_view m_pwm_sysfs {"/sys/class/pwm"};
    bool m_is_initialized {};
    uint8_t m_chip_no {};
    uint8_t m_channel_no {};
    uint32_t m_period_ns {};
    uint32_t m_duty_cycle_ns {};
    char m_path[256];
    char m_value[256];

    int write_to_file(std::string_view path, std::string_view value);
};

#endif // PWM_SYSFS_H
