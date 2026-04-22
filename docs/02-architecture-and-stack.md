# 02. Arxitektura va Texnologik Stek

## Arxitektura
- Backend: Drogon (C++) MVC uslubi.
- Ma'lumotlar bazasi: PostgreSQL.
- View qatlam: CSP template'lar.
- Deploy: Docker image + reverse proxy (productionda Nginx tavsiya).

## Qatlamlar
- `controllers/`: HTTP endpointlar.
- `filters/`: autentifikatsiya va authorization.
- `views/`: sahifa render.
- `models/`: domen modeli va metadata.

## Tanlangan Stek
- Til: C++17
- Framework: Drogon
- DB: PostgreSQL
- Build: CMake + Ninja
- Test: Drogon test framework
- CI/CD: GitHub Actions (build, test, docker image check)

## Responsivlik va Platforma
- Frontend layout mobil-first yondashuvda.
- Desktop va mobile breakpointlar:
  - Mobile: 360-767px
  - Tablet: 768-1023px
  - Desktop: >= 1024px

## Monitoring va SLO
- Uptime monitoring: health endpoint + tashqi monitor.
- Loglar: app log + reverse proxy access/error log.
- SLO: 99% uptime, 2s ichida asosiy sahifa javobi.

