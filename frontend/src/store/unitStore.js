import { defineStore } from "pinia";
import { ref, reactive } from "vue";

export const useUnitStore = defineStore(
  "units",
  () => {
    const units = reactive({
      tempF: false, // false = °C,  true = °F
      pressBar: false, // false = hPa, true = bar
      altFt: false, // false = m,   true = ft
    });

    function setUnit(key, value) {
      if (key in units) units[key] = value;
    }

    function convertTemp(celsius) {
      if (celsius == null) return null;
      return units.tempF
        ? parseFloat(((celsius * 9) / 5 + 32).toFixed(1))
        : parseFloat(celsius.toFixed(1));
    }

    function convertPress(hpa) {
      if (hpa == null) return null;
      return units.pressBar
        ? parseFloat((hpa / 1000).toFixed(4))
        : parseFloat(hpa.toFixed(1));
    }

    function convertAlt(meters) {
      if (meters == null) return null;
      return units.altFt
        ? parseFloat((meters * 3.28084).toFixed(1))
        : parseFloat(meters.toFixed(1));
    }

    function tempUnit() {
      return units.tempF ? "°F" : "°C";
    }
    function pressUnit() {
      return units.pressBar ? "bar" : "hPa";
    }
    function altUnit() {
      return units.altFt ? "ft" : "m";
    }

    return {
      units,
      setUnit,
      convertTemp,
      convertPress,
      convertAlt,
      tempUnit,
      pressUnit,
      altUnit,
    };
  },
  { persist: true },
);
