#include "mjpeg_recorder.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "esp_log.h"

#pragma pack(push, 1)
typedef struct {
    uint32_t dwMicroSecPerFrame;
    uint32_t dwMaxBytesPerSec;
    uint32_t dwPaddingGranularity;
    uint32_t dwFlags;
    uint32_t dwTotalFrames;
    uint32_t dwInitialFrames;
    uint32_t dwStreams;
    uint32_t dwSuggestedBufferSize;
    uint32_t dwWidth;
    uint32_t dwHeight;
    uint32_t dwReserved[4];
} AVIMAINHEADER;

typedef struct {
    uint32_t fccType;
    uint32_t fccHandler;
    uint32_t dwFlags;
    uint16_t wPriority;
    uint16_t wLanguage;
    uint32_t dwInitialFrames;
    uint32_t dwScale;
    uint32_t dwRate;
    uint32_t dwStart;
    uint32_t dwLength;
    uint32_t dwSuggestedBufferSize;
    uint32_t dwQuality;
    uint32_t dwSampleSize;
    struct {
        int16_t left;
        int16_t top;
        int16_t right;
        int16_t bottom;
    } rcFrame;
} AVISTREAMHEADER;

typedef struct {
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BITMAPINFOHEADER;

typedef struct {
    uint32_t ckid;     // '00dc'
    uint32_t dwFlags;  // 0x10 keyframe
    uint32_t dwOffset; // from start of 'movi' LIST data
    uint32_t dwSize;   // size of data
} AVIINDEXENTRY;
#pragma pack(pop)

struct mjpeg_recorder {
    FILE *fp;
    uint32_t width;
    uint32_t height;
    uint32_t fps;
    uint32_t total_frames;
    uint32_t movi_list_pos;
    uint32_t movi_data_start;
    AVIINDEXENTRY *index;
    uint32_t index_capacity;
    uint8_t *io_buf;
};

static const char *TAG = "mjpeg_rec";

static uint32_t fourcc(const char *s)
{
    return ((uint32_t)(uint8_t)s[0]) | ((uint32_t)(uint8_t)s[1] << 8) | ((uint32_t)(uint8_t)s[2] << 16) | ((uint32_t)(uint8_t)s[3] << 24);
}

static void write_u32(FILE *f, uint32_t v) { fwrite(&v, 4, 1, f); }
static void write_u16(FILE *f, uint16_t v) { fwrite(&v, 2, 1, f); }

esp_err_t mjpeg_recorder_start(const char *path, uint32_t width, uint32_t height, uint32_t fps, mjpeg_recorder_t **out)
{
    if (!path || !out || width == 0 || height == 0 || fps == 0) return ESP_ERR_INVALID_ARG;
    mjpeg_recorder_t *rec = calloc(1, sizeof(*rec));
    if (!rec) return ESP_ERR_NO_MEM;
    rec->fp = fopen(path, "wb+");
    if (!rec->fp) {
        ESP_LOGE(TAG, "open %s failed: %s (errno=%d)", path, strerror(errno), errno);
        free(rec);
        return ESP_FAIL;
    }
    rec->width = width;
    rec->height = height;
    rec->fps = fps;
    rec->index_capacity = 1024;
    rec->index = calloc(rec->index_capacity, sizeof(AVIINDEXENTRY));
    if (!rec->index) {
        fclose(rec->fp);
        free(rec);
        return ESP_ERR_NO_MEM;
    }

    // Large stdio buffer to reduce VFS overhead
    rec->io_buf = (uint8_t *)malloc(256 * 1024);
    if (rec->io_buf) {
        setvbuf(rec->fp, (char *)rec->io_buf, _IOFBF, 256 * 1024);
    }

    // RIFF header
    write_u32(rec->fp, fourcc("RIFF"));
    write_u32(rec->fp, 0); // total size placeholder
    write_u32(rec->fp, fourcc("AVI "));

    // LIST hdrl
    write_u32(rec->fp, fourcc("LIST"));
    write_u32(rec->fp, 0); // hdrl size placeholder
    long hdrl_size_pos = ftell(rec->fp);
    write_u32(rec->fp, fourcc("hdrl"));

    // avih
    write_u32(rec->fp, fourcc("avih"));
    write_u32(rec->fp, sizeof(AVIMAINHEADER));
    AVIMAINHEADER avih = {0};
    avih.dwMicroSecPerFrame = 1000000U / fps;
    avih.dwMaxBytesPerSec = 0;
    avih.dwPaddingGranularity = 0;
    avih.dwFlags = 0x10; // AVIF_HASINDEX
    avih.dwTotalFrames = 0; // placeholder
    avih.dwInitialFrames = 0;
    avih.dwStreams = 1;
    avih.dwSuggestedBufferSize = width * height * 2;
    avih.dwWidth = width;
    avih.dwHeight = height;
    fwrite(&avih, sizeof(avih), 1, rec->fp);

    // LIST strl
    write_u32(rec->fp, fourcc("LIST"));
    write_u32(rec->fp, 0); // strl size placeholder
    long strl_size_pos = ftell(rec->fp);
    write_u32(rec->fp, fourcc("strl"));

    // strh
    write_u32(rec->fp, fourcc("strh"));
    write_u32(rec->fp, sizeof(AVISTREAMHEADER));
    AVISTREAMHEADER strh = {0};
    strh.fccType = fourcc("vids");
    strh.fccHandler = fourcc("MJPG");
    strh.dwFlags = 0;
    strh.wPriority = 0;
    strh.wLanguage = 0;
    strh.dwInitialFrames = 0;
    strh.dwScale = 1;
    strh.dwRate = fps;
    strh.dwStart = 0;
    strh.dwLength = 0; // placeholder
    strh.dwSuggestedBufferSize = width * height * 2;
    strh.dwQuality = 0xFFFFFFFF;
    strh.dwSampleSize = 0;
    strh.rcFrame.left = 0;
    strh.rcFrame.top = 0;
    strh.rcFrame.right = (int16_t)width;
    strh.rcFrame.bottom = (int16_t)height;
    fwrite(&strh, sizeof(strh), 1, rec->fp);

    // strf (BITMAPINFO)
    write_u32(rec->fp, fourcc("strf"));
    write_u32(rec->fp, sizeof(BITMAPINFOHEADER));
    BITMAPINFOHEADER bi = {0};
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = (int32_t)width;
    bi.biHeight = (int32_t)height;
    bi.biPlanes = 1;
    bi.biBitCount = 24; // not used for MJPG but commonly 24
    bi.biCompression = fourcc("MJPG");
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;
    fwrite(&bi, sizeof(bi), 1, rec->fp);

    // Patch strl size
    long strl_end = ftell(rec->fp);
    uint32_t strl_size = (uint32_t)(strl_end - strl_size_pos);
    fseek(rec->fp, strl_size_pos - 4, SEEK_SET);
    write_u32(rec->fp, strl_size);
    fseek(rec->fp, strl_end, SEEK_SET);

    // Patch hdrl size
    long hdrl_end = ftell(rec->fp);
    uint32_t hdrl_size = (uint32_t)(hdrl_end - hdrl_size_pos);
    fseek(rec->fp, hdrl_size_pos - 4, SEEK_SET);
    write_u32(rec->fp, hdrl_size);
    fseek(rec->fp, hdrl_end, SEEK_SET);

    // LIST movi
    write_u32(rec->fp, fourcc("LIST"));
    rec->movi_list_pos = (uint32_t)ftell(rec->fp);
    write_u32(rec->fp, 0); // movi size placeholder
    write_u32(rec->fp, fourcc("movi"));
    rec->movi_data_start = (uint32_t)ftell(rec->fp);

    *out = rec;
    return ESP_OK;
}

static esp_err_t ensure_index_capacity(mjpeg_recorder_t *rec)
{
    if (rec->total_frames < rec->index_capacity) return ESP_OK;
    uint32_t new_cap = rec->index_capacity * 2;
    void *p = realloc(rec->index, new_cap * sizeof(AVIINDEXENTRY));
    if (!p) return ESP_ERR_NO_MEM;
    rec->index = (AVIINDEXENTRY *)p;
    rec->index_capacity = new_cap;
    return ESP_OK;
}

esp_err_t mjpeg_recorder_write_frame(mjpeg_recorder_t *rec, const uint8_t *jpeg, uint32_t size)
{
    if (!rec || !rec->fp || !jpeg || size == 0) return ESP_ERR_INVALID_ARG;
    if (ensure_index_capacity(rec) != ESP_OK) return ESP_ERR_NO_MEM;

    uint32_t chunk_id = fourcc("00dc");
    // record offset relative to movi_data_start
    uint32_t cur_pos = (uint32_t)ftell(rec->fp);
    uint32_t offset = cur_pos + 8 - rec->movi_data_start;

    // write chunk header and data
    write_u32(rec->fp, chunk_id);
    write_u32(rec->fp, size);
    fwrite(jpeg, 1, size, rec->fp);
    if (size & 1) { // pad to even
        uint8_t pad = 0;
        fwrite(&pad, 1, 1, rec->fp);
    }

    // add index entry
    rec->index[rec->total_frames].ckid = chunk_id;
    rec->index[rec->total_frames].dwFlags = 0x10; // keyframe
    rec->index[rec->total_frames].dwOffset = offset;
    rec->index[rec->total_frames].dwSize = size;
    rec->total_frames++;
    return ESP_OK;
}

esp_err_t mjpeg_recorder_stop(mjpeg_recorder_t *rec)
{
    if (!rec) return ESP_ERR_INVALID_ARG;
    if (!rec->fp) { free(rec->index); free(rec); return ESP_OK; }

    // Determine current end to compute proper movi size (before idx1)
    long idx_start = ftell(rec->fp);

    // Write idx1
    write_u32(rec->fp, fourcc("idx1"));
    uint32_t idx_size = rec->total_frames * sizeof(AVIINDEXENTRY);
    write_u32(rec->fp, idx_size);
    fwrite(rec->index, sizeof(AVIINDEXENTRY), rec->total_frames, rec->fp);

    // Patch movi size (exclude idx1)
    uint32_t movi_size = (uint32_t)(idx_start - (long)rec->movi_list_pos - 8);
    fseek(rec->fp, rec->movi_list_pos, SEEK_SET);
    write_u32(rec->fp, movi_size);

    // Patch avih total frames
    // RIFF(12) + LIST hdrl (8 + ...) => avih at fixed offset: RIFF + size + 'AVI ' + 'LIST' + size + 'hdrl' + 'avih'
    // We wrote in order, so avih starts at offset 12 + 8 + 4 + 8 = 32? But safer to search back: we know avih was written immediately after 'hdrl'
    // For simplicity, recompute positions based on our writing order:
    // RIFF(12)
    // LIST hdrl (4+4+4 + avih(4+4+sizeof) + LIST strl(...))
    // avih payload offset = 12 + 12 (LIST header) + 8 (avih tag+size)
    long avih_payload_off = 12 + 12 + 8;
    fseek(rec->fp, avih_payload_off + offsetof(AVIMAINHEADER, dwTotalFrames), SEEK_SET);
    write_u32(rec->fp, rec->total_frames);

    // Patch strh.dwLength
    // strh payload offset: avih chunk(8+sizeof) -> LIST strl(8+4) -> strh tag+size(8)
    long strh_payload_off = avih_payload_off + sizeof(AVIMAINHEADER) + 8 + 4 + 8;
    fseek(rec->fp, strh_payload_off + offsetof(AVISTREAMHEADER, dwLength), SEEK_SET);
    write_u32(rec->fp, rec->total_frames);

    // Patch RIFF size
    long file_end = ftell(rec->fp); // after last patch
    fseek(rec->fp, 4, SEEK_SET);
    write_u32(rec->fp, (uint32_t)(file_end - 8));

    fclose(rec->fp);
    free(rec->index);
    if (rec->io_buf) free(rec->io_buf);
    free(rec);
    return ESP_OK;
}
