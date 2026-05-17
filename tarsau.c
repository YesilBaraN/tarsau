/*
 * tarsau - basit arsivleme araci
 * sikistirma yapmadan dosyalari tek bir .sau dosyasinda birlestirir
 * G231210078
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#define EN_FAZLA_DOSYA   32
#define BOYUT_SINIRI     (200 * 1024 * 1024)   /* 200 MB */
#define BASLIK_UZUNLUK   10
#define ONAYLANMIS_UZANTI ".sau"
#define STANDART_CIKTI    "a.sau"

/* --------- yardimci fonksiyonlar --------- */

/* dosyanin ascii metin olup olmadigina bakar */
int metin_mi(const char *yol)
{
    FILE *f = fopen(yol, "rb");
    if (f == NULL)
        return 0;

    int c;
    while ((c = fgetc(f)) != EOF) {
        if (c > 127) {            /* ascii disinda */
            fclose(f);
            return 0;
        }
        /* tab newline cr disindaki kontrol karakterlerini reddet */
        if (c < 32 && c != '\t' && c != '\n' && c != '\r') {
            fclose(f);
            return 0;
        }
    }
    fclose(f);
    return 1;
}

/* stat ile boyut dondurur */
long dosya_boyu(const char *yol)
{
    struct stat bilgi;
    if (stat(yol, &bilgi) != 0)
        return -1;
    return (long)bilgi.st_size;
}

/* izinleri octal string olarak alir  orn "0644" */
void izinleri_al(const char *yol, char *sonuc)
{
    struct stat bilgi;
    if (stat(yol, &bilgi) != 0) {
        strcpy(sonuc, "0644");
        return;
    }
    sprintf(sonuc, "%04o", bilgi.st_mode & 0777);
}

/* --------- arsiv olusturma (-b) --------- */

int arsiv_olustur(char *liste[], int adet, const char *cikti_adi)
{
    int i;
    long toplam = 0;

    /* dosya sayisi kontrolu */
    if (adet == 0) {
        fprintf(stderr, "Hata: arsivlenecek dosya belirtilmedi!\n");
        return 1;
    }
    if (adet > EN_FAZLA_DOSYA) {
        fprintf(stderr, "Hata: en fazla %d dosya desteklenir!\n", EN_FAZLA_DOSYA);
        return 1;
    }

    /* girdi dosyalarini dogrula */
    for (i = 0; i < adet; i++) {
        if (access(liste[i], R_OK) != 0) {
            fprintf(stderr, "%s giris dosyasinin formati uyumsuzdur!\n", liste[i]);
            return 1;
        }
        if (!metin_mi(liste[i])) {
            fprintf(stderr, "%s giris dosyasinin formati uyumsuzdur!\n", liste[i]);
            return 1;
        }
        long b = dosya_boyu(liste[i]);
        if (b < 0) {
            fprintf(stderr, "%s giris dosyasinin formati uyumsuzdur!\n", liste[i]);
            return 1;
        }
        toplam += b;
    }

    if (toplam > BOYUT_SINIRI) {
        fprintf(stderr, "Hata: dosyalarin toplam boyutu 200MB sinirini asiyor!\n");
        return 1;
    }

    /* organizasyon bolumunu hazirla */
    char org[8192];
    org[0] = '\0';

    for (i = 0; i < adet; i++) {
        char izin[8];
        izinleri_al(liste[i], izin);
        long b = dosya_boyu(liste[i]);

        char parca[512];
        sprintf(parca, "|%s,%s,%ld|", liste[i], izin, b);
        strcat(org, parca);
    }

    int org_boy = (int)strlen(org);

    /* arsiv dosyasini olustur ve yaz */
    FILE *af = fopen(cikti_adi, "w");
    if (af == NULL) {
        fprintf(stderr, "Hata: %s dosyasi olusturulamiyor!\n", cikti_adi);
        return 1;
    }

    /* baslik: 10 baytlik org boyutu */
    fprintf(af, "%010d", org_boy);

    /* organizasyon kayitlari */
    fputs(org, af);

    /* dosya iceriklerini siraya koy */
    for (i = 0; i < adet; i++) {
        FILE *gf = fopen(liste[i], "r");
        if (gf == NULL) {
            fprintf(stderr, "Hata: %s okunamiyor!\n", liste[i]);
            fclose(af);
            return 1;
        }
        int ch;
        while ((ch = fgetc(gf)) != EOF)
            fputc(ch, af);
        fclose(gf);
    }

    fclose(af);
    printf("Dosyalar birlestirildi.\n");
    return 0;
}

/* --------- arsiv acma (-a) --------- */

int arsiv_ac(const char *arsiv, const char *klasor)
{
    FILE *af = fopen(arsiv, "r");
    if (af == NULL) {
        fprintf(stderr, "Arsiv dosyasi uygunsuz veya bozuk!\n");
        return 1;
    }

    /* ilk 10 bayti oku -> org bolum boyutu */
    char bas[BASLIK_UZUNLUK + 1];
    if (fread(bas, 1, BASLIK_UZUNLUK, af) != BASLIK_UZUNLUK) {
        fprintf(stderr, "Arsiv dosyasi uygunsuz veya bozuk!\n");
        fclose(af);
        return 1;
    }
    bas[BASLIK_UZUNLUK] = '\0';

    int org_boy = atoi(bas);
    if (org_boy <= 0) {
        fprintf(stderr, "Arsiv dosyasi uygunsuz veya bozuk!\n");
        fclose(af);
        return 1;
    }

    /* org bolumunu oku */
    char *org = (char *)malloc(org_boy + 1);
    if (org == NULL) {
        fprintf(stderr, "Bellek ayrilamadi!\n");
        fclose(af);
        return 1;
    }
    if ((int)fread(org, 1, org_boy, af) != org_boy) {
        fprintf(stderr, "Arsiv dosyasi uygunsuz veya bozuk!\n");
        free(org);
        fclose(af);
        return 1;
    }
    org[org_boy] = '\0';

    /* hedef klasoru olustur */
    if (klasor != NULL && strlen(klasor) > 0) {
        if (mkdir(klasor, 0755) != 0 && errno != EEXIST) {
            fprintf(stderr, "Hata: %s klasoru olusturulamiyor!\n", klasor);
            free(org);
            fclose(af);
            return 1;
        }
    }

    /* kayitlari ayristir ve dosyalari cikar */
    char isimler[EN_FAZLA_DOSYA][256];
    int sayac = 0;
    char *p = org;

    while (*p != '\0') {
        if (*p != '|') {
            p++;
            continue;
        }
        p++;   /* | karakterini atla */
        if (*p == '\0' || *p == '|')
            continue;

        char *son = strchr(p, '|');
        if (son == NULL)
            break;

        /* kaydi kopyala */
        int klen = (int)(son - p);
        char kayit[512];
        strncpy(kayit, p, klen);
        kayit[klen] = '\0';

        /* alanlari ayir:  ad,izin,boyut */
        char ad[256], izin_s[16];
        long boy;
        if (sscanf(kayit, "%[^,],%[^,],%ld", ad, izin_s, &boy) != 3) {
            fprintf(stderr, "Arsiv dosyasi uygunsuz veya bozuk!\n");
            free(org);
            fclose(af);
            return 1;
        }

        strcpy(isimler[sayac], ad);
        sayac++;

        /* dosya yolunu belirle */
        char yol[512];
        if (klasor != NULL && strlen(klasor) > 0)
            sprintf(yol, "%s/%s", klasor, ad);
        else
            strcpy(yol, ad);

        /* icerigi oku */
        char *veri = (char *)malloc(boy + 1);
        if (veri == NULL) {
            fprintf(stderr, "Bellek ayrilamadi!\n");
            free(org);
            fclose(af);
            return 1;
        }
        if ((long)fread(veri, 1, boy, af) != boy) {
            fprintf(stderr, "Arsiv dosyasi uygunsuz veya bozuk!\n");
            free(veri);
            free(org);
            fclose(af);
            return 1;
        }

        /* dosyayi diske yaz */
        FILE *df = fopen(yol, "w");
        if (df == NULL) {
            fprintf(stderr, "Hata: %s olusturulamiyor!\n", yol);
            free(veri);
            free(org);
            fclose(af);
            return 1;
        }
        fwrite(veri, 1, boy, df);
        fclose(df);

        /* izinleri geri yukle */
        int mod = (int)strtol(izin_s, NULL, 8);
        chmod(yol, mod);

        free(veri);
        p = son + 1;
    }

    /* sonuc mesaji */
    if (klasor != NULL && strlen(klasor) > 0)
        printf("%s dizininde ", klasor);
    else
        printf("Mevcut dizinde ");

    for (int k = 0; k < sayac; k++) {
        if (k > 0) {
            if (k == sayac - 1)
                printf(" ve ");
            else
                printf(", ");
        }
        printf("%s", isimler[k]);
    }
    printf(" dosyalari acildi.\n");

    free(org);
    fclose(af);
    return 0;
}

/* --------- ana program --------- */

void kullanim_bilgisi(void)
{
    printf("Kullanim:\n");
    printf("  tarsau -b dosya1 dosya2 ... [-o arsiv.sau]\n");
    printf("  tarsau -a arsiv.sau [hedef_dizin]\n");
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        kullanim_bilgisi();
        return 1;
    }

    if (strcmp(argv[1], "-b") == 0) {
        /* --- birlestirme modu --- */
        char *cikti = STANDART_CIKTI;
        char *dosyalar[EN_FAZLA_DOSYA];
        int dsay = 0;
        int i;

        for (i = 2; i < argc; i++) {
            if (strcmp(argv[i], "-o") == 0) {
                if (i + 1 < argc)
                    cikti = argv[++i];
                else {
                    fprintf(stderr, "Hata: -o sonrasi cikti dosyasi belirtilmedi!\n");
                    return 1;
                }
            } else {
                if (dsay >= EN_FAZLA_DOSYA) {
                    fprintf(stderr, "Hata: en fazla %d dosya!\n", EN_FAZLA_DOSYA);
                    return 1;
                }
                dosyalar[dsay++] = argv[i];
            }
        }

        return arsiv_olustur(dosyalar, dsay, cikti);

    } else if (strcmp(argv[1], "-a") == 0) {
        /* --- acma modu --- */
        if (argc < 3) {
            fprintf(stderr, "Hata: arsiv dosyasi belirtilmedi!\n");
            return 1;
        }

        const char *arsiv_adi = argv[2];

        /* uzanti kontrolu */
        int u = (int)strlen(arsiv_adi);
        if (u < 5 || strcmp(arsiv_adi + u - 4, ONAYLANMIS_UZANTI) != 0) {
            fprintf(stderr, "Arsiv dosyasi uygunsuz veya bozuk!\n");
            return 1;
        }

        const char *hedef = (argc > 3) ? argv[3] : NULL;
        return arsiv_ac(arsiv_adi, hedef);

    } else {
        fprintf(stderr, "Bilinmeyen parametre: %s\n", argv[1]);
        kullanim_bilgisi();
        return 1;
    }
}
