<template>
  <div class="home-wrapper">
    <div class="sky">
      <div class="stars"></div>
      <div class="cloud cloud-1"></div>
      <div class="cloud cloud-2"></div>
      <div class="cloud cloud-3"></div>
    </div>
    <VContainer class="home-content">
      <VRow justify="center" align="center" class="hero-row">
        <VCol cols="12" class="text-center">
          <div class="station-badge">ELET2415 · Live Readings</div>
          <h1 class="hero-title">
            <span class="title-line">Weather</span>
            <span class="title-line accent">Station</span>
          </h1>
          <div class="hero-btns mt-8">
            <RouterLink :to="{ name: 'Dashboard' }">
              <button class="btn-primary">
                <VIcon icon="mdi-view-dashboard" class="mr-2" />Open Dashboard
              </button>
            </RouterLink>
            <RouterLink :to="{ name: 'Analysis' }">
              <button class="btn-secondary">
                <VIcon icon="mdi-chart-bar" class="mr-2" />Data Analysis
              </button>
            </RouterLink>
            <RouterLink :to="{ name: 'Control' }">
              <button class="btn-tertiary">
                <VIcon icon="mdi-tune-variant" class="mr-2" />Control Panel
              </button>
            </RouterLink>
          </div>
        </VCol>
      </VRow>

      <VRow justify="center" class="stats-strip mt-6">
        <VCol cols="12" md="10">
          <div class="stats-card">
            <div class="stat-item" v-for="stat in liveStats" :key="stat.label">
              <VIcon
                :icon="stat.icon"
                class="stat-icon"
                :color="stat.color"
                size="28"
              />
              <div class="stat-info">
                <span class="stat-value"
                  >{{ stat.value ?? "—"
                  }}<span class="stat-unit">{{ stat.unit }}</span></span
                >
                <span class="stat-label">{{ stat.label }}</span>
              </div>
            </div>
          </div>
        </VCol>
      </VRow>

      <VRow justify="center" class="mt-10">
        <VCol cols="12" md="10"
          ><div class="section-label">SYSTEM OVERVIEW</div></VCol
        >
        <VCol
          v-for="card in featureCards"
          :key="card.title"
          cols="12"
          sm="6"
          md="3"
        >
          <div class="feature-card">
            <div class="feature-icon-wrap" :style="{ background: card.bg }">
              <VIcon :icon="card.icon" size="28" :color="card.color" />
            </div>
            <h3 class="feature-title">{{ card.title }}</h3>
            <p class="feature-desc">{{ card.desc }}</p>
          </div>
        </VCol>
      </VRow>

      <VRow justify="center" class="mt-8 mb-10">
        <VCol cols="12" md="10">
          <div class="section-label">CONNECTED SENSORS</div>
          <div class="sensor-grid">
            <div class="sensor-chip" v-for="s in sensors" :key="s.name">
              <span class="sensor-dot" :style="{ background: s.color }"></span>
              <span class="sensor-name">{{ s.name }}</span>
              <span class="sensor-type">{{ s.type }}</span>
            </div>
          </div>
        </VCol>
      </VRow>
    </VContainer>
  </div>
</template>

<script setup>
import { onMounted, onBeforeUnmount, computed } from "vue";
import { useAppStore } from "@/store/appStore";
import { useUnitStore } from "@/store/unitStore";
import { storeToRefs } from "pinia";

const store = useAppStore();
const unitStore = useUnitStore();
const { latest } = storeToRefs(store);
const { units } = storeToRefs(unitStore);

let refreshTimer = null;
onMounted(() => {
  store.fetchLatest();
  refreshTimer = setInterval(() => store.fetchLatest(), 5000);
});
onBeforeUnmount(() => clearInterval(refreshTimer));

const liveStats = computed(() => {
  const l = latest.value;
  return [
    {
      label: "DHT Temp",
      value: unitStore.convertTemp(l?.dht_temp_c)?.toFixed(1),
      unit: unitStore.tempUnit(),
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
      value: unitStore
        .convertPress(l?.pressure_hpa)
        ?.toFixed(units.value.pressBar ? 4 : 1),
      unit: unitStore.pressUnit(),
      icon: "mdi-gauge",
      color: "#45B7D1",
    },
    {
      label: "Heat Index",
      value: unitStore.convertTemp(l?.heat_index_c)?.toFixed(1),
      unit: unitStore.tempUnit(),
      icon: "mdi-fire",
      color: "#F7B731",
    },
    {
      label: "Soil Moisture",
      value: l?.soil_moisture_pct,
      unit: "%",
      icon: "mdi-sprout",
      color: "#26de81",
    },
  ];
});

const featureCards = [
  {
    title: "Live Sensors",
    desc: "BMP280, AM2302 and capacitive soil sensor stream data every 3 seconds.",
    icon: "mdi-access-point",
    color: "#4ECDC4",
    bg: "rgba(78,205,196,0.12)",
  },
  {
    title: "MQTT Broker",
    desc: "Data transmitted over WiFi via MQTT protocol to the backend server.",
    icon: "mdi-transit-connection-variant",
    color: "#45B7D1",
    bg: "rgba(69,183,209,0.12)",
  },
  {
    title: "MongoDB",
    desc: "Every reading is timestamped and stored in a persistent cloud database.",
    icon: "mdi-database",
    color: "#26de81",
    bg: "rgba(38,222,129,0.12)",
  },
  {
    title: "Control Panel",
    desc: "Toggle display units and push preferences live to the TFT screen.",
    icon: "mdi-tune-variant",
    color: "#F7B731",
    bg: "rgba(247,183,49,0.12)",
  },
];

const sensors = [
  { name: "BMP280", type: "Temp · Pressure · Altitude", color: "#45B7D1" },
  { name: "AM2302", type: "Temp · Humidity · Heat Index", color: "#FF6B6B" },
  { name: "Capacitive Soil", type: "Soil Moisture %", color: "#26de81" },
  { name: "ILI9341", type: "320×240 TFT Display", color: "#F7B731" },
];
</script>

<style scoped>
@import url("https://fonts.googleapis.com/css2?family=Syne:wght@400;700;800&family=DM+Sans:opsz,wght@9..40,300;9..40,400;9..40,500;9..40,600&display=swap");
* {
  box-sizing: border-box;
}
.home-wrapper {
  min-height: 100vh;
  position: relative;
  font-family: "DM Sans", sans-serif;
  overflow: hidden;
  background: #060d1f;
}
.sky {
  position: fixed;
  inset: 0;
  background:
    radial-gradient(ellipse at 20% 0%, #0d2137 0%, #060d1f 60%),
    radial-gradient(ellipse at 80% 100%, #0a1a2e 0%, transparent 60%);
  z-index: 0;
}
.stars {
  position: absolute;
  inset: 0;
  background-image:
    radial-gradient(
      1px 1px at 10% 15%,
      rgba(255, 255, 255, 0.6) 0%,
      transparent 100%
    ),
    radial-gradient(
      1px 1px at 30% 40%,
      rgba(255, 255, 255, 0.4) 0%,
      transparent 100%
    ),
    radial-gradient(
      1px 1px at 55% 10%,
      rgba(255, 255, 255, 0.7) 0%,
      transparent 100%
    ),
    radial-gradient(
      1px 1px at 70% 60%,
      rgba(255, 255, 255, 0.3) 0%,
      transparent 100%
    ),
    radial-gradient(
      1px 1px at 85% 25%,
      rgba(255, 255, 255, 0.5) 0%,
      transparent 100%
    ),
    radial-gradient(
      1px 1px at 45% 70%,
      rgba(255, 255, 255, 0.6) 0%,
      transparent 100%
    );
}
.cloud {
  position: absolute;
  background: rgba(255, 255, 255, 0.03);
  border-radius: 50px;
  filter: blur(20px);
  animation: drift linear infinite;
}
.cloud-1 {
  width: 400px;
  height: 80px;
  top: 15%;
  left: -100px;
  animation-duration: 60s;
}
.cloud-2 {
  width: 300px;
  height: 60px;
  top: 35%;
  left: -80px;
  animation-duration: 80s;
  animation-delay: -20s;
}
.cloud-3 {
  width: 500px;
  height: 90px;
  top: 60%;
  left: -120px;
  animation-duration: 100s;
  animation-delay: -40s;
}
@keyframes drift {
  from {
    transform: translateX(-10%);
  }
  to {
    transform: translateX(110vw);
  }
}
.home-content {
  position: relative;
  z-index: 1;
  padding-top: 80px;
}
.hero-row {
  min-height: 40vh;
}
.station-badge {
  display: inline-block;
  font-size: 11px;
  font-weight: 600;
  letter-spacing: 3px;
  color: #4ecdc4;
  border: 1px solid rgba(78, 205, 196, 0.3);
  border-radius: 20px;
  padding: 5px 16px;
  margin-bottom: 24px;
}
.hero-title {
  font-family: "Syne", sans-serif;
  font-size: clamp(52px, 10vw, 96px);
  font-weight: 800;
  line-height: 1;
  color: #f0f4ff;
  display: flex;
  flex-direction: column;
  align-items: center;
  letter-spacing: -2px;
}
.title-line.accent {
  color: transparent;
  -webkit-text-stroke: 2px #4ecdc4;
}
.hero-btns {
  display: flex;
  gap: 14px;
  justify-content: center;
  flex-wrap: wrap;
}
.btn-primary {
  background: linear-gradient(135deg, #4ecdc4, #45b7d1);
  color: #060d1f;
  font-family: "DM Sans", sans-serif;
  font-weight: 700;
  font-size: 15px;
  padding: 14px 28px;
  border-radius: 50px;
  border: none;
  cursor: pointer;
  display: inline-flex;
  align-items: center;
  transition: all 0.3s ease;
  box-shadow: 0 0 30px rgba(78, 205, 196, 0.3);
}
.btn-primary:hover {
  transform: translateY(-2px);
  box-shadow: 0 0 50px rgba(78, 205, 196, 0.5);
}
.btn-secondary {
  background: transparent;
  color: #f0f4ff;
  font-family: "DM Sans", sans-serif;
  font-weight: 700;
  font-size: 15px;
  padding: 13px 28px;
  border-radius: 50px;
  border: 1px solid rgba(255, 255, 255, 0.2);
  cursor: pointer;
  display: inline-flex;
  align-items: center;
  transition: all 0.3s ease;
  backdrop-filter: blur(6px);
}
.btn-secondary:hover {
  border-color: rgba(247, 183, 49, 0.6);
  color: #f7b731;
  box-shadow: 0 0 30px rgba(247, 183, 49, 0.15);
  transform: translateY(-2px);
}
.btn-tertiary {
  background: transparent;
  color: #f7b731;
  font-family: "DM Sans", sans-serif;
  font-weight: 700;
  font-size: 15px;
  padding: 13px 28px;
  border-radius: 50px;
  border: 1px solid rgba(247, 183, 49, 0.4);
  cursor: pointer;
  display: inline-flex;
  align-items: center;
  transition: all 0.3s ease;
  backdrop-filter: blur(6px);
}
.btn-tertiary:hover {
  background: rgba(247, 183, 49, 0.1);
  border-color: rgba(247, 183, 49, 0.7);
  box-shadow: 0 0 30px rgba(247, 183, 49, 0.2);
  transform: translateY(-2px);
}
.stats-card {
  background: rgba(255, 255, 255, 0.03);
  border: 1px solid rgba(255, 255, 255, 0.09);
  border-radius: 20px;
  display: flex;
  align-items: center;
  justify-content: space-around;
  flex-wrap: wrap;
  padding: 24px 16px;
  gap: 16px;
  backdrop-filter: blur(10px);
}
.stat-item {
  display: flex;
  align-items: center;
  gap: 12px;
}
.stat-info {
  display: flex;
  flex-direction: column;
}
.stat-value {
  font-family: "Syne", sans-serif;
  font-size: 22px;
  font-weight: 700;
  color: #f0f4ff;
  line-height: 1;
}
.stat-unit {
  font-size: 14px;
  font-weight: 600;
  color: rgba(200, 215, 240, 0.65);
  margin-left: 3px;
}
.stat-label {
  font-size: 12px;
  font-weight: 500;
  letter-spacing: 0.5px;
  color: rgba(200, 215, 240, 0.6);
  text-transform: uppercase;
  margin-top: 4px;
}
.section-label {
  font-size: 11px;
  font-weight: 600;
  letter-spacing: 3px;
  color: rgba(200, 215, 240, 0.4);
  margin-bottom: 20px;
  text-transform: uppercase;
}
.feature-card {
  background: rgba(255, 255, 255, 0.03);
  border: 1px solid rgba(255, 255, 255, 0.07);
  border-radius: 16px;
  padding: 24px 20px;
  height: 100%;
  transition: all 0.3s ease;
}
.feature-card:hover {
  background: rgba(255, 255, 255, 0.06);
  border-color: rgba(78, 205, 196, 0.2);
  transform: translateY(-4px);
}
.feature-icon-wrap {
  width: 52px;
  height: 52px;
  border-radius: 14px;
  display: flex;
  align-items: center;
  justify-content: center;
  margin-bottom: 16px;
}
.feature-title {
  font-family: "Syne", sans-serif;
  font-size: 17px;
  font-weight: 700;
  color: #f0f4ff;
  margin-bottom: 10px;
  line-height: 1.3;
}
.feature-desc {
  font-size: 14px;
  font-weight: 400;
  color: rgba(200, 215, 240, 0.65);
  line-height: 1.7;
}
.sensor-grid {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
}
.sensor-chip {
  display: flex;
  align-items: center;
  gap: 8px;
  background: rgba(255, 255, 255, 0.04);
  border: 1px solid rgba(255, 255, 255, 0.09);
  border-radius: 50px;
  padding: 9px 18px;
}
.sensor-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  flex-shrink: 0;
}
.sensor-name {
  font-family: "Syne", sans-serif;
  font-size: 14px;
  font-weight: 700;
  color: #f0f4ff;
}
.sensor-type {
  font-size: 13px;
  font-weight: 400;
  color: rgba(200, 215, 240, 0.6);
}
</style>
