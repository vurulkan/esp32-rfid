#include "rfid.h"

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

#include "messages.h"

namespace app {

namespace {
// SPI bus pins (VSPI default): SCK=18, MISO=19, MOSI=23
constexpr uint8_t kSpiSck = 18;
constexpr uint8_t kSpiMiso = 19;
constexpr uint8_t kSpiMosi = 23;

// Reader #1 pins
constexpr uint8_t kRfid1Ss = 5;
constexpr uint8_t kRfid1Rst = 26;

// Reader #2 pins
constexpr uint8_t kRfid2Ss = 27;
constexpr uint8_t kRfid2Rst = 25;

constexpr TickType_t kPollDelay = pdMS_TO_TICKS(40);

SPIClass spi_bus(VSPI);
MFRC522 rfid1(kRfid1Ss, kRfid1Rst);
MFRC522 rfid2(kRfid2Ss, kRfid2Rst);

void uid_to_hex(const MFRC522::Uid& uid, char* out, size_t out_len) {
  if (!out || out_len == 0) {
    return;
  }
  size_t idx = 0;
  for (byte i = 0; i < uid.size && idx + 2 < out_len; i++) {
    snprintf(out + idx, out_len - idx, "%02X", uid.uidByte[i]);
    idx += 2;
  }
  out[out_len - 1] = '\0';
}

bool read_card(MFRC522& reader, uint8_t reader_id, QueueHandle_t queue) {
  if (!reader.PICC_IsNewCardPresent() || !reader.PICC_ReadCardSerial()) {
    return false;
  }

  RfidEvent event{};
  event.reader_id = reader_id;
  uid_to_hex(reader.uid, event.uid, sizeof(event.uid));
  xQueueSend(queue, &event, 0);

  reader.PICC_HaltA();
  reader.PCD_StopCrypto1();
  return true;
}

} // namespace

void rfid_task(void* param) {
  auto* queues = static_cast<AppQueues*>(param);
  spi_bus.begin(kSpiSck, kSpiMiso, kSpiMosi);

  rfid1.PCD_Init();
  rfid2.PCD_Init();

  for (;;) {
    read_card(rfid1, 1, queues->rfid_queue);
    read_card(rfid2, 2, queues->rfid_queue);
    vTaskDelay(kPollDelay);
  }
}

} // namespace app
