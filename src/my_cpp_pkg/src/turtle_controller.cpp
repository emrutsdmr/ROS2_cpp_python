#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "turtlesim/msg/pose.hpp"
#include "turtlesim/srv/set_pen.hpp"
#include "my_robot_interfaces/srv/activate_turtle.hpp"

using SetPen = turtlesim::srv::SetPen;
using ActivateTurtle = my_robot_interfaces::srv::ActivateTurtle;
using namespace std::chrono_literals;
using namespace std::placeholders;

class TurtleControllerNode : public rclcpp::Node
{
public:
  TurtleControllerNode() : Node("turtle_controller"), is_active_(true), previous_x_(0.0)
  {
    set_pen_client_ = this->create_client<SetPen>("/turtle1/set_pen");
    activate_turtle_service_ = this->create_service<ActivateTurtle>(
      "activate_turtle", std::bind(&TurtleControllerNode::callbackActivateTurtle, this, _1, _2));
    cmd_vel_pub_ = this->create_publisher<geometry_msgs::msg::Twist>(
      "/turtle1/cmd_vel", 10);
    pose_sub_ = this->create_subscription<turtlesim::msg::Pose>(
      "/turtle1/pose", 10, std::bind(&TurtleControllerNode::poseCallback, this, _1));
  }

  void poseCallback(const turtlesim::msg::Pose::SharedPtr pose)
  {
    if (is_active_) {
      auto cmd = geometry_msgs::msg::Twist();
      if (pose->x < 5.5) {
        cmd.linear.x = 1.0;
        cmd.angular.z = 1.0;
      }
      else {
        cmd.linear.x = 2.0;
        cmd.angular.z = 2.0;
      }
      cmd_vel_pub_->publish(cmd);

      if (pose->x > 5.5 && previous_x_ <= 5.5) {
        previous_x_ = pose->x;
        RCLCPP_INFO(this->get_logger(), "Set color to red.");
        callSetPen(255, 0, 0);
      }
      else if (pose->x <= 5.5 && previous_x_ > 5.5) {
        previous_x_ = pose->x;
        RCLCPP_INFO(this->get_logger(), "Set color to green.");
        callSetPen(0, 255, 0);
      }
    }
  }

  void callSetPen(int r, int g, int b)
  {
    while (!set_pen_client_->wait_for_service(1s)) {
      RCLCPP_WARN(this->get_logger(), "Waiting for the server...");
    }
    auto request = std::make_shared<SetPen::Request>();
    request->r = r;
    request->g = g;
    request->b = b;
    set_pen_client_->async_send_request(
      request, std::bind(&TurtleControllerNode::callbackSetPenResponse, this, _1));
  }

private:
  void callbackSetPenResponse(rclcpp::Client<SetPen>::SharedFuture future)
  {
    (void)future;
    RCLCPP_INFO(this->get_logger(), "Successfully changed pen color");
  }

  void callbackActivateTurtle(
    const ActivateTurtle::Request::SharedPtr request,
    const ActivateTurtle::Response::SharedPtr response)
  {
    is_active_ = request->activate;
    if (request->activate) {
      response->message = "Starting the turtle";
    }
    else {
      response->message = "Stopping the turtle";
    }
  }

  bool is_active_;
  double previous_x_;
  rclcpp::Client<SetPen>::SharedPtr set_pen_client_;
  rclcpp::Service<ActivateTurtle>::SharedPtr activate_turtle_service_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub_;
  rclcpp::Subscription<turtlesim::msg::Pose>::SharedPtr pose_sub_;
};

int main(int argc, char **argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<TurtleControllerNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
