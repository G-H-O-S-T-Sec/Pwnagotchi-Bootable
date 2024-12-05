# Anon - Advanced Stealth Pwnagotchi

Anon is a highly sophisticated, AI-powered WiFi security research tool optimized for the Raspberry Pi Zero W. It features advanced neural networks, stealth capabilities, and mesh networking.

## Features

- **Advanced Neural Network**
  - Multi-head attention mechanisms
  - Adaptive learning rates
  - Pattern recognition
  - Target prioritization

- **Stealth System**
  - Power-efficient operation
  - Signal strength optimization
  - Channel hopping algorithms
  - MAC address randomization

- **Mesh Networking**
  - Encrypted communication
  - Distributed intelligence
  - Automatic peer discovery
  - Data synchronization

- **Handshake Processing**
  - PMKID capture
  - WPA/WPA2 handshakes
  - Efficient storage
  - Hashcat integration

- **AI Personality**
  - Adaptive behavior
  - Learning capabilities
  - Mood system
  - Event memory

- **Display System**
  - E-paper display support
  - Status visualization
  - Network mapping
  - Real-time statistics

## Hardware Requirements

- Raspberry Pi Zero W
- SD Card (16GB+ recommended)
- USB WiFi adapter (monitor mode capable)
- Waveshare 2.13" E-Paper Display (optional)
- LiPo battery (optional)

## Installation

1. Flash Raspberry Pi OS Lite to SD card
2. Clone this repository:
   ```bash
   git clone https://github.com/yourusername/anon.git
   cd anon
   ```

3. Run installation script:
   ```bash
   sudo chmod +x build_scripts/install.sh
   sudo ./build_scripts/install.sh
   ```

4. Optimize the system:
   ```bash
   sudo ./build_scripts/optimize.sh
   ```

5. Reboot:
   ```bash
   sudo reboot
   ```

## Usage

Anon starts automatically on boot. Check status with:
```bash
sudo systemctl status anon
```

View logs:
```bash
sudo journalctl -u anon -f
```

### Commands

- Update Anon:
  ```bash
  sudo anon_update
  ```

- Clean up storage:
  ```bash
  sudo anon_cleanup
  ```

- View captured handshakes:
  ```bash
  ls /opt/anon/handshakes
  ```

## Configuration

Main configuration file: `/opt/anon/config.json`

Example configuration:
```json
{
  "device_name": "Anon",
  "display": {
    "enabled": true,
    "type": "waveshare_213"
  },
  "wifi": {
    "interface": "wlan1",
    "tx_power": 10,
    "channels": [1,6,11]
  },
  "mesh": {
    "enabled": true,
    "encryption": true
  },
  "stealth": {
    "mac_randomization": true,
    "low_power_mode": false
  }
}
```

## Security Features

- Encrypted storage
- MAC address randomization
- Stealth operation modes
- Secure mesh communication
- Anti-detection measures

## Troubleshooting

1. Check system logs:
   ```bash
   sudo journalctl -u anon -n 100
   ```

2. Verify WiFi interface:
   ```bash
   sudo iwconfig
   ```

3. Test monitor mode:
   ```bash
   sudo airmon-ng check
   ```

4. Check hardware:
   ```bash
   sudo vcgencmd measure_temp
   sudo vcgencmd get_throttled
   ```

## Updating

Anon automatically updates nightly. Force update:
```bash
sudo anon_update
```

## Contributing

1. Fork the repository
2. Create feature branch
3. Commit changes
4. Push to branch
5. Create Pull Request

## Disclaimer

This tool is for educational and research purposes only. Use responsibly and legally.

## License

MIT License - see LICENSE file for details
