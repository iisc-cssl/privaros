#include <chrono>
#include <cstdio>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "rclcpp/rclcpp.hpp"
#include "rcutils/cmdline_parser.h"

#include "std_msgs/msg/string.hpp"

#include <stdio.h> 


using namespace std::chrono_literals;

void print_usage()
{
  printf("Usage for Colonel app:\n");
  printf("colonel \"2, pub, 11474, chatter, tmp1\"\n");
  printf("2: selection for redirection function\n");
  printf("pub/sub\n");
  printf("pid of process\n");
  printf("old topic\n");
  printf("new topic\n");
  
  
}

// Create a Colonel class that subclasses the generic rclcpp::Node base class.
// The main function below will instantiate the class as a ROS node.
class Colonel : public rclcpp::Node
{
public:
  
  explicit Colonel(const std::string & topic_name, const std::string ctrl_msg)
  : Node("colonel")
  {
    
    // Create a function for when messages are to be sent.
    auto publish_message =
      [this]() -> void
      {

		if (flag)
		{
			rclcpp::shutdown();
		}
		else
		{

			msg_ = std::make_unique<std_msgs::msg::String>();
			
			msg_->data = control_data[count];
			count += 1;
			RCLCPP_INFO(this->get_logger(), "Publishing: '%s'", msg_->data.c_str());
			pub_->publish(std::move(msg_));
		}
		if (count >= control_data.size())
			flag = true;
      };

    // Create a publisher with a custom Quality of Service profile.
    rclcpp::QoS qos(rclcpp::KeepLast(10));
    pub_ = this->create_publisher<std_msgs::msg::String>(topic_name, qos);
	count = 0;
	std::string tmp = "";
	
	for(int i = 0; i < ctrl_msg.size(); i++)
	{
		if (ctrl_msg[i] == ',')
		{
			count += 1;
			if (count == 4)
			{
				control_data.push_back(tmp);
				tmp = "";
				count = 0;
				i += 1; //to escape the space after ','
			}
			else
			{
				tmp+=ctrl_msg[i];
			}
		}
		else
		{
			tmp+=ctrl_msg[i];
		}
	}
	if (tmp.size() > 0)
		control_data.push_back(tmp);
	
	count = 0;
    // Use a timer to schedule periodic message publishing.
    timer_ = this->create_wall_timer(1s, publish_message);
  }

  
private:
  bool flag = false;
  int count;
  //std::string control_data;
  std::vector<std::string> control_data;
  std::unique_ptr<std_msgs::msg::String> msg_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};


int main(int argc, char * argv[])
{
  
  // Force flush of the stdout buffer.
  // This ensures a correct sync of all prints
  // even when executed simultaneously within the launch file.
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  if (rcutils_cli_option_exist(argv, argv + argc, "-h")) {
    print_usage();
    return 0;
  }

  // Initialize any global resources needed by the middleware and the client library.
  // You must call this before using any other part of the ROS system.
  // This should be called once per process.
  rclcpp::init(argc, argv);

  // Parse the command line options.
  auto topic = std::string("flowcontroller");
  std::string control_msg = "";
  if (argc >=2 )
	control_msg = argv[1];
	
  auto node = std::make_shared<Colonel>(topic, control_msg);


  rclcpp::spin(node);

  rclcpp::shutdown();
  return 0;
}

