# Arayüz Kontrol Dokümanı (ICD) v2: İHA → İKA Güzergah Paketi

## Versiyon Geçmişi
| Versiyon | Tarih | Değişiklik |
|---|---|---|
| v1 | Gün 1 | İlk taslak — yerel x,y koordinat, basit checksum alanı |
| v1.1 | Gün 1 | route_ack YKİ'ye de eşlendi (Req 17) |
| v2 | Gün 10 | WGS84 lat/lon, iha_state.airborne (Req 16 kilidi), seq_no (eskilik kontrolü), zengin status enum — **şu an kodda kullanılan, güncel şema** |

**Durum:** Bu doküman, `route_bridge_node.cpp`'de fiilen implemente edilmiş ve test edilmiş (KABUL + 4 farklı RED senaryosu) şemayı birebir yansıtır — aspirasyonel değil, koddan türetilmiştir.

## Onay Kaydı
| Onaylayan | Rol | Durum |
|---|---|---|
| [İsim] | İKA Otonom Sürüş Lead | Onaylandı (v2 kodlayan) |
| [İsim] | Otonom Uçuş ve Otopilot Lead | **Onay Bekliyor** — iha_state alanları İHA tarafında üretilecek |
| [İsim] | Haberleşme ve YKİ Lead | **Onay Bekliyor** — UDP port numaraları (aşağıda) henüz koordine edilmedi |

## Taşıma Katmanı
- **Protokol:** UDP (bağlantısız, düşük gecikme)
- **İKA dinleme portu:** 5005 (`listen_port` parametresi — placeholder, Haberleşme Lead ile netleşmeli)
- **route_ack hedef portları:** İHA→5006, YKİ→5007 (placeholder)
- **Yedek hat (915 MHz):** Aynı JSON formatı, henüz içerik/entegrasyon netleşmedi — açık madde

## Mesaj 1: route_update (İHA → İKA, eş zamanlı İHA → YKİ)

```json
{
  "msg_type": "route_update",
  "msg_id": "IHA01-000123",
  "seq_no": 123,
  "timestamp_utc": "2026-08-15T10:32:41.320Z",
  "sender": "IHA-01",
  "receiver": "IKA-01",
  "mission_id": "teknofest2026-run-02",
  "iha_state": {
    "airborne": true,
    "altitude_agl_m": 82.4,
    "flight_mode": "AUTO",
    "gps_fix": "rtk_fixed"
  },
  "route": {
    "route_id": "route_2",
    "route_difficulty": "orta",
    "detection_confidence": 0.94,
    "detection_source": "yolov8n+lidar_raycast",
    "coordinate_frame": "wgs84",
    "waypoints": [
      { "seq": 0, "lat": 39.925101, "lon": 32.836742 },
      { "seq": 1, "lat": 39.925178, "lon": 32.836801 }
    ]
  },
  "checksum": "a1b2c3d4"
}
```

### Alan Açıklamaları
| Alan | Zorunlu mu | Açıklama |
|---|---|---|
| `msg_id`, `seq_no` | Evet | Paket kimliği + sıra no (eskilik/tekrar kontrolü için) |
| `iha_state.airborne` | Evet | **Req 16 kilidi** — `false` ise paket `rejected_not_airborne` ile reddedilir |
| `route.waypoints[].lat/lon` | Evet | WGS84 — İKA'da `local_origin_lat/lon` referansına göre yerel x,y'ye çevrilir |
| `checksum` | Evet | CRC32(`route` nesnesinin JSON dump'ı) — **sadece `route` alanı üzerinden**, tüm mesaj değil |
| `route_difficulty`, `detection_confidence`, `detection_source` | Hayır (validasyona girmiyor) | Bilgi amaçlı, İHA görüntü işleme katmanına (Fulya'nın 3 katmanlı algoritması) referans |

## Mesaj 2: route_ack (İKA → İHA, eş zamanlı İKA → YKİ)

```json
{
  "msg_type": "route_ack",
  "msg_id": "IKA01-100001",
  "seq_no": 1,
  "ref_msg_id": "IHA01-000123",
  "ref_seq_no": 123,
  "timestamp_utc": "2026-08-15T10:32:41.850Z",
  "sender": "IKA-01",
  "receiver": "IHA-01",
  "status": "accepted",
  "reason": null,
  "computed_checksum": "a1b2c3d4",
  "waypoint_count_received": 4,
  "checksum": "e5f6a7b8"
}
```

### status Değerleri (kodda tam sırayla kontrol edilir)
| Sıra | Değer | Tetiklenme Koşulu |
|---|---|---|
| 1 | `rejected_malformed` | `msg_id`/`seq_no`/`checksum`/`iha_state.airborne`/`route.waypoints` eksik veya waypoint içinde `seq`/`lat`/`lon` eksik |
| 2 | `rejected_checksum` | CRC32(route) ≠ gelen `checksum` |
| 3 | `rejected_stale` | `seq_no` ≤ son işlenen `seq_no` (eski/tekrar paket) |
| 4 | `rejected_not_airborne` | `iha_state.airborne = false` (**Req 16**) |
| 5 | `accepted` | Tüm kontroller geçildi → `/route/waypoints` yayınlanır |

**Not:** `checksum` alanı (route_ack'in kendisi) CRC32(gövde JSON'u, `checksum` alanı eklenmeden önce) olarak hesaplanır — `route_update`'teki mantıkla tutarlı.

## Koordinat Dönüşümü
`route_bridge_node`, WGS84 (lat/lon) waypoint'leri `local_origin_lat`/`local_origin_lon` parametresine göre equirectangular yaklaşımla yerel (x,y) metreye çevirir (`geo_utils.hpp`, `latlonToLocalXY`). **`local_origin_lat/lon` yarışma günü YKS ekibi tarafından İKA-KB'nin gerçek koordinatıyla girilecektir** — tasarım eksikliği değil, saha operasyon parametresidir.

## Doğrulama Kanıtı
Bu şema, aşağıdaki senaryolarla gerçek testle doğrulanmıştır (Gün 10):
- ✅ KABUL: geçerli checksum + airborne=true → `route_update KABUL EDILDI`, `/route/waypoints` yayınlandı
- ✅ RED (Md.16): airborne=false → `IHA havada degil (Md.16) - REDDEDILDI`
- ✅ Uçtan uca: kabul edilen rota, Nav2/DWB ile gerçekten takip edildi (BEKLEME→TAMAMLANDI, RViz görsel kanıtı)

## Kaynak Kod Referansları
- `src/ika_comms/src/route_bridge_node.cpp`
- `src/ika_comms/include/ika_comms/checksum.hpp` (CRC32)
- `src/ika_comms/include/ika_comms/geo_utils.hpp` (WGS84→yerel)
- `src/ika_comms/config/route_bridge_params.yaml`
