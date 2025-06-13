import sys


class PwmSysfs:
    def __init__(self, chip_no: int, channel_no: int, period_ns: int, duty_cycle_ns: int):
        self._pwm_sysfs_base = "/sys/class/pwm"
        self._is_initialized = False

        self.m_chip_no = chip_no
        self.m_channel_no = channel_no
        self.m_period_ns = period_ns
        self.m_duty_cycle_ns = duty_cycle_ns

    def __del__(self):
        if self._is_initialized:
            self.set_enable("0")

    def initialize(self) -> int:
        try:
            with open(f"{self._pwm_sysfs_base}/pwmchip{self.m_chip_no}/export", "w") as export_file:
                export_file.write(str(self.m_channel_no))
        except OSError:
            print("Note: PWM channel might already be exported", file=sys.stderr)

        try:
            with open(
                f"{self._pwm_sysfs_base}/pwmchip{self.m_chip_no}/pwm{self.m_channel_no}/period", "w"
            ) as period_file:
                period_file.write(str(self.m_period_ns))
        except OSError as err:
            print(err, file=sys.stderr)
            print("Failed to set PWM period", file=sys.stderr)
            return 1

        try:
            with open(
                f"{self._pwm_sysfs_base}/pwmchip{self.m_chip_no}/pwm{self.m_channel_no}/duty_cycle", "w"
            ) as duty_cycle_file:
                duty_cycle_file.write(str(self.m_duty_cycle_ns))
        except OSError as err:
            print(err, file=sys.stderr)
            print("Failed to set PWM duty cycle", file=sys.stderr)
            return 1

        self._is_initialized = True
        return 0

    def is_initialized(self) -> bool:
        return self._is_initialized

    def set_enable(self, value: str) -> int:
        try:
            with open(
                f"{self._pwm_sysfs_base}/pwmchip{self.m_chip_no}/pwm{self.m_channel_no}/enable", "w"
            ) as enable_file:
                enable_file.write(value)
        except OSError:
            return 1

        return 0
