import os
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch.launch_description_sources import PythonLaunchDescriptionSource
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():

    launch_actions = []

    # -----> arguments for optional gdb debugging of nodes
    gdb_debug_arg_ffs = DeclareLaunchArgument(
        "gdb_debug_ffs_node",
        default_value="false",
        description="Launch the ffs node in gdb debug mode",
    )
    launch_actions.append(gdb_debug_arg_ffs)
    gdb_debug_conf_ffs = LaunchConfiguration("gdb_debug_ffs_node")

    gdb_debug_arg_pointcloud_node = DeclareLaunchArgument(
        "gdb_debug_pointcloud_node",
        default_value="false",
        description="Launch the pointcloud node in gdb debug mode",
    )
    launch_actions.append(gdb_debug_arg_pointcloud_node)
    gdb_debug_conf_pointcloud_node = LaunchConfiguration("gdb_debug_pointcloud_node")
    # <----- arguments for optional gdb debugging of nodes

    # S2M2 stereo inference node
    ffs_pkg_dir = get_package_share_directory("ffs_inference_cpp_pkg")
    ffs_inference_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(ffs_pkg_dir, "launch", "inference_node.launch.py")
        ),
        launch_arguments={"gdb_debug": gdb_debug_conf_ffs}.items(),
    )
    launch_actions.append(ffs_inference_launch)

    # Pointcloud generator node
    pointcloud_pkg_dir = get_package_share_directory("pointcloud_pkg")
    pointcloud_generator_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pointcloud_pkg_dir, "launch", "pointcloud_generator.launch.py")
        ),
        launch_arguments={"gdb_debug": gdb_debug_conf_pointcloud_node}.items(),
    )
    launch_actions.append(pointcloud_generator_launch)

    # GPU monitoring node
    gpu_monitoring_node = Node(
        package="gpu_monitoring_pkg",
        executable="gpu_monitoring_node",
        name="gpu_monitoring_node",
        output="screen",
        parameters=[
            {"poll_frequency_hz": 20},
            {"window_averaging_size": 100},  # samples
        ],
        #        prefix="xterm -e gdb --args",
    )
    launch_actions.append(gpu_monitoring_node)

    return LaunchDescription(launch_actions)
