/**
 * Build IoT O2 Concentrator Integration Doc — Rev 2.0
 * Fully rewritten for Hardware v2.0 + captive-portal firmware.
 * Image placeholders: replace schematic / Cirkit / PCB figures yourself.
 */
const fs = require("fs");
const path = require("path");
const {
  Document, Packer, Paragraph, TextRun, Table, TableRow, TableCell,
  Header, Footer, AlignmentType, HeadingLevel, BorderStyle, WidthType,
  ShadingType, PageNumber, LevelFormat, VerticalAlign,
} = require("docx");

const OUT = path.join(__dirname, "IoT_O2_Concentrator_Integration_Rev2.0.docx");

// Page geometry (US Letter, 0.85" margins → content ~9792 DXA)
const PAGE_W = 12240;
const PAGE_H = 15840;
const MARGIN = 1224; // 0.85"
const CONTENT_W = PAGE_W - 2 * MARGIN; // 9792

const C = {
  ink: "1A1A2E",
  muted: "555555",
  accent: "1B4F72",
  headBg: "1B4F72",
  headFg: "FFFFFF",
  altBg: "F0F4F8",
  noteBg: "FFF8E7",
  noteBorder: "D4A017",
  dangerBg: "FDEDEC",
  dangerBorder: "C0392B",
  okBg: "E8F8F5",
  line: "CCCCCC",
};

const thin = { style: BorderStyle.SINGLE, size: 4, color: C.line };
const borders = { top: thin, bottom: thin, left: thin, right: thin };
const noBorder = {
  top: { style: BorderStyle.NONE, size: 0, color: "FFFFFF" },
  bottom: { style: BorderStyle.NONE, size: 0, color: "FFFFFF" },
  left: { style: BorderStyle.NONE, size: 0, color: "FFFFFF" },
  right: { style: BorderStyle.NONE, size: 0, color: "FFFFFF" },
};

function p(text, opts = {}) {
  const {
    bold = false, size = 20, color = C.ink, align = AlignmentType.LEFT,
    spaceBefore = 60, spaceAfter = 60, italics = false, heading = null,
  } = opts;
  return new Paragraph({
    heading: heading || undefined,
    alignment: align,
    spacing: { before: spaceBefore, after: spaceAfter, line: 276 },
    children: [
      new TextRun({
        text,
        bold,
        italics,
        size,
        font: "Arial",
        color,
      }),
    ],
  });
}

function h1(text) {
  return new Paragraph({
    heading: HeadingLevel.HEADING_1,
    spacing: { before: 320, after: 160 },
    border: { bottom: { style: BorderStyle.SINGLE, size: 12, color: C.accent, space: 4 } },
    children: [new TextRun({ text, bold: true, size: 28, font: "Arial", color: C.accent })],
  });
}

function h2(text) {
  return new Paragraph({
    heading: HeadingLevel.HEADING_2,
    spacing: { before: 240, after: 100 },
    children: [new TextRun({ text, bold: true, size: 24, font: "Arial", color: C.ink })],
  });
}

function body(text) {
  return p(text, { size: 20, spaceBefore: 40, spaceAfter: 80 });
}

function note(text, kind = "note") {
  const bg = kind === "danger" ? C.dangerBg : kind === "ok" ? C.okBg : C.noteBg;
  const bc = kind === "danger" ? C.dangerBorder : kind === "ok" ? "1E8449" : C.noteBorder;
  const b = { style: BorderStyle.SINGLE, size: 8, color: bc };
  return new Table({
    width: { size: CONTENT_W, type: WidthType.DXA },
    columnWidths: [CONTENT_W],
    rows: [
      new TableRow({
        children: [
          new TableCell({
            borders: { top: b, bottom: b, left: b, right: b },
            width: { size: CONTENT_W, type: WidthType.DXA },
            shading: { fill: bg, type: ShadingType.CLEAR },
            margins: { top: 80, bottom: 80, left: 120, right: 120 },
            children: [p(text, { size: 18, bold: kind === "danger", spaceBefore: 0, spaceAfter: 0 })],
          }),
        ],
      }),
    ],
  });
}

function placeholder(label) {
  const b = { style: BorderStyle.DASHED, size: 12, color: "888888" };
  return new Table({
    width: { size: CONTENT_W, type: WidthType.DXA },
    columnWidths: [CONTENT_W],
    rows: [
      new TableRow({
        children: [
          new TableCell({
            borders: { top: b, bottom: b, left: b, right: b },
            width: { size: CONTENT_W, type: WidthType.DXA },
            shading: { fill: "F5F5F5", type: ShadingType.CLEAR },
            margins: { top: 200, bottom: 200, left: 120, right: 120 },
            verticalAlign: VerticalAlign.CENTER,
            children: [
              p(`[ FIGURE PLACEHOLDER ]`, {
                bold: true, size: 22, color: "666666", align: AlignmentType.CENTER,
                spaceBefore: 80, spaceAfter: 40,
              }),
              p(label, {
                size: 18, color: "666666", align: AlignmentType.CENTER, italics: true,
                spaceBefore: 0, spaceAfter: 80,
              }),
            ],
          }),
        ],
      }),
    ],
  });
}

function cell(text, width, opts = {}) {
  const {
    bold = false, header = false, align = AlignmentType.LEFT, fill = null, size = 17,
  } = opts;
  return new TableCell({
    borders,
    width: { size: width, type: WidthType.DXA },
    shading: header
      ? { fill: C.headBg, type: ShadingType.CLEAR }
      : fill
        ? { fill, type: ShadingType.CLEAR }
        : undefined,
    margins: { top: 50, bottom: 50, left: 70, right: 70 },
    children: [
      new Paragraph({
        alignment: align,
        spacing: { before: 20, after: 20 },
        children: [
          new TextRun({
            text: String(text),
            bold: bold || header,
            size,
            font: "Arial",
            color: header ? C.headFg : C.ink,
          }),
        ],
      }),
    ],
  });
}

function table(headers, rows, colWidths) {
  const sum = colWidths.reduce((a, b) => a + b, 0);
  if (sum !== CONTENT_W) {
    // allow slight mismatch but warn in console
    console.warn("colWidths sum", sum, "expected", CONTENT_W);
  }
  const headerRow = new TableRow({
    children: headers.map((h, i) => cell(h, colWidths[i], { header: true })),
  });
  const dataRows = rows.map((r, ri) =>
    new TableRow({
      children: r.map((c, i) =>
        cell(c, colWidths[i], {
          fill: ri % 2 === 1 ? C.altBg : null,
          bold: i === 0 && false,
        })
      ),
    })
  );
  return new Table({
    width: { size: CONTENT_W, type: WidthType.DXA },
    columnWidths: colWidths,
    rows: [headerRow, ...dataRows],
  });
}

function spacer(n = 80) {
  return new Paragraph({ spacing: { before: n, after: 0 }, children: [] });
}

function tocLine(num, title) {
  return p(`${num}.  ${title}`, { size: 20, spaceBefore: 40, spaceAfter: 40 });
}

const children = [];

// ========== COVER ==========
children.push(
  p("IoT Oxygen Concentrator Integration", {
    bold: true, size: 40, color: C.accent, align: AlignmentType.CENTER,
    spaceBefore: 600, spaceAfter: 120,
  }),
  p("Purity Monitoring, Alert System and Compressor Control", {
    size: 22, color: C.muted, align: AlignmentType.CENTER, italics: true, spaceAfter: 200,
  }),
  p("Project Documentation  |  Rev 2.0  |  Hardware v2.0 + Firmware Captive Portal", {
    size: 20, color: C.muted, align: AlignmentType.CENTER, spaceAfter: 60,
  }),
  p("10 July 2026", {
    size: 20, color: C.muted, align: AlignmentType.CENTER, spaceAfter: 200,
  }),
);

children.push(
  table(
    ["Baseline", "Hardware", "Firmware", "Docs layout"],
    [[
      "git tag v1.0 closed Phase 1–4 (EVB-era board + firmware)",
      "KiCad schematic/PCB Rev 2.0",
      "Input modes + WiFi captive portal",
      "docs/v1 archive · docs/v2 current",
    ]],
    [2448, 2448, 2448, 2448]
  )
);

children.push(
  spacer(120),
  note(
    "Rev 2.0 is the start of the Hardware v2 documentation line. All v1.x integration docs and " +
    "legacy figures live under docs/v1/. Replace the figure placeholders in Sections 14–17 with " +
    "updated Cirkit Designer, KiCad schematic, and PCB images when ready."
  ),
);

// ========== TOC ==========
children.push(
  h1("Table of Contents"),
  tocLine("1", "Project Overview"),
  tocLine("2", "What Changed in Rev 2.0 (from git tag v1.0)"),
  tocLine("3", "System Architecture"),
  tocLine("4", "Power Architecture"),
  tocLine("5", "Component Descriptions"),
  tocLine("6", "OCS-3FL2.0 Oxygen Purity Sensor"),
  tocLine("7", "UART Communication and Sensor Protocol"),
  tocLine("8", "Relay Driver Circuit"),
  tocLine("9", "SIM800 GSM — EVB and Bare Module"),
  tocLine("10", "Alert Logic and Threshold Behaviour"),
  tocLine("11", "LCD 2004 Display"),
  tocLine("12", "Caregiver Number Configuration"),
  tocLine("13", "ESP32 Pin Assignment"),
  tocLine("14", "Enclosure and Mechanical Integration"),
  tocLine("15", "Pictorial Wiring Diagram (placeholder)"),
  tocLine("16", "Wiring Connection Reference Table"),
  tocLine("17", "KiCad Schematic (placeholder)"),
  tocLine("18", "PCB Layout (placeholder)"),
  tocLine("19", "Firmware Architecture"),
  tocLine("20", "Captive Portal and Input Modes"),
  tocLine("A", "Sensor UART Packet Format"),
  tocLine("B", "Bill of Materials Summary"),
  tocLine("C", "config.h and portal_html.h Reference"),
  tocLine("D", "Revision History"),
);

// ========== 1 OVERVIEW ==========
children.push(
  h1("1. Project Overview"),
  body(
    "This document describes the design and implementation of an IoT integration system for an " +
    "existing PSA (Pressure Swing Adsorption) oxygen concentrator. The concentrator is a commercial " +
    "medical device. No modifications are made to its internal components. An external integration " +
    "module is housed in a separate enclosure and connects via oxygen sampling tubing and wires for " +
    "power and compressor control."
  ),
  h2("1.1 What the integration system does"),
  body(
    "Measures oxygen purity: One OCS-3FL2.0 ultrasonic sensor on the final oxygen outlet after all " +
    "filtration stages. That reading is what the patient receives and drives all threshold decisions."
  ),
  body(
    "Provides visual feedback: LCD 2004 (20×4, I2C) shows live O2 %, flow, gas temperature, status, " +
    "GSM signal quality, SIM state, and uptime."
  ),
  body(
    "Alerts the caregiver: Below the warning threshold the system beeps and can send SMS. Below the " +
    "danger threshold the buzzer is continuous, a voice call is placed, and the compressor relay opens."
  ),
  body(
    "Controls the compressor: A 5 V relay module cuts compressor power in the danger state " +
    "(fail-safe active-LOW module control as implemented in firmware)."
  ),
  body(
    "Operates independently: No smartphone app is required for field use. Caregiver number and " +
    "thresholds are stored in NVS and can be changed by USB serial or SMS. For laboratory demos, a " +
    "WiFi captive portal (SSID O2-Controller) selects input source and can inject purity/flow/temp."
  ),
  note(
    "NOTE: This integration does not modify concentrator internal circuits. Connections are limited " +
    "to (1) AC mains feed through the compressor relay and (2) the oxygen outlet tube for sensor sampling."
  ),
);

// ========== 2 DELTA ==========
children.push(
  h1("2. What Changed in Rev 2.0 (from git tag v1.0)"),
  body(
    "Git tag v1.0 closed the first hardware/firmware baseline: SIM800L evaluation board (EVB) on " +
    "UART1, single-layer PCB with hand solder bridges JP1–JP3, O2 voltage selector jumper J11, and " +
    "firmware without a captive portal. Hardware commit “Hardware v2.0” and the current firmware " +
    "form the Rev 2.0 documentation baseline."
  ),
  h2("2.1 Hardware (KiCad schematic / PCB sheet rev 2.0)"),
  table(
    ["Item", "v1.0 (tag)", "v2.0 (current)"],
    [
      ["GSM path", "J7 SIM_GSM_EVB only (5 V EVB)", "J7 EVB retained + J14 SIM_GSM_BARE for bare module"],
      ["Bare module VCC", "N/A", "D1 1N4001 series drop from 5 V rail into bare module VCC"],
      ["O2 sensor VCC select", "J11 O2_VCC_SEL (5 V / 12 V jumper)", "J11 removed — sensor accepts 5–12 V; no field jumper"],
      ["Single-layer bridges", "JP1, JP2, JP3 solder bridges (must close)", "GND net ties for DRC-legal continuity"],
      ["Libraries", "Mixed paths", "JST XH footprints/3D under hardware/libraries/"],
      ["ESP32 pin map", "Unchanged", "Unchanged (see Section 13)"],
    ],
    [2200, 3600, 3992]
  ),
  spacer(100),
  h2("2.2 Firmware"),
  table(
    ["Item", "v1.0 baseline", "v2.0 current"],
    [
      ["Input source", "Potentiometer demo or manual UART swap", "Runtime InputMode: POT / WIFI / SENSOR"],
      ["Mode persistence", "N/A", "Always boots MODE_POT; never stored in NVS"],
      ["WiFi", "None", "Soft-AP + DNS captive portal + AsyncWebServer"],
      ["webTask", "None", "Core 0, prio 1, stack 8192; DNS every 10 ms"],
      ["UI assets", "N/A", "portal_html.h (PORTAL_HTML in PROGMEM)"],
      ["Tasks", "5 FreeRTOS tasks", "6 FreeRTOS tasks (+ webTask)"],
      ["Sensor packet", "12-byte OCS-3FL2.0 (live path)", "Same 12-byte parse in MODE_SENSOR"],
      ["Build", "Arduino IDE", "Arduino IDE + PlatformIO platformio.ini"],
    ],
    [2200, 3600, 3992]
  ),
  spacer(100),
  note(
    "Populate either J7 (EVB) or J14 (bare SIM800 + D1), not both. Fit D1 only when using the bare path."
  ),
);

// ========== 3 ARCH ==========
children.push(
  h1("3. System Architecture"),
  body(
    "The integration module is built around a standard ESP32 DevKit. Sensors, GSM, display, buzzer, " +
    "relay, demo pot, and WiFi AP all attach to it."
  ),
  h2("3.1 Power flow"),
  body("AC Mains (concentrator) → 12 V AC-DC module → LM2596 buck → 5 V rail"),
  body("12 V rail: OCS-3FL2.0 purity sensor VCC (Pin 1)."),
  body(
    "5 V rail: ESP32 DevKit VIN/5V, LCD 2004, active buzzer, 5 V relay module, SIM800L EVB VCC (J7), " +
    "and — via D1 1N4001 — bare SIM800 VCC (J14). Bulk electrolytics C1/C2/C3 on 5 V, 12 V, and GSM rails."
  ),
  body("3.3 V (ESP32 onboard LDO): I2C logic levels and UART logic toward ESP32 GPIO."),
  h2("3.2 Data paths"),
  table(
    ["Signal path", "Interface", "Direction", "Notes"],
    [
      ["OCS-3FL2.0 ↔ ESP32", "UART2 @ 9600 8N1", "Sensor → MCU (auto report)", "GPIO17 TX → sensor RXD; GPIO16 RX ← sensor TXD"],
      ["SIM800 ↔ ESP32", "UART1 @ 9600 8N1", "Bidirectional AT", "GPIO14 TX → modem RX; GPIO13 RX ← modem TX"],
      ["LCD 2004", "I2C SDA/SCL", "MCU → display", "Addr 0x27 (or 0x3F); GPIO21/22"],
      ["Relay module", "GPIO26", "MCU → load", "Active-LOW IN on typical modules"],
      ["Buzzer", "GPIO27", "MCU → output", "Active 5 V buzzer"],
      ["Potentiometer", "GPIO34 ADC", "Analog in", "MODE_POT only"],
      ["Captive portal", "WiFi soft-AP", "Phone ↔ ESP32", "SSID O2-Controller · 192.168.4.1"],
    ],
    [2400, 2200, 2400, 2792]
  ),
);

// ========== 4 POWER ==========
children.push(
  h1("4. Power Architecture"),
  h2("4.1 12 V rail"),
  body(
    "Supplies the OCS-3FL2.0 on Pin 1 VCC. The sensor accepts DC 5–12 V; 12 V is taken from the AC-DC " +
    "module without an extra conversion stage. There is no O2_VCC_SEL jumper in Hardware v2.0."
  ),
  h2("4.2 LM2596 5 V rail"),
  body(
    "Set the buck output to 5.0 V (no load) with a multimeter before installation. The rail powers " +
    "ESP32, LCD, buzzer, relay, and GSM supply. Continuous capacity should be ≥ 2 A to cover SIM TX bursts."
  ),
  h2("4.3 GSM supply (EVB vs bare)"),
  body(
    "EVB (J7): Connect 5 V directly to EVB VCC. Leave VDD and RST floating. The EVB regulates the " +
    "SIM800L chip internally."
  ),
  body(
    "Bare module (J14): Feed module VCC from 5 V through D1 (1N4001) so the module sees roughly " +
    "4.3–4.4 V under load, which is safer for bare SIM800 modules that expect ~3.4–4.4 V. Observe diode polarity."
  ),
  body(
    "Bulk capacitance: Place a large electrolytic (board uses 2200 µF 25 V class parts C1/C2/C3) close " +
    "to the GSM supply pins. The radio can draw peaks near 2 A during transmit."
  ),
  table(
    ["Rail", "Voltage", "Source", "Consumers"],
    [
      ["12 V DC", "12 V", "AC-DC module", "OCS-3FL2.0 VCC"],
      ["5 V DC", "5 V", "LM2596 from 12 V", "ESP32, LCD, buzzer, relay, GSM (EVB direct / bare via D1)"],
      ["~4.3 V (bare only)", "5 V − Vf(D1)", "D1 1N4001", "Bare SIM800 VCC on J14"],
      ["3.3 V DC", "3.3 V", "ESP32 LDO", "GPIO/UART/I2C logic reference"],
    ],
    [2000, 1800, 2400, 3592]
  ),
);

// ========== 5 COMPONENTS ==========
children.push(
  h1("5. Component Descriptions"),
  h2("5.1 ESP32 DevKit (standard)"),
  body(
    "Central MCU: UART0 USB debug, UART1 GSM, UART2 O2 sensor, I2C LCD, ADC pot, WiFi soft-AP. " +
    "Dual-core FreeRTOS firmware (Arduino-ESP32)."
  ),
  h2("5.2 LM2596 buck and 12 V AC-DC"),
  body(
    "LM2596 module steps 12 V to 5 V. AC-DC module converts concentrator mains to 12 V DC for the " +
    "sensor and buck input."
  ),
  h2("5.3 LCD 2004 + I2C backpack"),
  body("PCF8574 backpack, typically address 0x27. Four wires: VCC, GND, SDA, SCL."),
  h2("5.4 Active buzzer and 5 V relay module"),
  body(
    "Active buzzer on GPIO27. Single-channel 5 V relay module on GPIO26 for compressor cut-out; " +
    "module includes driver, opto, and flyback."
  ),
  h2("5.5 Bulk capacitors and D1"),
  body(
    "Three 2200 µF 25 V class electrolytics on the PCB (5 V, 12 V, GSM). D1 1N4001 is required only " +
    "for the bare SIM800 path (J14)."
  ),
  h2("5.6 Potentiometer (10 k)"),
  body(
    "Demo O2 source on GPIO34 when inputMode is MODE_POT. Remains available alongside WiFi and sensor " +
    "modes; it is not removed from the board when the real sensor is used."
  ),
);

// ========== 6 SENSOR ==========
children.push(
  h1("6. OCS-3FL2.0 Oxygen Purity Sensor"),
  body(
    "Industrial ultrasonic sensor for PSA concentrators: concentration, flow, and temperature over " +
    "UART every 500 ms in automatic reporting mode."
  ),
  table(
    ["Parameter", "Value"],
    [
      ["Concentration range", "21.0 % to 95.6 % O2"],
      ["Resolution / accuracy", "0.1 % / ±1.5 % FS (5–55 °C)"],
      ["Flow range", "0 to 20 L/min (0.1 resolution)"],
      ["UART", "9600 8N1, 3.3 V TTL, auto packet every 500 ms"],
      ["Power", "DC 5–12 V, ~50 mA (this design uses 12 V on Pin 1)"],
      ["Warm-up", "~10 s to specified accuracy"],
    ],
    [3200, 6592]
  ),
  spacer(80),
  h2("6.1 Connector pinout (sensor J2 / board O2_SENSOR)"),
  table(
    ["Pin", "Function", "Board / ESP32"],
    [
      ["1", "VCC", "12 V rail"],
      ["2", "RXD (into sensor)", "ESP32 GPIO17 UART2 TX"],
      ["3", "TXD (from sensor)", "ESP32 GPIO16 UART2 RX"],
      ["4", "GND", "System GND"],
    ],
    [1200, 3600, 4992]
  ),
  spacer(80),
  note(
    "Cross TX/RX intentionally: sensor TXD → ESP32 RX, sensor RXD → ESP32 TX. " +
    "config.h: O2_TX_PIN=17, O2_RX_PIN=16."
  ),
  h2("6.2 Placement"),
  body(
    "Install on the final oxygen outlet after filtration. Observe the flow arrow on the sensor body."
  ),
  h2("6.3 Input modes vs “demo pot only”"),
  body(
    "v1 documentation described removing the pot when the sensor arrived. In Rev 2.0 firmware the pot " +
    "stays installed. Operators select Potentiometer, WiFi, or Sensor from the captive portal. " +
    "MODE_SENSOR enables the UART parser; MODE_POT continues to map ADC 0–4095 → 21.0–95.6 % O2."
  ),
);

// ========== 7 UART PROTOCOL ==========
children.push(
  h1("7. UART Communication and Sensor Protocol"),
  body(
    "Both the OCS-3FL2.0 and SIM800 use 9600 8N1. This project uses automatic reporting mode for the " +
    "sensor (no query required)."
  ),
  h2("7.1 Packet structure (12 bytes — authoritative)"),
  body(
    "Earlier draft notes mentioned a 9-byte layout. The shipping firmware and Appendix A use the " +
    "12-byte OCS-3FL2.0 frame. Do not implement the 9-byte variant."
  ),
  table(
    ["Byte", "Field", "Notes"],
    [
      ["0", "Header", "0x16"],
      ["1", "Header", "0x09"],
      ["2", "Header / mode", "0x01 (firmware accepts this header)"],
      ["3–4", "O2", "(hi<<8|lo)/10.0 → %"],
      ["5–6", "Flow", "(hi<<8|lo)/10.0 → L/min"],
      ["7–8", "Temp", "(hi<<8|lo)/10.0 → °C"],
      ["9–11", "Reserved + CS", "Sum of all 12 bytes ≡ 0 (mod 256)"],
    ],
    [1200, 2400, 6192]
  ),
  spacer(80),
  body(
    "Sanity: discard packets with O2 < 15 % or O2 > 100 %. On checksum or range failure the firmware " +
    "logs and does not update sharedData."
  ),
);

// ========== 8 RELAY ==========
children.push(
  h1("8. Relay Driver Circuit"),
  body(
    "A commercial 5 V single-channel relay module switches compressor AC. Interface: VCC, GND, IN " +
    "(GPIO26). Typical modules are active-LOW (IN low energises coil). Firmware drives RELAY_PIN HIGH " +
    "at boot (compressor off for low-level-trigger modules as coded) and uses HIGH for danger cut-out / " +
    "LOW for compressor on in normal and warning states — match your module’s sense when wiring."
  ),
  table(
    ["Module pin", "Connect to", "Notes"],
    [
      ["VCC", "5 V rail", "Coil + logic supply"],
      ["GND", "System GND", "Common ground"],
      ["IN", "ESP32 GPIO26", "Control"],
      ["COM / NO", "Compressor AC path", "Mains-rated contacts only"],
    ],
    [2000, 2800, 4992]
  ),
  note(
    "DANGER: Mains wiring must follow local electrical safety rules. Isolate power before working on " +
    "relay contacts.",
    "danger"
  ),
);

// ========== 9 GSM ==========
children.push(
  h1("9. SIM800 GSM — EVB and Bare Module"),
  body(
    "TinyGSM is configured for SIM800 (TINY_GSM_MODEM_SIM800). Hardware v2.0 supports two mutually " +
    "exclusive physical interfaces."
  ),
  h2("9.1 Option A — SIM800L EVB (J7 SIM_GSM_EVB)"),
  table(
    ["EVB pin", "Connect to", "Notes"],
    [
      ["VCC (5 V)", "5 V rail", "Direct; no series diode"],
      ["GND", "System GND", ""],
      ["SIM_RXD", "ESP32 GPIO14 (UART1 TX)", "MCU → modem"],
      ["SIM_TXD", "ESP32 GPIO13 (UART1 RX)", "modem → MCU"],
      ["VDD", "NC", "Leave floating — EVB logic reference out"],
      ["RST", "NC", "Leave floating; reset via AT if needed"],
    ],
    [2400, 3600, 3792]
  ),
  spacer(80),
  h2("9.2 Option B — Bare SIM800 module (J14 SIM_GSM_BARE)"),
  table(
    ["Bare path", "Connect to", "Notes"],
    [
      ["Module VCC", "5 V → D1 (1N4001) → module", "Series drop; observe polarity"],
      ["Module GND", "System GND", ""],
      ["Module RX", "ESP32 GPIO14 TX", "Same UART1 as EVB path"],
      ["Module TX", "ESP32 GPIO13 RX", "Same UART1 as EVB path"],
    ],
    [2400, 3600, 3792]
  ),
  spacer(80),
  note("Use J7 or J14, never both. Populate D1 for bare modules only."),
  h2("9.3 Network"),
  body(
    "Quad-band GSM. Insert a provisioned Nigerian SIM. Firmware gsmTask restarts the modem, checks " +
    "SIM, and attempts network registration every 5 s while holding modemMutex."
  ),
);

// ========== 10 ALERTS ==========
children.push(
  h1("10. Alert Logic and Threshold Behaviour"),
  body(
    "Default thresholds (NVS-overridable): NORMAL when O2 > O2_NORMAL_MIN (85 %), WARNING when " +
    "O2_WARNING_MIN ≤ O2 ≤ O2_NORMAL_MIN band behaviour as coded (warning below 85 % and above 70 %), " +
    "DANGER when O2 < O2_WARNING_MIN (70 %)."
  ),
  table(
    ["State", "O2 condition", "Debounce", "Actions"],
    [
      ["NORMAL", "> 85 % (default)", "counters clear", "Compressor on, buzzer off, SMS/call flags reset"],
      ["WARNING", "between warn and normal", "warningCount ≥ 3", "Compressor on, intermittent beep, one SMS"],
      ["DANGER", "< 70 % (default)", "dangerCount ≥ 3", "Compressor off, solid beep, one voice call"],
    ],
    [1600, 2600, 2200, 3392]
  ),
  spacer(80),
  body(
    "alertTask (Core 1, priority 2, stack 8192) owns relay/buzzer and places SMS/call under modemMutex " +
    "with exponential backoff retries (sendWithRetry)."
  ),
);

// ========== 11 LCD ==========
children.push(
  h1("11. LCD 2004 Display"),
  body("Updated every DISPLAY_INTERVAL_MS (500 ms) by displayTask:"),
  table(
    ["Row", "Content"],
    [
      ["0", "O2 % and flow LPM"],
      ["1", "Temperature °C and uptime seconds"],
      ["2", "STATUS: NORMAL / WARNING / DANGER"],
      ["3", "GSM RSSI, SIM OK/NO, gsmState name"],
    ],
    [1200, 8592]
  ),
);

// ========== 12 CONFIG ==========
children.push(
  h1("12. Caregiver Number Configuration"),
  body(
    "Stored in NVS namespace \"config\" key caregiver_num. Also o2_warn_min and o2_norm_min. Loaded at " +
    "boot. Identical command syntax on USB serial (115200) and inbound SMS."
  ),
  table(
    ["Command", "Example", "Effect"],
    [
      ["SET WARN <value>", "SET WARN 72", "Warning threshold → NVS"],
      ["SET NORM <value>", "SET NORM 87", "Normal threshold → NVS"],
      ["SET NUM <e164>", "SET NUM +2348012345678", "Caregiver MSISDN → NVS"],
      ["STATUS", "STATUS", "O2 + thresholds"],
      ["HELP", "HELP", "List commands"],
    ],
    [2800, 3400, 3592]
  ),
  spacer(80),
  note(
    "inputMode is not an NVS key. Power-cycle always returns to potentiometer mode."
  ),
);

// ========== 13 PINS ==========
children.push(
  h1("13. ESP32 Pin Assignment"),
  body("Authoritative map matches firmware/config.h (do not use older docs that listed UART1 on GPIO1/3)."),
  table(
    ["GPIO", "Function", "Dir", "Connects to"],
    [
      ["21", "I2C SDA", "OD", "LCD backpack SDA"],
      ["22", "I2C SCL", "OD", "LCD backpack SCL"],
      ["14", "UART1 TX (SIM_TX_PIN)", "Out", "Modem RX (EVB SIM_RXD or bare RX)"],
      ["13", "UART1 RX (SIM_RX_PIN)", "In", "Modem TX (EVB SIM_TXD or bare TX)"],
      ["17", "UART2 TX (O2_TX_PIN)", "Out", "Sensor Pin 2 RXD"],
      ["16", "UART2 RX (O2_RX_PIN)", "In", "Sensor Pin 3 TXD"],
      ["26", "RELAY_PIN", "Out", "Relay module IN"],
      ["27", "BUZZER_PIN", "Out", "Buzzer +"],
      ["34", "POT_PIN ADC", "In", "Pot wiper (MODE_POT)"],
    ],
    [1400, 2800, 1200, 4392]
  ),
  spacer(80),
  note("Avoid GPIO 6–11 (flash). GPIO34 is input-only (ADC) — correct for pot wiper."),
);

// ========== 14 ENCLOSURE ==========
children.push(
  h1("14. Enclosure and Mechanical Integration"),
  body(
    "Mount the custom PCB, ESP32, buck, AC-DC, GSM antenna clearance, LCD window, and cable glands " +
    "in a medium insulating enclosure. Keep mains relay wiring segregated from low-voltage signal " +
    "harnesses. Provide USB access for programming or a panel USB extension."
  ),
);

// ========== 15 PICTORIAL ==========
children.push(
  h1("15. Pictorial Wiring Diagram"),
  body(
    "Phase-2 Cirkit Designer style diagram. Replace the placeholder with the Hardware v2.0 pictorial " +
    "(include J14 bare path and remove J11 if shown)."
  ),
  placeholder("Replace with updated Cirkit Designer / pictorial wiring image (Hardware v2.0)"),
  p("Figure 1: Pictorial wiring — placeholder for author-supplied v2.0 image.", {
    italics: true, size: 18, color: C.muted, align: AlignmentType.CENTER, spaceAfter: 120,
  }),
);

// ========== 16 WIRING TABLE ==========
children.push(
  h1("16. Wiring Connection Reference Table"),
  body("Wire-by-wire list aligned with config.h and Hardware v2.0 connectors."),
  table(
    ["From", "From pin", "To", "To pin", "Signal"],
    [
      ["AC source", "L / N", "AC-DC module", "AC in", "Mains"],
      ["AC-DC", "+12 V", "LM2596", "VIN+", "12 V"],
      ["AC-DC", "GND", "LM2596 / GND bus", "GND", "GND"],
      ["AC-DC", "+12 V", "O2 sensor", "Pin 1 VCC", "Sensor power"],
      ["LM2596", "5 V", "ESP32", "VIN/5V", "Logic power"],
      ["LM2596", "5 V", "LCD / buzzer / relay VCC", "VCC", "5 V loads"],
      ["LM2596", "5 V", "J7 EVB VCC", "VCC", "GSM EVB (option A)"],
      ["LM2596", "5 V", "D1 anode → cathode → J14 VCC", "Bare VCC", "GSM bare (option B)"],
      ["Bulk C*", "±", "Rails / GSM", "local", "2200 µF class"],
      ["ESP32", "GPIO26", "Relay", "IN", "Compressor control"],
      ["ESP32", "GPIO27", "Buzzer", "+", "Alert"],
      ["ESP32", "GPIO17 TX", "Sensor", "Pin 2 RXD", "UART2"],
      ["ESP32", "GPIO16 RX", "Sensor", "Pin 3 TXD", "UART2"],
      ["ESP32", "GPIO14 TX", "Modem RX", "SIM_RXD / bare RX", "UART1"],
      ["ESP32", "GPIO13 RX", "Modem TX", "SIM_TXD / bare TX", "UART1"],
      ["ESP32", "GPIO21/22", "LCD", "SDA/SCL", "I2C"],
      ["ESP32", "GPIO34", "Pot", "Wiper", "ADC demo"],
      ["ESP32", "3V3", "Pot high", "+", "Divider"],
      ["EVB only", "VDD, RST", "—", "NC", "Leave open"],
      ["Relay", "COM/NO", "Compressor AC", "switched live", "Mains"],
      ["All modules", "GND", "GND bus", "—", "Common reference"],
    ],
    [1600, 2000, 2200, 2200, 1792]
  ),
  spacer(80),
  note(
    "Common GND is mandatory. Floating grounds cause UART errors and modem brown-outs."
  ),
);

// ========== 17 SCHEMATIC ==========
children.push(
  h1("17. KiCad Schematic (Hardware v2.0)"),
  body(
    "Schematic sheet revision 2.0 in hardware/IoT_O2_Concentrator.kicad_sch. Connector summary:"
  ),
  table(
    ["Ref", "Label", "Type", "Purpose"],
    [
      ["J1", "5V_BUCK_OUT", "Screw 2", "5 V from LM2596"],
      ["J2", "BUZZER", "JST 2", "Active buzzer"],
      ["J3", "O2_SENSOR", "JST 4", "OCS-3FL2.0 VCC/RXD/TXD/GND"],
      ["J4", "GPIOs", "JST 4", "Spare GPIO breakout"],
      ["J5", "RELAY", "JST 3", "Relay VCC/GND/IN"],
      ["J6", "LCD_20x04", "JST 4", "I2C LCD"],
      ["J7", "SIM_GSM_EVB", "JST 6", "SIM800L EVB (option A)"],
      ["J8", "POTENTIOMETER", "JST 3", "Demo pot"],
      ["J9", "GPIO_SUPPLY", "JST 4", "5V/3V3/GND breakout"],
      ["J10", "12V_BUCK_IN", "Screw 2", "12 V into LM2596"],
      ["J12", "BOX_SWITCH", "JST 2", "Enclosure power switch"],
      ["J13", "DC_MODULE", "Screw 2", "Raw 12 V from AC-DC"],
      ["J14", "SIM_GSM_BARE", "JST", "Bare SIM800 (option B)"],
      ["D1", "1N4001", "Diode", "Series drop for J14 VCC"],
      ["C1–C3", "2200µ class", "Electrolytic", "Bulk on 5 V / 12 V / GSM"],
      ["—", "Net ties", "GND", "Replace former JP1–JP3 bridges"],
    ],
    [1000, 2200, 1600, 4992]
  ),
  spacer(100),
  placeholder("Replace with exported KiCad schematic image (sheet rev 2.0)"),
  p("Figure 2: KiCad schematic — placeholder for author-supplied Hardware v2.0 export.", {
    italics: true, size: 18, color: C.muted, align: AlignmentType.CENTER, spaceAfter: 80,
  }),
  note("J11 O2_VCC_SEL does not exist on Hardware v2.0. Do not reintroduce it on new boards."),
);

// ========== 18 PCB ==========
children.push(
  h1("18. PCB Layout (Hardware v2.0)"),
  body(
    "Single-layer board with ground pour, JST and screw terminals for student plug-and-play. Routing " +
    "cleanup after tag v1.0 replaced mandatory solder-bridge jumpers with GND net ties and added the " +
    "bare GSM path."
  ),
  placeholder("Replace with PCB 2D/3D render images (Hardware v2.0)"),
  p("Figure 3: PCB layout / 3D — placeholder for author-supplied Hardware v2.0 renders.", {
    italics: true, size: 18, color: C.muted, align: AlignmentType.CENTER, spaceAfter: 80,
  }),
  body(
    "Placement notes: centre ESP32 with USB accessible; keep bulk caps close to rails; group screw " +
    "terminals for power; keep GSM antenna clear of large copper pours where possible."
  ),
);

// ========== 19 FIRMWARE ==========
children.push(
  h1("19. Firmware Architecture"),
  body(
    "C++ / Arduino framework on ESP32 with FreeRTOS. Six concurrent tasks share SensorData and the " +
    "modem under mutexes. Sources: firmware/IoT_O2_Concentrator/ " +
    "(IoT_O2_Concentrator.ino, config.h, portal_html.h, platformio.ini)."
  ),
  h2("19.1 Task summary"),
  table(
    ["Task", "Core", "Prio", "Stack", "Loop", "Role"],
    [
      ["sensorTask", "0", "1", "4096", "50–500 ms", "switch(inputMode): pot / wifi uptime / UART parse"],
      ["displayTask", "0", "1", "4096", "500 ms", "LCD + RSSI/SIM snapshot"],
      ["webTask", "0", "1", "8192", "10 ms", "AP + DNS + AsyncWebServer"],
      ["gsmTask", "1", "1", "4096", "5 s", "Modem init + network registration"],
      ["configTask", "1", "1", "4096", "1 s", "USB + SMS commands → NVS"],
      ["alertTask", "1", "2", "8192", "1 s", "Thresholds, buzzer, relay, SMS/call"],
    ],
    [1600, 800, 800, 1000, 1200, 4392]
  ),
  spacer(80),
  h2("19.2 Mutexes"),
  body(
    "dataMutex protects SensorData (o2, flow, temp, uptime). modemMutex serialises all TinyGSM / AT " +
    "access across displayTask, gsmTask, alertTask, and configTask. Never hold modemMutex across long " +
    "blocking waits without need."
  ),
  h2("19.3 GSM state machine"),
  body(
    "volatile GsmState: IDLE, READY, NOSIM, CALLING, CALLED, SMS, SENT, RETRY, WAIT — shown on LCD row 3."
  ),
  h2("19.4 Alert debounce"),
  body(
    "dangerCount / warningCount require three consecutive qualifying samples before SMS or call. " +
    "smsSent / callMade prevent repeat actions until O2 returns to NORMAL."
  ),
  h2("19.5 NVS keys"),
  table(
    ["Key", "Type", "Default", "Updated by"],
    [
      ["o2_warn_min", "float", "70.0", "SET WARN"],
      ["o2_norm_min", "float", "85.0", "SET NORM"],
      ["caregiver_num", "string", "+234…", "SET NUM"],
    ],
    [2400, 1600, 2000, 3792]
  ),
);

// ========== 20 PORTAL ==========
children.push(
  h1("20. Captive Portal and Input Modes"),
  h2("20.1 Soft-AP parameters (config.h)"),
  table(
    ["Define", "Value"],
    [
      ["WIFI_AP_SSID", "O2-Controller"],
      ["WIFI_AP_PASS", "1234567890"],
      ["WIFI_AP_IP", "192.168.4.1"],
    ],
    [3200, 6592]
  ),
  spacer(80),
  h2("20.2 webTask behaviour"),
  body(
    "Starts WIFI_AP, softAPConfig to 192.168.4.1/24, DNSServer on port 53 with wildcard redirect to " +
    "the AP IP, AsyncWebServer on port 80. Loop: dnsServer.processNextRequest(); vTaskDelay(10 ms)."
  ),
  h2("20.3 HTTP routes"),
  table(
    ["Method / path", "Behaviour"],
    [
      ["GET /", "Mobile HTML UI from PORTAL_HTML (portal_html.h)"],
      ["GET /setMode?mode=pot|wifi|sensor", "Sets volatile inputMode; 200 OK"],
      ["GET /update?purity&flow&temp", "If MODE_WIFI, writes sharedData under dataMutex"],
      ["GET /status", "JSON {mode,o2,flow,temp} under dataMutex"],
      ["GET /generate_204", "Redirect → http://192.168.4.1 (Android)"],
      ["GET /hotspot-detect.html", "Redirect (iOS)"],
      ["GET /connecttest.txt", "Redirect (Windows)"],
      ["GET /redirect", "Redirect"],
    ],
    [4200, 5592]
  ),
  spacer(80),
  h2("20.4 sensorTask modes"),
  table(
    ["Mode", "Behaviour", "Period"],
    [
      ["MODE_POT (boot default)", "ADC pot → O2 21.0–95.6 %; flow=0; temp=25; write sharedData", "500 ms"],
      ["MODE_WIFI", "Only sharedData.uptime refreshed; sliders via /update", "500 ms"],
      ["MODE_SENSOR", "Non-blocking UART2 drain; 12-byte parse + checksum + sanity", "50 ms"],
    ],
    [2400, 5592, 1800]
  ),
  spacer(80),
  h2("20.5 Portal UI behaviour"),
  body(
    "Title O2 Controller. Radio: Potentiometer (default), WiFi, Sensor. When WiFi is selected, show " +
    "sliders: O2 21.0–95.6 step 0.1 default 85.0; Flow 0–20 step 0.1 default 5.0; Temp 5–55 step 0.1 " +
    "default 25.0 with live value labels. Mode changes fetch /setMode; slider input fetches /update. " +
    "On load, GET /status restores mode and values."
  ),
  h2("20.6 Libraries and build"),
  body(
    "LiquidCrystal_I2C, TinyGSM, ESPAsyncWebServer + AsyncTCP from ESP32Async GitHub only " +
    "(https://github.com/ESP32Async/ESPAsyncWebServer and https://github.com/ESP32Async/AsyncTCP — " +
    "do not use the old me-no-dev forks); ESP32 core WiFi, DNSServer, Preferences, FreeRTOS. " +
    "PlatformIO env esp32dev verified ~14 % RAM / ~63 % Flash (Arduino-ESP32 2.x)."
  ),
);

// ========== APPENDIX A ==========
children.push(
  h1("Appendix A — Sensor UART Packet Format"),
  body("12-byte automatic report. Validate header and checksum before use."),
  table(
    ["B0", "B1", "B2", "B3", "B4", "B5", "B6", "B7", "B8", "B9–11"],
    [
      ["0x16", "0x09", "0x01", "O2 hi", "O2 lo", "Fl hi", "Fl lo", "T hi", "T lo", "res/CS"],
    ],
    [900, 900, 900, 1000, 1000, 1000, 1000, 900, 900, 1292]
  ),
  spacer(80),
  body("O2 % = (b3<<8|b4)/10.0    Flow L/min = (b5<<8|b6)/10.0    Temp °C = (b7<<8|b8)/10.0"),
  body("Checksum: sum(bytes[0..11]) mod 256 == 0."),
);

// ========== APPENDIX B ==========
children.push(
  h1("Appendix B — Bill of Materials Summary"),
  table(
    ["S/N", "Item", "Qty", "Notes"],
    [
      ["1", "OCS-3FL2.0 oxygen sensor", "1", "UART purity/flow/temp"],
      ["2", "ESP32 DevKit (standard)", "1", "Main MCU + WiFi AP"],
      ["3", "Custom PCB single-layer Rev 2.0", "1", "With net ties, J7+J14"],
      ["4", "LCD 2004 + I2C backpack", "1", "0x27 / 0x3F"],
      ["5", "5 V relay module 1-ch", "1", "Compressor control"],
      ["6", "5 V active buzzer", "1", "Alerts"],
      ["7", "SIM800L GSM EVB and/or bare SIM800", "1", "Use one path"],
      ["8", "1N4001 diode", "1+", "Required for bare path D1"],
      ["9", "2200 µF 25 V electrolytic", "3", "C1/C2/C3 class bulk"],
      ["10", "LM2596 buck module", "1", "12→5 V"],
      ["11", "12 V AC-DC module", "1", "From concentrator AC"],
      ["12", "SIM card (active)", "1", "Local carrier"],
      ["13", "10 k potentiometer", "1", "MODE_POT demo"],
      ["14", "JST XH housings/cables", "set", "Match J2–J9, J12, J14"],
      ["15", "Enclosure (medium)", "1", "Insulating"],
      ["16", "BC337 / PC817 (optional spares)", "0–2", "Not required if using relay module"],
    ],
    [800, 4200, 1000, 3792]
  ),
);

// ========== APPENDIX C ==========
children.push(
  h1("Appendix C — config.h and portal_html.h Reference"),
  body("Pin, timing, and WiFi defines (edit before flash; runtime thresholds use NVS)."),
  body(
    "#define I2C_SDA_PIN 21 / I2C_SCL_PIN 22  |  SIM_TX_PIN 14 / SIM_RX_PIN 13  |  " +
    "O2_TX_PIN 17 / O2_RX_PIN 16  |  RELAY_PIN 26 / BUZZER_PIN 27 / POT_PIN 34  |  " +
    "LCD_I2C_ADDR 0x27  |  GSM_BAUD 9600 / O2_BAUD 9600"
  ),
  body(
    "SENSOR_INTERVAL_MS 500 · DISPLAY_INTERVAL_MS 500 · ALERT_INTERVAL_MS 1000 · " +
    "BUZZER_WARNING_MS 200 · BUZZER_WARNING_GAP 1000"
  ),
  body(
    "O2_MIN_PERCENT 21.0 · O2_MAX_PERCENT 95.6 · ADC_MAX_VALUE 4095.0"
  ),
  body(
    "WIFI_AP_SSID \"O2-Controller\" · WIFI_AP_PASS \"1234567890\" · WIFI_AP_IP \"192.168.4.1\""
  ),
  body(
    "portal_html.h: static const char PORTAL_HTML[] PROGMEM — full captive-portal page (modes + sliders + JS)."
  ),
  body(
    "Globals: enum InputMode { MODE_POT, MODE_WIFI, MODE_SENSOR }; volatile InputMode inputMode = MODE_POT; " +
    "AsyncWebServer server(80); DNSServer dnsServer;"
  ),
);

// ========== APPENDIX D ==========
children.push(
  h1("Appendix D — Revision History"),
  table(
    ["Rev", "Date", "Scope", "Changes"],
    [
      ["1.0", "Jun 2025", "Phase 1", "Architecture, power, sensor protocol, relay, alerts, pins"],
      ["1.1", "Jun 2025", "Phase 2", "Cirkit pictorial; interim hardware variants"],
      ["1.2", "Jun 2026", "Phase 2 upd", "LCD 2004, 5 V relay, standard ESP32, SIM800L EVB, pot demo"],
      ["1.3", "26 Jun 2026", "Phase 3–4", "KiCad + PCB figures, firmware tasks/NVS/SMS, 12-byte packet, BOM"],
      ["1.4", "10 Jul 2026", "Draft only", "Incomplete patch attempt (superseded — do not use)"],
      [
        "2.0",
        "10 Jul 2026",
        "HW v2 + FW",
        "Full rewrite for Hardware v2.0 (J14 bare SIM + D1, no J11, net ties) and captive-portal " +
        "firmware (6 tasks, input modes, portal_html.h). Pins/wiring/packet/BOM reconciled to source. " +
        "Figure placeholders for author image drop-in. Docs split docs/v1 vs docs/v2.",
      ],
    ],
    [900, 1400, 1600, 5892]
  ),
  spacer(160),
  p("Project by Smart Ayodele.  Documentation Rev 2.0 — July 2026.", {
    italics: true, size: 18, color: C.muted, align: AlignmentType.CENTER, spaceBefore: 200,
  }),
);

const doc = new Document({
  styles: {
    default: {
      document: {
        run: { font: "Arial", size: 20, color: C.ink },
      },
    },
    paragraphStyles: [
      {
        id: "Heading1",
        name: "Heading 1",
        basedOn: "Normal",
        next: "Normal",
        quickFormat: true,
        run: { size: 28, bold: true, font: "Arial", color: C.accent },
        paragraph: { spacing: { before: 320, after: 160 }, outlineLevel: 0 },
      },
      {
        id: "Heading2",
        name: "Heading 2",
        basedOn: "Normal",
        next: "Normal",
        quickFormat: true,
        run: { size: 24, bold: true, font: "Arial", color: C.ink },
        paragraph: { spacing: { before: 240, after: 100 }, outlineLevel: 1 },
      },
    ],
  },
  numbering: {
    config: [
      {
        reference: "bullets",
        levels: [
          {
            level: 0,
            format: LevelFormat.BULLET,
            text: "•",
            alignment: AlignmentType.LEFT,
            style: { paragraph: { indent: { left: 720, hanging: 360 } } },
          },
        ],
      },
    ],
  },
  sections: [
    {
      properties: {
        page: {
          size: { width: PAGE_W, height: PAGE_H },
          margin: { top: MARGIN, right: MARGIN, bottom: MARGIN, left: MARGIN },
        },
      },
      headers: {
        default: new Header({
          children: [
            new Paragraph({
              border: { bottom: { style: BorderStyle.SINGLE, size: 6, color: C.accent, space: 4 } },
              spacing: { after: 80 },
              children: [
                new TextRun({
                  text: "IoT Oxygen Concentrator Integration",
                  bold: true,
                  size: 16,
                  font: "Arial",
                  color: C.accent,
                }),
                new TextRun({
                  text: "   |   Rev 2.0   |   Hardware v2.0 + Firmware",
                  size: 16,
                  font: "Arial",
                  color: C.muted,
                }),
              ],
            }),
          ],
        }),
      },
      footers: {
        default: new Footer({
          children: [
            new Paragraph({
              border: { top: { style: BorderStyle.SINGLE, size: 6, color: C.line, space: 4 } },
              spacing: { before: 80 },
              children: [
                new TextRun({ text: "Page ", size: 16, font: "Arial", color: C.muted }),
                new TextRun({ children: [PageNumber.CURRENT], size: 16, font: "Arial", color: C.muted }),
                new TextRun({
                  text: "  |  IoT O2 Concentrator Integration  |  Rev 2.0",
                  size: 16,
                  font: "Arial",
                  color: C.muted,
                }),
              ],
            }),
          ],
        }),
      },
      children,
    },
  ],
});

Packer.toBuffer(doc).then((buffer) => {
  fs.writeFileSync(OUT, buffer);
  console.log("Wrote", OUT, buffer.length, "bytes");
});
