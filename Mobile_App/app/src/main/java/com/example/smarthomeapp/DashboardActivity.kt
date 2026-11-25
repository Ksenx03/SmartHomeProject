package com.example.smarthomeapp

import android.content.Context
import android.content.Intent
import android.graphics.Color
import android.os.Bundle
import android.widget.ImageView
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.app.AppCompatDelegate
import androidx.cardview.widget.CardView
import org.json.JSONObject
import java.util.Locale

class DashboardActivity : AppCompatActivity() {

    private lateinit var mqttHandler: MqttHandler

    private lateinit var tvDashboardTemp: TextView
    private lateinit var tvDashboardTempOut: TextView
    private lateinit var tvDashboardHum: TextView
    private lateinit var tvDashboardHumOut: TextView
    private lateinit var tvDashboardLight: TextView
    private lateinit var tvDashboardPress: TextView

    private lateinit var statusLight: TextView
    private lateinit var statusHeating: TextView
    private lateinit var statusVent: TextView
    private lateinit var statusSecurity: TextView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_dashboard)

        val themeBtn = findViewById<ImageView>(R.id.ivThemeToggle)
        val ivLogout = findViewById<ImageView>(R.id.ivLogout)
        val ivCloud = findViewById<ImageView>(R.id.ivConnectionStatus)

        tvDashboardTemp = findViewById(R.id.tvDashboardTemp)
        tvDashboardTempOut = findViewById(R.id.tvDashboardTempOut)
        tvDashboardHum = findViewById(R.id.tvDashboardHum)
        tvDashboardHumOut = findViewById(R.id.tvDashboardHumOut)
        tvDashboardLight = findViewById(R.id.tvDashboardLight)
        tvDashboardPress = findViewById(R.id.tvDashboardPress)

        statusLight = findViewById(R.id.statusLight)
        statusHeating = findViewById(R.id.statusHeating)
        statusVent = findViewById(R.id.statusVent)
        statusSecurity = findViewById(R.id.statusSecurity)

        val btnHeating = findViewById<CardView>(R.id.btnHeating)
        val btnVentilation = findViewById<CardView>(R.id.btnVentilation)
        val btnSensors = findViewById<CardView>(R.id.btnWater)
        val btnAccess = findViewById<CardView>(R.id.btnAccess)
        val btnLighting = findViewById<CardView>(R.id.btnLighting)
        val btnBlinds = findViewById<CardView>(R.id.btnBlinds)
        val btnCamera = findViewById<CardView>(R.id.btnCamera)

        mqttHandler = MqttHandler(this)

        mqttHandler.connect(
            onConnected = {
                runOnUiThread {
                    ivCloud.setColorFilter(Color.parseColor("#4CAF50"))
                }
                mqttHandler.subscribe("makieta/czujniki/srodowisko/#")
                mqttHandler.subscribe("makieta/czujniki/swiatlo")
                mqttHandler.subscribe("makieta/status/systemy")
                mqttHandler.subscribe("makieta/serwo/status")
            },
            onMessage = { msg ->
                runOnUiThread {
                    parseDashboardData(msg)
                }
            }
        )

        btnSensors.setOnClickListener { startActivity(Intent(this, SensorsActivity::class.java)) }
        btnAccess.setOnClickListener { startActivity(Intent(this, AccessActivity::class.java)) }
        btnLighting.setOnClickListener { startActivity(Intent(this, LightingActivity::class.java)) }
        btnVentilation.setOnClickListener { startActivity(Intent(this, VentilationActivity::class.java)) }
        btnHeating.setOnClickListener { startActivity(Intent(this, HeatingActivity::class.java)) }
        btnBlinds.setOnClickListener { startActivity(Intent(this, BlindsActivity::class.java)) }
        btnCamera.setOnClickListener { startActivity(Intent(this, CameraActivity::class.java)) }

        ivLogout.setOnClickListener {
            startActivity(Intent(this, MainActivity::class.java))
            finish()
        }

        themeBtn.setOnClickListener {
            if (AppCompatDelegate.getDefaultNightMode() == AppCompatDelegate.MODE_NIGHT_YES) {
                AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_NO)
            } else {
                AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_YES)
            }
        }
    }

    override fun onResume() {
        super.onResume()
        loadCachedSystemStatuses()
    }

    private fun formatTwoDecimals(value: Double): String {
        return String.format(Locale.US, "%.2f", value)
    }

    private fun loadCachedSystemStatuses() {
        val prefs = getSharedPreferences("SmartHomePrefs", Context.MODE_PRIVATE)

        val lightState = prefs.getString("status_light", "OFF") ?: "OFF"
        val heatingState = prefs.getString("status_heating", "OFF") ?: "OFF"
        val ventState = prefs.getString("status_vent", "OFF") ?: "OFF"
        val securityState = prefs.getString("status_security", "DISARMED") ?: "DISARMED"

        updateSystemStatus(statusLight, lightState)
        updateSystemStatus(statusHeating, heatingState)
        updateSystemStatus(statusVent, ventState)

        statusSecurity.text = securityState
        if (securityState == "ARMED" || securityState == "ALARM") {
            statusSecurity.setTextColor(Color.parseColor("#FF5252"))
        } else {
            statusSecurity.setTextColor(Color.parseColor("#808080"))
        }
    }

    private fun parseDashboardData(msg: String) {
        try {
            val cleanMsg = msg.trim()
            if (cleanMsg.startsWith("{")) {
                val json = JSONObject(cleanMsg)
                val prefs = getSharedPreferences("SmartHomePrefs", Context.MODE_PRIVATE)

                // Wnętrze: Temperatura, Wilgotność, Ciśnienie (2 miejsca po przecinku)
                if (json.has("indoor")) {
                    val indoor = json.getJSONObject("indoor")
                    if (indoor.has("temperature")) {
                        val t = indoor.optDouble("temperature", 0.0)
                        prefs.edit().putFloat("cached_indoor_temp", t.toFloat()).apply()
                        tvDashboardTemp.text = "${formatTwoDecimals(t)} \u00B0C"
                    }
                    if (indoor.has("humidity")) {
                        val h = indoor.optDouble("humidity", 0.0)
                        tvDashboardHum.text = "${formatTwoDecimals(h)} %"
                    }
                    if (indoor.has("pressure")) {
                        val p = indoor.optDouble("pressure", 0.0)
                        tvDashboardPress.text = "${formatTwoDecimals(p)} hPa"
                    }
                }

                // Zewnątrz: Temperatura, Wilgotność (2 miejsca po przecinku)
                if (json.has("outdoor")) {
                    val outdoor = json.getJSONObject("outdoor")
                    if (outdoor.has("temperature")) {
                        val tOut = outdoor.optDouble("temperature", 0.0)
                        tvDashboardTempOut.text = "${formatTwoDecimals(tOut)} \u00B0C"
                    }
                    if (outdoor.has("humidity")) {
                        val hOut = outdoor.optDouble("humidity", 0.0)
                        tvDashboardHumOut.text = "${formatTwoDecimals(hOut)} %"
                    }
                }

                // Światło (2 miejsca po przecinku)
                if (json.has("lux")) {
                    val light = json.optDouble("lux", 0.0)
                    prefs.edit().putFloat("cached_lux", light.toFloat()).apply()
                    tvDashboardLight.text = "${formatTwoDecimals(light)} lx"
                }

                // Synchronizacja pozycji rolet w tle
                if (json.optString("target") == "zaluzja" && json.has("position")) {
                    prefs.edit().putInt("blinds_position", json.getInt("position")).apply()
                }
                if (json.has("blinds_pos")) {
                    prefs.edit().putInt("blinds_position", json.getInt("blinds_pos")).apply()
                }

                // Statusy Systemów
                if (json.has("status_light")) {
                    val state = json.getString("status_light")
                    prefs.edit().putString("status_light", state).apply()
                    updateSystemStatus(statusLight, state)
                }
                if (json.has("status_heating")) {
                    val lastCmdTime = prefs.getLong("last_heating_cmd_time", 0L)
                    if (System.currentTimeMillis() - lastCmdTime > 2500) {
                        val state = json.getString("status_heating")
                        prefs.edit().putString("status_heating", state).apply()
                        updateSystemStatus(statusHeating, state)
                    }
                }
                if (json.has("status_vent")) {
                    val state = json.getString("status_vent")
                    prefs.edit().putString("status_vent", state).putString("fanState", state).apply()
                    updateSystemStatus(statusVent, state)
                }
                if (json.has("fan_speed")) {
                    val spd = json.getInt("fan_speed")
                    if (spd > 0) prefs.edit().putInt("fanSpeed", spd).apply()
                }
                if (json.has("status_security")) {
                    val state = json.getString("status_security")
                    prefs.edit().putString("status_security", state).apply()
                    statusSecurity.text = state
                    if (state == "ARMED" || state == "ALARM") {
                        statusSecurity.setTextColor(Color.parseColor("#FF5252"))
                    } else {
                        statusSecurity.setTextColor(Color.parseColor("#808080"))
                    }
                }
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    private fun updateSystemStatus(textView: TextView, status: String) {
        textView.text = status
        if (status == "ON" || status == "RUNNING") {
            textView.setTextColor(Color.parseColor("#4CAF50"))
        } else {
            textView.setTextColor(Color.parseColor("#808080"))
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        mqttHandler.disconnect()
    }
}