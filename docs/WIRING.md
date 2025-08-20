# Wiring

Komponen:
- Arduino Uno/Nano/kompatibel 5V
- LCD 16x2 I2C (0x27 default)
- RTC DS3231
- Keypad 4x4
- Servo (SG90/MG90S) + relay 1 channel (aktif LOW)
- PIR
- Buzzer
- LED hijau + LED merah (+ resistor)

Koneksi pin (default di sketch):
- Servo signal → D2
- Buzzer → D3
- LED hijau → D4
- LED merah → D5
- PIR → D6 (aktif HIGH saat deteksi)
- Keypad rows → D7, D8, D9, D10
- Keypad cols → D11, D12, D13, A0
- Relay control → A1 (Aktif LOW, OFF = HIGH)
- I2C LCD + RTC → SDA/SCL (Uno: A4/A5)

Catatan:
- Satukan GND semua modul dan sumber daya.
- Servo idealnya diberi daya terpisah (misal 5V 2A), jangan dari 5V Arduino saja.
- Relay menyalakan supply servo hanya saat dibutuhkan.
- Pastikan alamat I2C LCD benar (default 0x27), gunakan I2C scanner jika perlu.
