# LCD Clock

**LCD Clock** is a Windows application that displays current time and date on a USB character LCD (e.g., based on Prolific PL2303GS with HD44780).  
It can run as a **tray application** or as a **Windows service**.

## Features

- Auto-detection of displays by USB instance signature (`S142T16`)
- Support for multiple displays – switch via tray menu
- Configurable via `clock.ini` (baud rate, columns, rows, display mode)
- Hot‑plug detection – automatically reconnects when USB is unplugged/plugged
- Clears display on shutdown, sleep, service stop, or device change
- Single instance (mutex)
- Built without CRT – no dependency on `msvcrt.dll`

## Command-line options

| Option              | Description                                   |
|---------------------|-----------------------------------------------|
| (none)              | Run as tray application                       |
| `-i` / `--install`  | Install Windows service                       |
| `-is` / `--install-start` | Install and start service                 |
| `-u` / `--uninstall`| Uninstall service                             |
| `-s` / `--service`  | Run as service (used by SCM)                  |

## Configuration

Place `clock.ini` next to `clock.exe`. Example:

```ini
[settings]
speed=9600
columns=20
rows=2
display=dt
```

| Key       | Values                                          | Default | Description                                      |
|-----------|-------------------------------------------------|---------|--------------------------------------------------|
| `speed`   | `9600`, `19200`, `115200` (also hex: `0x2580`) | `9600`  | Baud rate for serial communication              |
| `columns` | `1` – `80`                                      | `20`    | Number of characters per line                    |
| `rows`    | `1` or `2`                                      | `2`     | Number of lines on the display                   |
| `display` | `t` (time only), `d` (date only), `dt` (both)  | `dt`    | What information to show                         |

## License

MIT License – see [LICENSE](LICENSE) file.

## Author
Pavel Bashkardin
