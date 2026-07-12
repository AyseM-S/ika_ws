from launch import LaunchDescription
from launch_ros.actions import Node
import os

def generate_launch_description():
    cfg = os.path.join(os.path.expanduser('~'), 'ika_ws', 'src', 'ika_localization', 'config')
    return LaunchDescription([
        Node(package='tf2_ros', executable='static_transform_publisher',
             arguments=['--x','0','--y','0','--z','0','--roll','0','--pitch','0','--yaw','0',
                        '--frame-id','base_link','--child-frame-id','imu_link']),
        Node(package='robot_localization', executable='ekf_node', name='ekf_filter_node',
             parameters=[os.path.join(cfg, 'ekf.yaml')]),
        Node(package='ika_localization', executable='wheel_odom_node',
             parameters=[os.path.join(cfg, 'vehicle_params.yaml')]),
        Node(package='ika_localization', executable='fake_sensors'),
        Node(package='ika_comms', executable='route_bridge_node'),
        Node(package='ika_safety', executable='interlock_node'),
        Node(package='ika_bringup', executable='mission_manager_node'),
    ])
