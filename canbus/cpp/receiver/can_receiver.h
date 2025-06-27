#ifndef CAN_RECEIVER_H
#define CAN_RECEIVER_H

#include <linux/can.h>
#include <linux/can/raw.h>
#include <string>

class CanReceiver
{
  public:
    CanReceiver(std::string_view interface_name);
    ~CanReceiver();

    int initialize();
    void run();
    void stop();

  private:
    std::string m_interface_name {};
    int m_socket {-1};
    bool m_is_running {false};

    int setup_socket();
    int bind_socket();
    void process_frame(const can_frame& frame);
    void print_frame(const can_frame& frame);
    bool is_end_message(const can_frame& frame);
};

#endif // CAN_RECEIVER_H
