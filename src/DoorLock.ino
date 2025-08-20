#include <Arduino.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <Servo.h>
#include <RTClib.h>
#include <ctype.h>
#include <string.h>

// ----- Peripherals -----
RTC_DS3231 rtc;
Servo servo;
LiquidCrystal_I2C lcd(0x27, 16, 2);

void lcdCenter(const char* text, int row);  // Function prototype for lcdCenter

void beep(int f, int t);  // Function prototype for beep

void beep(int f, int t);  // Function prototype for beep

// Animasi interaktif PIN BENAR
void animasiPINBenar() {
  // Animasi bintang berputar di LCD
  byte star[8] = {
    B00100,
    B01110,
    B11111,
    B01110,
    B00100,
    B00000,
    B00000,
    B00000
  };
  lcd.createChar(1, star);
  lcd.clear();
  lcdCenter("PIN BENAR", 0);
  for (int i = 0; i < 16; i++) {
    lcd.setCursor(i, 1);
    lcd.write(byte(1));
    beep(1800, 40);
    delay(40);
    lcd.setCursor(i, 1);
    lcd.print(" ");
  }
  lcdCenter("PIN BENAR", 0);
  lcdCenter("AKSES DITERIMA", 1);
  beep(2000, 120);
  delay(400);
}

// Animasi interaktif AKSES DITOLAK
void animasiAksesDitolak() {
  // Animasi silang bergerak di LCD
  byte cross[8] = {
    B10001,
    B01010,
    B00100,
    B01010,
    B10001,
    B00000,
    B00000,
    B00000
  };
  lcd.createChar(2, cross);
  lcd.clear();
  lcdCenter("AKSES DITOLAK", 0);
  for (int i = 15; i >= 0; i--) {
    lcd.setCursor(i, 1);
    lcd.write(byte(2));
    beep(600, 30);
    delay(40);
    lcd.setCursor(i, 1);
    lcd.print(" ");
  }
  lcdCenter("AKSES DITOLAK", 0);
  lcdCenter("COBA LAGI", 1);
  beep(600, 200);
  delay(400);
}

// ----- Keypad -----
const byte ROWS = 4, COLS = 4;
char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};
byte myRowPins[ROWS] = { 7, 8, 9, 10 };
byte customColPins[COLS] = { 11, 12, 13, A0 };
Keypad keypad(makeKeymap(keys), myRowPins, customColPins, ROWS, COLS);

void lcdCenter(const char* text, int row);  // Function declaration for lcdCenter
void beep(int f, int t);                    // Function declaration for beep

// Tambahan: prototipe fungsi Manajemen User (Admin -> Tombol A)
void adminUserManagement();
void viewUsersAutoScroll();
void deleteUserMenu();
void addNewUser();
static void copyTrimmedName(int idx, char* out);
static void clearEEPROMUserSlot(int slot);
static void rewriteUsersToEEPROM();

// Fungsi pembungkus keypad agar setiap tombol ditekan mengeluarkan bunyi
char getKeyWithBeep() {
  char k = keypad.getKey();
  if (k) beep(1200, 30);
  return k;
}

// Fungsi validasi PIN admin
bool askPIN(const char* correct) {
  lcd.clear();
  lcdCenter("MASUKKAN PIN:", 0);
  int startCol = 6;
  char in[5] = { 0 };
  byte idx = 0;
  unsigned long t = millis();
  while (idx < 4 && millis() - t < 15000) {
    lcd.setCursor(startCol + idx, 1);
    char k = getKeyWithBeep();
    if (isdigit(k)) {
      in[idx++] = k;
      lcd.print("*");
    }
  }
  return (strncmp(in, correct, 4) == 0);
}

// ----- Pin -----
#define SERVO_PIN 2
#define BUZZER_PIN 3
#define LED_GREEN 4
#define LED_RED 5
#define PIR_PIN 6
#define RELAY_PIN A1  // MODIFIKASI: Menambahkan pin untuk relay. JANGAN GUNAKAN PIN 7!

// ----- EEPROM Layout -----

#define MAX_USERS 10
#define USER_PIN_LEN 4
#define USER_NAME_LEN 12
#define E_USER_START 0
#define E_USER_SIZE (USER_PIN_LEN + USER_NAME_LEN)
#define E_ADMIN_PIN (MAX_USERS * E_USER_SIZE)
#define E_LOG_INDEX (E_ADMIN_PIN + USER_PIN_LEN)
#define E_LOG_START (E_LOG_INDEX + 1)
#define LOG_SIZE 24
#define MAX_LOGS 40

struct User {
  char pin[USER_PIN_LEN + 1];
  char name[USER_NAME_LEN + 1];
};
User users[MAX_USERS];
int userCount = 0;
char adminPIN[5] = "0000";
int logIndex = 0;

// ----- Servo pos -----
const int POS_CLOSED = 0;
const int POS_OPEN = 90;

// ===== UTIL =====
void beep(int f, int t) {
  tone(BUZZER_PIN, f, t);
}

void lcdCenter(const char* text, int row) {
  lcd.setCursor(0, row);
  int len = strlen(text);
  int pad = (16 - len) / 2;
  for (int i = 0; i < pad; i++) lcd.print(" ");
  lcd.print(text);
  for (int i = len + pad; i < 16; i++) lcd.print(" ");
}

void lcdClearRow(int row) {
  lcd.setCursor(0, row);
  for (int i = 0; i < 16; i++) lcd.print(" ");
}


void loadUsers() {
  userCount = 0;
  for (int i = 0; i < MAX_USERS; i++) {
    int addr = E_USER_START + i * E_USER_SIZE;
    char pin[USER_PIN_LEN + 1];
    char name[USER_NAME_LEN + 1];
    for (int j = 0; j < USER_PIN_LEN; j++) pin[j] = EEPROM.read(addr + j);
    pin[USER_PIN_LEN] = '\0';
    for (int j = 0; j < USER_NAME_LEN; j++) name[j] = EEPROM.read(addr + USER_PIN_LEN + j);
    name[USER_NAME_LEN] = '\0';
    if (pin[0] >= '0' && pin[0] <= '9') {
      strncpy(users[userCount].pin, pin, USER_PIN_LEN + 1);
      strncpy(users[userCount].name, name, USER_NAME_LEN + 1);
      userCount++;
    }
  }
}

void saveUser(int idx) {
  if (idx < 0 || idx >= MAX_USERS) return;
  int addr = E_USER_START + idx * E_USER_SIZE;
  for (int j = 0; j < USER_PIN_LEN; j++) EEPROM.write(addr + j, users[idx].pin[j]);
  for (int j = 0; j < USER_NAME_LEN; j++) EEPROM.write(addr + USER_PIN_LEN + j, users[idx].name[j]);
}

void loadAdminPIN() {
  for (int i = 0; i < 4; i++) adminPIN[i] = EEPROM.read(E_ADMIN_PIN + i);
  adminPIN[4] = '\0';
  if (adminPIN[0] < '0' || adminPIN[0] > '9') strcpy(adminPIN, "0000");
}

void saveAdminPIN() {
  for (int i = 0; i < 4; i++) EEPROM.write(E_ADMIN_PIN + i, adminPIN[i]);
}

void loadPINs() {
  loadUsers();
  loadAdminPIN();
  logIndex = EEPROM.read(E_LOG_INDEX);
  if (logIndex < 0 || logIndex >= MAX_LOGS) logIndex = 0;
}

// ===== BEEP SPESIAL PIN BENAR =====
void beepPINBenar() {
  for (int i = 0; i < 3; i++) {
    beep(2000, 80);
    delay(100);
  }
}


int askUserPIN() {
  lcd.clear();
  lcdCenter("MASUKKAN PIN:", 0);
  int startCol = 6;
  char in[USER_PIN_LEN + 1] = { 0 };
  byte idx = 0;
  unsigned long t = millis();
  while (idx < USER_PIN_LEN && millis() - t < 15000) {
    lcd.setCursor(startCol + idx, 1);
    char k = getKeyWithBeep();
    if (isDigit(k)) {
      in[idx++] = k;
      lcd.print("*");
    }
  }
  for (int i = 0; i < userCount; i++) {
    if (strncmp(in, users[i].pin, USER_PIN_LEN) == 0) return i;
  }
  return -1;
}

bool askNewPIN(char* target) {
  lcd.clear();
  lcdCenter("PIN BARU:", 0);
  int startCol = 6;
  char in[5] = { 0 };
  byte idx = 0;
  while (idx < 4) {
    lcd.setCursor(startCol + idx, 1);
    char k = getKeyWithBeep();
    if (isdigit(k)) {
      in[idx++] = k;
      lcd.print("*");
    }
  }
  lcd.clear();
  lcdCenter("KONFIRMASI PIN:", 0);
  char cf[5] = { 0 };
  idx = 0;
  while (idx < 4) {
    lcd.setCursor(startCol + idx, 1);
    char k = getKeyWithBeep();
    if (isdigit(k)) {
      cf[idx++] = k;
      lcd.print("*");
    }
  }
  if (strncmp(in, cf, 4) == 0) {
    strncpy(target, in, 4);
    return true;
  }
  return false;
}

void handleCountdownBeeps(unsigned long startMs, unsigned long timeoutMs,
                          bool& b3, bool& b2, bool& b1) {
  unsigned long elapsed = millis() - startMs;
  if (elapsed >= timeoutMs) return;
  unsigned long remaining = timeoutMs - elapsed;
  if (!b3 && remaining <= 3000 && remaining > 2000) {
    beep(1500, 80);
    b3 = true;
  } else if (!b2 && remaining <= 2000 && remaining > 1000) {
    beep(1500, 80);
    b2 = true;
  } else if (!b1 && remaining <= 1000) {
    beep(1800, 120);
    b1 = true;
  }
}

// ===== Logging =====
void logEvent(const char* evt) {
  DateTime now = rtc.now();
  char buf[LOG_SIZE + 1];
  // "DD/MM HH:MM:SS<sp>EVT(8)" = 14 + 1 + 8 = 23 chars, pad to 24
  snprintf(buf, sizeof(buf), "%02d/%02d %02d:%02d:%02d %-8s",
           now.day(), now.month(), now.hour(), now.minute(), now.second(), evt);
  for (int i = strlen(buf); i < LOG_SIZE; i++) buf[i] = ' ';
  buf[LOG_SIZE] = '\0';
  int addr = E_LOG_START + (logIndex * LOG_SIZE);
  for (int i = 0; i < LOG_SIZE; i++) EEPROM.write(addr + i, buf[i]);
  logIndex = (logIndex + 1) % MAX_LOGS;
  EEPROM.write(E_LOG_INDEX, logIndex);
}

// ===== ACTIONS =====
void animasiBukaPintu();      // Function declaration added
void animasiTutupPintu();     // Function declaration for animasiTutupPintu
void animasiPintuTertutup();  // Function declaration for animasiPintuTertutup


void openDoor(const char* userName) {
  // MODIFIKASI: Aktifkan relay untuk memberi daya pada servo
  digitalWrite(RELAY_PIN, LOW);  // LOW untuk mengaktifkan relay (umumnya)
  delay(300);                    // Beri waktu sedikit agar daya servo stabil

  servo.write(POS_OPEN);
  digitalWrite(LED_GREEN, HIGH);
  digitalWrite(LED_RED, LOW);

  // 🔹 Buzzer buka pintu dua nada naik
  beep(1000, 150);  // nada pertama
  delay(100);
  beep(1500, 200);  // nada kedua

  lcd.clear();
  lcdCenter("Selamat Datang", 0);
  // Buat salinan nama user dan trim spasi kanan
  char nameCentered[USER_NAME_LEN + 1];
  strncpy(nameCentered, userName, USER_NAME_LEN);
  nameCentered[USER_NAME_LEN] = '\0';
  int len = USER_NAME_LEN;
  while (len > 0 && (nameCentered[len - 1] == ' ' || nameCentered[len - 1] == '\0')) len--;
  nameCentered[len] = '\0';
  lcdCenter(nameCentered, 1);
  delay(1200);

  animasiBukaPintu();  // 🔹 animasi buka pintu

  // tunggu PIR HIGH (orang lewat)
  while (digitalRead(PIR_PIN) == LOW) {
    delay(50);  // cek tiap 50ms
  }

  // PIR terdeteksi, beri delay singkat agar orang benar-benar lewat
  delay(800);

  servo.write(POS_CLOSED);

  // MODIFIKASI: Beri waktu servo untuk kembali ke posisi tertutup
  delay(1000);

  // MODIFIKASI: Matikan relay setelah servo selesai bergerak
  digitalWrite(RELAY_PIN, HIGH);  // HIGH untuk mematikan relay

  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, HIGH);

  animasiTutupPintu();  // 🔹 animasi tutup pintu + beep
  delay(500);
  animasiPintuTertutup();  // 🔹 animasi pintu tertutup + beep panjang
  delay(1500);
}

// ===== ANIMASI MASUK MENU =====
void animateLoadingBar(const char* text) {
  lcd.clear();
  lcdCenter(text, 0);   // judul di baris 0
  lcd.setCursor(0, 1);  // baris 1 untuk progress bar
  for (int i = 0; i < 16; i++) {
    lcd.write(byte(255));  // blok penuh
    delay(120);
  }
  delay(200);
  lcdClearRow(1);
}

// ===== ANIMASI DINAMIS =====
void animasiBukaPintu() {
  lcd.clear();
  lcdCenter("MEMBUKA PINTU", 0);
  for (int step = 0; step <= 8; step++) {
    lcd.setCursor(0, 1);
    for (int i = 0; i < step; i++) lcd.print(" ");
    for (int i = step; i < 8; i++) lcd.write(byte(255));
    for (int i = 0; i < 8 - step; i++) lcd.write(byte(255));
    for (int i = 0; i < step; i++) lcd.print(" ");
    delay(120);
  }
}

void animasiTutupPintu() {
  lcd.clear();
  lcdCenter("MENUTUP PINTU", 0);
  for (int step = 8; step >= 0; step--) {
    lcd.setCursor(0, 1);
    for (int i = 0; i < step; i++) lcd.print(" ");
    for (int i = step; i < 8; i++) lcd.write(byte(255));
    for (int i = 0; i < 8 - step; i++) lcd.write(byte(255));
    for (int i = 0; i < step; i++) lcd.print(" ");

    // 🔹 beep pendek tiap langkah menutup
    beep(1000, 50);
    delay(100);
  }
}

void animasiPintuTertutup() {
  // 🔹 beep panjang tanda pintu tertutup
  beep(1500, 300);

  for (int n = 0; n < 3; n++) {
    lcd.clear();
    lcdCenter("PINTU TERTUTUP", 0);
    lcd.setCursor(0, 1);
    for (int i = 0; i < 16; i++) lcd.write(byte(255));
    delay(300);
    lcd.clear();
    lcdCenter("PINTU TERTUTUP", 0);
    delay(200);
  }
  lcdCenter("PINTU TERTUTUP", 0);
}

void animasiTimeoutAdmin() {
  lcd.clear();
  lcdCenter("TIMEOUT ADMIN", 0);

  // karakter panah custom
  byte panah[8] = {
    B00000,
    B00100,
    B00110,
    B11111,
    B11111,
    B00110,
    B00100,
    B00000
  };
  lcd.createChar(0, panah);

  // animasi panah bergerak di baris kedua
  for (int pos = 0; pos < 16; pos++) {
    lcd.setCursor(pos, 1);
    lcd.write(byte(0));  // tampilkan panah
    beep(800, 30);       // suara setiap langkah
    delay(100);
    lcd.setCursor(pos, 1);
    lcd.print(" ");  // hapus panah
  }
}

// ===== UTIL LOG (VIEW/CLEAR) =====
static inline void trimRight(char* s) {
  int n = strlen(s);
  while (n > 0 && (s[n - 1] == ' ' || s[n - 1] == '\r' || s[n - 1] == '\n')) s[--n] = '\0';
}

void viewLogs(unsigned long& lastAction) {
  int idx = (logIndex + MAX_LOGS - 1) % MAX_LOGS;  // mulai dari entri terbaru
  while (true) {
    // Baca satu entri log
    char logbuf[LOG_SIZE + 1];
    int addr = E_LOG_START + (idx * LOG_SIZE);
    for (int i = 0; i < LOG_SIZE; i++) logbuf[i] = EEPROM.read(addr + i);
    logbuf[LOG_SIZE] = '\0';

    // Baris 0: tanggal & waktu (14 char) -> "DD/MM HH:MM:SS"
    char line0[17];
    memset(line0, ' ', 16);
    line0[16] = '\0';
    memcpy(line0, logbuf, 14);

    // Baris 1: event (ambil dari posisi 15, maksimal 16 karakter, tanpa di-center)
    char evt[17];
    memcpy(evt, logbuf + 15, 16);
    evt[16] = '\0';
    // Tampilkan
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line0);
    lcd.setCursor(0, 1);
    lcd.print(evt);

    // Navigasi: A=up (lebih lama), B=down (lebih baru). Tidak ditulis di LCD.
    unsigned long t0 = millis();
    while (true) {
      char k = getKeyWithBeep();
      if (k == 'A') {
        idx = (idx + MAX_LOGS - 1) % MAX_LOGS;
        lastAction = millis();
        break;
      } else if (k == 'B') {
        idx = (idx + 1) % MAX_LOGS;
        lastAction = millis();
        break;
      } else if (k == 'D' || k == '*' || k == '#') {
        return;
      }  // keluar dari view log
      // kecilkan beban CPU
      delay(60);
      // Jangan biarkan timeout admin berbunyi saat cuma melihat layar yang sama
      if (millis() - t0 > 1500) {
        lastAction = millis();
        t0 = millis();
      }
    }
  }
}

void clearLogs() {
  lcd.clear();
  lcdCenter("KONFIRMASI CLEAR", 0);
  lcd.setCursor(0, 1);
  lcd.print("*=NO  #=YES");
  while (true) {
    char c = getKeyWithBeep();
    if (c == '#') {  // YES
      for (int i = 0; i < MAX_LOGS * LOG_SIZE; i++) EEPROM.write(E_LOG_START + i, ' ');
      EEPROM.write(E_LOG_INDEX, 0);
      logIndex = 0;
      beep(2000, 200);  // berhasil
      lcd.clear();
      lcdCenter("CLEAR LOG", 0);
      lcdCenter("BERHASIL", 1);
      delay(1000);
      return;
    } else if (c == '*') {  // NO
      beep(600, 300);       // batal
      lcd.clear();
      lcdCenter("CLEAR LOG", 0);
      lcdCenter("BATAL", 1);
      delay(1000);
      return;
    }
    delay(30);
  }
}

// ===== Tambahan: Manajemen User (Admin -> Tombol A) =====
static void copyTrimmedName(int idx, char* out) {
  strncpy(out, users[idx].name, USER_NAME_LEN);
  out[USER_NAME_LEN] = '\0';
  trimRight(out);
}

static void clearEEPROMUserSlot(int slot) {
  if (slot < 0 || slot >= MAX_USERS) return;
  int addr = E_USER_START + slot * E_USER_SIZE;
  for (int j = 0; j < E_USER_SIZE; j++) EEPROM.write(addr + j, 0xFF);
}

static void rewriteUsersToEEPROM() {
  for (int i = 0; i < userCount; i++) saveUser(i);
  for (int i = userCount; i < MAX_USERS; i++) clearEEPROMUserSlot(i);
}

// Lihat daftar user dengan auto-scroll. Keluar dengan *, #, atau D.
void viewUsersAutoScroll() {
  if (userCount <= 0) {
    lcd.clear();
    lcdCenter("DAFTAR USER", 0);
    lcdCenter("BELUM ADA", 1);
    beep(600, 300);
    delay(1200);
    return;
  }

  int idx = 0;
  int lastIdx = -1;
  unsigned long lastSwap = millis();
  const unsigned long interval = 1500UL;  // jeda pergantian user (ms)

  while (true) {
    // Auto-advance index tiap interval
    if (millis() - lastSwap >= interval) {
      idx = (idx + 1) % userCount;
      lastSwap = millis();
    }

    // Redraw hanya saat berganti user
    if (idx != lastIdx) {
      char name[USER_NAME_LEN + 1];
      copyTrimmedName(idx, name);

      char line0[17];
      snprintf(line0, sizeof(line0), "USER %02d/%02d", idx + 1, userCount);

      // Tulis baris tanpa clear, full 16 kolom (menghindari flicker)
      lcdCenter(line0, 0);
      lcdCenter(name, 1);

      lastIdx = idx;
    }

    // Keluar hanya dengan '*'
    char k = getKeyWithBeep();
    if (k == '*') return;

    delay(20);
  }
}

// Hapus user: pilih dengan A/B, konfirmasi dengan '#', batal '*'
// Versi anti-flicker + notifikasi saat menekan '*'
void deleteUserMenu() {
  if (userCount <= 0) {
    lcd.clear();
    lcdCenter("TIDAK ADA USER", 0);
    lcdCenter("UNTUK DIHAPUS", 1);
    beep(600, 300);
    delay(1200);
    return;
  }

  int sel = 0;
  int lastSel = -1;
  bool needRedraw = true;

  auto draw = [&](int idx) {
    char name[USER_NAME_LEN + 1];
    copyTrimmedName(idx, name);

    // Potong nama ke 13 kolom (format: "01: <13char>")
    char nameShort[14];
    strncpy(nameShort, name, 13);
    nameShort[13] = '\0';

    char line[17];
    snprintf(line, sizeof(line), "%02d: %s", idx + 1, nameShort);

    lcd.clear();
    lcdCenter("HAPUS USER", 0);
    lcd.setCursor(0, 1);
    int L = strlen(line);
    for (int i = 0; i < 16; i++) {
      char c = (i < L) ? line[i] : ' ';
      lcd.print(c);
    }
  };

  while (true) {
    if (needRedraw || sel != lastSel) {
      draw(sel);
      lastSel = sel;
      needRedraw = false;
    }

    char k = getKeyWithBeep();
    if (!k) {
      delay(30);
      continue;
    }

    if (k == 'A') {
      sel = (sel + userCount - 1) % userCount;
      needRedraw = true;
    } else if (k == 'B') {
      sel = (sel + 1) % userCount;
      needRedraw = true;
    } else if (k == '#') {
      // Konfirmasi hapus
      lcd.clear();
      lcdCenter("HAPUS USER INI?", 0);
      lcd.setCursor(0, 1);
      lcd.print("*=NO  #=YES");
      while (true) {
        char c = getKeyWithBeep();
        if (!c) {
          delay(30);
          continue;
        }
        if (c == '*') {
          // Notifikasi batal
          beep(600, 200);
          char nm[USER_NAME_LEN + 1];
          copyTrimmedName(sel, nm);
          lcd.clear();
          lcdCenter("BATAL HAPUS", 0);
          lcdCenter(nm, 1);
          delay(700);
          needRedraw = true;  // kembali ke daftar, redraw sekali
          break;
        } else if (c == '#') {
          char nm[USER_NAME_LEN + 1];
          copyTrimmedName(sel, nm);

          // Geser array RAM ke kiri
          for (int i = sel; i < userCount - 1; i++) {
            users[i] = users[i + 1];
          }
          userCount--;

          // Tulis ulang EEPROM dan kosongkan slot sisa
          rewriteUsersToEEPROM();

          // Log
          char logmsg[LOG_SIZE + 1];
          snprintf(logmsg, LOG_SIZE, "DEL %s", nm);
          logEvent(logmsg);

          // Notifikasi berhasil
          beep(2000, 200);
          lcd.clear();
          lcdCenter("USER DIHAPUS", 0);
          lcdCenter(nm, 1);
          delay(800);

          if (userCount <= 0) return;
          if (sel >= userCount) sel = userCount - 1;
          needRedraw = true;  // redraw daftar setelah penghapusan
          break;
        }
      }
    } else if (k == '*') {
      // Notifikasi keluar menu hapus
      beep(600, 200);
      lcd.clear();
      lcdCenter("KEMBALI KE MENU", 0);
      lcdCenter("USER MANAGEMENT", 1);
      delay(700);
      return;  // kembali ke adminUserManagement
    }
  }
}

// Tambahkan ini (global, sebelum adminUserManagement)
bool umTimeoutToMain = false;

// Submenu Manajemen User (Admin -> Tombol A)
void adminUserManagement() {
  // Header sekali
  lcd.clear();
  lcdCenter("USER MANAGEMENT:", 0);

  auto drawRow1 = [&](bool first) {
    lcd.setCursor(0, 1);
    const char* t = first ? "A=LIHAT B=TAMBAH" : "C=HAPUS *=KELUAR";
    for (int i = 0; i < 16; i++) {
      char c = t[i] ? t[i] : ' ';
      lcd.print(c);
    }
  };

  const unsigned long SWAP_MS = 5000UL;
  const unsigned long TIMEOUT_MS = 15000UL;

  bool showFirst = true;
  unsigned long lastSwap = millis();
  unsigned long lastAction = millis();  // aktivitas = input tombol

  // Flags beep countdown 3s/2s/1s
  bool b3 = false, b2 = false, b1 = false;

  drawRow1(showFirst);

  while (true) {
    // Auto-toggle instruksi setiap 5 detik (tidak dianggap aktivitas)
    if (millis() - lastSwap >= SWAP_MS) {
      showFirst = !showFirst;
      drawRow1(showFirst);
      lastSwap = millis();
    }

    // Beep countdown 3-2-1 detik sebelum timeout
    handleCountdownBeeps(lastAction, TIMEOUT_MS, b3, b2, b1);

    // Cek timeout 15 detik tanpa aktivitas
    if (millis() - lastAction >= TIMEOUT_MS) {
      animasiTimeoutAdmin();   // animasi yang sudah ada
      umTimeoutToMain = true;  // minta kembali ke menu utama
      return;
    }

    // Baca tombol
    char k = getKeyWithBeep();
    if (!k) {
      delay(20);
      continue;
    }

    // Reset timer & flags beep saat ada input pengguna
    lastAction = millis();
    b3 = b2 = b1 = false;

    if (k == 'A') {
      viewUsersAutoScroll();
      // Kembali ke layar manajemen, reset tampilan & timer
      lcd.clear();
      lcdCenter("USER MANAGEMENT:", 0);
      drawRow1(showFirst);
      lastSwap = millis();
      lastAction = millis();
      b3 = b2 = b1 = false;
    } else if (k == 'B') {
      addNewUser();
      lcd.clear();
      lcdCenter("USER MANAGEMENT:", 0);
      drawRow1(showFirst);
      lastSwap = millis();
      lastAction = millis();
      b3 = b2 = b1 = false;
    } else if (k == 'C') {
      deleteUserMenu();
      lcd.clear();
      lcdCenter("USER MANAGEMENT:", 0);
      drawRow1(showFirst);
      lastSwap = millis();
      lastAction = millis();
      b3 = b2 = b1 = false;
    } else if (k == '*') {
      // Kembali ke menu ADMIN (bukan ke menu utama)
      return;
    }
  }
}

// ===== MAIN MENU =====
void showMainMenuRealtime() {
  static int lastSecond = -1;
  DateTime now = rtc.now();
  if (now.second() != lastSecond) {
    char line[17];
    snprintf(line, sizeof(line), "%02d/%02d %02d:%02d:%02d",
             now.day(), now.month(), now.hour(), now.minute(), now.second());
    lcdCenter(line, 0);
    lcd.setCursor(0, 1);
    lcd.print("*USER     #ADMIN");
    lastSecond = now.second();
  }
}

// ===== SETUP =====
void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);  // MODIFIKASI: Inisialisasi pin relay sebagai OUTPUT

  digitalWrite(LED_RED, HIGH);
  digitalWrite(RELAY_PIN, HIGH);  // MODIFIKASI: Pastikan relay mati saat program dimulai

  servo.attach(SERVO_PIN);

  // MODIFIKASI: Atur posisi awal servo dengan aman menggunakan relay
  digitalWrite(RELAY_PIN, LOW);   // Nyalakan relay
  delay(300);                     // Tunggu daya stabil
  servo.write(POS_CLOSED);        // Pindahkan servo ke posisi tertutup
  delay(1000);                    // Beri waktu untuk bergerak
  digitalWrite(RELAY_PIN, HIGH);  // Matikan relay lagi

  lcd.init();
  lcd.backlight();
  // Animasi interaktif awal setelah LCD diinisialisasi
  lcd.clear();
  byte wave[8] = {
    B00000,
    B00100,
    B01110,
    B11111,
    B01110,
    B00100,
    B00000,
    B00000
  };
  lcd.createChar(3, wave);
  for (int r = 0; r < 2; r++) {
    for (int i = 0; i < 16; i++) {
      lcd.setCursor(i, 0);
      lcd.write(byte(3));
      beep(1200 + i * 30, 20);
      digitalWrite(LED_GREEN, (i % 2 == 0));
      digitalWrite(LED_RED, (i % 2 == 1));
      delay(30);
      lcd.setCursor(i, 0);
      lcd.print(" ");
    }
    for (int i = 15; i >= 0; i--) {
      lcd.setCursor(i, 1);
      lcd.write(byte(3));
      beep(1200 + (15 - i) * 30, 20);
      digitalWrite(LED_GREEN, (i % 2 == 0));
      digitalWrite(LED_RED, (i % 2 == 1));
      delay(30);
      lcd.setCursor(i, 1);
      lcd.print(" ");
    }
  }
  // Matikan kedua LED setelah animasi gelombang
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
  lcd.clear();
  lcdCenter("MEMULAI SISTEM", 0);
  lcd.setCursor(0, 1);
  for (int i = 0; i < 16; i++) {
    lcd.write(byte(255));
    beep(1500, 15);
    // LED berkedip bergantian juga saat loading
    digitalWrite(LED_GREEN, (i % 2 == 0));
    digitalWrite(LED_RED, (i % 2 == 1));
    delay(40);
  }
  delay(200);
  lcdClearRow(1);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, HIGH);  // Kembali ke status awal
  rtc.begin();
  if (rtc.lostPower()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  loadPINs();
  lcd.clear();
  lcdCenter("SISTEM DOOR LOCK", 0);
  lcdCenter("SIAP DIGUNAKAN", 1);
  delay(1200);

  // 🔹 Animasi loading
  lcd.clear();
  lcdCenter("MEMUAT...", 0);
  for (int i = 0; i <= 15; i++) {
    lcd.setCursor(i, 1);
    lcd.write(byte(255));  // blok penuh
    delay(100);
  }

  // 🔹 Bunyi beep pembuka (nada naik)
  tone(3, 1000, 200);  // 1000 Hz selama 200 ms
  delay(250);
  tone(3, 1500, 300);  // 1500 Hz selama 300 ms
  delay(350);

  // lanjut ke menu utama...
}

// ===== LOOP =====

void addNewUser() {
  if (userCount >= MAX_USERS) {
    lcd.clear();
    lcdCenter("USER PENUH!", 0);
    lcdCenter("MAX 10 USER", 1);
    beep(600, 300);
    delay(1200);
    return;
  }
  char pin[USER_PIN_LEN + 1] = { 0 };
  char pinConfirm[USER_PIN_LEN + 1] = { 0 };
  char name[USER_NAME_LEN + 1] = { 0 };
  lcd.clear();
  lcdCenter("PIN USER BARU:", 0);
  int startCol = 6;
  byte idx = 0;
  while (idx < USER_PIN_LEN) {
    lcd.setCursor(startCol + idx, 1);
    char k = getKeyWithBeep();
    if (isdigit(k)) {
      pin[idx++] = k;
      lcd.print("*");
    }
  }

  lcd.clear();
  lcdCenter("KONFIRMASI PIN:", 0);
  idx = 0;
  while (idx < USER_PIN_LEN) {
    lcd.setCursor(startCol + idx, 1);
    char k = getKeyWithBeep();
    if (isdigit(k)) {
      pinConfirm[idx++] = k;
      lcd.print("*");
    }
  }

  if (strncmp(pin, pinConfirm, USER_PIN_LEN) != 0) {
    beep(600, 300);
    lcd.clear();
    lcdCenter("PIN TIDAK SAMA", 0);
    lcdCenter("GAGAL TAMBAH", 1);
    delay(1200);
    return;
  }

  // Generate nama otomatis USER-1 sampai USER-10
  snprintf(name, USER_NAME_LEN + 1, "USER-%d", userCount + 1);
  int namelen = strlen(name);
  for (int i = namelen; i < USER_NAME_LEN; i++) name[i] = ' ';
  name[USER_NAME_LEN] = '\0';

  strncpy(users[userCount].pin, pin, USER_PIN_LEN + 1);
  strncpy(users[userCount].name, name, USER_NAME_LEN + 1);
  saveUser(userCount);
  userCount++;
  // Log penambahan user
  char logmsg[LOG_SIZE + 1];
  snprintf(logmsg, LOG_SIZE, "ADD %s", name);
  // Trim spasi kanan
  int loglen = strlen(logmsg);
  while (loglen > 0 && (logmsg[loglen - 1] == ' ' || logmsg[loglen - 1] == '\0')) loglen--;
  logmsg[loglen] = '\0';
  logEvent(logmsg);
  beep(2000, 200);
  lcd.clear();
  lcdCenter("USER DITAMBAH", 0);
  // Trim spasi kanan pada nama user sebelum ditampilkan
  char nameCentered[USER_NAME_LEN + 1];
  strncpy(nameCentered, name, USER_NAME_LEN);
  nameCentered[USER_NAME_LEN] = '\0';
  int len = USER_NAME_LEN;
  while (len > 0 && (nameCentered[len - 1] == ' ' || nameCentered[len - 1] == '\0')) len--;
  nameCentered[len] = '\0';
  lcdCenter(nameCentered, 1);
  delay(1200);
}

void loop() {
  showMainMenuRealtime();
  char k = getKeyWithBeep();
  if (!k) {
    delay(100);
    return;
  }

  if (k == '*') {  // ===== USER =====
    animateLoadingBar("MENU USER");
    int userIdx = askUserPIN();
    if (userIdx >= 0) {
      beepPINBenar();     // 🔹 bunyi beep PIN benar
      animasiPINBenar();  // 🔹 animasi interaktif PIN BENAR
      // Log user masuk pintu
      char logmsg[LOG_SIZE + 1];
      // Trim spasi kanan nama user
      char nameTrim[USER_NAME_LEN + 1];
      strncpy(nameTrim, users[userIdx].name, USER_NAME_LEN);
      nameTrim[USER_NAME_LEN] = '\0';
      int nlen = USER_NAME_LEN;
      while (nlen > 0 && (nameTrim[nlen - 1] == ' ' || nameTrim[nlen - 1] == '\0')) nlen--;
      nameTrim[nlen] = '\0';
      snprintf(logmsg, LOG_SIZE, "OPEN %s", nameTrim);
      logEvent(logmsg);
      // lcd.clear(); lcdCenter("PIN BENAR",0); delay(800); // diganti animasi
      openDoor(users[userIdx].name);
    } else {
      logEvent("PIN_ERR");
      beep(600, 300);
      animasiAksesDitolak();  // 🔹 animasi interaktif AKSES DITOLAK
      // lcd.clear(); lcdCenter("AKSES DITOLAK",0); delay(800); // diganti animasi
    }
  } else if (k == '#') {  // ===== ADMIN =====
    animateLoadingBar("MENU ADMIN");
    if (askPIN(adminPIN)) {
      beepPINBenar();     // 🔹 bunyi beep PIN benar
      animasiPINBenar();  // 🔹 animasi interaktif PIN BENAR
      // lcd.clear(); lcdCenter("PIN BENAR",0); delay(800); // diganti animasi
      const unsigned long TIMEOUT = 15000UL;
      unsigned long lastAction = millis();
      bool b3 = false, b2 = false, b1 = false;
      bool exitAdmin = false;

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("A=USR B=LOG");
      lcd.setCursor(0, 1);
      lcd.print("C=ADM D=CLR");

      while (!exitAdmin) {
        char a = getKeyWithBeep();
        if (a) {
          lastAction = millis();
          b3 = b2 = b1 = false;
        }

        // Hitung mundur + beep per detik terakhir
        handleCountdownBeeps(lastAction, TIMEOUT, b3, b2, b1);

        unsigned long elapsed = millis() - lastAction;
        long remaining = (long)(TIMEOUT - elapsed + 500) / 1000;
        if (remaining < 0) remaining = 0;

        static long lastRemaining = -1;  // taruh di atas atau sebelum loop

        // Hapus hanya jika jumlah digit berubah
        if (String(remaining).length() != String(lastRemaining).length()) {
          lcd.setCursor(13, 1);
          lcd.print("   ");
        }

        lcd.setCursor(13, 1);
        if (remaining < 10) lcd.print(" ");  // padding kalau satu digit
        lcd.print(remaining);
        lcd.print("s");

        lastRemaining = remaining;

        if (elapsed > TIMEOUT) {
          animasiTimeoutAdmin();  // 🔹 tampilkan animasi panah
          break;                  // balik ke menu utama
        }

        if (a == 'A') {  // Manajemen user: lihat/tambah/hapus
          adminUserManagement();
          if (umTimeoutToMain) {
            // Jika timeout di User Management, keluar ke menu utama
            umTimeoutToMain = false;
            exitAdmin = true;
          } else {
            // Jika keluar dengan '*', kembali ke layar ADMIN
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("A=USR B=LOG");
            lcd.setCursor(0, 1);
            lcd.print("C=ADM D=CLR");
            lastAction = millis();
            b3 = b2 = b1 = false;
          }
        }

        else if (a == 'C') {  // Ubah ADMIN PIN -> notifikasi -> balik ke menu utama
          if (askNewPIN(adminPIN)) {
            saveAdminPIN();
            beep(2000, 200);
            lcd.clear();
            lcdCenter("PIN ADMIN", 0);
            lcdCenter("BERHASIL DIUBAH", 1);
            delay(1000);
          } else {
            beep(600, 300);
            lcd.clear();
            lcdCenter("PIN ADMIN", 0);
            lcdCenter("GAGAL DIUBAH", 1);
            delay(1000);
          }
          exitAdmin = true;     // langsung balik ke menu utama
        } else if (a == 'B') {  // LIHAT LOG dengan A/B
          viewLogs(lastAction);
          // Kembali ke layar admin setelah keluar dari view
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("A=USR B=LOG");
          lcd.setCursor(0, 1);
          lcd.print("C=ADM D=CLR");
          lastAction = millis();
          b3 = b2 = b1 = false;
        } else if (a == 'D') {  // CLEAR LOG dengan konfirmasi
          clearLogs();
          // Kembali ke layar admin
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("A=USR B=LOG");
          lcd.setCursor(0, 1);
          lcd.print("C=ADM D=CLR");
          lastAction = millis();
          b3 = b2 = b1 = false;
        } else if (a == '*' || a == '#') {
          // Back ke menu utama tanpa menampilkan instruksi di LCD
          break;
        }

        delay(40);
      }
    } else {
      beep(600, 300);
      animasiAksesDitolak();  // 🔹 animasi interaktif AKSES DITOLAK
      // lcd.clear();
      // lcdCenter("AKSES ADMIN", 0);
      // lcdCenter("DITOLAK", 1);
      // delay(800); // diganti animasi
    }
  }
}
