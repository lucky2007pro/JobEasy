# JobEasy Product Blueprint

JobEasy bu foydalanuvchi uchun qulay, tezkor va moslashuvchan e-commerce platforma.

## Maqsadli Qulayliklar
- Avtomatlashtirilgan funksiyalar: buyurtma oqimi, seans boshqaruvi, tavsiya va qidiruv.
- Tezkor kirish: tez topish, 1-2 klikda asosiy amallar, aniq navigatsiya.
- Shaxsiylashtirish: foydalanuvchi profili, tarix, xohishlar ro'yxati.
- Oddiy interfeys: intuitiv menyu, minimal qadam, tushunarli CTA.
- Mobil/Desktop moslashuvchanlik: responsive UI va cross-browser qo'llab-quvvatlash.

## Loyiha Bosqichlari
1. Ehtiyojlarni aniqlash va user story yozish.
2. Arxitektura va texnologik stekni tanlash.
3. MVP ishlab chiqish.
4. Unit + integration testlar.
5. UI/UX test va feedback sikli.
6. Xavfsizlik auditi + performance optimizatsiya.
7. Dokumentatsiya va foydalanuvchi qo'llanmasi.

## Sifat Mezoni (Definition of Done)
- Sahifa yuklanishi: <= 2 soniya (asosiy sahifalar uchun).
- Uptime: >= 99%.
- Accessibility: WCAG 2.1 AA ga mos.
- Moslik: kamida 3 brauzer va 2 qurilmada to'liq ishlashi.
- Kod sifati: Git repository, aniq commit xabarlar, CI/CD pipeline.

## Hujjatlar
- [01 Requirements & User Stories](docs/01-requirements-and-user-stories.md)
- [02 Architecture & Stack](docs/02-architecture-and-stack.md)
- [03 MVP Plan](docs/03-mvp-plan.md)
- [04 Testing Plan](docs/04-testing-plan.md)
- [05 UI UX Plan](docs/05-ui-ux-plan.md)
- [06 Security Performance Plan](docs/06-security-performance-plan.md)
- [07 Delivery Checklist](docs/07-delivery-checklist.md)
- [Commit Convention](docs/commit-convention.md)

## Lokal Ishga Tushirish
```bash
cmake -B build .
cmake --build build --config Debug
./build/JobEasy
```

Server `config.json` bo'yicha `8848` portda ishga tushadi.

