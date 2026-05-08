# myhttp-server

Bu proje, C dili ile yazilmis basit bir HTTP sunucusudur. Temel amac,
socket programlama, HTTP request/response yapisi, statik dosya sunumu,
basit routing ve thread havuzu mantigini ogrenmektir.

Bu proje production amacli bir web server degildir. Bilgisayar muhendisligi
ogrencisi seviyesinde, HTTP'nin temel calisma mantigini gosteren egitsel bir
projedir.

## Ozellikler

- TCP socket uzerinden baglanti kabul eder.
- Sabit sayida worker thread ile istemci baglantilarini isler.
- HTTP request header'ini okur ve method, path, version bilgilerini ayristirir.
- `GET` istekleri ile `www/` altindaki statik dosyalari sunar.
- `POST /api/hello` endpoint'i ile JSON body icinden isim alir ve JSON cevap doner.
- Bulunamayan dosyalar icin `404 Not Found` cevabi uretir.
- Bozuk request'ler icin `400 Bad Request` cevabi uretir.
- Bilinen dosya uzantilarina gore temel MIME type belirler.

## Proje Dizini

```text
myhttp-server/
|-- include/
|   |-- api_server.h     # API endpoint bildirimleri
|   |-- base.h           # Ortak include'lar, temel sabitler ve logger baglantisi
|   |-- cJSON.h          # cJSON kutuphanesi header dosyasi
|   |-- file_helper.h    # Dosya boyutu ve MIME type yardimcilari
|   |-- file_server.h    # Statik dosya sunumu bildirimi
|   |-- http.h           # HTTP request/response struct'lari ve fonksiyonlari
|   |-- logger.h         # Basit log makrolari
|   |-- router.h         # Request yonlendirme bildirimi
|   `-- server.h         # Socket server, thread ve kuyruk yapilari
|-- src/
|   |-- api_server.c     # POST /api/hello endpoint'i ve JSON cevaplari
|   |-- cJSON.c          # cJSON kutuphanesi implementasyonu
|   |-- file_helper.c    # MIME type secimi ve dosya boyutu hesaplama
|   |-- file_server.c    # www/ altindan statik dosya okuma ve gonderme
|   |-- http.c           # HTTP parse ve response gonderme fonksiyonlari
|   |-- main.c           # Program giris noktasi
|   |-- router.c         # Method/path'e gore ilgili handler secimi
|   `-- server.c         # Socket kurulum, accept loop, thread pool ve kuyruk
|-- www/
|   |-- css/
|   |   `-- style.css    # Statik CSS dosyasi
|   |-- js/
|   |   `-- index.js     # POST /api/hello istegi atan frontend kodu
|   |-- media/
|   |   `-- guide.mp4    # Statik video dosyasi
|   |-- hero-img.png     # Statik resim dosyasi
|   `-- index.html       # Ana sayfa
|-- .gitignore
|-- Dockerfile           # Docker konteyner yapilandirmasi
|-- Makefile             # Linux/POSIX ortam icin derleme komutlari
|-- log.txt
|-- README.md
```

## Derleme ve Calistirma

Proje POSIX socket API kullandigi icin Linux veya WSL gibi POSIX uyumlu bir
ortam hedeflenmistir. Windows uzerindeki bazi GCC kurulumlari
`sys/socket.h`, `netinet/in.h` ve `unistd.h` gibi header dosyalarini
desteklemeyebilir.

Derlemek icin:

```bash
make
```

Calistirmak icin:

```bash
make run
```

Varsayilan port:

```text
8080
```

Tarayicida:

```text
http://localhost:8080/
```

## Desteklenen Endpoint'ler

| Method | Path             | Aciklama                                                            |
| ------ | ---------------- | ------------------------------------------------------------------- |
| `GET`  | `/`              | `www/index.html` dosyasini dondurur.                                |
| `GET`  | `/index.html`    | Ana HTML dosyasini dondurur.                                        |
| `GET`  | `/css/style.css` | CSS dosyasini dondurur.                                             |
| `GET`  | `/js/index.js`   | JavaScript dosyasini dondurur.                                      |
| `GET`  | `/hero-img.png`  | Resim dosyasini dondurur.                                           |
| `POST` | `/api/hello`     | JSON body icinden `name` alanini okur ve selamlama mesaji dondurur. |

## POST /api/hello Ornegi

Request:

```http
POST /api/hello HTTP/1.1
Host: localhost:8080
Content-Type: application/json
Content-Length: 16

{"name":"Yusuf"}
```

Basarili response ornegi:

```json
{
    "status": "success",
    "data": {
        "message": "Merhaba Yusuf!\n"
    }
}
```

Gecersiz JSON veya eksik `name` alani icin API hata cevabi doner.

## GET Istek Akisi

```text
Client
  |
  v
server.c
  socket -> bind -> listen -> accept
  |
  v
worker thread
  kuyruktan client_fd alir
  |
  v
http.c
  request header okunur
  method, path, version ayristirilir
  |
  v
router.c
  method GET mi?
  |
  v
file_server.c
  path guvenlik kontrolunden gecer
  www/ altindaki dosya acilir
  |
  v
file_helper.c
  MIME type ve dosya boyutu belirlenir
  |
  v
http.c
  HTTP response header'i ve dosya icerigi client'a gonderilir
```

## POST /api/hello Istek Akisi

```text
Client
  |
  v
server.c
  client baglantisi kabul edilir ve worker thread'e verilir
  |
  v
http.c
  header okunur
  Content-Length bulunur
  gerekirse body'nin kalani okunur
  |
  v
router.c
  method POST ve path /api/hello mi?
  |
  v
api_server.c
  body cJSON ile parse edilir
  name alani kontrol edilir
  |
  v
http.c
  JSON response client'a gonderilir
```

## Temel Tasarim

Sunucu tek bir ana thread ile client baglantilarini kabul eder. Kabul edilen
`client_fd` degerleri thread-safe bir kuyruga eklenir. Worker thread'ler bu
kuyruktan baglanti alir, request'i okur, route eder ve response gonderir.

Bu yapi, her baglanti icin yeni thread acmak yerine sabit sayida worker thread
kullanir. Boylece thread olusturma maliyeti azalir ve ayni anda islenebilecek
baglanti sayisi daha kontrollu hale gelir.

## Kapsam

Bu projenin mevcut kapsami:

- Temel HTTP/1.1 request satirini okumak
- Header sonunu `\r\n\r\n` ile tespit etmek
- `Content-Length` ile basit request body okumak
- `GET` ile statik dosya sunmak
- `POST /api/hello` ile basit JSON API cevabi uretmek
- Basit hata cevaplari uretmek
- Thread pool ve kuyruk mantigini gostermek

## Sinirlar

Bu projede su ozellikler desteklenmez:

- HTTPS/TLS
- HTTP keep-alive
- Chunked transfer encoding
- Query string parsing
- URL decode islemi
- Cookie/session yonetimi
- Dosya upload
- Buyuk dosya optimizasyonlari
- Dinamik route parametreleri
- Kapsamli HTTP header parsing
- CORS header yonetimi
- Production seviyesinde guvenlik

## Bilinen Davranislar

- Baglanti basina tek request islenir, ardindan baglanti kapatilir.
- Statik dosyalar sadece `www/` klasoru altindan sunulur.
- Path icinde `..`, ters slash veya `%` karakteri varsa istek reddedilir.
- API endpoint'i sadece `/api/hello` path'i icin tanimlidir.
- Desteklenmeyen method'lar su anda genel hata cevabina dusmektedir.

## Gelistirme Onerileri

Projeyi daha temiz ve ilerletmek icin mantikli sonraki adimlar:

1. HTTP response olusturma isini tek bir yardimci fonksiyonda toplamak.
2. `README`, router ve test isteklerini ayni endpoint tablosuna gore hizalamak.
3. Basit test komutlari eklemek: `curl /`, `curl /olmayan-dosya`, `curl -X POST /api/hello`.

## Neden Bu Proje Degerli?

Hazir framework kullanmadan HTTP server yazmak, web'in alt katmanlarini anlamak
icin iyi bir egzersizdir. Bu proje sayesinde su sorulara pratik cevap verilir:

- Bir tarayici server'a baglaninca socket seviyesinde ne olur?
- HTTP request aslinda hangi metinden olusur?
- Header ile body arasindaki ayrim nedir?
- Bir dosya HTTP response olarak nasil gonderilir?
- Birden fazla client'i ayni anda islemek icin thread pool nasil kullanilir?

Bu nedenle proje kucuk tutulsa bile, temel sistem programlama ve web protokolu
bilgisini gostermek icin anlamlidir.
