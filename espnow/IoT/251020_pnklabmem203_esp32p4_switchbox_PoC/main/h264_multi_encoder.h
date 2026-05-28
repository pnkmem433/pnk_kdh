/**
 * @file h264_multi_encoder.h
 * @brief 병렬 H.264 인코더 매니저 (30fps 달성을 위한 멀티스레딩)
 *
 * 3개의 H.264 인코더를 병렬로 실행하여 30fps를 달성합니다.
 * - 각 인코더는 10fps 처리 가능
 * - 3개 병렬 실행 시 30fps 달성
 * - 프레임 순서 보장 (재조합)
 */

#ifndef H264_MULTI_ENCODER_H
#define H264_MULTI_ENCODER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "h264_encoder.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 멀티 인코더 핸들
 */
typedef struct h264_multi_encoder_t *h264_multi_encoder_handle_t;

/**
 * @brief 멀티 인코더 설정
 */
typedef struct {
    uint32_t width;          ///< 프레임 너비
    uint32_t height;         ///< 프레임 높이
    uint32_t fps;            ///< 목표 FPS (30fps)
    uint32_t gop_size;       ///< GOP 크기
    uint32_t bitrate;        ///< 비트레이트 (0이면 자동)
    uint32_t num_encoders;   ///< 인코더 개수 (권장: 3)
} h264_multi_encoder_config_t;

/**
 * @brief 인코딩 완료 콜백
 *
 * @param frame 인코딩된 프레임
 * @param user_data 사용자 데이터
 */
typedef void (*h264_multi_encoder_cb_t)(const h264_encoded_frame_t *frame, void *user_data);

/**
 * @brief 멀티 인코더 초기화
 *
 * @param config 설정
 * @param callback 인코딩 완료 콜백
 * @param user_data 콜백에 전달할 사용자 데이터
 * @param out_handle 생성된 핸들 (출력)
 * @return ESP_OK 성공, 기타 오류
 */
esp_err_t h264_multi_encoder_init(const h264_multi_encoder_config_t *config,
                                   h264_multi_encoder_cb_t callback,
                                   void *user_data,
                                   h264_multi_encoder_handle_t *out_handle);

/**
 * @brief YUV420 프레임 인코딩 (비동기)
 *
 * 프레임을 가장 여유로운 인코더에 할당하여 처리합니다.
 * 인코딩 완료 시 콜백이 호출됩니다.
 *
 * @param handle 멀티 인코더 핸들
 * @param yuv420_data YUV420 데이터 (복사됨)
 * @param yuv420_size 데이터 크기
 * @param pts Presentation timestamp
 * @param frame_index 프레임 인덱스 (순서 보장용)
 * @return ESP_OK 성공, 기타 오류
 */
esp_err_t h264_multi_encoder_encode_async(h264_multi_encoder_handle_t handle,
                                           const uint8_t *yuv420_data,
                                           uint32_t yuv420_size,
                                           uint64_t pts,
                                           uint32_t frame_index);

/**
 * @brief 모든 대기 중인 인코딩 작업 완료 대기
 *
 * @param handle 멀티 인코더 핸들
 * @param timeout_ms 타임아웃 (밀리초), 0이면 무한 대기
 * @return ESP_OK 성공, ESP_ERR_TIMEOUT 타임아웃
 */
esp_err_t h264_multi_encoder_wait_all(h264_multi_encoder_handle_t handle, uint32_t timeout_ms);

/**
 * @brief 통계 가져오기
 *
 * @param handle 멀티 인코더 핸들
 * @param out_total_frames 총 인코딩 요청 프레임 수
 * @param out_completed_frames 완료된 프레임 수
 * @param out_dropped_frames 버퍼 가득 차서 드롭된 프레임 수
 * @return ESP_OK 성공
 */
esp_err_t h264_multi_encoder_get_stats(h264_multi_encoder_handle_t handle,
                                        uint32_t *out_total_frames,
                                        uint32_t *out_completed_frames,
                                        uint32_t *out_dropped_frames);

/**
 * @brief 멀티 인코더 해제
 *
 * @param handle 멀티 인코더 핸들
 */
void h264_multi_encoder_deinit(h264_multi_encoder_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif // H264_MULTI_ENCODER_H
