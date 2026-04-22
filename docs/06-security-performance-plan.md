# 06. Xavfsizlik Auditi va Performance Rejasi

## Xavfsizlik
- Session cookie uchun `HttpOnly`, `Secure`, `SameSite` siyosatini tekshirish.
- SQL injectiondan himoya:
  - parametrli querylar.
- Authentication va authorization route audit.
- Input validation:
  - profile update,
  - checkout,
  - search parametrlari.
- Dependency audit (container va kutubxonalar).

## Performance
- Caching:
  - static assetlar,
  - tez-tez so'raladigan listinglar.
- DB indexlar:
  - product search maydonlari,
  - order user_id.
- Load test:
  - p95 latency <= 2s.
- Compression:
  - gzip yoqilgan holatni tekshirish.

## SLO/SLA Monitoring
- Uptime >= 99%.
- Error rate dashboard.
- Health-check endpoint va alertlar.

