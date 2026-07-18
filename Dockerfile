# Render.com için C++ ve Ruby Ortamı
# Temel Linux (Ubuntu) tabanlı resmi Ruby imajını kullanıyoruz.
# Çünkü içinde Ruby hazır gelir, C++ derleyicisini de biz ekleyeceğiz.
FROM ruby:3.2-slim

# Çalışma dizinini ayarla
WORKDIR /app

# İşletim sistemi güncellemelerini yap ve C++ derleyicisi (g++) yükle
RUN apt-get update -qq && apt-get install -y build-essential

# Projedeki tüm dosyaları (C++ kaynak kodu ve Ruby dosyaları) konteynere kopyala
COPY . .

# C++ WAF motorunu derle (Maksimum performans için -O3 bayrağı ile)
RUN g++ -O3 waf_core.cpp -o waf_core

# WAF sunucusunun dinlediği 8080 portunu dışa aç
EXPOSE 8080

# Konteyner başladığında çalıştırılacak komut: 
# (WAF Core sonsuz döngüde çalışıp trafiği filtreleyecektir)
CMD ["./waf_core"]
