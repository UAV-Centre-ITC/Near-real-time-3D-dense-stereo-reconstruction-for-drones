import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PythonExpression
from ament_index_python.packages import get_package_share_directory
from launch_ros.actions import Node


def generate_launch_description():
    my_package_dir = get_package_share_directory("s2m2_inference_cpp_pkg")
    my_params_file = os.path.join(my_package_dir, "config", "params.yaml")

    # Merge Docker-specific path overrides if present (e.g. /models/, /data/).
    # This file is optional — only shipped in the Docker image.
    params_files = [my_params_file]
    docker_params = os.path.join(my_package_dir, "config", "params_docker.yaml")
    if os.path.exists(docker_params):
        params_files.append(docker_params)

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
    s2m2_inference_node = Node(
        package="s2m2_inference_cpp_pkg",
        executable="s2m2_node",
        name="s2m2_node",
        output="screen",
        parameters=params_files,
        prefix=prefix_expr,
    )

    return LaunchDescription(
        [
            gdb_debug_arg,
            s2m2_inference_node,
        ]
    )
