#!/bin/bash
# tarsau derleme ve test scripti
# G231210078

echo "========================================="
echo "  tarsau - Derleme ve Test"
echo "========================================="

# derleme
echo ""
echo "[1] Derleniyor..."
make clean 2>/dev/null
make
if [ $? -ne 0 ]; then
    echo "HATA: Derleme basarisiz!"
    exit 1
fi
echo "Derleme basarili."

# test dosyalari olustur
echo ""
echo "[2] Test dosyalari hazirlaniyor..."
echo "bu birinci dosyanin icerigi" > test_d1.txt
echo "ikinci dosya burada" > test_d2.txt
echo "ucuncu ve son dosya icerigi de bu satirda" > test_d3.txt
chmod 0755 test_d1.txt
chmod 0644 test_d2.txt
chmod 0600 test_d3.txt

echo "  test_d1.txt (izin: 0755)"
echo "  test_d2.txt (izin: 0644)"
echo "  test_d3.txt (izin: 0600)"

# arsiv olustur
echo ""
echo "[3] Arsiv olusturuluyor..."
./tarsau -b test_d1.txt test_d2.txt test_d3.txt -o test_arsiv.sau
if [ $? -ne 0 ]; then
    echo "HATA: Arsiv olusturulamadi!"
    exit 1
fi

echo "  test_arsiv.sau icerigi (hex dump):"
xxd test_arsiv.sau | head -5

# arsiv ac
echo ""
echo "[4] Arsiv aciliyor..."
rm -rf test_cikti
./tarsau -a test_arsiv.sau test_cikti
if [ $? -ne 0 ]; then
    echo "HATA: Arsiv acilamadi!"
    exit 1
fi

# karsilastir
echo ""
echo "[5] Dosyalar karsilastiriliyor..."
hata=0
for d in test_d1.txt test_d2.txt test_d3.txt; do
    if diff "$d" "test_cikti/$d" > /dev/null 2>&1; then
        echo "  $d -> TAMAM (icerik eslestirildi)"
    else
        echo "  $d -> HATALI (icerik uyusmuyor!)"
        hata=1
    fi
done

# izin kontrolu
echo ""
echo "[6] Izinler kontrol ediliyor..."
for d in test_d1.txt test_d2.txt test_d3.txt; do
    orijinal=$(stat -c "%a" "$d" 2>/dev/null)
    acilan=$(stat -c "%a" "test_cikti/$d" 2>/dev/null)
    if [ "$orijinal" = "$acilan" ]; then
        echo "  $d -> TAMAM (izin: $orijinal)"
    else
        echo "  $d -> HATALI (beklenen: $orijinal, bulunan: $acilan)"
        hata=1
    fi
done

# varsayilan cikti testi
echo ""
echo "[7] Varsayilan cikti testi (a.sau)..."
./tarsau -b test_d1.txt test_d2.txt
if [ -f "a.sau" ]; then
    echo "  a.sau olusturuldu -> TAMAM"
else
    echo "  a.sau olusturulamadi -> HATALI"
    hata=1
fi

# temizlik
echo ""
echo "[8] Test dosyalari temizleniyor..."
rm -f test_d1.txt test_d2.txt test_d3.txt
rm -f test_arsiv.sau a.sau
rm -rf test_cikti

echo ""
echo "========================================="
if [ $hata -eq 0 ]; then
    echo "  TUM TESTLER BASARILI!"
else
    echo "  BAZI TESTLER BASARISIZ!"
fi
echo "========================================="
