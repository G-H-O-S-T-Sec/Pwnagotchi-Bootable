#!/bin/bash

# Exit on error
set -e

echo "Installing Anon on Raspberry Pi Zero W..."

# Install dependencies
echo "Installing dependencies..."
apt-get update
apt-get install -y \
    build-essential \
    cmake \
    git \
    libpcap-dev \
    aircrack-ng \
    tcpdump \
    python3-pip \
    python3-rpi.gpio \
    libhdf5-dev \
    libjpeg-dev \
    libatlas-base-dev \
    screen \
    wireless-tools \
    iw \
    macchanger \
    hashcat \
    hcxtools \
    hcxdumptool \
    libssl-dev

# Install Python dependencies for AI
pip3 install --no-cache-dir \
    numpy \
    tensorflow-lite \
    pillow \
    waveshare-epd

# Create directories
mkdir -p /opt/anon/{bin,lib,data,handshakes,models}
chmod 700 /opt/anon/handshakes

# Build and install Anon
echo "Building Anon..."
cd /opt/anon/build
cmake ..
make -j4
make install

# Install systemd service
echo "Installing systemd service..."
cat > /etc/systemd/system/anon.service << EOL
[Unit]
Description=Anon AI System
After=network.target

[Service]
Type=simple
User=root
WorkingDirectory=/opt/anon
ExecStart=/opt/anon/bin/anon
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
EOL

# Configure network interfaces
echo "Configuring network interfaces..."
cat > /etc/network/interfaces.d/anon << EOL
allow-hotplug wlan1
iface wlan1 inet manual
    wireless-mode monitor
EOL

# Set up persistent monitor mode
echo "Setting up persistent monitor mode..."
cat > /etc/udev/rules.d/70-persistent-net.rules << EOL
SUBSYSTEM=="net", ACTION=="add", ATTR{address}=="*", KERNEL=="wlan*", NAME="wlan1", RUN+="/usr/local/sbin/anon_setup_interface.sh"
EOL

# Create interface setup script
cat > /usr/local/sbin/anon_setup_interface.sh << EOL
#!/bin/bash
ip link set wlan1 down
iw dev wlan1 set type monitor
ip link set wlan1 up
iwconfig wlan1 txpower 10
EOL
chmod +x /usr/local/sbin/anon_setup_interface.sh

# Configure power management
echo "Configuring power management..."
cat > /etc/modprobe.d/8192cu.conf << EOL
options 8192cu rtw_power_mgnt=0 rtw_enusbss=0
EOL

# Set up MAC address randomization
echo "Setting up MAC address randomization..."
cat > /etc/systemd/system/mac-randomize.service << EOL
[Unit]
Description=Randomize MAC addresses
Before=network.target
BindsTo=sys-subsystem-net-devices-wlan1.device
After=sys-subsystem-net-devices-wlan1.device

[Service]
Type=oneshot
ExecStart=/usr/bin/macchanger -r wlan1
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
EOL

# Enable services
systemctl enable anon.service
systemctl enable mac-randomize.service

# Create update script
cat > /usr/local/sbin/anon_update << EOL
#!/bin/bash
cd /opt/anon
git pull
cd build
make clean
make -j4
make install
systemctl restart anon
EOL
chmod +x /usr/local/sbin/anon_update

# Configure stealth settings
echo "Configuring stealth settings..."
cat > /etc/sysctl.d/99-anon.conf << EOL
# Disable ICMP echo
net.ipv4.icmp_echo_ignore_all=1
# Disable IPv6
net.ipv6.conf.all.disable_ipv6=1
net.ipv6.conf.default.disable_ipv6=1
# TCP/IP hardening
net.ipv4.tcp_timestamps=0
net.ipv4.tcp_syncookies=1
EOL

# Apply sysctl settings
sysctl --system

# Set up cron jobs for maintenance
echo "Setting up maintenance tasks..."
(crontab -l 2>/dev/null; echo "0 4 * * * /usr/local/sbin/anon_update") | crontab -
(crontab -l 2>/dev/null; echo "*/30 * * * * /usr/bin/macchanger -r wlan1") | crontab -

# Final setup
echo "Performing final setup..."
systemctl daemon-reload
systemctl start anon.service

echo "Installation complete! Anon is now running."
echo "You can check the status with: systemctl status anon"
echo "View logs with: journalctl -u anon -f"
