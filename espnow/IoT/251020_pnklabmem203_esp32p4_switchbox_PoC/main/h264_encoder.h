/**
 * @file h264_encoder.h
 * @brief H.264 하드웨어 인코더 래퍼 모듈
 *
 * ESP32-P4의 하드웨어 H.264 인코더를 사용하여 YUV420 프레임을
 * H.264 NAL unit으로 압축합니다.
 */

#ifndef H264_ENCODER_H
#define H264_ENCODER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief H.264 인코더 핸들
 */
typedef struct h264_encoder_t *h264_encoder_handle_t;

/**
 * @brief H.264 인코더 설정
 */
typedef struct {
    uint32_t width;          ///< 프레임 너비 (픽셀)
    uint32_t height;         ///< 프레임 높이 (픽셀)
    uint32_t fps;            ///< 프레임 레이트
    uint32_t gop_size;       ///< GOP 크기 (I-프레임 간격)
    uint32_t bitrate;        ///< 목표 비트레이트 (bps), 0이면 자동
} h264_encoder_config_t;

/**
 * @brief H.264 NAL unit 타입
 */
typedef enum {
    H264_NAL_SPS = 7,        ///< Sequence Parameter Set
    H264_NAL_PPS = 8,        ///< Picture Parameter Set
    H264_NAL_IDR = 5,        ///< IDR (I-frame)
    H264_NAL_SLICE = 1,      ///< P-frame slice
} h264_nal_type_t;

/**
 * @brief H.264 인코딩 결과
 */
typedef struct {
    const uint8_t *data;     ///< NAL unit 데이터 (헤더 포함)
    uint32_t size;           ///< NAL unit 크기 (바이트)
    h264_nal_type_t type;    ///< NAL unit 타입
    bool is_keyframe;        ///< 키프레임 여부 (I-frame)
    uint64_t pts;            ///< Presentation timestamp (마이크로초)
} h264_encoded_frame_t;

/**
 * @brief H.264 인코더 초기화
 *
 * @param config 인코더 설정
 * @param out_handle 생성된 인코더 핸들 (출력)
 * @return
 *     - ESP_OK: 성공
 *     - ESP_ERR_INVALID_ARG: 잘못된 인자
 *     - ESP_ERR_NO_MEM: 메모리 부족
 *     - ESP_FAIL: 기타 오류
 */
esp_err_t h264_encoder_init(const h264_encoder_config_t *config, h264_encoder_handle_t *out_handle);

/**
 * @brief YUV420 프레임을 H.264로 인코딩
 *
 * @param handle 인코더 핸들
 * @param yuv420_data YUV420 프레임 데이터
 * @param yuv420_size YUV420 프레임 크기 (바이트)
 * @param pts Presentation timestamp (마이크로초)
 * @param out_frame 인코딩 결과 (출력)
 * @return
 *     - ESP_OK: 성공
 *     - ESP_ERR_INVALID_ARG: 잘못된 인자
 *     - ESP_FAIL: 인코딩 실패
 *
 * @note out_frame->data는 다음 h264_encoder_encode() 호출 전까지 유효합니다.
 *       데이터를 보관하려면 복사해야 합니다.
 */
esp_err_t h264_encoder_encode(h264_encoder_handle_t handle,
                               const uint8_t *yuv420_data,
                               uint32_t yuv420_size,
                               uint64_t pts,
                               h264_encoded_frame_t *out_frame);

/**
 * @brief SPS/PPS 헤더 가져오기
 *
 * @param handle 인코더 핸들
 * @param out_sps SPS 데이터 (출력, NULL 가능)
 * @param out_sps_size SPS 크기 (출력, NULL 가능)
 * @param out_pps PPS 데이터 (출력, NULL 가능)
 * @param out_pps_size PPS 크기 (출력, NULL 가능)
 * @return
 *     - ESP_OK: 성공
 *     - ESP_ERR_INVALID_ARG: 잘못된 인자
 *     - ESP_ERR_NOT_FOUND: SPS/PPS 아직 생성되지 않음
 *
 * @note SPS/PPS는 첫 프레임 인코딩 후 생성됩니다.
 *       반환된 포인터는 인코더가 해제될 때까지 유효합니다.
 */
esp_err_t h264_encoder_get_headers(h264_encoder_handle_t handle,
                                    const uint8_t **out_sps,
                                    uint32_t *out_sps_size,
                                    const uint8_t **out_pps,
                                    uint32_t *out_pps_size);

/**
 * @brief H.264 인코더 해제
 *
 * @param handle 인코더 핸들
 */
void h264_encoder_deinit(h264_encoder_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif // H264_ENCODER_H
