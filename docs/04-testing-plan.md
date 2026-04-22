# 04. Test Rejasi

## Unit Testlar
- Controller handlerlarining biznes mantiqini test qilish.
- Filterlar:
  - `LoginFilter` foydalanuvchi yo'q bo'lsa redirect.
  - `AdminFilter` role tekshiruvi.

## Integration Testlar
- End-to-end API flow:
  - Login -> Cart add/remove -> Checkout -> My orders
  - Login -> Profile update
  - Login -> Wishlist add/remove

## Test Muhit
- CI da alohida test ishga tushishi.
- Test DB yoki mock ma'lumotlar.

## Minimal Coverage Maqsadi
- Critical flowlar uchun kamida 70% branch coverage.

## Manual Regression Checklist
- Guest route'lar.
- Auth talab qiladigan route'lar.
- Admin route'lar.

