package com.example.smarthomeapp

import android.content.Context
import android.content.SharedPreferences
import android.os.Bundle
import android.view.View
import android.widget.Button
import android.widget.ImageView
import android.widget.SeekBar
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import org.json.JSONObject

class BlindsActivity : AppCompatActivity() {

    private lateinit var mqttHandler: MqttHandler
    private val topicSet = "makieta/serwo/ustaw"
    private val topicStatus = "makieta/serwo/status"
    private val topicSystemStatus = "makieta/status/systemy"

    private lateinit var sharedPref: SharedPreferences
    private val PREFS_NAME = "SmartHomePrefs"
    private val KEY_BLINDS_POS = "blinds_position"

    private lateinit var blindsView: View
    private lateinit var sbBlinds: SeekBar
    private lateinit var tvStatus: TextView
    private var isUserDragging = false

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_blinds)

        sharedPref = getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)

        blindsView = findViewById(R.id.blindsView)
        sbBlinds = findViewById(R.id.sbBlinds)
        tvStatus = findViewById(R.id.tvStatus)
        val btnBack = findViewById<ImageView>(R.id.btnBack)

        btnBack.setOnClickListener { finish() }

        // Wczytanie zapamiętanej pozycji od razu przy otwarciu ekranu
        val savedPosition = sharedPref.getInt(KEY_BLINDS_POS, 0)
        blindsView.post {
            applyUIUpdate(savedPosition)
        }

        mqttHandler = MqttHandler(this)
        mqttHandler.connect(
            onConnected = {
                mqttHandler.subscribe(topicStatus)
                mqttHandler.subscribe(topicSystemStatus)
            },
            onMessage = { payload ->
                runOnUiThread {
                    if (!isUserDragging) {
                        handleIncomingStatus(payload)
                    }
                }
            }
        )

        sbBlinds.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                if (fromUser) {
                    updateBlindsVisual(progress)
                }
            }

            override fun onStartTrackingTouch(seekBar: SeekBar?) {
                isUserDragging = true
            }

            override fun onStopTrackingTouch(seekBar: SeekBar?) {
                isUserDragging = false
                seekBar?.let {
                    saveBlindsPosition(it.progress)
                    sendBlindsCommand(it.progress)
                }
            }
        })

        findViewById<Button>(R.id.btnOpen).setOnClickListener { applyPreset(0) }
        findViewById<Button>(R.id.btnClosed).setOnClickListener { applyPreset(100) }
        findViewById<Button>(R.id.btn30).setOnClickListener { applyPreset(30) }
        findViewById<Button>(R.id.btn60).setOnClickListener { applyPreset(60) }
    }

    private fun updateBlindsVisual(percentage: Int) {
        val safePercentage = percentage.coerceIn(0, 100)
        tvStatus.text = "Closed: $safePercentage%"

        val parentHeight = (blindsView.parent as? View)?.height ?: 0
        val maxWindowHeightPx = if (parentHeight > 0) parentHeight.toFloat() else (250 * resources.displayMetrics.density)

        val params = blindsView.layoutParams
        // Gdy 0% zostawiamy 4px paska u góry, żeby widok był widoczny
        params.height = ((maxWindowHeightPx * (safePercentage / 100f)).toInt()).coerceAtLeast(4)
        blindsView.layoutParams = params
        blindsView.requestLayout()
    }

    private fun applyUIUpdate(percent: Int) {
        val safePercent = percent.coerceIn(0, 100)
        sbBlinds.progress = safePercent
        updateBlindsVisual(safePercent)
    }

    private fun saveBlindsPosition(percent: Int) {
        sharedPref.edit().putInt(KEY_BLINDS_POS, percent.coerceIn(0, 100)).apply()
    }

    private fun applyPreset(percent: Int) {
        val safePercent = percent.coerceIn(0, 100)
        applyUIUpdate(safePercent)
        saveBlindsPosition(safePercent)
        sendBlindsCommand(safePercent)
    }

    private fun handleIncomingStatus(jsonString: String) {
        try {
            val clean = jsonString.trim()
            if (!clean.startsWith("{")) return
            val json = JSONObject(clean)

            // 1. Odbiór z makieta/serwo/status
            if (json.optString("target") == "zaluzja" && json.has("position")) {
                val newPos = json.getInt("position")
                applyUIUpdate(newPos)
                saveBlindsPosition(newPos)
            }
            // 2. Odbiór z makieta/status/systemy (stała synchronizacja co 5s)
            else if (json.has("blinds_pos")) {
                val newPos = json.getInt("blinds_pos")
                applyUIUpdate(newPos)
                saveBlindsPosition(newPos)
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    private fun sendBlindsCommand(percent: Int) {
        val payload = """{"target":"zaluzja","position":$percent}"""
        mqttHandler.publish(topicSet, payload, retained = true)
    }

    override fun onDestroy() {
        super.onDestroy()
        mqttHandler.disconnect()
    }
}