# DuoSight

Thermal imaging reader and AI analysis application for Verdin i.MX8M Plus + FLIR/Melexis sensors (MLX90640).\
Cross-compiled using Docker + Yocto SDK. Runs headless or with Qt/GPU frontend (planned).

(C) 2025 Highland Biosciences Ltd.  

Dr. Richard Day
Highland Biosciences Ltd
Ness House
Greenleonachs Road
Duncanston
Dingwall.
Scotland IV7 8JD

+44 1 349 848 163

Usage: for research use only

See disclaimer at the end of this document

---

## 📁 Project Structure

```
duosight/
├── mlx90640-library/       # ✅ Git submodule: Melexis MLX90640 C++ driver
├── mlx90640-reader/        # 🧠 My thermal reader app (C++)
│   ├── src/
│   └── CMakeLists.txt
├── .devcontainer/          # VSCode + Docker cross-compile environment
├── .gitmodules             # Git submodule tracking
└── README.md               # You're reading it
```

---

## 🤩 MLX90640 Library (Submodule)

This project uses the official [mlx90640-library](https://github.com/melexis/mlx90640-library) as a **submodule**.

**Do not modify it.** It will be updated via Git.

### 🔁 Cloning This Project

Use the `--recurse-submodules` flag to clone both this repo **and** the driver:

```bash
git clone --recurse-submodules git@github.com:HBL001/duosight.git
```

Or, if already cloned:

```bash
git submodule update --init --recursive
```

---

## 🛠️ Building the Reader

This project uses a Docker-based VSCode devcontainer that includes the Yocto SDK for Verdin i.MX8M Plus.

### 🔧 Build steps inside the container:

```bash
cd mlx90640-reader
mkdir -p build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=/usr/local/oecore-x86_64/environment-setup-aarch64-tdx-linux ..
make -j
```

Result: `mlx90640_reader` binary is created in `build/`.

---

## 🚀 Running on Verdin Board

### 1. Copy the binary:

```bash
scp build/mlx90640_reader root@192.168.x.x:/home/root/
```

### 2. Run it on the board:

```bash
ssh root@192.168.x.x
chmod +x mlx90640_reader
./mlx90640_reader
```

Expected output:

- Ambient temperature
- A few raw pixel temperatures

---

## 🧪 Sensor Diagnostics

### Check if the MLX90640 is present:

```bash
i2cdetect -y 3
```

You should see address `0x33`.

### Check for new frame data:

```bash
i2ctransfer -y 3 w2@0x33 0x80 0x00 r2
```

When **bit 3** of the response (e.g. `0x0808`) is set → new subpage is ready.

---

## ✅ Git Setup Notes (SSH + Submodules)

If pushing code:

```bash
git remote -v
# should show: git@github.com:HBL001/duosight.git

# Make sure SSH agent is running:
eval "$(ssh-agent -s)"
ssh-add ~/.ssh/id_ed25519

# Test:
ssh -T git@github.com
```

---

## 𞷹 .gitignore

Recommended `.gitignore`:

```
# Build artifacts
build/
*.o
*.so
*.a
*.swp

# VSCode
.vscode/
*.code-workspace

# CMake
CMakeFiles/
CMakeCache.txt
cmake_install.cmake
Makefile

# Logs / temp
*.log
*.tmp
```

---

## 📄 License

- **DuoSight code**: MIT License
- **MLX90640 library**: Melexis license (see `mlx90640-library/LICENSE`)

### ⚠️ Disclaimer

This software is provided for experimental, educational, and prototyping use only. It is **not intended or certified for use in medical devices or clinical diagnostics**. Any deployment in such settings is strictly at your own risk and responsibility.

The author(s) and contributors make **no warranties or guarantees** regarding suitability for medical, life-support, or safety-critical applications.

---

Let me know if you want to add:

- A project badge
- A screenshot of the thermal output
- CI setup for auto-compilation

