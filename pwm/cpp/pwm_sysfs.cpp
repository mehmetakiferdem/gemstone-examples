// Copyright (c) 2025 by T3 Foundation. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//     https://www.t3gemstone.org/license
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "pwm_sysfs.h"
#include <cstdio>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

PwmSysfs::PwmSysfs(uint8_t chip_no, uint8_t channel_no, uint32_t period_ns, uint32_t duty_cycle_ns)
    : m_chip_no {chip_no}, m_channel_no {channel_no}, m_period_ns {period_ns}, m_duty_cycle_ns {duty_cycle_ns} {};

PwmSysfs::~PwmSysfs()
{
    if (m_is_initialized)
    {
        set_enable("0");
    }
};

int PwmSysfs::initialize()
{
    snprintf(m_path, sizeof(m_path), "%s/pwmchip%d/export", m_pwm_sysfs.data(), m_chip_no);
    snprintf(m_value, sizeof(m_value), "%d", m_channel_no);
    if (write_to_file(m_path, m_value) < 0)
    {
        printf("Note: PWM channel might already be exported\n");
    }

    snprintf(m_path, sizeof(m_path), "%s/pwmchip%d/pwm%d/period", m_pwm_sysfs.data(), m_chip_no, m_channel_no);
    snprintf(m_value, sizeof(m_value), "%d", m_period_ns);
    if (write_to_file(m_path, m_value) < 0)
    {
        fprintf(stderr, "Failed to set PWM period\n");
        return 1;
    }

    snprintf(m_path, sizeof(m_path), "%s/pwmchip%d/pwm%d/duty_cycle", m_pwm_sysfs.data(), m_chip_no, m_channel_no);
    snprintf(m_value, sizeof(m_value), "%d", m_duty_cycle_ns);
    if (write_to_file(m_path, m_value) < 0)
    {
        fprintf(stderr, "Failed to set PWM duty cycle\n");
        return 1;
    }

    m_is_initialized = true;
    return 0;
};

int PwmSysfs::is_initialized()
{
    return m_is_initialized;
}

int PwmSysfs::set_enable(std::string_view value)
{
    snprintf(m_path, sizeof(m_path), "%s/pwmchip%d/pwm%d/enable", m_pwm_sysfs.data(), m_chip_no, m_channel_no);
    if (write_to_file(m_path, value) < 0)
    {
        return 1;
    }
    return 0;
};

int PwmSysfs::write_to_file(std::string_view path, std::string_view value)
{
    int fd = open(path.data(), O_WRONLY);
    if (fd < 0)
    {
        perror("open");
        return -1;
    }

    if (write(fd, value.data(), strlen(value.data())) < 0)
    {
        perror("write");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
};
