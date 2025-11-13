package com.switchbot.doorbell

import android.nfc.Tag
import android.os.Bundle
import android.os.Handler
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.tooling.preview.Preview
import com.switchbot.doorbell.ui.theme.DoorbellTheme
import kotlinx.coroutines.Runnable

class MainActivity : ComponentActivity() {
    val TAG: String = "MainActivityA"

    var isBind = false
    var Handler: Handler? = null
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            DoorbellTheme {
                Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                    Greeting(
                        name = "Android",
                        modifier = Modifier.padding(innerPadding)
                    )
                }
            }
        }
        Handler = Handler(mainLooper)
    }

    override fun onResume() {
        super.onResume()
        Log.d(TAG, "onResume: ")



        val tcpClient = TcpClient()
        tcpClient.init("10.8.41.216",1600)
        val  connect = tcpClient.connect()
        Log.d(TAG, "onResume: connect $connect")
        val mac = "B0E9FE8B3D0C"
        val localmac = "B0E9FEF6E749"
        val key = "VQikblIrZXQ42Hng"

        isBind = tcpClient.bindDevice("10.8.41.216", Utils.hexStringToBytes(mac), Utils.hexStringToBytes(localmac),
            key.toByteArray(Charsets.UTF_8)) == 0
        Log.d(TAG, "onResume: isBind $isBind")

        val keyStr = tcpClient.aesKey?.toString(Charsets.US_ASCII)
        val ivStr = tcpClient.aesIv?.toString(Charsets.US_ASCII)
        Log.d(TAG, "onResume: get key=$keyStr iv=$ivStr")

        if(isBind){
                    val kcp = KcpClient
////        kcp.start("10.111.92.39", 8888, 1234)
        kcp.start("10.8.41.216", 43210, 1234)
//// 发送数据
        kcp.send("来自ai hub  Hello from Android".toByteArray())
////        Handler?.postDelayed(mRunnable,500)
        }
    }
    var mRunnable = object : Runnable {
        override fun run() {
            // 这里放需要定时执行的代码
            // 例如，发送心跳包或检查连接状态
            // 然后再次安排下一次执行
            KcpClient.send("来自服务器B Message after 500 seconds".toByteArray())
            Handler?.postDelayed(this, 500) // 每隔1秒执行一次
        }
    }

    override fun onStop() {
        super.onStop()

    }
}


@Composable
fun Greeting(name: String, modifier: Modifier = Modifier) {
    Text(
        text = "Hello $name!",
        modifier = modifier
    )
}

@Preview(showBackground = true)
@Composable
fun GreetingPreview() {
    DoorbellTheme {
        Greeting("Android")
    }
}