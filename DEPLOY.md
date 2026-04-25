# JobEasy Loyihasini Ubuntu Serverga O'rnatish

JobEasy loyihasi Docker orqali ishlashga tayyorlangan. Serverga o'rnatish uchun quyidagi qadamlarni bajaring:

## 1. Serverga kerakli dasturlarni o'rnatish

Ubuntu serveringizda Docker va Docker Compose o'rnatilgan bo'lishi kerak. Agar o'rnatilmagan bo'lsa, quyidagi buyruqlarni ishlating:

```bash
# Tizimni yangilash
sudo apt update && sudo apt upgrade -y

# Docker o'rnatish
sudo apt install docker.io -y
sudo systemctl enable docker
sudo systemctl start docker

# Docker Compose o'rnatish
sudo apt install docker-compose -y
```

## 2. Loyihani serverga yuklash

Loyihani serverga GitHub orqali (yoki SCP/SFTP) yuklab oling.

```bash
git clone <loyihangiz_github_siltamasi>
cd JobEasy
```
yoki SCP orqali kompyuteringizdan yuboring:
```bash
scp -r /sizning/kompyuteringizdagi/JobEasy user@server_ip:/home/user/
```

## 3. Loyihani ishga tushirish

Loyiha jildida bo'lganingizda (Dockerfile va docker-compose.yml turgan joyda) quyidagi buyruqni ishlating:

```bash
sudo docker-compose up -d --build
```

Bu buyruq:
1. PostgreSQL ma'lumotlar bazasini yaratadi va sozlaydi (`schema.sql` va `upgrade_v2.sql` bazaga kiritiladi).
2. Dastur bazaga ulana olishi uchun `config.prod.json` faylidan foydalanadi (docker tarmog'i orqali).
3. JobEasy C++ dasturini kompilatsiya qiladi.
4. Dasturni `docker-compose.yml` da ko'rsatilganidek ishga tushiradi (`80`-port orqali ulanasiz).

## 4. Dastur holatini tekshirish

Dastur qanday ishlayotganini ko'rish uchun:
```bash
sudo docker-compose logs -f app
```

Qaysi konteynerlar ishlayotganini tekshirish uchun:
```bash
sudo docker ps
```

🚀 **Endi brauzeringiz orqali `http://<server_ip>` manziliga kirsangiz, JobEasy platformasi ochiladi!**
