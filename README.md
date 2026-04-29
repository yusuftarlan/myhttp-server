# myhttp-server

Bu projede C dili ile basit bir HTTP sunucusu oluşturulmaktadır.

## Projenin hedefleri

Bu projenin amacı, C dilinde socket programlama ve HTTP protokolünün temel çalışma mantığını öğrenmektir.

İlk aşamada hedeflenen özellikler:

- TCP socket üzerinden bağlantı kabul etmek
- HTTP `GET` isteklerini okuyabilmek
- `GET /` isteğine karşılık statik bir HTML sayfası döndürmek
- Belirli endpoint'lerde JSON veri döndürmek
- Geçersiz veya bulunamayan isteklerde uygun HTTP hata cevabı üretmek

## İlk sürüm kapsamı

İlk sürümde yalnızca temel HTTP davranışları desteklenecektir:

- `GET /`
- `GET /index.html`
- `GET /api/hello`
- `404 Not Found`
- `405 Method Not Allowed`
