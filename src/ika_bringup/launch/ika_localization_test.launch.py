from launch import LaunchDescription
from launch_ros.actions import Node
import os
from launch.substitutions import Command

def generate_launch_description():
    home = os.path.expanduser('~')
    loc_cfg = os.path.join(home, 'ika_ws', 'src', 'ika_localization', 'config')
    comms_cfg = os.path.join(home, 'ika_ws', 'src', 'ika_comms', 'config', 'route_bridge_params.yaml')
    nav2_cfg = os.path.join(home, 'ika_ws', 'src', 'ika_navigation', 'config', 'nav2_params.yaml')

    urdf_path = os.path.join(home, 'ika_ws', 'src', 'ika_bringup', 'urdf', 'ika.urdf')
    with open(urdf_path, 'r') as f:
        robot_desc = f.read()

    return LaunchDescription([
        Node(package='robot_state_publisher', executable='robot_state_publisher',
             parameters=[{'robot_description': robot_desc}]),
        Node(package='robot_localization', executable='ekf_node', name='ekf_filter_node',
             parameters=[os.path.join(loc_cfg, 'ekf.yaml')]),
        Node(package='ika_localization', executable='wheel_odom_node',
             parameters=[os.path.join(loc_cfg, 'vehicle_params.yaml')]),
        Node(package='ika_localization', executable='fake_sensors',
             parameters=[os.path.join(loc_cfg, 'vehicle_params.yaml')]),
        Node(package='ika_navigation', executable='fake_lidar'),
        Node(package='ika_localization', executable='fake_gps'),
        Node(package='robot_localization', executable='navsat_transform_node',
             name='navsat_transform_node',
             parameters=[os.path.join(loc_cfg, 'ekf_global.yaml')],
             remappings=[('imu/data', '/imu/data'), ('gps/fix', '/gps/fix'),
                         ('odometry/filtered', '/odometry/filtered'),
                         ('odometry/gps', '/odometry/gps')]),
        Node(package='robot_localization', executable='ekf_node',
             name='ekf_filter_node_map',
             parameters=[os.path.join(loc_cfg, 'ekf_global.yaml')],
             remappings=[('odometry/filtered', '/odometry/filtered_map')]),
        Node(package='ika_comms', executable='route_bridge_node', parameters=[comms_cfg]),
        Node(package='ika_safety', executable='interlock_node'),
        Node(package='ika_bringup', executable='mission_manager_node'),
        Node(package='ika_bringup', executable='marker_robot_viz'),
        Node(package='nav2_controller', executable='controller_server', name='controller_server',
             parameters=[nav2_cfg], remappings=[('cmd_vel', 'cmd_vel_nav')]),
        Node(package='nav2_lifecycle_manager', executable='lifecycle_manager',
             name='lifecycle_manager_navigation',
             parameters=[{'autostart': True, 'node_names': ['controller_server']}]),
    ])
