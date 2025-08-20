# EEPROM Layout

Konstanta:
- MAX_USERS = 10
- USER_PIN_LEN = 4
- USER_NAME_LEN = 12
- LOG_SIZE = 24
- MAX_LOGS = 40

Struktur:
- User table: mulai `E_USER_START = 0`
  - Setiap slot: 4 byte PIN + 12 byte nama (`E_USER_SIZE = 16`)
  - 10 slot → alamat 0..159
  - Slot dianggap kosong jika `PIN[0]` bukan digit
- Admin PIN: `E_ADMIN_PIN = MAX_USERS * E_USER_SIZE` (offset 160) → 4 byte
- Log index: `E_LOG_INDEX = E_ADMIN_PIN + 4` (offset 164) → 1 byte (0..MAX_LOGS-1)
- Log data: `E_LOG_START = E_LOG_INDEX + 1` (offset 165)
  - 40 entri × 24 byte = 960 byte

Catatan:
- Penghapusan user menulis ulang daftar agar slot kontigu dan rapi.
- Perhatikan umur tulis EEPROM; gunakan clear log seperlunya.
