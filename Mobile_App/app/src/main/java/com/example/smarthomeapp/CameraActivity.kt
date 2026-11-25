package com.example.smarthomeapp

import android.content.Context
import android.os.Bundle
import android.webkit.WebView
import android.webkit.WebViewClient
import android.widget.EditText
import android.widget.ImageView
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.widget.AppCompatButton

class CameraActivity : AppCompatActivity() {

    private lateinit var webViewCamera: WebView
    private lateinit var etStreamUrl: EditText

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_camera)

        val defaultUrl = "http://172.20.10.4:5000/video_feed"
        val prefs = getSharedPreferences("SmartHomePrefs", Context.MODE_PRIVATE)
        val rawUrl = prefs.getString("rpi_camera_url", defaultUrl) ?: defaultUrl
        val savedUrl = if (rawUrl == "http://192.168.1.100:5000/video_feed") defaultUrl else rawUrl

        findViewById<ImageView>(R.id.btnBack).setOnClickListener { finish() }

        etStreamUrl = findViewById(R.id.etStreamUrl)
        val btnConnectStream = findViewById<AppCompatButton>(R.id.btnConnectStream)
        webViewCamera = findViewById(R.id.webViewCamera)

        etStreamUrl.setText(savedUrl)

        webViewCamera.webViewClient = WebViewClient()
        webViewCamera.settings.apply {
            loadWithOverviewMode = true
            useWideViewPort = true
            builtInZoomControls = true
            displayZoomControls = false
        }

        webViewCamera.loadUrl(savedUrl)

        btnConnectStream.setOnClickListener {
            val newUrl = etStreamUrl.text.toString().trim()
            if (newUrl.isNotEmpty()) {
                prefs.edit().putString("rpi_camera_url", newUrl).apply()
                webViewCamera.stopLoading()
                webViewCamera.loadUrl(newUrl)
            }
        }
    }

    override fun onDestroy() {
        webViewCamera.stopLoading()
        webViewCamera.destroy()
        super.onDestroy()
    }
}