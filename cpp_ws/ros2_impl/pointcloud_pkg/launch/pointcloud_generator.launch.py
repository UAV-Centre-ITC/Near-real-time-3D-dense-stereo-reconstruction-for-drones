import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PythonExpression
from ament_index_python.packages import get_package_share_directory
from launch_ros.actions import Node


def generate_launch_description():

    my_package_dir = get_package_share_directory("pointcloud_pkg")
    my_params_file = os.path.join(my_package_dir, "config", "params.yaml")

    gdb_debug_arg = DeclareLaunchArgument(
        "gdb_debug",
        default_value="false",
        description="Run the node in a debugger",
    )
    gdb_debug_conf = LaunchConfiguration("gdb_debug")

    prefix_expr = PythonExpression(
        [
            '"xterm -e gdb --args" if "',
            LaunchConfiguration("gdb_debug"),
            '".lower() == "true" else ""',
        ]
    )

    pointcloud_generator_node = Node(
        package="pointcloud_pkg",
        executable="pointcloud_generator",
        name="pointcloud_generator",
        output="screen",
        parameters=[my_params_file],
        prefix=prefix_expr,
    )

    return LaunchDescription(
        [
            gdb_debug_arg,
            pointcloud_generator_node,
        ]
    )
