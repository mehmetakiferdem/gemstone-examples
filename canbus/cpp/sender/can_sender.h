#ifndef CAN_SENDER_H
#define CAN_SENDER_H

#include <linux/can.h>
#include <linux/can/raw.h>
#include <string>

class CanSender
{
  public:
    CanSender(std::string_view interface_name);
    ~CanSender();

    int initialize();
    void run();
    void stop();

  private:
    std::string m_interface_name {};
    int m_socket {-1};
    volatile bool m_is_running {false};
    unsigned int m_frame_index {0};

    int setup_socket();
    int bind_socket();
    int send_frame(const can_frame& frame);
    void send_data_frame();
    void send_end_frame();
};

#endif // CAN_SENDER_H
