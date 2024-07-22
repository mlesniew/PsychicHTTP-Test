#include <mutex>

#include <Arduino.h>
#include <PsychicHttp.h>
#include <WiFi.h>

PsychicHttpServer server;
PsychicWebSocketHandler websocketHandler;

std::mutex mutex;
std::list<String> buffer;

void setup()
{
   //optional low level setup server config stuff here.
   //server.config is an ESP-IDF httpd_config struct
   //see: https://docs.espressif.com/projects/esp-idf/en/v4.4.6/esp32/api-reference/protocols/esp_http_server.html#_CPPv412httpd_config
   //increase maximum number of uri endpoint handlers (.on() calls)
   Serial.begin(115200);
   server.config.max_uri_handlers = 20;

   Serial.println("Connecting...");

   WiFi.begin();
   while (WiFi.status() != WL_CONNECTED) {
      delay(100);
   }

   Serial.println(WiFi.localIP());
   websocketHandler.onOpen([](PsychicWebSocketClient *client) {});
   websocketHandler.onClose([](PsychicWebSocketClient *client) {});
   websocketHandler.onFrame([](PsychicWebSocketRequest * request, httpd_ws_frame * frame) -> esp_err_t {
        const std::lock_guard<std::mutex> lock(mutex);
        buffer.push_back(String((char*) frame->payload));
	// Serial.printf("[socket] #%d sent: %s\n", request->client()->socket(), (char *)frame->payload);
        return 0;
    });


   //start the server listening on port 80 (standard HTTP port)
   server.listen(80);

   server.on("/ws", &websocketHandler);
}

void loop() {
   delay(100);
   const std::lock_guard<std::mutex> lock(mutex);
   if (buffer.empty()) {
      return;
   }

   /* find out how much data we have buffered */
   size_t buffered_bytes = 0;
   for (const String & s: buffer) {
      buffered_bytes += s.length();
   }
   Serial.printf("Buffered bytes: %u\n", buffered_bytes);

   /* consume all */
   for (; !buffer.empty(); buffer.pop_front()) {
       Serial.println(buffer.front());
   }
}
