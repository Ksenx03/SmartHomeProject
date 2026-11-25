package com.example.smarthomeapp

import android.content.Context
import android.os.Handler
import android.os.Looper
import android.util.Log
import org.eclipse.paho.client.mqttv3.*
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence
import java.util.concurrent.ConcurrentLinkedQueue

class MqttHandler(context: Context) {
    // Adres IP zgodny z MQTT_SERVER w Config.h na ESP32
    private val serverUri = "tcp://192.168.1.88:1883"
    private val clientId = "AndroidClient_" + System.currentTimeMillis() + "_" + (0..999).random()
    private val client = MqttAsyncClient(serverUri, clientId, MemoryPersistence())

    private data class PendingMessage(val topic: String, val payload: String, val retained: Boolean)
    private val pendingPublishes = ConcurrentLinkedQueue<PendingMessage>()
    private val pendingSubscriptions = ConcurrentLinkedQueue<String>()

    fun connect(onConnected: () -> Unit, onMessage: (String) -> Unit) {
        val options = MqttConnectOptions().apply {
            isAutomaticReconnect = true
            isCleanSession = true
            connectionTimeout = 5
        }

        client.setCallback(object : MqttCallback {
            override fun messageArrived(topic: String?, message: MqttMessage?) {
                if (message != null) {
                    onMessage(String(message.payload))
                }
            }
            override fun connectionLost(cause: Throwable?) {
                Log.w("MQTT", "Connection lost: ${cause?.message}")
            }
            override fun deliveryComplete(token: IMqttDeliveryToken?) {}
        })

        try {
            client.connect(options, null, object : IMqttActionListener {
                override fun onSuccess(asyncActionToken: IMqttToken?) {
                    Log.d("MQTT", "Connected to $serverUri")
                    flushPendingQueues()
                    onConnected()
                }
                override fun onFailure(asyncActionToken: IMqttToken?, exception: Throwable?) {
                    Log.e("MQTT", "Connect failed: ${exception?.message}")
                }
            })
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    private fun flushPendingQueues() {
        try {
            while (pendingSubscriptions.isNotEmpty()) {
                val topic = pendingSubscriptions.poll() ?: break
                if (client.isConnected) client.subscribe(topic, 0)
            }
            while (pendingPublishes.isNotEmpty()) {
                val item = pendingPublishes.poll() ?: break
                if (client.isConnected) {
                    val msg = MqttMessage(item.payload.toByteArray()).apply {
                        qos = 0
                        isRetained = item.retained
                    }
                    client.publish(item.topic, msg)
                }
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    fun subscribe(topic: String) {
        try {
            if (client.isConnected) {
                client.subscribe(topic, 0)
            } else {
                pendingSubscriptions.add(topic)
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    fun publish(topic: String, payload: String, retained: Boolean = false) {
        try {
            if (client.isConnected) {
                val msg = MqttMessage(payload.toByteArray()).apply {
                    qos = 0
                    isRetained = retained
                }
                client.publish(topic, msg)
            } else {
                pendingPublishes.add(PendingMessage(topic, payload, retained))
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    fun disconnect() {
        // Krótkie opóźnienie gwarantuje, że ostatnia komenda wysłana przed zamknięciem ekranu dotrze do brokera
        Handler(Looper.getMainLooper()).postDelayed({
            try {
                if (client.isConnected) {
                    client.disconnect()
                }
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }, 250)
    }
}