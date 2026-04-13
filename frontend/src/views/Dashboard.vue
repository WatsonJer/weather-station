<template>
  <div class="dash-wrapper">
    <VContainer fluid class="pa-4 pa-md-6">
      <VRow align="center" class="mb-4">
        <VCol>
          <div class="dash-eyebrow">LIVE MONITORING</div>
          <h1 class="dash-title">Dashboard</h1>
        </VCol>
        <VCol cols="auto" class="d-flex align-center gap-3">
          <div class="refresh-badge">
            <span class="pulse-dot"></span>
            Auto-refresh {{ countdown }}s
          </div>
          <VBtn
            size="small"
            variant="tonal"
            color="primary"
            @click="fetchAll"
            :loading="loading"
          >
            <VIcon icon="mdi-refresh" size="16" class="mr-1" />Refresh
          </VBtn>
        </VCol>
      </VRow>

      <VRow class="mb-4">
        <VCol v-for="kpi in kpiCards" :key="kpi.label" cols="6" sm="4" md="2">
          <div class="kpi-card" :style="{ '--accent': kpi.color }">
            <VIcon
              :icon="kpi.icon"
              size="20"
              :color="kpi.color"
              class="kpi-icon"
            />
            <div class="kpi-value">
              {{ kpi.value ?? "—" }}<span class="kpi-unit">{{ kpi.unit }}</span>
            </div>
            <div class="kpi-label">{{ kpi.label }}</div>
          </div>
        </VCol>
      </VRow>

      <VRow class="mb-4">
        <VCol cols="12" md="7">
          <div class="chart-card">
            <figure class="highcharts-figure"><div id="hc-temp"></div></figure>
          </div>
        </VCol>
        <VCol cols="12" md="5">
          <div class="chart-card">
            <figure class="highcharts-figure"><div id="hc-hum"></div></figure>
          </div>
        </VCol>
      </VRow>

      <VRow class="mb-4">
        <VCol cols="12" md="7">
          <div class="chart-card">
            <figure class="highcharts-figure"><div id="hc-press"></div></figure>
          </div>
        </VCol>
        <VCol cols="12" md="5">
          <div class="chart-card fluid-card">
            <div class="chart-header">
              <span class="chart-title">Soil Moisture</span>
              <span
                class="soil-status-badge"
                :style="{
                  background: soilBadgeColor + '22',
                  color: soilBadgeColor,
                  border: '1px solid ' + soilBadgeColor + '55',
                }"
              >
                {{ soilStatus }}
              </span>
            </div>
            <div class="fluid-meter-wrap">
              <div
                ref="fluidMeterContainer"
                class="fluid-meter-container"
              ></div>
            </div>
          </div>
        </VCol>
      </VRow>

      <VRow>
        <VCol cols="12">
          <div class="chart-card">
            <div class="chart-header">
              <span class="chart-title">Console Log — Recent Readings</span>
              <span class="log-count">{{ history.length }} records</span>
            </div>
            <div class="console-wrap">
              <table class="console-table">
                <thead>
                  <tr>
                    <th>Timestamp</th>
                    <th>Temp ({{ unitStore.tempUnit() }})</th>
                    <th>Pressure ({{ unitStore.pressUnit() }})</th>
                    <th>Altitude ({{ unitStore.altUnit() }})</th>
                    <th>DHT Temp ({{ unitStore.tempUnit() }})</th>
                    <th>Humidity (%)</th>
                    <th>Heat Index ({{ unitStore.tempUnit() }})</th>
                    <th>Soil (%)</th>
                  </tr>
                </thead>
                <tbody>
                  <tr
                    v-for="(row, i) in history"
                    :key="i"
                    :class="{ 'row-new': i === 0 }"
                  >
                    <td class="ts-cell">{{ formatTs(row.timestamp) }}</td>
                    <td>{{ dispTemp(row.temperature_c) }}</td>
                    <td>{{ dispPress(row.pressure_hpa) }}</td>
                    <td>{{ dispAlt(row.altitude_m) }}</td>
                    <td>{{ dispTemp(row.dht_temp_c) }}</td>
                    <td>{{ row.humidity_pct?.toFixed(1) }}</td>
                    <td :class="heatIndexClass(row.heat_index_c)">
                      {{ dispTemp(row.heat_index_c) }}
                    </td>
                    <td :class="soilClass(row.soil_moisture_pct)">
                      {{ row.soil_moisture_pct }}
                    </td>
                  </tr>
                </tbody>
              </table>
            </div>
          </div>
        </VCol>
      </VRow>
    </VContainer>
  </div>
</template>

<script setup>
import {
  ref,
  computed,
  onMounted,
  onBeforeUnmount,
  watch,
  nextTick,
} from "vue";
import { useAppStore } from "@/store/appStore";
import { useUnitStore } from "@/store/unitStore";
import { storeToRefs } from "pinia";
import Highcharts from "highcharts";
import more from "highcharts/highcharts-more";
import Exporting from "highcharts/modules/exporting";
Exporting(Highcharts);
more(Highcharts);

const store = useAppStore();
const unitStore = useUnitStore();
const { latest, history } = storeToRefs(store);
const { units } = storeToRefs(unitStore);

const loading = ref(false);
const countdown = ref(5);
const fluidMeterContainer = ref(null);

let tempChart = null,
  humChart = null,
  pressChart = null;
let fluidMeter = null,
  refreshTimer = null,
  countdownTimer = null;

function dispTemp(v) {
  return v != null ? unitStore.convertTemp(v).toFixed(1) : "—";
}
function dispPress(v) {
  return v != null
    ? unitStore.convertPress(v).toFixed(units.value.pressBar ? 4 : 1)
    : "—";
}
function dispAlt(v) {
  return v != null ? unitStore.convertAlt(v).toFixed(1) : "—";
}

const HC_THEME = {
  chart: {
    backgroundColor: "transparent",
    style: { fontFamily: "'DM Sans', sans-serif" },
    animation: { duration: 300 },
  },
  title: {
    style: {
      color: "#f0f4ff",
      fontFamily: "'Syne', sans-serif",
      fontSize: "15px",
      fontWeight: "700",
    },
    align: "left",
  },
  xAxis: {
    type: "datetime",
    labels: { style: { color: "rgba(200,215,240,0.65)", fontSize: "11px" } },
    gridLineColor: "rgba(255,255,255,0.05)",
    lineColor: "rgba(255,255,255,0.1)",
    tickColor: "rgba(255,255,255,0.1)",
  },
  yAxis: {
    labels: { style: { color: "rgba(200,215,240,0.65)", fontSize: "11px" } },
    gridLineColor: "rgba(255,255,255,0.05)",
    title: { style: { color: "rgba(200,215,240,0.7)", fontSize: "12px" } },
  },
  tooltip: {
    backgroundColor: "rgba(6,13,31,0.97)",
    borderColor: "rgba(255,255,255,0.15)",
    borderWidth: 1,
    style: { color: "#f0f4ff", fontSize: "13px" },
    shared: true,
  },
  legend: {
    itemStyle: {
      color: "rgba(200,215,240,0.8)",
      fontSize: "12px",
      fontWeight: "500",
    },
    itemHoverStyle: { color: "#f0f4ff" },
  },
  credits: { enabled: false },
  exporting: { enabled: false },
};

const soilPct = computed(() => latest.value?.soil_moisture_pct ?? 0);
const soilBucket = computed(() => {
  const p = soilPct.value;
  if (p < 20) return "very-dry";
  if (p < 40) return "dry";
  if (p < 70) return "optimal";
  return "wet";
});
const soilBadgeColor = computed(
  () =>
    ({
      "very-dry": "#FF6B6B",
      dry: "#F7B731",
      optimal: "#26de81",
      wet: "#45B7D1",
    })[soilBucket.value],
);
const soilStatus = computed(
  () =>
    ({ "very-dry": "Very Dry", dry: "Dry", optimal: "Optimal", wet: "Wet" })[
      soilBucket.value
    ],
);

const kpiCards = computed(() => [
  {
    label: "DHT Temp",
    value: dispTemp(latest.value?.dht_temp_c),
    unit: unitStore.tempUnit(),
    icon: "mdi-thermometer",
    color: "#FF6B6B",
  },
  {
    label: "Humidity",
    value: latest.value?.humidity_pct?.toFixed(1),
    unit: "%",
    icon: "mdi-water-percent",
    color: "#4ECDC4",
  },
  {
    label: "Pressure",
    value: dispPress(latest.value?.pressure_hpa),
    unit: unitStore.pressUnit(),
    icon: "mdi-gauge",
    color: "#45B7D1",
  },
  {
    label: "Altitude",
    value: dispAlt(latest.value?.altitude_m),
    unit: unitStore.altUnit(),
    icon: "mdi-image-filter-hdr",
    color: "#26de81",
  },
  {
    label: "Heat Index",
    value: dispTemp(latest.value?.heat_index_c),
    unit: unitStore.tempUnit(),
    icon: "mdi-fire",
    color: "#F7B731",
  },
  {
    label: "Soil",
    value: latest.value?.soil_moisture_pct,
    unit: "%",
    icon: "mdi-sprout",
    color: soilBadgeColor.value,
  },
]);

function toMs(ts) {
  return ts < 1e12 ? ts * 1000 : ts;
}
function buildSD(rows, field, conv) {
  return [...rows].reverse().map((d) => {
    const raw = d[field];
    const val =
      raw != null ? (conv ? conv(raw) : parseFloat(raw.toFixed(2))) : null;
    return [toMs(d.timestamp), val];
  });
}

function initCharts() {
  const rows = history.value;
  if (!rows.length) return;
  const tU = unitStore.tempUnit(),
    pU = unitStore.pressUnit(),
    aU = unitStore.altUnit();
  if (tempChart) tempChart.destroy();
  tempChart = Highcharts.chart("hc-temp", {
    ...HC_THEME,
    title: { ...HC_THEME.title, text: "Temperature & Heat Index" },
    yAxis: { ...HC_THEME.yAxis, title: { ...HC_THEME.yAxis.title, text: tU } },
    series: [
      {
        name: `BMP Temp (${tU})`,
        type: "spline",
        data: buildSD(rows, "temperature_c", unitStore.convertTemp),
        color: "#FF6B6B",
        lineWidth: 2,
        marker: { radius: 2 },
      },
      {
        name: `Heat Index (${tU})`,
        type: "spline",
        data: buildSD(rows, "heat_index_c", unitStore.convertTemp),
        color: "#F7B731",
        lineWidth: 2,
        marker: { radius: 2 },
      },
      {
        name: `DHT Temp (${tU})`,
        type: "spline",
        data: buildSD(rows, "dht_temp_c", unitStore.convertTemp),
        color: "#a29bfe",
        lineWidth: 2,
        marker: { radius: 2 },
      },
    ],
  });
  if (humChart) humChart.destroy();
  humChart = Highcharts.chart("hc-hum", {
    ...HC_THEME,
    title: { ...HC_THEME.title, text: "Humidity" },
    yAxis: {
      ...HC_THEME.yAxis,
      title: { ...HC_THEME.yAxis.title, text: "%" },
      min: 0,
      max: 100,
    },
    series: [
      {
        name: "Humidity",
        type: "area",
        data: buildSD(rows, "humidity_pct"),
        color: "#4ECDC4",
        fillColor: {
          linearGradient: { x1: 0, y1: 0, x2: 0, y2: 1 },
          stops: [
            [0, "rgba(78,205,196,0.35)"],
            [1, "rgba(78,205,196,0)"],
          ],
        },
        lineWidth: 2,
        marker: { radius: 2 },
      },
    ],
  });
  if (pressChart) pressChart.destroy();
  pressChart = Highcharts.chart("hc-press", {
    ...HC_THEME,
    title: { ...HC_THEME.title, text: "Pressure & Altitude" },
    yAxis: [
      { ...HC_THEME.yAxis, title: { ...HC_THEME.yAxis.title, text: pU } },
      {
        ...HC_THEME.yAxis,
        title: { ...HC_THEME.yAxis.title, text: aU },
        opposite: true,
        gridLineWidth: 0,
      },
    ],
    series: [
      {
        name: `Pressure (${pU})`,
        type: "area",
        data: buildSD(rows, "pressure_hpa", unitStore.convertPress),
        color: "#45B7D1",
        fillColor: {
          linearGradient: { x1: 0, y1: 0, x2: 0, y2: 1 },
          stops: [
            [0, "rgba(69,183,209,0.35)"],
            [1, "rgba(69,183,209,0)"],
          ],
        },
        lineWidth: 2,
        marker: { radius: 2 },
        yAxis: 0,
      },
      {
        name: `Altitude (${aU})`,
        type: "spline",
        data: buildSD(rows, "altitude_m", unitStore.convertAlt),
        color: "#26de81",
        lineWidth: 2,
        marker: { radius: 2 },
        yAxis: 1,
      },
    ],
  });
}

function pushPoint(doc) {
  if (!tempChart || !humChart || !pressChart) return;
  const x = toMs(doc.timestamp),
    shift = (tempChart.series[0].data.length || 0) >= 30;
  tempChart.series[0].addPoint(
    [
      x,
      doc.temperature_c != null
        ? unitStore.convertTemp(doc.temperature_c)
        : null,
    ],
    false,
    shift,
  );
  tempChart.series[1].addPoint(
    [
      x,
      doc.heat_index_c != null ? unitStore.convertTemp(doc.heat_index_c) : null,
    ],
    false,
    shift,
  );
  tempChart.series[2].addPoint(
    [x, doc.dht_temp_c != null ? unitStore.convertTemp(doc.dht_temp_c) : null],
    true,
    shift,
  );
  humChart.series[0].addPoint([x, doc.humidity_pct ?? null], true, shift);
  pressChart.series[0].addPoint(
    [
      x,
      doc.pressure_hpa != null
        ? unitStore.convertPress(doc.pressure_hpa)
        : null,
    ],
    false,
    shift,
  );
  pressChart.series[1].addPoint(
    [x, doc.altitude_m != null ? unitStore.convertAlt(doc.altitude_m) : null],
    true,
    shift,
  );
}

function buildFluidMeter() {
  if (!fluidMeterContainer.value || typeof FluidMeter === "undefined") return;
  fluidMeterContainer.value.innerHTML = "";
  fluidMeter = null;
  const fg = soilBadgeColor.value;
  fluidMeter = new FluidMeter();
  fluidMeter.init({
    targetContainer: fluidMeterContainer.value,
    fillPercentage: soilPct.value,
    options: {
      size: 200,
      borderWidth: 12,
      backgroundColor: "#0d1829",
      foregroundColor: "#1a2540",
      drawShadow: true,
      drawText: true,
      drawBubbles: true,
      fontSize: "40px",
      fontFamily: "Syne",
      fontFillStyle: "#f0f4ff",
      foregroundFluidLayer: {
        fillStyle: fg,
        angularSpeed: 100,
        maxAmplitude: 8,
        frequency: 30,
        horizontalSpeed: -150,
      },
      backgroundFluidLayer: {
        fillStyle: fg + "88",
        angularSpeed: 140,
        maxAmplitude: 12,
        frequency: 40,
        horizontalSpeed: 150,
      },
    },
  });
}

let lastTimestamp = null;
async function fetchAll() {
  loading.value = true;
  await Promise.all([store.fetchLatest(), store.fetchHistory(30)]);
  loading.value = false;
  nextTick(() => {
    initCharts();
    buildFluidMeter();
  });
}

function startTimers() {
  refreshTimer = setInterval(async () => {
    countdown.value = 5;
    await store.fetchLatest();
    if (latest.value && latest.value.timestamp !== lastTimestamp) {
      lastTimestamp = latest.value.timestamp;
      pushPoint(latest.value);
      if (fluidMeter) fluidMeter.setPercentage(soilPct.value);
    }
  }, 5000);
  countdownTimer = setInterval(() => {
    countdown.value = Math.max(0, countdown.value - 1);
  }, 1000);
}

function formatTs(ts) {
  if (!ts) return "—";
  return new Date(ts < 1e12 ? ts * 1000 : ts).toLocaleString("en-US", {
    month: "short",
    day: "numeric",
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
  });
}
function heatIndexClass(v) {
  if (v == null) return "";
  if (v >= 32) return "cell-danger";
  if (v >= 27) return "cell-warn";
  return "cell-ok";
}
function soilClass(v) {
  if (v == null) return "";
  if (v < 20) return "cell-danger";
  if (v < 40) return "cell-warn";
  if (v < 70) return "cell-ok";
  return "cell-info";
}

watch(soilBucket, () => nextTick(() => buildFluidMeter()));
watch(units, () => nextTick(() => initCharts()), { deep: true });

onMounted(async () => {
  await fetchAll();
  startTimers();
});
onBeforeUnmount(() => {
  clearInterval(refreshTimer);
  clearInterval(countdownTimer);
  if (tempChart) tempChart.destroy();
  if (humChart) humChart.destroy();
  if (pressChart) pressChart.destroy();
});
</script>

<style scoped>
@import url("https://fonts.googleapis.com/css2?family=Syne:wght@400;700;800&family=DM+Sans:opsz,wght@9..40,300;9..40,400;9..40,500;9..40,600&display=swap");
.dash-wrapper {
  min-height: 100vh;
  background: #060d1f;
  font-family: "DM Sans", sans-serif;
}
.dash-eyebrow {
  font-size: 11px;
  letter-spacing: 3px;
  color: rgba(200, 215, 240, 0.45);
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
.refresh-badge {
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 13px;
  font-weight: 500;
  color: rgba(200, 215, 240, 0.7);
  background: rgba(255, 255, 255, 0.04);
  border: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: 20px;
  padding: 6px 14px;
}
.pulse-dot {
  width: 7px;
  height: 7px;
  background: #26de81;
  border-radius: 50%;
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
.kpi-card {
  background: rgba(255, 255, 255, 0.04);
  border: 1px solid rgba(255, 255, 255, 0.09);
  border-top: 2px solid var(--accent);
  border-radius: 12px;
  padding: 14px 16px;
  transition: all 0.3s;
}
.kpi-card:hover {
  background: rgba(255, 255, 255, 0.07);
  transform: translateY(-2px);
}
.kpi-icon {
  margin-bottom: 8px;
  display: block;
}
.kpi-value {
  font-family: "Syne", sans-serif;
  font-size: 22px;
  font-weight: 700;
  color: #f0f4ff;
  line-height: 1;
}
.kpi-unit {
  font-size: 13px;
  font-weight: 500;
  color: rgba(200, 215, 240, 0.65);
  margin-left: 2px;
}
.kpi-label {
  font-size: 12px;
  font-weight: 500;
  letter-spacing: 0.5px;
  color: rgba(200, 215, 240, 0.65);
  text-transform: uppercase;
  margin-top: 6px;
}
.chart-card {
  background: rgba(255, 255, 255, 0.04);
  border: 1px solid rgba(255, 255, 255, 0.09);
  border-radius: 16px;
  padding: 20px;
  height: 100%;
}
.highcharts-figure {
  margin: 0;
}
.chart-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-wrap: wrap;
  gap: 8px;
  margin-bottom: 16px;
}
.chart-title {
  font-family: "Syne", sans-serif;
  font-size: 16px;
  font-weight: 700;
  color: #f0f4ff;
}
.fluid-card {
  display: flex;
  flex-direction: column;
}
.fluid-meter-wrap {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 10px 0;
}
.fluid-meter-container {
  line-height: 0;
}
.soil-status-badge {
  font-size: 12px;
  font-weight: 600;
  letter-spacing: 1px;
  text-transform: uppercase;
  padding: 4px 12px;
  border-radius: 20px;
}
.console-wrap {
  overflow-x: auto;
  max-height: 300px;
  overflow-y: auto;
}
.console-table {
  width: 100%;
  border-collapse: collapse;
  font-size: 13px;
  font-family: "DM Mono", "Fira Mono", monospace;
}
.console-table th {
  text-align: left;
  padding: 9px 13px;
  color: rgba(200, 215, 240, 0.7);
  font-size: 11px;
  font-weight: 600;
  letter-spacing: 0.8px;
  text-transform: uppercase;
  border-bottom: 1px solid rgba(255, 255, 255, 0.1);
  position: sticky;
  top: 0;
  background: #0d1829;
  white-space: nowrap;
}
.console-table td {
  padding: 9px 13px;
  color: rgba(200, 215, 240, 0.85);
  border-bottom: 1px solid rgba(255, 255, 255, 0.05);
  white-space: nowrap;
}
.console-table tr:hover td {
  background: rgba(255, 255, 255, 0.04);
}
.row-new td {
  color: #f0f4ff;
}
.ts-cell {
  color: rgba(200, 215, 240, 0.5) !important;
}
.log-count {
  font-size: 12px;
  color: rgba(200, 215, 240, 0.5);
  letter-spacing: 1px;
}
.cell-danger {
  color: #ff6b6b !important;
  font-weight: 700;
}
.cell-warn {
  color: #f7b731 !important;
  font-weight: 700;
}
.cell-ok {
  color: #26de81 !important;
  font-weight: 700;
}
.cell-info {
  color: #45b7d1 !important;
  font-weight: 700;
}
</style>
