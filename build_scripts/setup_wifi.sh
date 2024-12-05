#!/bin/bash

# Install required packages
apt-get update
apt-get install -y \
    libpcap-dev \
    aircrack-ng \
    tcpdump \
    wireless-tools \
    iw \
    usbutils \
    pciutils \
    net-tools

# Set up second WiFi interface (assuming using USB WiFi adapter)
echo "options 8192cu rtw_power_mgnt=0 rtw_enusbss=0" > /etc/modprobe.d/8192cu.conf
echo "options mt7601u rtusb_rx_thread=1" > /etc/modprobe.d/mt7601u.conf
echo "options rt2800usb nohwcrypt=1" > /etc/modprobe.d/rt2800usb.conf

# Create service to automatically set up monitor mode
cat > /etc/systemd/system/monitor-mode.service << EOL
[Unit]
Description=Set up monitor mode for WiFi adapter
After=network.target

[Service]
Type=oneshot
ExecStart=/usr/local/bin/setup_monitor_mode.sh
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
EOL

# Create monitor mode setup script
cat > /usr/local/bin/setup_monitor_mode.sh << EOL
#!/bin/bash
ip link set wlan1 down
iw dev wlan1 set type monitor
ip link set wlan1 name wlan1mon
ip link set wlan1mon up
iwconfig wlan1mon txpower 10
EOL

# Make script executable
chmod +x /usr/local/bin/setup_monitor_mode.sh

# Enable service
systemctl enable monitor-mode.service

# Set up MAC address randomization
cat > /etc/systemd/system/mac-randomize.service << EOL
[Unit]
Description=Randomize MAC address of WiFi adapter
After=network.target

[Service]
Type=oneshot
ExecStart=/usr/local/bin/randomize_mac.sh
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
EOL

# Create MAC randomization script
cat > /usr/local/bin/randomize_mac.sh << EOL
#!/bin/bash
ip link set wlan1mon down
macchanger -r wlan1mon
ip link set wlan1mon up
EOL

# Make script executable
chmod +x /usr/local/bin/randomize_mac.sh

# Enable service
systemctl enable mac-randomize.service

# Disable unnecessary services for stealth
systemctl disable bluetooth.service
systemctl disable avahi-daemon.service
systemctl disable cups.service
systemctl disable dhcpcd.service
systemctl disable triggerhappy.service

# Set up power management
cat > /etc/udev/rules.d/70-wifi-power.rules << EOL
ACTION=="add", SUBSYSTEM=="net", KERNEL=="wlan[0-9]", RUN+="/usr/sbin/iw dev %k set power_save off"
EOL

# Create handshake directory
mkdir -p /opt/anon/handshakes
chmod 700 /opt/anon/handshakes

echo "WiFi setup complete!"
