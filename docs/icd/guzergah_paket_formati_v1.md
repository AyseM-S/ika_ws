# Arayüz Kontrol Dokümanı (ICD): İHA → İKA Güzergah Paketi

## Versiyon Bilgisi
| Versiyon | Tarih | Hazırlayan | Açıklama |
|---|---|---|---|
| V1 | GG.AA.2026 | [İKA Otonom Sürüş Lead - Ayşe] | İlk taslak |

## Onay Kaydı
| Onaylayan | Rol | Durum | Tarih |
|---|---|---|---|
| [İsim] | İKA Otonom Sürüş Lead | Onay Bekliyor | - |
| [İsim] | Otonom Uçuş ve Otopilot Lead | Onay Bekliyor | - |
| [İsim] | Haberleşme ve YKİ Lead (bilgi/görüş) | Onay Bekliyor | - |

## Amaç
Bu doküman, İHA'nın tespit ettiği güzergah bilgisinin İKA'ya ve Yer Kontrol 
İstasyonu'na aktarımında kullanılacak mesaj formatını tanımlar.
Şartname Req 15, 16, 17, 23, 24 ile ilişkilidir.

## Mesaj Formatı: route_update
{
  "msg_type": "route_update",
  "msg_id": "IHA01-000123",
  "timestamp_utc": "2026-08-15T10:32:41Z",
  "sender": "IHA-01",
  "mission_id": "teknofest2026-run-02",
  "route": {
    "detection_confidence": 0.94,
    "coordinate_frame": "local_enu",
    "start_point": { "x": 0.0, "y": 0.0 },
    "end_point":   { "x": 41.2, "y": 27.5 },
    "waypoints": [
      { "seq": 0, "x": 0.0,  "y": 0.0 },
      { "seq": 1, "x": 8.4,  "y": 3.1 },
      { "seq": 2, "x": 22.7, "y": 15.0 },
      { "seq": 3, "x": 41.2, "y": 27.5 }
    ]
  },
  "checksum": "a1b2c3d4"
}
## Mesaj Formatı: route_ack{
  "msg_type": "route_ack",
  "ref_msg_id": "IHA01-000123",
  "received_by": "IKA-01",
  "timestamp_utc": "2026-08-15T10:32:41.850Z",
  "status": "accepted"
}
## Taşıma Katmanı
UDP üzerinden, 5.8 GHz hat birincil, 915 MHz SiK hat yedek (JSON aynı formatta).

## Değişiklik Kuralı
Bu formatta yapılacak her değişiklik, yukarıdaki onay tablosundaki iki Lead'in 
onayı alınmadan koda yansıtılamaz.
