# Getting the Verdin Online So You Can Program It

The Toradex Verdin iMX8M Plus board is perfectly capable of joining your local home or lab network. In production, however, it is highly recommended that you do **not** expose the microcontroller to the open internet without a hardened OTA (Over-the-Air) update mechanism and proper security audit. That means:

- ✅ Contract a professional OTA vendor (Toradex, Mender, etc.)
- ✅ Harden your system (disable unused services, configure firewall, set strong passwords)
- ✅ Only then consider letting it online unattended

Until then, treat your board like a development-only device. Give it a **static local IP address** and share internet access through your PC via **Ethernet** or **Wi-Fi bridging**.

This process is not foolproof and requires precision. Here's the full process.

---

## 🔧 Temporary Access (per boot session)

Run the following directly on the Verdin:

```sh
# Manually bring up the Ethernet interface
ip link set end0 up

# Assign a temporary static IP (adjust as needed)
ip addr add 192.168.137.10/24 dev end0

# Add a gateway route via the PC (assuming it's 192.168.137.1)
ip route add default via 192.168.137.1

# Optional: Add a DNS resolver (Google DNS)
echo 'nameserver 8.8.8.8' > /etc/resolv.conf

# Confirm network
ping 8.8.8.8
```

This works until the next reboot.

---

## 🛠️ Boot Persistent Static IP Setup

1. **Create network config file**:

```sh
mkdir -p /etc/systemd/network
vi /etc/systemd/network/10-static-end0.network
```

Paste this in:

```ini
[Match]
Name=end0

[Network]
Address=192.168.137.10/24
Gateway=192.168.137.1
DNS=8.8.8.8
```

2. **Disable ConnMan (if present)**:

```sh
systemctl stop connman
systemctl disable connman
```

3. **Enable and restart systemd-networkd**:

```sh
systemctl enable systemd-networkd
systemctl restart systemd-networkd
```

4. **Reboot to verify**:

```sh
reboot
```

After reboot, verify with:

```sh
ip a show end0
ping 8.8.8.8
```

---

## 🧪 Confirming Setup

- `ip a` should show `192.168.137.10/24` on `end0`
- `systemctl status systemd-networkd` should show **active (running)**
- `ping 8.8.8.8` should work

Once this works, your PC can `scp` files to:

```sh
scp <file> root@192.168.137.10:/home/root/
```

---

Keep this config around. Rebuilding Yocto images may wipe `/etc/` unless you bake it into your image.

To automate this further and persist it into Yocto using `meta-custom`.


