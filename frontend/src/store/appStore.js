import { defineStore } from "pinia";
import { ref } from "vue";

export const useAppStore = defineStore(
  "app",
  () => {
    /*  
    The composition API way of defining a Pinia store
    ref() s become state properties
    computed() s become getters
    function() s become actions  
    */

    // STATES
    const latest = ref(null);

    const history = ref([]);

    // ACTIONS
    async function fetchWithTimeout(url) {
      const controller = new AbortController();
      const signal = controller.signal;

      // Timeout after 60 seconds
      const timer = setTimeout(() => controller.abort(), 60000);

      try {
        const response = await fetch(url, { method: "GET", signal });

        clearTimeout(timer);

        if (!response.ok) {
          console.warn(`Request failed: ${url}`, await response.text());
          return null;
        }

        return await response.json();
      } catch (err) {
        console.error(`Fetch error (${url}):`, err.message);
        return null;
      }
    }

    async function fetchLatest() {
      const data = await fetchWithTimeout(`/api/latest`);

      if (!data || data.error) {
        console.warn("fetchLatest returned no data");
        return null;
      }

      latest.value = data;
      return data;
    }

    async function fetchHistory(limit = 30) {
      const data = await fetchWithTimeout(`/api/history?limit=${limit}`);

      if (!data || !Array.isArray(data)) {
        console.warn("fetchHistory returned no data");
        return [];
      }

      history.value = data;
      return data;
    }

    async function fetchRange(start, end) {
      const data = await fetchWithTimeout(`/api/range/get/${start}/${end}`);

      if (!data || data.status !== "found") {
        console.log("fetchRange returned no data");
        return [];
      }

      return data.data;
    }

    async function fetchStats(field, start, end) {
      const data = await fetchWithTimeout(
        `/api/mmar/field?field=${field}&start=${start}&end=${end}`,
      );

      if (!data || data.error) {
        console.log("fetchStats returned no data");
        return null;
      }

      return data;
    }

    async function fetchDistribution(field, start, end) {
      const data = await fetchWithTimeout(
        `/api/stats/distribution?field=${field}&start=${start}&end=${end}`,
      );

      if (!data || data.error) {
        console.log("fetchDistribution returned no data");
        return [];
      }

      return data;
    }

    return {
      // EXPORTS
      fetchRange,
      fetchStats,
      fetchDistribution,
      latest,
      fetchLatest,
      history,
      fetchHistory,
    };
  },
  { persist: true },
);
