```bash
[Sensor Update] 2026-02-19 11:00:06
Temperature: 24.60 °C Status: Comfortable
Humidity: 49.10 %RH Status: Comfortable
Pressure: 1013.64 hPa Status: Normal
```

### Status Interpretation

| Parameter    | Low                   | Normal / Comfortable        | High / Alert              |
|--------------|----------------------|-----------------------------|---------------------------|
| Temperature  | < 18 °C (Cold)       | 18–26 °C (Comfortable)      | > 26 °C (Hot)            |
| Humidity     | < 30 %RH (Dry)       | 30–60 %RH (Comfortable)     | > 60 %RH (Humid)         |
| Pressure     | < 1000 hPa (Low)     | 1000–1020 hPa (Normal)      | > 1020 hPa (High)        |

> The built-in LED blinks on each sensor update to indicate a new measurement.

