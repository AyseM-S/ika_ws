# İKA Bağlantı Kaybı (İKA↔YKİ) Davranış Mantığı (DDR Bölüm 3.3.1)

İlişkili: ÖDR Bölüm 6.5 (Bağlantı Kaybı Failsafe Stratejisi), Req 19 (tam otonomi)

```mermaid
flowchart TD
    A[Normal operasyon<br/>YKİ bağlantısı aktif] --> B{Bağlantı kesildi mi?}
    B -->|Hayır| A
    B -->|Evet| C[Son bilinen güzergahla devam<br/>Otonom sürüş sürdürülür]
    C --> D{60 sn'de bağlantı döndü mü?}
    D -->|Evet| A
    D -->|Hayır| E[HOLD: güvenli bekleme<br/>YKİ'ye yeniden bağlanmayı dener]
```

## Notlar
- ÖDR Bölüm 6.5'teki "İKA hold + 60 sn timeout" satırının somut yazılım mantığıdır.
- Bağlantı kesikken bile İKA **tam otonom** çalışmaya devam eder (Req 19) — son alınan güzergah verisiyle DWA/Nav2 planlaması sürer, YKİ'den yeni komut beklenmez.
- 60 saniye timeout aşılırsa İKA güvenli bekleme (HOLD) durumuna geçer; bu noktadan sonrasının (manuel müdahale mi, görev iptali mi) hakem/şartname kurallarına göre netleştirilmesi gerekiyor — bu konuyu toplantıda gündeme alalım.
