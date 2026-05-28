#ifdef FIRMWARE_PARTITION_BRIDGE
/*********************************************************************************************\
 * xdrv_97_partition_bridge.ino - one-shot custom OTA -> Tasmota safeboot/app0 migration
\*********************************************************************************************/

#define XDRV_97 97

#include <HTTPClient.h>
#include <WiFiClient.h>
#include <esp_flash.h>
#include <esp_flash_partitions.h>
#include <esp_image_format.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <mbedtls/md5.h>

#ifndef TASMOTA_MIGRATION_SAFEBOOT_URL
#define TASMOTA_MIGRATION_SAFEBOOT_URL "http://ota.tasmota.com/tasmota32/release/tasmota32c3-safeboot.bin"
#endif

namespace PnkPartitionBridge {

const uint32_t kSafebootSize = 832 * 1024;
const uint32_t kSectorSize = 4096;
bool started = false;
bool finished = false;

struct PartitionEntry {
  esp_partition_info_t info;
};

uint32_t AlignUp(uint32_t value, uint32_t align) {
  return (value + align - 1) & ~(align - 1);
}

bool IsOtaApp(const esp_partition_t* part) {
  return part &&
         part->type == ESP_PARTITION_TYPE_APP &&
         part->subtype >= ESP_PARTITION_SUBTYPE_APP_OTA_MIN &&
         part->subtype <= ESP_PARTITION_SUBTYPE_APP_OTA_MAX;
}

const esp_partition_t* FindOtaSlot(uint8_t ota_index) {
  return esp_partition_find_first(
    ESP_PARTITION_TYPE_APP,
    static_cast<esp_partition_subtype_t>(ESP_PARTITION_SUBTYPE_APP_OTA_MIN + ota_index),
    nullptr);
}

uint32_t BridgeRunningImageSize(const esp_partition_t* running) {
  if (!running) { return 0; }

  esp_partition_pos_t part_pos = {
    .offset = running->address,
    .size = running->size,
  };
  esp_image_metadata_t metadata;
  metadata.start_addr = part_pos.offset;
  if (ESP_OK == esp_image_verify(ESP_IMAGE_VERIFY, &part_pos, &metadata)) {
    return AlignUp(metadata.image_len, kSectorSize);
  }
  return AlignUp(ESP.getSketchSize(), kSectorSize);
}

bool CopyFlash(uint32_t from, uint32_t to, uint32_t size) {
  uint8_t buffer[1024];
  const uint32_t aligned_size = AlignUp(size, kSectorSize);
  AddLog(LOG_LEVEL_INFO, PSTR("PNKBRG: copy 0x%06X -> 0x%06X (%u KB)"), from, to, aligned_size / 1024);

  if (ESP_OK != esp_flash_erase_region(nullptr, to, aligned_size)) {
    AddLog(LOG_LEVEL_ERROR, PSTR("PNKBRG: erase target failed"));
    return false;
  }

  for (uint32_t offset = 0; offset < aligned_size; offset += sizeof(buffer)) {
    const uint32_t chunk = ((aligned_size - offset) > sizeof(buffer)) ? sizeof(buffer) : (aligned_size - offset);
    if (ESP_OK != esp_flash_read(nullptr, buffer, from + offset, chunk)) {
      AddLog(LOG_LEVEL_ERROR, PSTR("PNKBRG: read failed at 0x%06X"), from + offset);
      return false;
    }
    if (ESP_OK != esp_flash_write(nullptr, buffer, to + offset, chunk)) {
      AddLog(LOG_LEVEL_ERROR, PSTR("PNKBRG: write failed at 0x%06X"), to + offset);
      return false;
    }
    delay(1);
  }
  return true;
}

bool RebootFromOta1IfNeeded() {
  const esp_partition_t* running = esp_ota_get_running_partition();
  const esp_partition_t* ota0 = FindOtaSlot(0);
  const esp_partition_t* ota1 = FindOtaSlot(1);

  if (!running || !ota0 || !ota1) {
    AddLog(LOG_LEVEL_ERROR, PSTR("PNKBRG: missing running/ota0/ota1 partition"));
    return false;
  }

  AddLog(LOG_LEVEL_INFO, PSTR("PNKBRG: running=%s subtype=0x%02X addr=0x%06X"), running->label, running->subtype, running->address);
  if (running->subtype != ESP_PARTITION_SUBTYPE_APP_OTA_0) {
    return true;
  }

  const uint32_t image_size = PnkPartitionBridge::BridgeRunningImageSize(running);
  if (!image_size || image_size > ota1->size) {
    AddLog(LOG_LEVEL_ERROR, PSTR("PNKBRG: image too large for ota1 (%u > %u)"), image_size, ota1->size);
    return false;
  }

  if (!CopyFlash(ota0->address, ota1->address, image_size)) {
    return false;
  }

  if (ESP_OK != esp_ota_set_boot_partition(ota1)) {
    AddLog(LOG_LEVEL_ERROR, PSTR("PNKBRG: set ota1 boot failed"));
    return false;
  }

  AddLog(LOG_LEVEL_INFO, PSTR("PNKBRG: copied bridge to ota1, restart"));
  TasmotaGlobal.restart_flag = 2;
  return false;
}

bool DownloadSafebootToOta0() {
  const esp_partition_t* ota0 = FindOtaSlot(0);
  if (!ota0) {
    AddLog(LOG_LEVEL_ERROR, PSTR("PNKBRG: ota0 not found"));
    return false;
  }

  WiFiClient client;
  HTTPClient http;
  AddLog(LOG_LEVEL_INFO, PSTR("PNKBRG: GET %s"), PSTR(TASMOTA_MIGRATION_SAFEBOOT_URL));
  if (!http.begin(client, TASMOTA_MIGRATION_SAFEBOOT_URL)) {
    AddLog(LOG_LEVEL_ERROR, PSTR("PNKBRG: http begin failed"));
    return false;
  }

  const int code = http.GET();
  if (code != 200 && code != 201) {
    AddLog(LOG_LEVEL_ERROR, PSTR("PNKBRG: safeboot GET failed code=%d"), code);
    http.end();
    return false;
  }

  const int total_size = http.getSize();
  if (total_size <= 500000 || static_cast<uint32_t>(total_size) > kSafebootSize) {
    AddLog(LOG_LEVEL_ERROR, PSTR("PNKBRG: invalid safeboot size=%d"), total_size);
    http.end();
    return false;
  }

  if (ESP_OK != esp_flash_erase_region(nullptr, ota0->address, kSafebootSize)) {
    AddLog(LOG_LEVEL_ERROR, PSTR("PNKBRG: erase safeboot region failed"));
    http.end();
    return false;
  }

  WiFiClient* stream = http.getStreamPtr();
  uint8_t buffer[1024];
  uint32_t written = 0;

  while (http.connected() && written < static_cast<uint32_t>(total_size)) {
    const size_t available = stream->available();
    if (!available) {
      delay(1);
      continue;
    }

    const size_t to_read = (available > sizeof(buffer)) ? sizeof(buffer) : available;
    const size_t read_count = stream->readBytes(buffer, to_read);
    if (!read_count) {
      continue;
    }

    if (ESP_OK != esp_flash_write(nullptr, buffer, ota0->address + written, read_count)) {
      AddLog(LOG_LEVEL_ERROR, PSTR("PNKBRG: safeboot write failed at %u"), written);
      http.end();
      return false;
    }
    written += read_count;
    delay(1);
  }

  http.end();
  AddLog(LOG_LEVEL_INFO, PSTR("PNKBRG: safeboot written %u/%d"), written, total_size);
  return written == static_cast<uint32_t>(total_size);
}

void BridgeFillEntry(void* entry_ptr, uint8_t type, uint8_t subtype, uint32_t offset, uint32_t size, const char* label) {
  esp_partition_info_t* entry = static_cast<esp_partition_info_t*>(entry_ptr);
  memset(entry, 0, sizeof(*entry));
  entry->magic = ESP_PARTITION_MAGIC;
  entry->type = type;
  entry->subtype = subtype;
  entry->pos.offset = offset;
  entry->pos.size = size;
  strlcpy(reinterpret_cast<char*>(entry->label), label, sizeof(entry->label));
}

int CompareEntryByOffset(const void* a, const void* b) {
  const PartitionEntry* left = static_cast<const PartitionEntry*>(a);
  const PartitionEntry* right = static_cast<const PartitionEntry*>(b);
  if (left->info.pos.offset < right->info.pos.offset) { return -1; }
  if (left->info.pos.offset > right->info.pos.offset) { return 1; }
  return 0;
}

bool BridgeAddEntry(void* entries_ptr, size_t* count, const void* info_ptr) {
  PartitionEntry* entries = static_cast<PartitionEntry*>(entries_ptr);
  const esp_partition_info_t* info = static_cast<const esp_partition_info_t*>(info_ptr);
  if (*count >= ESP_PARTITION_TABLE_MAX_ENTRIES - 2) {
    return false;
  }
  entries[*count].info = *info;
  (*count)++;
  return true;
}

bool SaveSafebootPartitionTable() {
  const esp_partition_t* ota0 = FindOtaSlot(0);
  const esp_partition_t* ota1 = FindOtaSlot(1);
  const esp_partition_t* otadata = esp_partition_find_first(
    ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, nullptr);

  if (!ota0 || !ota1 || !otadata) {
    AddLog(LOG_LEVEL_ERROR, PSTR("PNKBRG: required partitions missing"));
    return false;
  }

  PartitionEntry entries[ESP_PARTITION_TABLE_MAX_ENTRIES] = {};
  size_t count = 0;
  const uint32_t old_app_end = ota1->address + ota1->size;

  esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, nullptr);
  for (; it != nullptr; it = esp_partition_next(it)) {
    const esp_partition_t* part = esp_partition_get(it);
    if (!part) { continue; }
    if (part->type == ESP_PARTITION_TYPE_APP) { continue; }
    if (part->address >= ota0->address && part->address < old_app_end) { continue; }

    esp_partition_info_t info;
    PnkPartitionBridge::BridgeFillEntry(&info, part->type, part->subtype, part->address, part->size, part->label);
    PnkPartitionBridge::BridgeAddEntry(entries, &count, &info);
  }
  esp_partition_iterator_release(it);

  esp_partition_info_t safeboot;
  PnkPartitionBridge::BridgeFillEntry(&safeboot, ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, ota0->address, kSafebootSize, "safeboot");
  PnkPartitionBridge::BridgeAddEntry(entries, &count, &safeboot);

  esp_partition_info_t app0;
  const uint32_t new_app0_start = ota0->address + kSafebootSize;
  const uint32_t new_app0_size = old_app_end - new_app0_start;
  PnkPartitionBridge::BridgeFillEntry(&app0, ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, new_app0_start, new_app0_size, "app0");
  PnkPartitionBridge::BridgeAddEntry(entries, &count, &app0);

  qsort(entries, count, sizeof(entries[0]), CompareEntryByOffset);

  uint8_t table[0x1000];
  memset(table, 0xFF, sizeof(table));
  uint32_t cursor = 0;
  for (size_t i = 0; i < count; i++) {
    memcpy(table + cursor, &entries[i].info, sizeof(esp_partition_info_t));
    cursor += sizeof(esp_partition_info_t);
  }

  uint8_t md5[16];
  mbedtls_md5(table, cursor, md5);
  table[cursor++] = 0xEB;
  table[cursor++] = 0xEB;
  memset(table + cursor, 0xFF, 14);
  cursor += 14;
  memcpy(table + cursor, md5, sizeof(md5));

  int verified_count = 0;
  if (ESP_OK != esp_partition_table_verify(reinterpret_cast<const esp_partition_info_t*>(table), true, &verified_count)) {
    AddLog(LOG_LEVEL_ERROR, PSTR("PNKBRG: generated partition table verify failed"));
    return false;
  }

  AddLog(LOG_LEVEL_INFO, PSTR("PNKBRG: writing partition table entries=%d app0=0x%06X/%uKB"), verified_count, new_app0_start, new_app0_size / 1024);
  if (ESP_OK != esp_flash_erase_region(nullptr, ESP_PARTITION_TABLE_OFFSET, 0x1000)) {
    AddLog(LOG_LEVEL_ERROR, PSTR("PNKBRG: partition table erase failed"));
    return false;
  }
  if (ESP_OK != esp_flash_write(nullptr, table, ESP_PARTITION_TABLE_OFFSET, sizeof(table))) {
    AddLog(LOG_LEVEL_ERROR, PSTR("PNKBRG: partition table write failed"));
    return false;
  }

  if (ESP_OK != esp_partition_erase_range(otadata, 0, otadata->size)) {
    AddLog(LOG_LEVEL_ERROR, PSTR("PNKBRG: otadata erase failed"));
    return false;
  }
  return true;
}

void RunMigration() {
  if (finished) { return; }
  started = true;
  AddLog(LOG_LEVEL_INFO, PSTR("PNKBRG: automatic migration start"));

  if (!RebootFromOta1IfNeeded()) {
    finished = true;
    return;
  }
  if (!DownloadSafebootToOta0()) {
    finished = true;
    return;
  }
  if (!SaveSafebootPartitionTable()) {
    finished = true;
    return;
  }

  AddLog(LOG_LEVEL_INFO, PSTR("PNKBRG: migration done, rebooting to safeboot"));
  finished = true;
  TasmotaGlobal.restart_flag = 2;
}

void EverySecond() {
  if (started || finished) { return; }
  if (TasmotaGlobal.uptime < 8) { return; }
  if (TasmotaGlobal.global_state.network_down || TasmotaGlobal.global_state.wifi_down) { return; }
  RunMigration();
}

}  // namespace PnkPartitionBridge

bool Xdrv97(uint32_t function) {
  switch (function) {
    case FUNC_EVERY_SECOND:
      PnkPartitionBridge::EverySecond();
      break;
  }
  return false;
}

#endif  // FIRMWARE_PARTITION_BRIDGE
