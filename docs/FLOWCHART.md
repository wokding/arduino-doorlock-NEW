# Flowchart Detail Sistem Door Lock

## 1. Main Loop
```mermaid
flowchart TD
  A["Start"] --> B["setup()"]
  B --> C["loop()"]
  C --> D["showMainMenuRealtime()"]
  D --> E["getKeyWithBeep()"]
  E -->|"no key"| C
  E -->|"*"| U["User Mode"]
  E -->|"#"| M["Admin Mode"]
  U --> C
  M --> C


---

## 2. User Mode
```mermaid
flowchart TD
  U1["User Mode"] --> U2["animateLoadingBar('MENU USER')"]
  U2 --> U3["askUserPIN()"]
  U3 -->|"valid"| U4["beepPINBenar() & animasiPINBenar()"]
  U4 --> U5["logEvent('OPEN')"]
  U5 --> U6["openDoor()"]
  U6 --> U7["Return to Main"]
  U3 -->|"invalid"| U8["logEvent('PIN_ERR')"]
  U8 --> U9["beep(600,300) & animasiAksesDitolak()"]
  U9 --> U7


---

## 3. Admin Mode
```mermaid
flowchart TD
  A1["Admin Mode"] --> A2["animateLoadingBar('MENU ADMIN')"]
  A2 --> A3["askPIN(adminPIN)"]
  A3 -->|"fail"| A4["beep(600,300) & animasiAksesDitolak()"] --> A5["Return to Main"]
  A3 -->|"ok"| A6["Enter Admin Loop"]

  subgraph Admin_Loop
    A6 --> A7["Display: A=USR B=LOG / C=ADM D=CLR"]
    A7 --> A8["getKeyWithBeep()"]
    A8 -->|"A"| A9["adminUserManagement()"]
    A8 -->|"B"| A10["viewLogs()"]
    A8 -->|"C"| A11["changeAdminPIN()"]
    A8 -->|"D"| A12["clearLogs()"]
    A8 -->|"* , #"| A5

    A9 --> A7
    A10 --> A7
    A11 --> A5
    A12 --> A7
  end


---

## 4. Admin → User Management
```mermaid
flowchart TB
  UM1["Admin → A"] --> UM2["Display header & row1"]
  UM2 --> UM3["Init timers: swap=5s, timeout=15s"]
  UM3 --> UM4{"idle ≥ 15s?"}
  UM4 -->|"yes"| UM5["animasiTimeoutAdmin() → exit"]
  UM4 -->|"no"| UM6{"swap ≥ 5s?"}
  UM6 -->|"yes"| UM7["toggle row1 instruksi"]
  UM6 -->|"no"| UM8["getKeyWithBeep()"]
  UM7 --> UM8
  UM8 -->|"none"| UM3
  UM8 -->|"A"| UM9["viewUsersAutoScroll()"] --> UM2
  UM8 -->|"B"| UM10["addNewUser()"] --> UM2
  UM8 -->|"C"| UM11["deleteUserMenu()"] --> UM2
  UM8 -->|"*"| UM12["exit to Admin Loop"]


---

## 5. View Users Auto Scroll
```mermaid
flowchart TD
  V1["viewUsersAutoScroll()"] --> V2{"userCount == 0?"}
  V2 -->|"yes"| V3["Show 'BELUM ADA'"] --> V4["Return"]
  V2 -->|"no"| V5["Init idx=0, lastIdx=-1, timer=now"]
  V5 --> V6{"timer ≥ 1.5s?"}
  V6 -->|"yes"| V7["idx=(idx+1)%N; timer=now"]
  V6 -->|"no"| V8
  V7 --> V8
  V8 --> V9{"idx != lastIdx?"}
  V9 -->|"yes"| V10["display 'USER idx/N' & name"]
  V9 -->|"no"| V11
  V10 --> V11
  V11 --> V12["getKeyWithBeep()"]
  V12 -->|"*"| V4
  V12 -->|"none"| V6


---

## 6. Delete User Menu
```mermaid
flowchart TB
  D1["deleteUserMenu()"] --> D2{"userCount==0?"}
  D2 -->|"yes"| D3["Show 'UNTUK DIHAPUS'"] --> D4["Return"]
  D2 -->|"no"| D5["sel=0,lastSel=-1,needRedraw=true"]
  D5 --> D6{"needRedraw or sel!=lastSel?"}
  D6 -->|"yes"| D7["draw(sel); needRedraw=false; lastSel=sel"]
  D6 -->|"no"| D8
  D7 --> D8
  D8 --> D9["getKeyWithBeep()"]
  D9 -->|"A"| D10["sel=(sel-1)%N; needRedraw=true"]
  D9 -->|"B"| D11["sel=(sel+1)%N; needRedraw=true"]
  D9 -->|"#"| D12["Confirm modal"]
  D9 -->|"*"| D20["Notify 'KEMBALI' → return"]

  subgraph CONFIRM
    D12 --> D13["getKeyWithBeep()"]
    D13 -->|"*"| D14["Notify 'BATAL HAPUS'; needRedraw=true"] --> D8
    D13 -->|"#"| D15["Shift users[], userCount–"]
    D15 --> D16["rewriteEEPROM(); logEvent('DEL')"]
    D16 --> D17["Notify 'USER DIHAPUS'"]
    D17 --> D18{"userCount>0?"}
    D18 -->|"yes"| D10
    D18 -->|"no"| D4
  end


---

## 7. Change Admin PIN
```mermaid
flowchart TD
  P1["changeAdminPIN()"] --> P2["askNewPIN(tmp)"]
  P2 -->|"ok"| P3["saveAdminPIN(); logEvent('ADM PIN'); Notify success"] --> P4["return"]
  P2 -->|"fail"| P5["Notify fail"] --> P4


---

## 8. Clear Logs
```mermaid
flowchart TD
  C1["clearLogs()"] --> C2["Show '*=NO  #=YES'"]
  C2 --> C3["getKeyWithBeep()"]
  C3 -->|"#"| C4["Erase EEPROM log; Notify success"] --> C5["return"]
  C3 -->|"*"| C6["Notify cancel"] --> C5

