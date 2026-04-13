<template>
  <div class="control-wrapper">
    <VContainer fluid class="pa-4 pa-md-6">
      <!-- Header -->
      <VRow align="center" class="mb-6">
        <VCol>
          <div class="dash-eyebrow">DEVICE CONTROL</div>
          <h1 class="dash-title">Control Panel</h1>
        </VCol>
        <VCol cols="auto">
          <div class="mqtt-badge" :class="mqttStatus">
            <span class="status-dot"></span>
            {{ mqttLabel }}
          </div>
        </VCol>
      </VRow>

      <!-- Unit Conversion Section -->
      <VRow class="mb-4">
        <VCol cols="12">
          <div class="section-eyebrow">UNIT PREFERENCES</div>
          <p class="section-desc">
            Toggle units below. Changes are broadcast to the TFT display via
            MQTT and applied across the dashboard and home page live stats.
          </p>
        </VCol>
      </VRow>

      <VRow class="mb-6">
        <!-- Temperature -->
        <VCol cols="12" sm="6" md="4">
          <div class="unit-card" :class="{ active: units.tempF }">
            <div class="unit-card-icon-wrap" style="--uc: #ff6b6b">
              <VIcon icon="mdi-thermometer" size="28" color="#FF6B6B" />
            </div>
            <div class="unit-card-body">
              <div class="unit-card-label">Temperature</div>
              <div class="unit-toggle-row">
                <span class="unit-opt" :class="{ selected: !units.tempF }"
                  >°C</span
                >
                <button
                  class="toggle-pill"
                  :class="{ on: units.tempF }"
                  @click="setUnit('tempF', !units.tempF)"
                >
                  <span class="toggle-thumb"></span>
                </button>
                <span class="unit-opt" :class="{ selected: units.tempF }"
                  >°F</span
                >
              </div>
              <div class="unit-example">
                Currently: <strong>{{ exampleTemp }}</strong>
              </div>
            </div>
          </div>
        </VCol>

        <!-- Pressure -->
        <VCol cols="12" sm="6" md="4">
          <div class="unit-card" :class="{ active: units.pressBar }">
            <div class="unit-card-icon-wrap" style="--uc: #45b7d1">
              <VIcon icon="mdi-gauge" size="28" color="#45B7D1" />
            </div>
            <div class="unit-card-body">
              <div class="unit-card-label">Pressure</div>
              <div class="unit-toggle-row">
                <span class="unit-opt" :class="{ selected: !units.pressBar }"
                  >hPa</span
                >
                <button
                  class="toggle-pill"
                  :class="{ on: units.pressBar }"
                  @click="setUnit('pressBar', !units.pressBar)"
                >
                  <span class="toggle-thumb"></span>
                </button>
                <span class="unit-opt" :class="{ selected: units.pressBar }"
                  >bar</span
                >
              </div>
              <div class="unit-example">
                Currently: <strong>{{ examplePress }}</strong>
              </div>
            </div>
          </div>
        </VCol>

        <!-- Altitude -->
        <VCol cols="12" sm="6" md="4">
          <div class="unit-card" :class="{ active: units.altFt }">
            <div class="unit-card-icon-wrap" style="--uc: #26de81">
              <VIcon icon="mdi-image-filter-hdr" size="28" color="#26de81" />
            </div>
            <div class="unit-card-body">
              <div class="unit-card-label">Altitude</div>
              <div class="unit-toggle-row">
                <span class="unit-opt" :class="{ selected: !units.altFt }"
                  >m</span
                >
                <button
                  class="toggle-pill"
                  :class="{ on: units.altFt }"
                  @click="setUnit('altFt', !units.altFt)"
                >
                  <span class="toggle-thumb"></span>
                </button>
                <span class="unit-opt" :class="{ selected: units.altFt }"
                  >ft</span
                >
              </div>
              <div class="unit-example">
                Currently: <strong>{{ exampleAlt }}</strong>
              </div>
            </div>
          </div>
        </VCol>
      </VRow>

      <!-- Push to TFT button -->
      <VRow class="mb-8">
        <VCol cols="12" sm="auto">
          <button
            class="btn-push"
            :class="{ loading: pushing }"
            @click="pushToTFT"
            :disabled="pushing"
          >
            <span v-if="!pushing">
              <VIcon icon="mdi-broadcast" size="16" class="mr-2" />
              Push to TFT Display
            </span>
            <span v-else class="spinner"></span>
          </button>
        </VCol>
        <VCol cols="12" sm="auto" v-if="pushMsg">
          <div class="push-feedback" :class="pushMsgType">
            <VIcon
              :icon="
                pushMsgType === 'ok' ? 'mdi-check-circle' : 'mdi-alert-circle'
              "
              size="16"
              class="mr-1"
            />
            {{ pushMsg }}
          </div>
        </VCol>
      </VRow>

      <!-- Live Preview -->
      <VRow class="mb-4">
        <VCol cols="12">
          <div class="section-eyebrow">LIVE PREVIEW — CURRENT READING</div>
        </VCol>
      </VRow>

      <VRow class="mb-6">
        <VCol
          v-for="card in previewCards"
          :key="card.label"
          cols="6"
          sm="4"
          md="2"
        >
          <div class="preview-card" :style="{ '--pc': card.color }">
            <VIcon
              :icon="card.icon"
              size="18"
              :color="card.color"
              class="mb-2"
              style="display: block"
            />
            <div class="preview-value">
              {{ card.value ?? "—"
              }}<span class="preview-unit">{{ card.unit }}</span>
            </div>
            <div class="preview-label">{{ card.label }}</div>
          </div>
        </VCol>
      </VRow>

      <!-- TFT Display Mockup -->
      <VRow class="mb-4">
        <VCol cols="12">
          <div class="section-eyebrow">TFT DISPLAY PREVIEW</div>
        </VCol>
        <VCol cols="12" md="6" lg="5">
          <div class="tft-mockup">
            <div class="tft-screen">
              <div class="tft-header">ESP32 WEATHER STATION</div>
              <div class="tft-divider-h"></div>
              <div class="tft-body">
                <div class="tft-col">
                  <div class="tft-section-lbl">-- BMP280 --</div>
                  <div class="tft-row">
                    <span class="tft-lbl">Air Temp :</span>
                    <span class="tft-val">{{ tftTemp }}</span>
                  </div>
                  <div class="tft-row">
                    <span class="tft-lbl">Pressure :</span>
                    <span class="tft-val">{{ tftPress }}</span>
                  </div>
                  <div class="tft-row">
                    <span class="tft-lbl">Altitude :</span>
                    <span class="tft-val">{{ tftAlt }}</span>
                  </div>
                </div>
                <div class="tft-vdivider"></div>
                <div class="tft-col">
                  <div class="tft-section-lbl">-- AM2302 --</div>
                  <div class="tft-row">
                    <span class="tft-lbl">Humidity :</span>
                    <span class="tft-val tft-cyan">{{ tftHum }}</span>
                  </div>
                  <div class="tft-row">
                    <span class="tft-lbl">DHT Temp :</span>
                    <span class="tft-val">{{ tftDhtTemp }}</span>
                  </div>
                  <div class="tft-row">
                    <span class="tft-lbl">Heat Index :</span>
                    <span class="tft-val" :class="tftHiClass">{{
                      tftHeatIndex
                    }}</span>
                  </div>
                  <div class="tft-section-lbl mt-2">-- SOIL --</div>
                  <div class="tft-row">
                    <span class="tft-lbl">Moisture :</span>
                    <span class="tft-val" :class="tftSoilClass">{{
                      tftSoil
                    }}</span>
                  </div>
                  <div class="tft-progress-bg">
                    <div
                      class="tft-progress-fill"
                      :style="{ width: soilBarWidth, background: soilBarColor }"
                    ></div>
                  </div>
                </div>
              </div>
            </div>
          </div>
        </VCol>
      </VRow>
    </VContainer>
  </div>
</template>

<script setup>
import { ref, computed, onMounted, onBeforeUnmount } from "vue";
import { useAppStore } from "@/store/appStore";
import { useUnitStore } from "@/store/unitStore";
import { storeToRefs } from "pinia";

const store = useAppStore();
const unitStore = useUnitStore();
const { latest } = storeToRefs(store);
const { units } = storeToRefs(unitStore);

const pushing = ref(false);
const pushMsg = ref("");
const pushMsgType = ref("ok");
const mqttStatus = ref("unknown");
const mqttLabel = ref("MQTT Unknown");

let refreshTimer = null;

onMounted(() => {
  store.fetchLatest();
  refreshTimer = setInterval(() => store.fetchLatest(), 5000);
});
onBeforeUnmount(() => clearInterval(refreshTimer));

// ── Conversion helpers ────────────────────────────────────────
function toF(c) {
  return c != null ? +((c * 9) / 5 + 32).toFixed(1) : null;
}
function toBar(h) {
  return h != null ? +(h / 1000).toFixed(4) : null;
}
function toFt(m) {
  return m != null ? +(m * 3.28084).toFixed(1) : null;
}

function dispTemp(v) {
  const val = units.value.tempF ? toF(v) : v?.toFixed(1);
  return val != null ? `${val}${units.value.tempF ? " °F" : " °C"}` : "—";
}
function dispPress(v) {
  const val = units.value.pressBar ? toBar(v) : v?.toFixed(1);
  return val != null ? `${val}${units.value.pressBar ? " bar" : " hPa"}` : "—";
}
function dispAlt(v) {
  const val = units.value.altFt ? toFt(v) : v?.toFixed(1);
  return val != null ? `${val}${units.value.altFt ? " ft" : " m"}` : "—";
}

const exampleTemp = computed(() => dispTemp(latest.value?.temperature_c));
const examplePress = computed(() => dispPress(latest.value?.pressure_hpa));
const exampleAlt = computed(() => dispAlt(latest.value?.altitude_m));

const previewCards = computed(() => {
  const l = latest.value;
  const tempVal = units.value.tempF
    ? toF(l?.dht_temp_c)
    : l?.dht_temp_c?.toFixed(1);
  const tempUnit = units.value.tempF ? "°F" : "°C";
  const hiVal = units.value.tempF
    ? toF(l?.heat_index_c)
    : l?.heat_index_c?.toFixed(1);
  const pressVal = units.value.pressBar
    ? toBar(l?.pressure_hpa)
    : l?.pressure_hpa?.toFixed(1);
  const pressUnit = units.value.pressBar ? "bar" : "hPa";
  const altVal = units.value.altFt
    ? toFt(l?.altitude_m)
    : l?.altitude_m?.toFixed(1);
  const altUnit = units.value.altFt ? "ft" : "m";

  return [
    {
      label: "DHT Temp",
      value: tempVal,
      unit: tempUnit,
      icon: "mdi-thermometer",
      color: "#FF6B6B",
    },
    {
      label: "Humidity",
      value: l?.humidity_pct?.toFixed(1),
      unit: "%",
      icon: "mdi-water-percent",
      color: "#4ECDC4",
    },
    {
      label: "Pressure",
      value: pressVal,
      unit: pressUnit,
      icon: "mdi-gauge",
      color: "#45B7D1",
    },
    {
      label: "Altitude",
      value: altVal,
      unit: altUnit,
      icon: "mdi-image-filter-hdr",
      color: "#26de81",
    },
    {
      label: "Heat Index",
      value: hiVal,
      unit: tempUnit,
      icon: "mdi-fire",
      color: "#F7B731",
    },
    {
      label: "Soil",
      value: l?.soil_moisture_pct,
      unit: "%",
      icon: "mdi-sprout",
      color: "#a29bfe",
    },
  ];
});

// ── TFT display preview ────────────────────────────────────────
const tftTemp = computed(() => dispTemp(latest.value?.temperature_c));
const tftPress = computed(() => dispPress(latest.value?.pressure_hpa));
const tftAlt = computed(() => dispAlt(latest.value?.altitude_m));
const tftHum = computed(() =>
  latest.value?.humidity_pct != null
    ? `${latest.value.humidity_pct.toFixed(1)} %`
    : "—",
);
const tftDhtTemp = computed(() => dispTemp(latest.value?.dht_temp_c));
const tftHeatIndex = computed(() => dispTemp(latest.value?.heat_index_c));
const tftSoil = computed(() =>
  latest.value?.soil_moisture_pct != null
    ? `${latest.value.soil_moisture_pct} %`
    : "—",
);

const tftHiClass = computed(() => {
  const hi = latest.value?.heat_index_c;
  if (hi == null) return "";
  if (hi >= 32) return "tft-red";
  if (hi >= 27) return "tft-orange";
  return "tft-yellow";
});
const tftSoilClass = computed(() => {
  const s = latest.value?.soil_moisture_pct;
  if (s == null) return "";
  if (s < 20) return "tft-red";
  if (s < 40) return "tft-orange";
  if (s < 70) return "tft-green";
  return "tft-cyan";
});
const soilBarWidth = computed(() => {
  const s = latest.value?.soil_moisture_pct ?? 0;
  return `${Math.min(s, 100)}%`;
});
const soilBarColor = computed(() => {
  const s = latest.value?.soil_moisture_pct ?? 0;
  if (s < 20) return "#ff4444";
  if (s < 40) return "#ff8800";
  if (s < 70) return "#00cc44";
  return "#4499ff";
});

// ── Set unit and auto-push ────────────────────────────────────
function setUnit(key, val) {
  unitStore.setUnit(key, val);
}

// ── Push MQTT command to ESP32 ────────────────────────────────
async function pushToTFT() {
  pushing.value = true;
  pushMsg.value = "";

  const payload = {
    cmd: "set_units",
    temp_f: units.value.tempF ? 1 : 0,
    press_bar: units.value.pressBar ? 1 : 0,
    alt_ft: units.value.altFt ? 1 : 0,
  };

  try {
    const res = await fetch("/api/control/units", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload),
    });
    if (res.ok) {
      pushMsg.value = "Sent to TFT display successfully.";
      pushMsgType.value = "ok";
      mqttStatus.value = "connected";
      mqttLabel.value = "MQTT Connected";
    } else {
      throw new Error("Server error");
    }
  } catch (e) {
    pushMsg.value = "Failed to reach backend.";
    pushMsgType.value = "err";
    mqttStatus.value = "error";
    mqttLabel.value = "MQTT Error";
  } finally {
    pushing.value = false;
    setTimeout(() => {
      pushMsg.value = "";
    }, 4000);
  }
}
</script>

<style scoped>
@import url("https://fonts.googleapis.com/css2?family=Syne:wght@400;700;800&family=DM+Sans:wght@300;400;500;600&family=JetBrains+Mono:wght@400;600&display=swap");

.control-wrapper {
  min-height: 100vh;
  background: #060d1f;
  font-family: "DM Sans", sans-serif;
}

/* ── Header ── */
.dash-eyebrow {
  font-size: 11px;
  letter-spacing: 3px;
  color: rgba(200, 215, 240, 0.35);
  text-transform: uppercase;
  margin-bottom: 4px;
  font-weight: 500;
}
.dash-title {
  font-family: "Syne", sans-serif;
  font-size: 28px;
  font-weight: 800;
  color: #f0f4ff;
  letter-spacing: -1px;
}

/* ── MQTT badge ── */
.mqtt-badge {
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 12px;
  font-weight: 500;
  border-radius: 20px;
  padding: 6px 14px;
  border: 1px solid;
}
.mqtt-badge.unknown {
  color: rgba(200, 215, 240, 0.4);
  border-color: rgba(255, 255, 255, 0.08);
  background: rgba(255, 255, 255, 0.03);
}
.mqtt-badge.connected {
  color: #26de81;
  border-color: rgba(38, 222, 129, 0.3);
  background: rgba(38, 222, 129, 0.08);
}
.mqtt-badge.error {
  color: #ff6b6b;
  border-color: rgba(255, 107, 107, 0.3);
  background: rgba(255, 107, 107, 0.08);
}
.status-dot {
  width: 7px;
  height: 7px;
  border-radius: 50%;
  background: currentColor;
  animation: pulse 1.5s infinite;
}
@keyframes pulse {
  0%,
  100% {
    opacity: 1;
    transform: scale(1);
  }
  50% {
    opacity: 0.4;
    transform: scale(0.8);
  }
}

/* ── Section heading ── */
.section-eyebrow {
  font-size: 10px;
  letter-spacing: 3px;
  color: rgba(200, 215, 240, 0.3);
  text-transform: uppercase;
  margin-bottom: 8px;
  font-weight: 500;
}
.section-desc {
  font-size: 14px;
  color: rgba(200, 215, 240, 0.5);
  line-height: 1.7;
  max-width: 680px;
  margin-bottom: 4px;
}

/* ── Unit Cards ── */
.unit-card {
  background: rgba(255, 255, 255, 0.03);
  border: 1px solid rgba(255, 255, 255, 0.07);
  border-radius: 16px;
  padding: 20px;
  display: flex;
  gap: 16px;
  align-items: flex-start;
  transition: all 0.3s;
  height: 100%;
}
.unit-card.active {
  background: rgba(255, 255, 255, 0.055);
  border-color: rgba(255, 255, 255, 0.14);
}
.unit-card-icon-wrap {
  width: 48px;
  height: 48px;
  border-radius: 12px;
  background: color-mix(in srgb, var(--uc) 15%, transparent);
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
}
.unit-card-body {
  flex: 1;
}
.unit-card-label {
  font-family: "Syne", sans-serif;
  font-size: 15px;
  font-weight: 700;
  color: #f0f4ff;
  margin-bottom: 12px;
}
.unit-toggle-row {
  display: flex;
  align-items: center;
  gap: 12px;
  margin-bottom: 10px;
}
.unit-opt {
  font-family: "JetBrains Mono", monospace;
  font-size: 13px;
  font-weight: 600;
  color: rgba(200, 215, 240, 0.3);
  transition: color 0.2s;
  min-width: 28px;
}
.unit-opt.selected {
  color: #f0f4ff;
}

/* Toggle pill */
.toggle-pill {
  width: 48px;
  height: 26px;
  border-radius: 13px;
  background: rgba(255, 255, 255, 0.1);
  border: 1px solid rgba(255, 255, 255, 0.15);
  position: relative;
  cursor: pointer;
  transition:
    background 0.25s,
    border-color 0.25s;
  flex-shrink: 0;
}
.toggle-pill.on {
  background: rgba(78, 205, 196, 0.35);
  border-color: rgba(78, 205, 196, 0.6);
}
.toggle-thumb {
  position: absolute;
  top: 3px;
  left: 3px;
  width: 18px;
  height: 18px;
  border-radius: 50%;
  background: rgba(200, 215, 240, 0.5);
  transition:
    transform 0.25s,
    background 0.25s;
}
.toggle-pill.on .toggle-thumb {
  transform: translateX(22px);
  background: #4ecdc4;
}
.unit-example {
  font-size: 12px;
  color: rgba(200, 215, 240, 0.4);
  font-family: "JetBrains Mono", monospace;
}
.unit-example strong {
  color: rgba(200, 215, 240, 0.75);
}

/* ── Push button ── */
.btn-push {
  background: linear-gradient(135deg, #4ecdc4, #45b7d1);
  color: #060d1f;
  font-family: "DM Sans", sans-serif;
  font-weight: 600;
  font-size: 14px;
  padding: 12px 28px;
  border-radius: 50px;
  border: none;
  cursor: pointer;
  display: inline-flex;
  align-items: center;
  transition: all 0.3s;
  box-shadow: 0 0 20px rgba(78, 205, 196, 0.25);
}
.btn-push:hover:not(:disabled) {
  transform: translateY(-2px);
  box-shadow: 0 0 35px rgba(78, 205, 196, 0.45);
}
.btn-push:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}
.spinner {
  width: 18px;
  height: 18px;
  border: 2px solid rgba(6, 13, 31, 0.3);
  border-top-color: #060d1f;
  border-radius: 50%;
  animation: spin 0.7s linear infinite;
  display: inline-block;
}
@keyframes spin {
  to {
    transform: rotate(360deg);
  }
}

.push-feedback {
  display: inline-flex;
  align-items: center;
  font-size: 13px;
  font-weight: 500;
  padding: 10px 18px;
  border-radius: 50px;
  border: 1px solid;
}
.push-feedback.ok {
  color: #26de81;
  border-color: rgba(38, 222, 129, 0.3);
  background: rgba(38, 222, 129, 0.08);
}
.push-feedback.err {
  color: #ff6b6b;
  border-color: rgba(255, 107, 107, 0.3);
  background: rgba(255, 107, 107, 0.08);
}

/* ── Preview cards ── */
.preview-card {
  background: rgba(255, 255, 255, 0.03);
  border: 1px solid rgba(255, 255, 255, 0.07);
  border-top: 2px solid var(--pc);
  border-radius: 12px;
  padding: 14px 16px;
  transition: all 0.25s;
}
.preview-card:hover {
  background: rgba(255, 255, 255, 0.055);
  transform: translateY(-2px);
}
.preview-value {
  font-family: "Syne", sans-serif;
  font-size: 20px;
  font-weight: 700;
  color: #f0f4ff;
  line-height: 1;
}
.preview-unit {
  font-size: 11px;
  font-weight: 400;
  color: rgba(200, 215, 240, 0.5);
  margin-left: 2px;
}
.preview-label {
  font-size: 10px;
  letter-spacing: 1px;
  color: rgba(200, 215, 240, 0.4);
  text-transform: uppercase;
  margin-top: 5px;
}

/* ── TFT mockup ── */
.tft-mockup {
  background: #111;
  border-radius: 12px;
  padding: 12px;
  border: 2px solid rgba(255, 255, 255, 0.1);
  box-shadow:
    0 0 40px rgba(0, 0, 0, 0.6),
    inset 0 0 8px rgba(0, 0, 0, 0.4);
  display: inline-block;
  width: 100%;
  max-width: 480px;
}
.tft-screen {
  background: #000;
  border-radius: 6px;
  overflow: hidden;
  font-family: "JetBrains Mono", monospace;
  font-size: 11px;
}
.tft-header {
  background: #1a5f8a;
  color: #00ffff;
  padding: 8px 10px;
  font-size: 12px;
  font-weight: 600;
  text-align: center;
  letter-spacing: 1px;
}
.tft-divider-h {
  height: 1px;
  background: rgba(255, 255, 255, 0.1);
}
.tft-body {
  display: flex;
  padding: 10px 6px;
  gap: 0;
  min-height: 160px;
}
.tft-col {
  flex: 1;
  padding: 0 6px;
}
.tft-vdivider {
  width: 1px;
  background: rgba(255, 255, 255, 0.1);
  margin: 4px 0;
}
.tft-section-lbl {
  color: #00ffff;
  font-size: 10px;
  margin-bottom: 6px;
}
.tft-row {
  display: flex;
  flex-direction: column;
  margin-bottom: 8px;
}
.tft-lbl {
  color: #ccc;
  font-size: 9px;
  margin-bottom: 1px;
}
.tft-val {
  color: #ffe000;
  font-size: 13px;
  font-weight: 600;
}
.tft-cyan {
  color: #00ffff !important;
}
.tft-yellow {
  color: #ffe000 !important;
}
.tft-orange {
  color: #ff8800 !important;
}
.tft-red {
  color: #ff4444 !important;
}
.tft-green {
  color: #00cc44 !important;
}
.mt-2 {
  margin-top: 8px;
}
.tft-progress-bg {
  height: 6px;
  background: rgba(255, 255, 255, 0.1);
  border-radius: 3px;
  margin-top: 4px;
  overflow: hidden;
}
.tft-progress-fill {
  height: 100%;
  border-radius: 3px;
  transition: width 0.4s ease;
}
</style>
