# Odometri Kalibrasyon Prosedürü

Donanım teslim alındıktan sonra, `vehicle_params.yaml`'daki tahmini değerlerin
(track_width=0.40 m, encoder_ppr=500) gerçek ölçümle değiştirilmesi için uygulanacak prosedür.

## 1. İz Genişliği (track_width) — Doğrudan Ölçüm
- Araç düz zeminde, tekerlekler düz konumdayken.
- Sol ve sağ tekerlek grubunun **yere temas merkezleri** arasındaki mesafe şeritmetreyle ölçülür.
- 3 kez ölçülüp ortalaması alınır (mm hassasiyetle).
- Rocker-bogie süspansiyon nedeniyle tekerlekler farklı yüksekliklerde olabilir; ölçüm düz zeminde,
  tüm tekerlekler temas hâlindeyken yapılmalıdır.

## 2. Enkoder PPR — Datasheet + Doğrulama
- **Adım 1:** Motor/enkoder datasheet'inden nominal PPR ve redüktör oranı okunur.
  Tekerlek milindeki efektif PPR = ham enkoder PPR × redüktör oranı × 4 (quadrature ise).
- **Adım 2 (doğrulama):** Tekerlek elle tam 10 tur döndürülür, `/motor/encoder_ticks`
  topic'indeki sayaç farkı okunur. Efektif PPR = (sayaç farkı) / 10.
- Datasheet ile ölçüm uyuşmuyorsa **ölçüm esas alınır**.

## 3. Uçtan Uca Doğrulama (her iki değer girildikten sonra)
- **Düz hat testi:** Araca 5 m düz ilerleme komutu verilir; gerçek kat edilen mesafe şeritmetreyle
  ölçülür. `/odometry/filtered`'daki x değişimi ile karşılaştırılır. Hata > %2 ise wheel_radius
  ve/veya encoder_ppr yeniden kontrol edilir.
- **Dönüş testi:** Araca yerinde 360° dönüş yaptırılır. EKF'in yaw değişimi 2π'ye yakın olmalıdır.
  Sapma varsa track_width hatalıdır (bu değer doğrudan açısal hız hesabını etkiler).

## 4. Sonuçların İşlenmesi
Ölçülen değerler `src/ika_localization/config/vehicle_params.yaml` dosyasına yazılır,
commit edilir ve DDR'nin ilgili bölümü "tahmini" ibaresinden arındırılır.

## Sorumlu
İKA Otonom Sürüş Lead (yazılım tarafı) + İKA Mekanik Tasarım Lead (fiziksel ölçüm)
