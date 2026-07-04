# Yazılım Altyapısı Gerekçeleri (DDR Bölüm 3.3.5)

## İşletim Sistemi
Ubuntu 22.04.5 LTS — ROS 2 Humble'ın Tier 1 desteklediği tek sürüm, 
destek 2027'ye kadar (yarışma takvimini kapsıyor).

## Orta Katman
ROS 2 Humble Hawksbill — ROS 1 Noetic 2025'te EOL oldu, yarışma boyunca 
güncelleme almayacaktı. DDS tabanlı mimari dağıtık node yapısına uygun.

## Programlama Dili
C++ (rclcpp), Python (rclpy) değil — güvenlik-kritik interlock döngüsünde 
düşük/öngörülebilir gecikme gerekiyor; Raspberry Pi 4'te Nav2+EKF+haberleşme 
aynı anda çalışacağından CPU verimliliği kritik.

## Kütüphaneler

| Kütüphane | Gerekçe | Alternatife Göre Avantaj |
|---|---|---|
| Nav2 | Resmi ROS2 navigasyon yığını, DWA hazır | Sıfırdan planlayıcı yazma riski yok |
| robot_localization | BNO055+enkoder EKF füzyonu | Test edilmiş, endüstri standardı |
| MAVROS | Pixhawk 6C (ArduRover) köprüsü | Kendi MAVLink parser'ından güvenilir |
| tf2 | Koordinat çerçeve dönüşümleri | Nav2 için zorunlu önkoşul |
| nlohmann/json | route_update/route_ack ayrıştırma | C++'ta yerleşik JSON yok, hafif çözüm |
| OpenCV — KULLANILMIYOR | İKA kendi görüntü işleme yapmıyor, güzergah İHA'dan hazır geliyor (ÖDR 5.5) | Gereksiz CPU yükünden kaçınma |
