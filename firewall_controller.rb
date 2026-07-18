require 'net/http'
require 'uri'
require 'json'

puts "========================================="
puts " IAL Ruby Firewall Controller Started    "
puts "========================================="

# WAF C++ Core adresi
WAF_URL = "http://localhost:8080"

# Cihaz Parmak izi (Device Fingerprint) ile Güvenli QR İsteği Testi
def test_secure_qr_request(fingerprint)
  uri = URI.parse("#{WAF_URL}/api/qr/verify")
  request = Net::HTTP::Get.new(uri)
  
  if fingerprint
    request["X-Device-Fingerprint"] = fingerprint
    puts "[Ruby] Gonderilen Istek: QR Dogrulama (Parmak Izi: #{fingerprint})"
  else
    puts "[Ruby] Gonderilen Istek: QR Dogrulama (Parmak Izi YOK - Kopyalanmis Link Similasyonu)"
  end

  begin
    response = Net::HTTP.start(uri.hostname, uri.port) do |http|
      http.request(request)
    end
    puts "[C++ WAF Yaniti] #{response.code} - #{response.body}"
  rescue => e
    puts "[HATA] C++ Sunucusuna baglanilamadi: #{e.message}"
  end
end

# Zararli SQL Injection Istegi Testi
def test_malicious_request
  uri = URI.parse("#{WAF_URL}/login")
  request = Net::HTTP::Post.new(uri)
  request.body = "username=admin' UNION SELECT * FROM users--"
  
  puts "\n[Ruby] Gonderilen Istek: SQL Injection Saldirisi"
  begin
    response = Net::HTTP.start(uri.hostname, uri.port) do |http|
      http.request(request)
    end
    puts "[C++ WAF Yaniti] #{response.code} - #{response.body}"
  rescue => e
    puts "[HATA] C++ Sunucusuna baglanilamadi: #{e.message}"
  end
end


sleep 1 # C++ sunucusunun hazir olmasini bekle

# 1. TEST: Zararli bir SQL Injection Saldirisi (Reddedilmeli)
test_malicious_request()

# 2. TEST: Duz (kopyalanmis) bir QR okutma istegi - Parmak Izi Yok (Reddedilmeli)
test_secure_qr_request(nil)

# 3. TEST: Orijinal cihazdan (Uygulama uzerinden) QR okutma istegi - Parmak Izi Var (Kabul Edilmeli)
test_secure_qr_request("Orijinal-iPhone-Hardware-ID-987654")

puts "\n[Ruby] Testler tamamlandi. IAL WAF Core su an trafigi dinlemeye devam ediyor..."
