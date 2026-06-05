package com.example.smarthomeapp

import android.os.Bundle
import android.widget.Button
import android.widget.ImageView
import android.widget.SeekBar
import android.widget.TextView
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import org.json.JSONObject

class HeatingActivity : AppCompatActivity() {

    // Zostawiamy tylko jedną zmienną, która faktycznie istnieje w XML
    private lateinit var tvCurrentTemp: TextView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContentView(R.layout.activity_heating)

        // Przypisanie prawidłowego ID z pliku activity_heating.xml
        tvCurrentTemp = findViewById(R.id.tvCurrentTemp)

        val btnBack = findViewById<ImageView>(R.id.btnBack)
        val sbTemperature = findViewById<SeekBar>(R.id.sbTemperature)
        val tvTargetTemp = findViewById<TextView>(R.id.tvTargetTemp)
        val btnEco = findViewById<Button>(R.id.btnEco)
        val btnComfort = findViewById<Button>(R.id.btnComfort)
        val btnBoost = findViewById<Button>(R.id.btnBoost)

        btnBack.setOnClickListener { finish() }

        sbTemperature.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                tvTargetTemp.text = "$progress°C"
            }
            override fun onStartTrackingTouch(seekBar: SeekBar?) {}
            override fun onStopTrackingTouch(seekBar: SeekBar?) {}
        })

        btnEco.setOnClickListener {
            sbTemperature.progress = 18
            tvTargetTemp.text = "18°C"
        }
        btnComfort.setOnClickListener {
            sbTemperature.progress = 23
            tvTargetTemp.text = "23°C"
        }
        btnBoost.setOnClickListener {
            sbTemperature.progress = 28
            tvTargetTemp.text = "28°C"
        }

        // UŻYWAMY TWOJEJ ORYGINALNEJ LOGIKI POŁĄCZENIA!
        val mqttHandler = MqttHandler(this)
        mqttHandler.connect(
            onConnected = {
                // Nasłuchujemy tylko na środowisko wewnętrzne, bo tylko to nas tu interesuje
                mqttHandler.subscribe("makieta/czujniki/srodowisko/wew")
            },
            onMessage = { msg ->
                updateEnvironmentData(msg)
            }
        )
    }

    private fun updateEnvironmentData(jsonPayload: String) {
        try {
            val jsonObject = JSONObject(jsonPayload)

            // Sprawdzamy czy mamy dane "indoor"
            if (jsonObject.has("indoor")) {
                val indoor = jsonObject.getJSONObject("indoor")
                val inTemp = indoor.optDouble("temperature", 0.0)

                // Aktualizujemy tylko temperaturę, bo na inne dane nie ma tu miejsca
                runOnUiThread {
                    tvCurrentTemp.text = String.format("%.1f °C", inTemp)
                }
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }
}