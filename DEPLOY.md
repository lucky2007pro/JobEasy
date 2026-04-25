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
1. PostgreSQL ma'lumotlar bazasini yaratadi. (Boshqa loyihalar ma'lumotlar bazasiga aralashmasligi uchun u faqat docker ichki tarmog'ida ishlaydi, tashqariga port ochilmaydi).
2. Dastur bazaga ulana olishi uchun `config.prod.json` faylidan foydalanadi.
3. JobEasy C++ dasturini kompilatsiya qiladi va uni `8848`-port da ishga tushiradi (serverdagi boshqa saytlarning 80-porti bilan to'qnashmasligi uchun).

## 4. Nginx orqali Reverse Proxy sozlash (Boshqa saytlar bilan birga ishlashi uchun)

Serveringizda boshqa sayt borligi sababli, port ziddiyatlarini oldini olish uchun Nginx ishlatib, domeningizni ushbu portga to'g'rilash tavsiya etiladi:

1. Nginx sozlamalari faylini yarating:
```bash
sudo nano /etc/nginx/sites-available/jobeasy
```

2. Faylning ichiga shuni kiritib saqlang (`<sizning_domeningiz>` o'rniga haqiqiy domen/sub-domenni yozing):
```nginx
server {
    listen 80;
    server_name <sizning_domeningiz.uz>;

    location / {
        proxy_pass http://127.0.0.1:8848;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
}
```

3. Sozlamani yoqing va Nginx'ni qaytadan ishga tushiring:
```bash
sudo ln -s /etc/nginx/sites-available/jobeasy /etc/nginx/sites-enabled/
sudo nginx -t
sudo systemctl restart nginx
```

## 5. Dastur holatini tekshirish

Dastur qanday ishlayotganini ko'rish uchun:
```bash
sudo docker-compose logs -f app
```

🚀 **Endi brauzeringiz orqali sozlangan domeningizga kirsangiz, JobEasy boshqa saytlarga xalaqit qilmagan holda muammosiz ochiladi!**
