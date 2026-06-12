# Ironfang

Its a robot inspired by spider but with four legs.

Ironfang is a 4-legged quadruped robot with 3 degrees of freedom per leg (12 DOF total), designed for stable locomotion over varied terrain. It is controlled wirelessly over WiFi and features inverse kinematics and tripod gait locomotion.

---

## Features

- **12-DOF quadruped** — 3 servos per leg (coxa, femur, tibia)
- **Inverse kinematics** — foot position to joint angle computation
- **Tripod gait** — stable and fast alternating-triangle locomotion
- **WiFi control** — ESP32 in AP mode with WebSocket server
- **Directional movement** — forward, backward, lateral, and turning

---

## Hardware

| Component | Specification |
|---|---|
| Microcontroller | ESP32 (dual-core, WiFi/BT) |
| Servos | 12× MG995 (180°, ~11 kg·cm torque) |
| Power | 3S LiPo battery |
| Voltage Regulator | Step-down for ESP32 logic power |
| Frame | Custom 3D printed / fabricated |

### Leg Geometry

| Segment | Length |
|---|---|
| Coxa | 65 mm |
| Femur | 70 mm |
| Tibia | 70 mm |

---

## Software Architecture

```
Ironfang/
├── src/               # ESP32 firmware (C++)
│   └── main.cpp            # WiFi AP, WebSocket server, IK, gait control
│
├── Cad/                    # 3D printable STL files
│   ├── chassie_front.stl
│   ├── chassie_base.stl
│   ├── chassie_side.stl
│   ├── chassie_top.stl
│   ├── Link_1.stl
│   ├── Link_2.stl
│   ├── link_3.stl
│   └── Motor_cover_1.stl
|
├── LICENSE
└── README.md
```

---

## Getting Started

1. Open `firmware/` in PlatformIO or Arduino IDE.
2. Set your WiFi AP credentials in `main.cpp`:

```cpp
const char* AP_SSID = "Ironfang";
const char* AP_PASSWORD = "yourpassword";
```

3. Flash to ESP32 via USB.

---

## Kinematics

Ironfang uses **geometric inverse kinematics** to compute joint angles from a desired foot position `(x, y, z)` relative to the coxa joint origin.

Given:
- `L1` = femur length, `L2` = tibia length
- Foot position `(x, y, z)` in leg-local frame

The coxa, femur, and tibia angles are derived using 2D planar IK in the sagittal plane, with coxa handling the azimuthal rotation.

---

## Gait

### Tripod Gait
Three legs move simultaneously (alternating triangles), offering fast and stable locomotion. One triangle of legs lifts and swings forward while the other three remain grounded, then they alternate.

---

## Wiring

Servos are driven directly from ESP32 GPIO pins. Each servo signal wire connects to a dedicated GPIO, with power supplied from the 3S LiPo through the voltage regulator.

---

## Roadmap

- [x] Mechanical design & leg geometry
- [x] Torque and power analysis
- [x] ESP32 firmware — WiFi AP + WebSocket server
- [x] IK solver
- [x] Tripod gait
- [ ] IMU integration for terrain adaptation
- [ ] ROS2 integration
- [ ] Autonomous navigation

---

## Author

**Navdeep** **Swarnava** **Musthafa** **Shourya** **Tanishq**— IIT Guwahati  
GitHub: [github.com/Swarnava-IITG/Spider-Bot](https://github.com/Swarnava-IITG/Spider-Bot.git)

---

## License

MIT License. See `LICENSE` for details.
