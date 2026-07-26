#!/bin/bash
set -e
source /opt/ros/${ROS_DISTRO}/setup.bash
cd /workspace/ros2_impl
exec "$@"
