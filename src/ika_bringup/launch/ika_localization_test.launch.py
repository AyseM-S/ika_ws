from launch import LaunchDescription
from launch_ros.actions import Node
import os

def generate_launch_description():
    home = os.path.expanduser('~')
    loc_cfg = os.path.join(home, 'ika_ws', 'src', 'ika_localization', 'config')
    comms_cfg = os.path.join(home, 'ika_ws', 'src', 'ika_comms', 'config', 'route_bridge_params.yaml')
    nav2_cfg = os.path.join(home, 'ika_ws', 'src', 'ika_navigation', 'config', 'nav2_params.yaml')

    return LaunchDescription([
        Node(package='tf2_ros', executable='static_transform_publisher',
             arguments=['--x','0','--y','0','--z','0','--roll','0','--pitch','0','--yaw','0',
                        '--frame-id','base_link','--child-frame-id','imu_link']),
        Node(package='robot_localization', executable='ekf_node', name='ekf_filter_node',
             parameters=[os.path.join(loc_cfg, 'ekf.yaml')]),
        Node(package='ika_localization', executable='wheel_odom_node',
             parameters=[os.path.join(loc_cfg, 'vehicle_params.yaml')]),
        Node(package='ika_localization', executable='fake_sensors',
             parameters=[os.path.join(loc_cfg, 'vehicle_params.yaml')]),
        Node(package='ika_navigation', executable='fake_lidar'),
        Node(package='ika_comms', executable='route_bridge_node', parameters=[comms_cfg]),
        Node(package='ika_safety', executable='interlock_node'),
        Node(package='ika_bringup', executable='mission_manager_node'),
        Node(package='nav2_controller', executable='controller_server', name='controller_server',
             parameters=[nav2_cfg], remappings=[('cmd_vel', 'cmd_vel_nav')]),
        Node(package='nav2_lifecycle_manager', executable='lifecycle_manager',
             name='lifecycle_manager_navigation',
             parameters=[{'autostart': True, 'node_names': ['controller_server']}]),
    ])
