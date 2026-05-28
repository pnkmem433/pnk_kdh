/**
 * @file mp4_muxer.h
 * @brief 간단한 MP4 muxer (H.264 → MP4 컨테이너)
 *
 * H.264 NAL unit 스트림을 MP4 파일로 래핑합니다.
 * ISO/IEC 14496-12 (MP4 파일 포맷) 기본 구조만 구현
 */

#ifndef MP4_MUXER_H
#define MP4_MUXER_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MP4 muxer 핸들
 */
typedef struct mp4_muxer_t *mp4_muxer_handle_t;

/**
 * @brief MP4 muxer 설정
 */
typedef struct {
    uint32_t width;          ///< 비디오 너비
    uint32_t height;         ///< 비디오 높이
    uint32_t fps;            ///< 프레임 레이트
    float duration_sec;      ///< 총 길이 (초)
} mp4_muxer_config_t;

/**
 * @brief MP4 muxer 생성 및 파일 열기
 *
 * @param output_path 출력 MP4 파일 경로
 * @param config Muxer 설정
 * @param out_handle 생성된 muxer 핸들 (출력)
 * @return
 *     - ESP_OK: 성공
 *     - ESP_ERR_INVALID_ARG: 잘못된 인자
 *     - ESP_FAIL: 파일 열기 실패
 */
esp_err_t mp4_muxer_create(const char *output_path,
                            const mp4_muxer_config_t *config,
                            mp4_muxer_handle_t *out_handle);

/**
 * @brief H.264 NAL unit 추가
 *
 * @param handle Muxer 핸들
 * @param nal_data NAL unit 데이터
 * @param nal_size NAL unit 크기
 * @return
 *     - ESP_OK: 성공
 *     - ESP_ERR_INVALID_ARG: 잘못된 인자
 *     - ESP_FAIL: 쓰기 실패
 */
esp_err_t mp4_muxer_add_nal(mp4_muxer_handle_t handle,
                             const uint8_t *nal_data,
                             uint32_t nal_size);

/**
 * @brief MP4 파일 마무리 및 헤더 완성
 *
 * @param handle Muxer 핸들
 * @return
 *     - ESP_OK: 성공
 *     - ESP_FAIL: 실패
 */
esp_err_t mp4_muxer_finalize(mp4_muxer_handle_t handle);

/**
 * @brief MP4 muxer 해제
 *
 * @param handle Muxer 핸들
 */
void mp4_muxer_destroy(mp4_muxer_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif // MP4_MUXER_H
