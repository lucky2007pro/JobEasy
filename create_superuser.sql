-- Ushbu skript orqali JobEasy bazasida Super Admin (Admin) foydalanuvchisi yaratiladi
-- Parol: admin
-- Email: admin@jobeasy.uz

INSERT INTO users (full_name, email, password_hash, role) 
VALUES ('Super Admin', 'admin@jobeasy.uz', '8c6976e5b5410415bde908bd4dee15dfb167a9c873fc4bb8a81f6f2ab448a918', 'admin')
ON CONFLICT (email) DO NOTHING;
