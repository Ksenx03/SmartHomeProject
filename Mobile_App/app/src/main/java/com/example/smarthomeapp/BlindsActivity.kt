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
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken
import org.eclipse.paho.client.mqttv3.MqttCallback
import org.eclipse.paho.client.mqttv3.MqttClient
import org.eclipse.paho.client.mqttv3.MqttMessage
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence
import org.json.JSONObject

class BlindsActivity : AppCompatActivity() {

    private var mqttClient: MqttClient? = null
    private val serverUri = "tcp://172.20.10.4:1883"
    private val topicSet = "makieta/serwo/ustaw"
    private val topicStatus = "makieta/serwo/status"

    private lateinit var sharedPref: SharedPreferences
    private val PREFS_NAME = "SmartHomePrefs"
    private val KEY_BLINDS_POS = "blinds_position"

    private lateinit var blindsView: View
    private lateinit var sbBlinds: SeekBar
    private lateinit var tvStatus: TextView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_blinds)

        sharedPref = getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)

        blindsView = findViewById(R.id.blindsView)
        sbBlinds = findViewById(R.id.sbBlinds)
        tvStatus = findViewById(R.id.tvStatus)
        val btnBack = findViewById<ImageView>(R.id.btnBack)

        btnBack.setOnClickListener { finish() }

        connectMqtt()

        // Слушатель ручного перемещения ползунка
        sbBlinds.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                if (fromUser) {
                    updateBlindsVisual(progress)
                }
            }

            override fun onStartTrackingTouch(seekBar: SeekBar?) {}

            override fun onStopTrackingTouch(seekBar: SeekBar?) {
                seekBar?.let {
                    saveBlindsPosition(it.progress)
                    sendBlindsCommand(it.progress)
                }
            }
        })

        // Кнопки-пресеты
        findViewById<Button>(R.id.btnOpen).setOnClickListener { applyPreset(0) }
        findViewById<Button>(R.id.btnClosed).setOnClickListener { applyPreset(100) }
        findViewById<Button>(R.id.btn30).setOnClickListener { applyPreset(30) }
        findViewById<Button>(R.id.btn60).setOnClickListener { applyPreset(60) }

        // Загрузка начального положения из памяти
        val savedPosition = sharedPref.getInt(KEY_BLINDS_POS, 100)
        applyUIUpdate(savedPosition)
    }

    private fun updateBlindsVisual(percentage: Int) {
        val safePercentage = percentage.coerceIn(0, 100)
        tvStatus.text = "Closed: $safePercentage%"

        val density = resources.displayMetrics.density
        val maxWindowHeightPx = 250 * density

        val params = blindsView.layoutParams
        params.height = (maxWindowHeightPx * (safePercentage / 100f)).toInt()
        blindsView.layoutParams = params
    }

    private fun applyUIUpdate(percent: Int) {
        val safePercent = percent.coerceIn(0, 100)
        sbBlinds.progress = safePercent
        updateBlindsVisual(safePercent)
    }

    private fun saveBlindsPosition(percent: Int) {
        sharedPref.edit().putInt(KEY_BLINDS_POS, percent).apply()
    }

    private fun applyPreset(percent: Int) {
        val safePercent = percent.coerceIn(0, 100)
        applyUIUpdate(safePercent)
        saveBlindsPosition(safePercent)
        sendBlindsCommand(safePercent)
    }

    private fun connectMqtt() {
        Thread {
            try {
                val clientId = "BlindsApp_" + System.currentTimeMillis()
                mqttClient = MqttClient(serverUri, clientId, MemoryPersistence())

                mqttClient?.setCallback(object : MqttCallback {
                    override fun connectionLost(cause: Throwable?) {}

                    override fun messageArrived(topic: String?, message: MqttMessage?) {
                        if (topic == topicStatus) {
                            val payload = String(message?.payload ?: return)
                            handleIncomingStatus(payload)
                        }
                    }

                    override fun deliveryComplete(token: IMqttDeliveryToken?) {}
                })

                mqttClient?.connect()
                mqttClient?.subscribe(topicStatus)
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }.start()
    }

    private fun handleIncomingStatus(jsonString: String) {
        try {
            val json = JSONObject(jsonString)
            if (json.optString("target") == "zaluzja" && json.has("position")) {
                val newPos = json.getInt("position")
                runOnUiThread {
                    applyUIUpdate(newPos)
                    saveBlindsPosition(newPos)
                }
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    private fun sendBlindsCommand(percent: Int) {
        Thread {
            try {
                if (mqttClient?.isConnected == true) {
                    val payload = """{"target":"zaluzja","position":$percent}"""
                    val message = MqttMessage(payload.toByteArray())
                    mqttClient?.publish(topicSet, message)
                }
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }.start()
    }

    override fun onDestroy() {
        super.onDestroy()
        Thread {
            try {
                mqttClient?.disconnect()
                mqttClient?.close()
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }.start()
    }
}