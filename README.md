# myhttp-server

Bu proje, C dili ile yazılmış basit bir HTTP sunucusudur. Temel amaç,
socket programlama, HTTP request/response yapısı, statik dosya sunumu,
basit routing ve thread havuzu mantığını öğrenmektir.

Bu proje production amaçlı bir web server değildir. Bilgisayar mühendisliği
öğrencisi seviyesinde, HTTP'nin temel çalışma mantığını gösteren eğitsel bir
projedir.

## Özellikler

- TCP socket üzerinden bağlantı kabul eder.
- Sabit sayıda worker thread ile istemci bağlantılarını işler.
- HTTP request header'ini okur ve method, path, version bilgilerini ayrıştırır.
- `GET` istekleri ile `www/` altındaki statik dosyaları sunar.
- `POST /api/hello` endpoint'i ile JSON body içinden isim alır ve JSON cevap döner.
- Bulunamayan dosyalar için `404 Not Found` cevabı üretir.
- Bozuk request'ler için `400 Bad Request` cevabı üretir.
- Bilinen dosya uzantılarına göre temel MIME type belirler.

## Proje Dizini

```text
myhttp-server/
|-- include/
|   |-- api_server.h     # API endpoint bildirimleri
|   |-- base.h           # Ortak include'lar, temel sabitler ve logger bağlantısı
|   |-- cJSON.h          # cJSON kütüphanesi header dosyası
|   |-- file_helper.h    # Dosya boyutu ve MIME type yardımcıları
|   |-- file_server.h    # Statik dosya sunumu bildirimi
|   |-- http.h           # HTTP request/response struct'ları ve fonksiyonları
|   |-- logger.h         # Basit log makroları
|   |-- router.h         # Request yönlendirme bildirimi
|   `-- server.h         # Socket server, thread ve kuyruk yapıları
|-- src/
|   |-- api_server.c     # POST /api/hello endpoint'i ve JSON cevapları
|   |-- cJSON.c          # cJSON kütüphanesi implementasyonu
|   |-- file_helper.c    # MIME type seçimi ve dosya boyutu hesaplama
|   |-- file_server.c    # www/ altından statik dosya okuma ve gönderme
|   |-- http.c           # HTTP parse ve response gönderme fonksiyonları
|   |-- main.c           # Program giriş noktası
|   |-- router.c         # Method/path'e göre ilgili handler seçimi
|   `-- server.c         # Socket kurulum, accept loop, thread pool ve kuyruk
|-- www/
|   |-- css/
|   |   `-- style.css    # Statik CSS dosyası
|   |-- js/
|   |   `-- index.js     # POST /api/hello isteği atan frontend kodu
|   |-- media/
|   |   `-- guide.mp4    # Statik video dosyası
|   |-- hero-img.png     # Statik resim dosyası
|   `-- index.html       # Ana sayfa
|-- .gitignore
|-- Dockerfile           # Docker konteyner yapılandırması
|-- Makefile             # Linux/POSIX ortam için derleme komutları
|-- log.txt
|-- README.md
```

## Derleme ve Calistirma

Proje POSIX socket API kullandığı için Linux veya WSL gibi POSIX uyumlu bir
ortham hedeflenmektedir. Windows üzerindeki bazı GCC kurulumları
`sys/socket.h`, `netinet/in.h` ve `unistd.h` gibi header dosyalarını
desteklemeyebilir.

Derlemek için:

```bash
make
```

Çalıştırmak için:

```bash
make run
```

Varsayılan port:

```text
8080
```

Tarayıcıda:

```text
http://localhost:8080/
```

## Desteklenen Endpoint'ler

| Method | Path             | Açıklama                                                            |
| ------ | ---------------- | ------------------------------------------------------------------- |
| `GET`  | `/`              | `www/index.html` dosyasını döndürür.                                |
| `GET`  | `/index.html`    | Ana HTML dosyasını döndürür.                                        |
| `GET`  | `/css/style.css` | CSS dosyasını döndürür.                                             |
| `GET`  | `/js/index.js`   | JavaScript dosyasını döndürür.                                      |
| `GET`  | `/hero-img.png`  | Resim dosyasını döndürür.                                           |
| `POST` | `/api/hello`     | JSON body içinden `name` alanını okur ve selamlama mesajı döndürür. |

## POST /api/hello Örneği

Request:

```http
POST /api/hello HTTP/1.1
Host: localhost:8080
Content-Type: application/json
Content-Length: 16

{"name":"Yusuf"}
```

Başarılı response örneği:

```json
{
    "status": "success",
    "data": {
        "message": "Merhaba Yusuf!\n"
    }
}
```

Geçersiz JSON veya eksik `name` alanı için API hata cevabı döner.

## GET İstek Akışı

```text
Client
  |
  v
server.c
  socket -> bind -> listen -> accept
  |
  v
worker thread
  kuyruktan client_fd alır
  |
  v
http.c
  request header okunur
  method, path, version ayrıştırılır
  |
  v
router.c
  method GET mi?
  |
  v
file_server.c
  path güvenlik kontrolünden geçer
  www/ altındaki dosya açılır
  |
  v
file_helper.c
  MIME type ve dosya boyutu belirlenir
  |
  v
http.c
  HTTP response header'i ve dosya içeriği client'a gönderilir
```

## POST /api/hello İstek Akışı

```text
Client
  |
  v
server.c
  client bağlantısı kabul edilir ve worker thread'e verilir
  |
  v
http.c
  header okunur
  Content-Length bulunur
  gerekirse body'nin kalanı okunur
  |
  v
router.c
  method POST ve path /api/hello mi?
  |
  v
api_server.c
  body cJSON ile parse edilir
  name alanı kontrol edilir
  |
  v
http.c
  JSON response client'a gönderilir
```

## Temel Tasarım

Sunucu tek bir ana thread ile client bağlantılarını kabul eder. Kabul edilen
`client_fd` değerleri thread-safe bir kuyruğa eklenir. Worker thread'ler bu
kuyruktan bağlantı alır, request'i okur, route eder ve response gönderir.

Bu yapı, her bağlantı için yeni thread açmak yerine sabit sayıda worker thread
kullanır. Böylece thread oluşturma maliyeti azalır ve aynı anda işlenebilecek
bağlantı sayısı daha kontrollü hale gelir.

## Kapsam

Bu projenin mevcut kapsamı:

- Temel HTTP/1.1 request satırını okumak
- Header sonunu `\r\n\r\n` ile tespit etmek
- `Content-Length` ile basit request body okumak
- `GET` ile statik dosya sunmak
- `POST /api/hello` ile basit JSON API cevabı üretmek
- Basit hata cevapları üretmek
- Thread pool ve kuyruk mantığını göstermek

## Sınırlar

Bu projede şu özellikler desteklenmez:

- HTTPS/TLS
- HTTP keep-alive
- Chunked transfer encoding
- Query string parsing
- URL decode işlemi
- Cookie/session yönetimi
- Dosya upload
- Büyük dosya optimizasyonları
- Dinamik route parametreleri
- Kapsamlı HTTP header parsing
- CORS header yönetimi
- Production seviyesinde güvenlik

## Bilinen Davranışlar

- Bağlantı başına tek request işlenir, ardından bağlantı kapatılır.
- Statik dosyalar sadece `www/` klasörü altından sunulur.
- Path içinde `..`, ters slash veya `%` karakteri varsa istek reddedilir.
- API endpoint'i sadece `/api/hello` path'i için tanımlanmıştır.
- Desteklenmeyen method'lar şu anda genel hata cevabına düşmektedir.
