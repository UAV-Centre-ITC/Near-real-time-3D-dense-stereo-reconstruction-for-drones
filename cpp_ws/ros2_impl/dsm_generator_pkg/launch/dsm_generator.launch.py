import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PythonExpression
from ament_index_python.packages import get_package_share_directory
from launch_ros.actions import Node


def generate_launch_description():
    my_package_dir = get_package_share_directory("dsm_generator_pkg")
    my_params_file = os.path.join(my_package_dir, "config", "params.yaml")

    gdb_debug_arg = DeclareLaunchArgument(
        "gdb_debug",
        default_value="False",
        description="Launch the node in gdb debug mode",
    )
    gdb_debug_conf = LaunchConfiguration("gdb_debug")

    prefix_expr = PythonExpression(
        [
            '"xterm -e gdb --args" if "',
            LaunchConfiguration("gdb_debug"),
            '".lower() == "true" else ""',
        ]
    )

    gdb_debug = LaunchConfiguration("gdb_debug")
    dsm_node = Node(
        package="dsm_generator_pkg",
        executable="dsm_node",
        name="dsm_node",
        output="screen",
        parameters=[my_params_file],
        prefix=prefix_expr,
    )

    return LaunchDescription(
        [
            gdb_debug_arg,
            dsm_node,
        ]
    )
