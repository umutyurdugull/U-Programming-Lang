#  ULang Yorumlayıcı

ULang, temel programlama yapılarını, matris işlemlerini, aktivasyon fonksiyonlarını ve basit HTTP isteklerini destekleyen deneysel bir dildir. Bu yorumlayıcı, C++ ile yazılmıştır ve kodu çalıştırmak için **GCC/G++** derleyicisine ve **cURL** kütüphanesine ihtiyaç duyar.

Bu rehber, makinenizde hiçbir şey yüklü olmasa bile, projeyi kurmanız ve çalıştırmanız için gereken adımları içermektedir.

## 🛠️ Ön Koşullar ve Kurulum

Projeyi derlemek ve çalıştırmak için aşağıdaki araçlara ihtiyacınız vardır:

### 1. C++ Derleyicisi (GCC/G++)
ULang yorumlayıcısı C++ ile yazılmıştır.

* **Windows Kullanıcıları:**
    * **WSL (Windows Subsystem for Linux)** kurmanız ve kullanmanız önerilir.
    * Alternatif olarak, **MinGW** gibi bir araç seti kurabilirsiniz.
* **Linux Kullanıcıları (Ubuntu/Debian):**
    ```bash
    sudo apt update
    sudo apt install build-essential
    ```
* **macOS Kullanıcıları:**
    * **Xcode Command Line Tools** yüklenmelidir.

### 2. GNU Make
Projeyi yönetmek için (derleme, temizleme) `Makefile` kullanırız. Genellikle `build-essential` veya Command Line Tools ile birlikte gelir.

### 3. cURL Geliştirme Kütüphanesi
ULang, `http_post()` yerleşik fonksiyonu ile HTTP istekleri yapabilir. Bunun için **cURL** kütüphanesinin makinenizde kurulu olması gerekir.

* **Linux Kullanıcıları (Ubuntu/Debian):**
    ```bash
    sudo apt install libcurl4-openssl-dev
    ```
* **macOS Kullanıcıları (Homebrew ile):**
    ```bash
    brew install curl
    ```
* **Windows Kullanıcıları (WSL veya MinGW):**
    * Gerekli geliştirme dosyalarını (lib/include) kendiniz indirip projenin dizinine yerleştirmeniz veya manuel olarak linklemeniz gerekebilir. Yukarıdaki `Makefile` varsayılan olarak Linux/macOS ortamlarını hedefler. Windows'ta daha karmaşık bir kurulum gerektirebilir.

---

##  Projenin Derlenmesi

Yorumlayıcının kaynak kodu (`main.cpp`) ve derleme dosyası (`Makefile`) aynı dizinde bulunmalıdır.

### Adım 1: Makefile'ı Başlatın
### Adım 2: Derleme Komutu 

Terminal/Komut İstemi'nde proje dizininize gidin ve make komutunu çalıştırın:
```
make
```
Başarılı bir derleme sonucunda, dizininizde ulang adında çalıştırılabilir bir dosya oluşacaktır.
### 3. ULang Kodunu Çalıştırma
ULang kodunu içeren dosyanızı (örneğin, my_script.ul) oluşturduktan sonra, yorumlayıcıyı çalıştırmak için make run komutunu kullanın:
Çalıştırma Komutu
```
make run SCRIPT=dosya_adı.ul
```
Örnek:
```
make run SCRIPT=test.ul
```

Yerleşik Fonksiyonlar (Built-in)ULang, aşağıdaki yerleşik fonksiyonları destekler
output(değer1, ...): Değerleri ekrana yazar ve bir satır atlar (\n).
rand(): $0.0$ ile $1.0$ arasında rastgele bir ondalık sayı döndürür.
sigmoid(x): Sigmoid aktivasyon fonksiyonunu hesaplar: $\frac{1}{1 + e^{-x}}$
tanh(x): Hiperbolik tanjant aktivasyon fonksiyonunu hesaplar.
transpose(matris): Verilen 2D listeyi (matrisi) transpoze eder.
mat_multiply(A, B): İki matrisi (2D liste) çarpar.
http_post(url, body, headers): Belirtilen URL'ye POST isteği gönderir. (headers bir string listesi olmalıdır).


