<template>
  <div class="analysis-wrapper">
    <VContainer fluid class="pa-4 pa-md-6">
      <!-- Header -->
      <VRow align="center" class="mb-6">
        <VCol>
          <div class="dash-eyebrow">HISTORICAL ANALYSIS</div>
          <h1 class="dash-title">Data Analysis</h1>
        </VCol>
      </VRow>

      <!-- Row 1: Date inputs + MMAR stat cards -->
      <VRow class="mb-4" align="stretch">
        <!-- Date picker panel -->
        <VCol cols="12" md="3">
          <div class="panel">
            <div class="panel-label">DATE RANGE</div>

            <div class="field-group">
              <label class="field-label">Start Date</label>
              <input v-model="start" type="date" class="date-input" />
            </div>
            <div class="field-group mt-3">
              <label class="field-label">End Date</label>
              <input v-model="end" type="date" class="date-input" />
            </div>

            <div v-if="errorText" class="msg msg-error mt-3">
              {{ errorText }}
            </div>
            <div v-if="statusText" class="msg msg-status mt-2">
              {{ statusText }}
            </div>

            <button
              class="btn-analyze mt-4"
              :class="{ loading }"
              @click="runAnalysis"
              :disabled="loading"
            >
              <span v-if="!loading">
                <VIcon icon="mdi-chart-line" size="16" class="mr-1" />
                Analyze
              </span>
              <span v-else class="spinner"></span>
            </button>
          </div>
        </VCol>

        <!-- MMAR Stat cards -->
        <VCol cols="12" md="9">
          <VRow class="h-100">
            <VCol
              v-for="card in statCards"
              :key="card.field"
              cols="12"
              sm="6"
              lg="3"
            >
              <div class="stat-card" :style="{ '--accent': card.color }">
                <div class="stat-card-header">
                  <VIcon :icon="card.icon" size="18" :color="card.color" />
                  <span class="stat-card-title">{{ card.label }}</span>
                  <span class="stat-card-unit">{{ card.unit }}</span>
                </div>

                <div class="stat-avg">
                  {{ card.stats.avg ?? "—" }}
                </div>
                <div class="stat-sub">avg</div>

                <div class="stat-pills">
                  <div class="stat-pill">
                    <span class="pill-label">MIN</span>
                    <span class="pill-val">{{ card.stats.min ?? "—" }}</span>
                  </div>
                  <div class="stat-pill">
                    <span class="pill-label">MAX</span>
                    <span class="pill-val">{{ card.stats.max ?? "—" }}</span>
                  </div>
                  <div class="stat-pill">
                    <span class="pill-label">RNG</span>
                    <span class="pill-val">{{ card.stats.range ?? "—" }}</span>
                  </div>
                </div>
              </div>
            </VCol>
          </VRow>
        </VCol>
      </VRow>

      <!-- Row 2: Line charts -->
      <VRow class="mb-4">
        <VCol cols="12" lg="7">
          <div class="chart-card">
            <figure class="hc-figure"><div id="hc-temp-line"></div></figure>
          </div>
        </VCol>
        <VCol cols="12" lg="5">
          <div class="chart-card">
            <figure class="hc-figure"><div id="hc-hum-line"></div></figure>
          </div>
        </VCol>
      </VRow>

      <!-- Row 3: Pressure + Soil line charts -->
      <VRow class="mb-4">
        <VCol cols="12" lg="7">
          <div class="chart-card">
            <figure class="hc-figure"><div id="hc-press-line"></div></figure>
          </div>
        </VCol>
        <VCol cols="12" lg="5">
          <div class="chart-card">
            <figure class="hc-figure"><div id="hc-soil-line"></div></figure>
          </div>
        </VCol>
      </VRow>

      <!-- Row 4: Frequency Distribution histogram -->
      <VRow class="mb-4">
        <VCol cols="12">
          <div class="chart-card">
            <figure class="hc-figure"><div id="hc-histogram"></div></figure>
          </div>
        </VCol>
      </VRow>

      <!-- Row 5: Scatter plots -->
      <VRow class="mb-4">
        <VCol cols="12" md="6">
          <div class="chart-card">
            <figure class="hc-figure"><div id="hc-scatter1"></div></figure>
          </div>
        </VCol>
        <VCol cols="12" md="6">
          <div class="chart-card">
            <figure class="hc-figure"><div id="hc-scatter2"></div></figure>
          </div>
        </VCol>
      </VRow>
    </VContainer>
  </div>
</template>

<script setup>
import { ref, reactive, onMounted, onBeforeUnmount, computed } from "vue";
import { useAppStore } from "@/store/appStore";
import Highcharts from "highcharts";
import more from "highcharts/highcharts-more";
import Exporting from "highcharts/modules/exporting";
Exporting(Highcharts);
more(Highcharts);

const store = useAppStore();

// ── State ──────────────────────────────────────────────────────
const start = ref("");
const end = ref("");
const loading = ref(false);
const errorText = ref("");
const statusText = ref("");

const mmar = reactive({
  temperature_c: { min: null, max: null, avg: null, range: null },
  humidity_pct: { min: null, max: null, avg: null, range: null },
  pressure_hpa: { min: null, max: null, avg: null, range: null },
  heat_index_c: { min: null, max: null, avg: null, range: null },
  soil_moisture_pct: { min: null, max: null, avg: null, range: null },
  altitude_m: { min: null, max: null, avg: null, range: null },
});

// ── Stat card definitions ──────────────────────────────────────
const statCards = computed(() => [
  {
    field: "temperature_c",
    label: "BMP Temp",
    unit: "°C",
    icon: "mdi-thermometer",
    color: "#FF6B6B",
    stats: mmar.temperature_c,
  },
  {
    field: "humidity_pct",
    label: "Humidity",
    unit: "%",
    icon: "mdi-water-percent",
    color: "#4ECDC4",
    stats: mmar.humidity_pct,
  },
  {
    field: "heat_index_c",
    label: "Heat Index",
    unit: "°C",
    icon: "mdi-fire",
    color: "#F7B731",
    stats: mmar.heat_index_c,
  },
  {
    field: "pressure_hpa",
    label: "Pressure",
    unit: "hPa",
    icon: "mdi-gauge",
    color: "#45B7D1",
    stats: mmar.pressure_hpa,
  },
  {
    field: "altitude_m",
    label: "Altitude",
    unit: "m",
    icon: "mdi-image-filter-hdr",
    color: "#26de81",
    stats: mmar.altitude_m,
  },
  {
    field: "soil_moisture_pct",
    label: "Soil",
    unit: "%",
    icon: "mdi-sprout",
    color: "#a29bfe",
    stats: mmar.soil_moisture_pct,
  },
]);

// ── Highcharts shared dark theme ───────────────────────────────
const HC = {
  chart: {
    backgroundColor: "transparent",
    style: { fontFamily: "'DM Sans', sans-serif" },
    animation: { duration: 400 },
  },
  title: {
    style: {
      color: "#f0f4ff",
      fontFamily: "'Syne', sans-serif",
      fontSize: "14px",
      fontWeight: "700",
    },
    align: "left",
  },
  subtitle: {
    style: { color: "rgba(200,215,240,0.4)", fontSize: "11px" },
    align: "left",
  },
  xAxis: {
    labels: { style: { color: "rgba(200,215,240,0.4)", fontSize: "10px" } },
    gridLineColor: "rgba(255,255,255,0.05)",
    lineColor: "rgba(255,255,255,0.08)",
    tickColor: "rgba(255,255,255,0.08)",
  },
  yAxis: {
    labels: { style: { color: "rgba(200,215,240,0.4)", fontSize: "10px" } },
    gridLineColor: "rgba(255,255,255,0.05)",
    title: { style: { color: "rgba(200,215,240,0.4)", fontSize: "11px" } },
  },
  tooltip: {
    backgroundColor: "rgba(6,13,31,0.95)",
    borderColor: "rgba(255,255,255,0.1)",
    borderWidth: 1,
    style: { color: "#f0f4ff", fontSize: "12px" },
    shared: true,
  },
  legend: {
    itemStyle: { color: "rgba(200,215,240,0.5)", fontSize: "11px" },
    itemHoverStyle: { color: "#f0f4ff" },
  },
  credits: { enabled: false },
  exporting: { enabled: false },
};

// ── Chart instances ────────────────────────────────────────────
let hcTempLine = null;
let hcHumLine = null;
let hcPressLine = null;
let hcSoilLine = null;
let hcHistogram = null;
let hcScatter1 = null;
let hcScatter2 = null;

function initCharts() {
  // Temperature & Heat Index line
  hcTempLine = Highcharts.chart("hc-temp-line", {
    ...HC,
    chart: { ...HC.chart, zoomType: "x" },
    title: { ...HC.title, text: "Temperature & Heat Index" },
    subtitle: {
      ...HC.subtitle,
      text: "BMP280 temp, DHT22 temp and derived heat index over time",
    },
    xAxis: { ...HC.xAxis, type: "datetime" },
    yAxis: { ...HC.yAxis, title: { ...HC.yAxis.title, text: "°C" } },
    series: [
      {
        name: "BMP Temp",
        type: "spline",
        data: [],
        color: "#FF6B6B",
        lineWidth: 2,
        marker: { radius: 2 },
      },
      {
        name: "Heat Index",
        type: "spline",
        data: [],
        color: "#F7B731",
        lineWidth: 2,
        marker: { radius: 2 },
      },
      {
        name: "DHT Temp",
        type: "spline",
        data: [],
        color: "#a29bfe",
        lineWidth: 2,
        marker: { radius: 2 },
      },
    ],
  });

  // Humidity line
  hcHumLine = Highcharts.chart("hc-hum-line", {
    ...HC,
    chart: { ...HC.chart, zoomType: "x" },
    title: { ...HC.title, text: "Humidity" },
    subtitle: {
      ...HC.subtitle,
      text: "Relative humidity (%) over the selected period",
    },
    xAxis: { ...HC.xAxis, type: "datetime" },
    yAxis: {
      ...HC.yAxis,
      min: 0,
      max: 100,
      title: { ...HC.yAxis.title, text: "%" },
    },
    series: [
      {
        name: "Humidity",
        type: "area",
        data: [],
        color: "#4ECDC4",
        fillColor: {
          linearGradient: { x1: 0, y1: 0, x2: 0, y2: 1 },
          stops: [
            [0, "rgba(78,205,196,0.3)"],
            [1, "rgba(78,205,196,0)"],
          ],
        },
        lineWidth: 2,
        marker: { radius: 2 },
      },
    ],
  });

  // Pressure + Altitude dual axis
  hcPressLine = Highcharts.chart("hc-press-line", {
    ...HC,
    chart: { ...HC.chart, zoomType: "x" },
    title: { ...HC.title, text: "Pressure & Altitude" },
    subtitle: {
      ...HC.subtitle,
      text: "Atmospheric pressure (hPa) and derived altitude (m)",
    },
    xAxis: { ...HC.xAxis, type: "datetime" },
    yAxis: [
      { ...HC.yAxis, title: { ...HC.yAxis.title, text: "hPa" } },
      {
        ...HC.yAxis,
        title: { ...HC.yAxis.title, text: "m" },
        opposite: true,
        gridLineWidth: 0,
      },
    ],
    series: [
      {
        name: "Pressure",
        type: "area",
        data: [],
        color: "#45B7D1",
        fillColor: {
          linearGradient: { x1: 0, y1: 0, x2: 0, y2: 1 },
          stops: [
            [0, "rgba(69,183,209,0.3)"],
            [1, "rgba(69,183,209,0)"],
          ],
        },
        lineWidth: 2,
        marker: { radius: 2 },
        yAxis: 0,
      },
      {
        name: "Altitude",
        type: "spline",
        data: [],
        color: "#26de81",
        lineWidth: 2,
        marker: { radius: 2 },
        yAxis: 1,
      },
    ],
  });

  // Soil moisture line
  hcSoilLine = Highcharts.chart("hc-soil-line", {
    ...HC,
    chart: { ...HC.chart, zoomType: "x" },
    title: { ...HC.title, text: "Soil Moisture" },
    subtitle: { ...HC.subtitle, text: "Capacitive soil sensor reading (%)" },
    xAxis: { ...HC.xAxis, type: "datetime" },
    yAxis: {
      ...HC.yAxis,
      min: 0,
      max: 100,
      title: { ...HC.yAxis.title, text: "%" },
    },
    series: [
      {
        name: "Soil Moisture",
        type: "area",
        data: [],
        color: "#a29bfe",
        fillColor: {
          linearGradient: { x1: 0, y1: 0, x2: 0, y2: 1 },
          stops: [
            [0, "rgba(162,155,254,0.3)"],
            [1, "rgba(162,155,254,0)"],
          ],
        },
        lineWidth: 2,
        marker: { radius: 2 },
      },
    ],
  });

  // Frequency distribution histogram
  hcHistogram = Highcharts.chart("hc-histogram", {
    ...HC,
    chart: { ...HC.chart, type: "column" },
    title: { ...HC.title, text: "Frequency Distribution" },
    subtitle: {
      ...HC.subtitle,
      text: "Count of readings falling in each 10-unit bucket across all sensor fields",
    },
    xAxis: {
      ...HC.xAxis,
      title: {
        ...HC.xAxis.title,
        text: "Value Range",
        style: { color: "rgba(200,215,240,0.4)" },
      },
      categories: [],
    },
    yAxis: { ...HC.yAxis, title: { ...HC.yAxis.title, text: "Count" } },
    plotOptions: { column: { borderRadius: 4, groupPadding: 0.1 } },
    series: [
      { name: "Temp (°C)", data: [], color: "#FF6B6B" },
      { name: "Humidity (%)", data: [], color: "#4ECDC4" },
      { name: "Heat Index", data: [], color: "#F7B731" },
    ],
  });

  // Scatter: Temperature vs Heat Index
  hcScatter1 = Highcharts.chart("hc-scatter1", {
    ...HC,
    chart: { ...HC.chart, type: "scatter", zoomType: "xy" },
    title: { ...HC.title, text: "Temp vs Heat Index" },
    subtitle: {
      ...HC.subtitle,
      text: "Correlation between air temperature and perceived heat",
    },
    xAxis: {
      ...HC.xAxis,
      title: {
        ...HC.xAxis.title,
        text: "Temperature (°C)",
        style: { color: "rgba(200,215,240,0.4)" },
      },
    },
    yAxis: {
      ...HC.yAxis,
      title: { ...HC.yAxis.title, text: "Heat Index (°C)" },
    },
    tooltip: {
      ...HC.tooltip,
      shared: false,
      pointFormat:
        "Temp: <b>{point.x} °C</b><br/>Heat Index: <b>{point.y} °C</b>",
    },
    plotOptions: {
      scatter: {
        marker: { radius: 3, symbol: "circle" },
        jitter: { x: 0.005 },
      },
    },
    series: [
      {
        name: "Readings",
        data: [],
        color: "#FF6B6B",
        marker: { symbol: "circle", radius: 3 },
      },
    ],
  });

  // Scatter: Humidity vs Heat Index
  hcScatter2 = Highcharts.chart("hc-scatter2", {
    ...HC,
    chart: { ...HC.chart, type: "scatter", zoomType: "xy" },
    title: { ...HC.title, text: "Humidity vs Heat Index" },
    subtitle: {
      ...HC.subtitle,
      text: "How humidity drives the apparent temperature",
    },
    xAxis: {
      ...HC.xAxis,
      title: {
        ...HC.xAxis.title,
        text: "Humidity (%)",
        style: { color: "rgba(200,215,240,0.4)" },
      },
    },
    yAxis: {
      ...HC.yAxis,
      title: { ...HC.yAxis.title, text: "Heat Index (°C)" },
    },
    tooltip: {
      ...HC.tooltip,
      shared: false,
      pointFormat:
        "Humidity: <b>{point.x} %</b><br/>Heat Index: <b>{point.y} °C</b>",
    },
    plotOptions: {
      scatter: {
        marker: { radius: 3, symbol: "circle" },
        jitter: { x: 0.005 },
      },
    },
    series: [
      {
        name: "Readings",
        data: [],
        color: "#4ECDC4",
        marker: { symbol: "circle", radius: 3 },
      },
    ],
  });
}

function toMs(ts) {
  return ts < 1e12 ? ts * 1000 : ts;
}
function fmt(v, dp = 1) {
  return v != null ? parseFloat(v.toFixed(dp)) : null;
}

function updateLineCharts(data) {
  const temp = [],
    hi = [],
    dht = [],
    hum = [],
    press = [],
    alt = [],
    soil = [];
  data.forEach((row) => {
    const x = toMs(row.timestamp);
    temp.push([x, fmt(row.temperature_c)]);
    hi.push([x, fmt(row.heat_index_c)]);
    dht.push([x, fmt(row.dht_temp_c)]);
    hum.push([x, fmt(row.humidity_pct)]);
    press.push([x, fmt(row.pressure_hpa)]);
    alt.push([x, fmt(row.altitude_m)]);
    soil.push([
      x,
      row.soil_moisture_pct != null ? row.soil_moisture_pct : null,
    ]);
  });

  hcTempLine.series[0].setData(temp, false);
  hcTempLine.series[1].setData(hi, false);
  hcTempLine.series[2].setData(dht, true);

  hcHumLine.series[0].setData(hum, true);

  hcPressLine.series[0].setData(press, false);
  hcPressLine.series[1].setData(alt, true);

  hcSoilLine.series[0].setData(soil, true);
}

function updateHistogram(distros) {
  const fields = ["temperature_c", "humidity_pct", "heat_index_c"];

  const sample = distros[fields.find((f) => distros[f]?.length)] || [];
  const categories = sample.map((b) =>
    b._id === "outliers" ? "outliers" : `${b._id}–${b._id + 10}`,
  );
  hcHistogram.xAxis[0].setCategories(categories, false);

  fields.forEach((field, i) => {
    const series = (distros[field] || []).map((b) => b.count);
    hcHistogram.series[i].setData(series, i === fields.length - 1);
  });
}

function updateScatterPlots(data) {
  const s1 = [],
    s2 = [];
  data.forEach((row) => {
    if (row.temperature_c != null && row.heat_index_c != null)
      s1.push([fmt(row.temperature_c), fmt(row.heat_index_c)]);
    if (row.humidity_pct != null && row.heat_index_c != null)
      s2.push([fmt(row.humidity_pct), fmt(row.heat_index_c)]);
  });
  hcScatter1.series[0].setData(s1, true);
  hcScatter2.series[0].setData(s2, true);
}

async function updateMMAR(startTs, endTs) {
  const fields = Object.keys(mmar);
  await Promise.all(
    fields.map(async (field) => {
      const result = await store.fetchStats(field, startTs, endTs);
      if (result) {
        mmar[field].min = fmt(result.min);
        mmar[field].max = fmt(result.max);
        mmar[field].avg = fmt(result.avg);
        mmar[field].range = fmt(result.range);
      }
    }),
  );
}

async function runAnalysis() {
  errorText.value = "";
  statusText.value = "";

  if (!start.value || !end.value) {
    errorText.value = "Please select both a start and end date.";
    return;
  }

  const startTs = new Date(`${start.value}T00:00:00`).getTime() / 1000;
  const endTs = new Date(`${end.value}T23:59:59`).getTime() / 1000;

  if (startTs > endTs) {
    errorText.value = "Start date cannot be after end date.";
    return;
  }

  loading.value = true;
  statusText.value = "Fetching data…";

  try {
    // 1. Fetch range data + MMAR + distribution in parallel
    const distroFields = ["temperature_c", "humidity_pct", "heat_index_c"];

    const [rangeData, ...distros] = await Promise.all([
      store.fetchRange(startTs, endTs),
      ...distroFields.map((f) => store.fetchDistribution(f, startTs, endTs)),
    ]);

    await updateMMAR(startTs, endTs);

    if (!rangeData || rangeData.length === 0) {
      statusText.value = "No records found for the selected range.";
      loading.value = false;
      return;
    }

    // Sort by timestamp ascending
    const sorted = [...rangeData].sort((a, b) => a.timestamp - b.timestamp);

    const distroMap = {};
    distroFields.forEach((f, i) => {
      distroMap[f] = distros[i] || [];
    });

    updateLineCharts(sorted);
    updateHistogram(distroMap);
    updateScatterPlots(sorted);

    statusText.value = `${sorted.length} records loaded.`;
  } catch (e) {
    errorText.value = "Failed to load data. Check backend connection.";
    console.error("runAnalysis error:", e);
  } finally {
    loading.value = false;
  }
}

// ── Lifecycle ──────────────────────────────────────────────────
onMounted(() => {
  initCharts();
});

onBeforeUnmount(() => {
  [
    hcTempLine,
    hcHumLine,
    hcPressLine,
    hcSoilLine,
    hcHistogram,
    hcScatter1,
    hcScatter2,
  ].forEach((c) => c?.destroy());
});
</script>

<style scoped>
@import url("https://fonts.googleapis.com/css2?family=Syne:wght@400;700;800&family=DM+Sans:wght@300;400;500&display=swap");

.analysis-wrapper {
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
}
.dash-title {
  font-family: "Syne", sans-serif;
  font-size: 28px;
  font-weight: 800;
  color: #f0f4ff;
  letter-spacing: -1px;
}

/* ── Date panel ── */
.panel {
  background: rgba(255, 255, 255, 0.03);
  border: 1px solid rgba(255, 255, 255, 0.07);
  border-radius: 16px;
  padding: 24px 20px;
  height: 100%;
}
.panel-label {
  font-size: 10px;
  letter-spacing: 3px;
  color: rgba(200, 215, 240, 0.3);
  text-transform: uppercase;
  margin-bottom: 20px;
}
.field-group {
  display: flex;
  flex-direction: column;
  gap: 6px;
}
.field-label {
  font-size: 11px;
  letter-spacing: 1px;
  text-transform: uppercase;
  color: rgba(200, 215, 240, 0.4);
}
.date-input {
  background: rgba(255, 255, 255, 0.05);
  border: 1px solid rgba(255, 255, 255, 0.1);
  border-radius: 10px;
  color: #f0f4ff;
  font-family: "DM Sans", sans-serif;
  font-size: 14px;
  padding: 10px 14px;
  width: 100%;
  outline: none;
  transition: border-color 0.2s;
  color-scheme: dark;
}
.date-input:focus {
  border-color: #4ecdc4;
}

.msg {
  font-size: 12px;
  border-radius: 8px;
  padding: 8px 12px;
}
.msg-error {
  background: rgba(255, 107, 107, 0.12);
  color: #ff6b6b;
  border: 1px solid rgba(255, 107, 107, 0.25);
}
.msg-status {
  background: rgba(78, 205, 196, 0.08);
  color: rgba(200, 215, 240, 0.6);
  border: 1px solid rgba(78, 205, 196, 0.15);
}

.btn-analyze {
  width: 100%;
  background: linear-gradient(135deg, #4ecdc4, #45b7d1);
  color: #060d1f;
  font-family: "DM Sans", sans-serif;
  font-weight: 600;
  font-size: 14px;
  padding: 12px 20px;
  border-radius: 50px;
  border: none;
  cursor: pointer;
  display: flex;
  align-items: center;
  justify-content: center;
  transition: all 0.3s ease;
  box-shadow: 0 0 20px rgba(78, 205, 196, 0.25);
}
.btn-analyze:hover:not(:disabled) {
  transform: translateY(-1px);
  box-shadow: 0 0 35px rgba(78, 205, 196, 0.4);
}
.btn-analyze:disabled {
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

/* ── MMAR stat cards ── */
.stat-card {
  background: rgba(255, 255, 255, 0.03);
  border: 1px solid rgba(255, 255, 255, 0.07);
  border-top: 2px solid var(--accent);
  border-radius: 14px;
  padding: 18px 16px;
  height: 100%;
  transition: all 0.25s;
}
.stat-card:hover {
  background: rgba(255, 255, 255, 0.055);
  transform: translateY(-2px);
}
.stat-card-header {
  display: flex;
  align-items: center;
  gap: 6px;
  margin-bottom: 12px;
}
.stat-card-title {
  font-size: 12px;
  font-weight: 500;
  color: rgba(200, 215, 240, 0.6);
  flex: 1;
}
.stat-card-unit {
  font-size: 10px;
  color: rgba(200, 215, 240, 0.3);
  letter-spacing: 1px;
}
.stat-avg {
  font-family: "Syne", sans-serif;
  font-size: 32px;
  font-weight: 800;
  color: #f0f4ff;
  line-height: 1;
}
.stat-sub {
  font-size: 10px;
  letter-spacing: 2px;
  text-transform: uppercase;
  color: rgba(200, 215, 240, 0.3);
  margin-top: 2px;
  margin-bottom: 14px;
}
.stat-pills {
  display: flex;
  gap: 6px;
}
.stat-pill {
  flex: 1;
  background: rgba(255, 255, 255, 0.05);
  border-radius: 8px;
  padding: 6px 4px;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 2px;
}
.pill-label {
  font-size: 9px;
  letter-spacing: 1px;
  color: rgba(200, 215, 240, 0.3);
}
.pill-val {
  font-family: "Syne", sans-serif;
  font-size: 13px;
  font-weight: 700;
  color: #f0f4ff;
}

/* ── Chart cards ── */
.chart-card {
  background: rgba(255, 255, 255, 0.03);
  border: 1px solid rgba(255, 255, 255, 0.07);
  border-radius: 16px;
  padding: 20px;
  height: 100%;
}
.hc-figure {
  margin: 0;
}
</style>
