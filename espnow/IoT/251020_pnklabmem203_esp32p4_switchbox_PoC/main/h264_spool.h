/**
 * @file h264_spool.h
 * @brief H.264 스풀링 모듈 (링버퍼 + SD 백그라운드 쓰기)
 *
 * 압축된 H.264 NAL unit을 SPIRAM 링버퍼에 저장하고,
 * 백그라운드 태스크로 SD 카드에 순차 쓰기를 수행합니다.
 */

#ifndef H264_SPOOL_H
#define H264_SPOOL_H

#include <stdint.h>
#include "esp_err.h"
#include "h264_encoder.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief H.264 스풀 핸들
 */
typedef struct h264_spool_t *h264_spool_handle_t;

/**
 * @brief 진행률 콜백 함수 타입
 *
 * @param current 현재 처리한 프레임 수
 * @param total 전체 프레임 수
 * @param user_data 사용자 데이터
 */
typedef void (*h264_spool_progress_cb_t)(uint32_t current, uint32_t total, void *user_data);

/**
 * @brief H.264 스풀 시작
 *
 * @param width 프레임 너비
 * @param height 프레임 높이
 * @param fps 프레임 레이트
 * @param out_handle 생성된 스풀 핸들 (출력)
 * @return
 *     - ESP_OK: 성공
 *     - ESP_ERR_INVALID_ARG: 잘못된 인자
 *     - ESP_ERR_NO_MEM: 메모리 부족
 *     - ESP_FAIL: 기타 오류
 */
esp_err_t h264_spool_start(uint32_t width, uint32_t height, uint32_t fps,
                            h264_spool_handle_t *out_handle);

/**
 * @brief H.264 압축 프레임 추가
 *
 * @param handle 스풀 핸들
 * @param encoded_frame H.264 인코딩된 프레임
 * @return
 *     - ESP_OK: 성공
 *     - ESP_ERR_INVALID_ARG: 잘못된 인자
 *     - ESP_ERR_NO_MEM: 링버퍼 가득 참 (프레임 드롭)
 */
esp_err_t h264_spool_add_frame(h264_spool_handle_t handle,
                                const h264_encoded_frame_t *encoded_frame);

/**
 * @brief FPS 업데이트 (실제 녹화 FPS 반영)
 *
 * @param handle 스풀 핸들
 * @param fps 새 FPS 값
 */
void h264_spool_set_fps(h264_spool_handle_t handle, uint32_t fps);

/**
 * @brief 스풀 중단 및 MP4 파일 저장
 *
 * @param handle 스풀 핸들
 * @param output_path 출력 MP4 파일 경로
 * @param progress_cb 진행률 콜백 (NULL 가능)
 * @param user_data 콜백 사용자 데이터
 * @return
 *     - ESP_OK: 성공
 *     - ESP_FAIL: 실패
 */
esp_err_t h264_spool_flush_to_file(h264_spool_handle_t handle,
                                    const char *output_path,
                                    h264_spool_progress_cb_t progress_cb,
                                    void *user_data);

/**
 * @brief H.264 스풀 정지 및 리소스 해제
 *
 * @param handle 스풀 핸들
 */
void h264_spool_stop(h264_spool_handle_t handle);

/**
 * @brief 현재 버퍼 상태 조회
 *
 * @param handle 스풀 핸들
 * @param out_buffered_frames 버퍼에 저장된 프레임 수 (출력, NULL 가능)
 * @param out_dropped_frames 드롭된 프레임 수 (출력, NULL 가능)
 * @param out_written_frames SD에 쓰여진 프레임 수 (출력, NULL 가능)
 * @return ESP_OK on success
 */
esp_err_t h264_spool_get_stats(h264_spool_handle_t handle,
                                uint32_t *out_buffered_frames,
                                uint32_t *out_dropped_frames,
                                uint32_t *out_written_frames);

#ifdef __cplusplus
}
#endif

#endif // H264_SPOOL_H
