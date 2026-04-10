package com.example.smarthomeapp

import android.graphics.Color
import android.os.Bundle
import android.view.View
import android.widget.ImageView
import android.widget.SeekBar
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity

class LightingActivity : AppCompatActivity() {

    private lateinit var mqttHandler: MqttHandler
    private var isLightOn = "OFF"
    private var brightness = 255
    private var r = 255; private var g = 255; private var b = 255

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_lighting)

        // Используем то же подключение
        mqttHandler = MqttHandler(this)
        mqttHandler.connect { }

        val ivLightButton: ImageView = findViewById(R.id.ivLightButton)
        val tvStatus: TextView = findViewById(R.id.tvLightStatus)
        val seekBar: SeekBar = findViewById(R.id.seekBarBrightness)

        ivLightButton.setOnClickListener {
            if (isLightOn == "OFF") {
                isLightOn = "ON"
                tvStatus.text = "ON"
                tvStatus.setTextColor(Color.GREEN)
                ivLightButton.setColorFilter(Color.YELLOW)
            } else {
                isLightOn = "OFF"
                tvStatus.text = "OFF"
                tvStatus.setTextColor(Color.RED)
                ivLightButton.clearColorFilter()
            }
            sendCommand() // Шлем мгновенно
        }

        seekBar.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(p0: SeekBar?, progress: Int, fromUser: Boolean) {
                if (fromUser) { // Шлем только если реально двигаем пальцем
                    brightness = progress
                    sendCommand()
                }
            }
            override fun onStartTrackingTouch(p0: SeekBar?) {}
            override fun onStopTrackingTouch(p0: SeekBar?) {}
        })

        findViewById<View>(R.id.btnBack).setOnClickListener { finish() }
        findViewById<View>(R.id.colorRed).setOnClickListener { setRGB(255, 0, 0) }
        findViewById<View>(R.id.colorGreen).setOnClickListener { setRGB(0, 255, 0) }
        findViewById<View>(R.id.colorBlue).setOnClickListener { setRGB(0, 0, 255) }
        findViewById<View>(R.id.colorWhite).setOnClickListener { setRGB(255, 255, 255) }
        // Новые обработчики
        findViewById<View>(R.id.colorOrange).setOnClickListener { setRGB(255, 140, 0) }
        findViewById<View>(R.id.colorYellow).setOnClickListener { setRGB(255, 255, 0) }
        findViewById<View>(R.id.colorCyan).setOnClickListener { setRGB(0, 255, 255) }
        findViewById<View>(R.id.colorMagenta).setOnClickListener { setRGB(255, 0, 255) }
        findViewById<View>(R.id.colorPurple).setOnClickListener { setRGB(128, 0, 128) }
        findViewById<View>(R.id.colorPink).setOnClickListener { setRGB(255, 192, 203) }
        findViewById<View>(R.id.colorTurquoise).setOnClickListener { setRGB(64, 224, 208) }
        findViewById<View>(R.id.colorWarm).setOnClickListener { setRGB(255, 228, 181) }
    }

    private fun setRGB(newR: Int, newG: Int, newB: Int) {
        r = newR; g = newG; b = newB
        sendCommand()
    }

    private fun sendCommand() {
        val topic = "makieta/oswietlenie/ustaw"
        val payload = "{\"target\":\"wew\",\"state\":\"$isLightOn\",\"brightness\":$brightness,\"color\":{\"r\":$r,\"g\":$g,\"b\":$b}}"
        mqttHandler.publish(topic, payload)
    }
}