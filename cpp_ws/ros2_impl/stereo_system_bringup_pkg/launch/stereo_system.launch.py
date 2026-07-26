import os
from launch import LaunchDescription
from launch_ros.actions import Node, ComposableNodeContainer
from launch_ros.descriptions import ComposableNode
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PythonExpression
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():

    # ---- arguments for optional gdb debugging of the container ----
    gdb_debug_arg = DeclareLaunchArgument(
        "gdb_debug_container",
        default_value="false",
        description="Launch the composed container in gdb debug mode",
    )
    gdb_debug_conf = LaunchConfiguration("gdb_debug_container")

    prefix_expr = PythonExpression(
        [
            '"xterm -e gdb --args" if "',
            LaunchConfiguration("gdb_debug_container"),
            '".lower() == "true" else ""',
        ]
    )

    s2m2_pkg_dir = get_package_share_directory("s2m2_inference_cpp_pkg")
    s2m2_params_file = os.path.join(s2m2_pkg_dir, "config", "params.yaml")

    pointcloud_pkg_dir = get_package_share_directory("pointcloud_pkg")
    pointcloud_params_file = os.path.join(pointcloud_pkg_dir, "config", "params.yaml")

    # Composed container with s2m2_node + pointcloud_generator
    stereo_container = ComposableNodeContainer(
        name="stereo_container",
        namespace="",
        package="rclcpp_components",
        executable="component_container_mt",
        prefix=prefix_expr,
        composable_node_descriptions=[
            ComposableNode(
                package="s2m2_inference_cpp_pkg",
                plugin="s2m2_inference_cpp_pkg::S2M2Node",
                name="s2m2_node",
                parameters=[s2m2_params_file],
                extra_arguments=[{"use_intra_process_comms": True}],
            ),
            ComposableNode(
                package="pointcloud_pkg",
                plugin="pointcloud_pkg::PointcloudGenerator",
                name="pointcloud_generator",
                parameters=[pointcloud_params_file],
                extra_arguments=[{"use_intra_process_comms": True}],
            ),
        ],
        output="screen",
    )

    # GPU monitoring node (standalone — not GPU-related for the stereo path)
    gpu_monitoring_node = Node(
        package="gpu_monitoring_pkg",
        executable="gpu_monitoring_node",
        name="gpu_monitoring_node",
        output="screen",
        parameters=[
            {"poll_frequency_hz": 20},
            {"window_averaging_size": 100},
        ],
    )

    return LaunchDescription(
        [
            gdb_debug_arg,
            stereo_container,
            gpu_monitoring_node,
        ]
    )
