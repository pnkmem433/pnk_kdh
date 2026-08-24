// 최적화된 player.cs (전체 상태 흐름 및 비디오 전환 개선 포함)
using System;
using System.Collections;
using System.Collections.Concurrent;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.Video;
using Newtonsoft.Json;
using TMPro;
using UnityEngine.Networking;

// ------------------- 보더리스 빌드 Using -------------------
using System.Runtime.InteropServices;

// ------------------- 좌표 보정 Using -------------------
using System.IO;
using System.Globalization;

public class VideoHandler : MonoBehaviour
{
    // ------------------- 보더리스 빌드 세팅 Start -------------------
#if UNITY_STANDALONE_WIN
    [DllImport("user32.dll")] static extern IntPtr GetActiveWindow();
    [DllImport("user32.dll")] static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter,
        int X, int Y, int cx, int cy, uint uFlags);
    [DllImport("user32.dll")] static extern int GetSystemMetrics(int nIndex);
    const int SM_XVIRTUALSCREEN = 76, SM_YVIRTUALSCREEN = 77;
    const uint SWP_NOZORDER = 0x0004, SWP_NOACTIVATE = 0x0010;
#endif
    // ------------------- 보더리스 빌드 세팅 End -------------------

    // ------------------- 좌표 보정용 기준 원점 Start -------------------
    const float L_DEF_X = -960f, L_DEF_Y = 0f;
    const float R_DEF_X = 960f, R_DEF_Y = 0f;
    readonly Dictionary<RectTransform, Vector2> original = new();
    // ------------------- 좌표 보정용 기준 원점 End -------------------

    public RawImage videoScreen, mainScreen;
    public RawImage clothesImage, clothesImage2, clothesImage3, introLogoLeft, introLogoRight, exitMDCAImage; // Inspector에서 연결
    public VideoPlayer mVideoPlayer;
    public Canvas mainCanvas, fittingCanvas, exitCanvas;
    public TextMeshProUGUI fittingTextLeft, fittingTextRight, exitTextLeft, exitTextRight;
    public TextMeshProUGUI internetStatusText; // 인터넷 연결 상태 표시용 텍스트

    private string urlLocal = "http://192.168.1.100:3000";
    private string url907 = "http://115.23.192.217:3030";
    private string urlUse = "http://115.23.192.217:3030";

    private enum ScreenState { Main, Fitting, Video, Exit }
    private ScreenState currentState;
    private bool isProcessingQR = false;
    private float mainStartTime = 0f;

    private ConcurrentQueue<string> messageQueue = new ConcurrentQueue<string>();
    private DateTime? lastDoorEventTime = null;

    private Coroutine pendingVideoCo;
    private bool videoTransitionArmed = false;

    private SessionData sessionData;
    private ClothesData clothesData;

    // --- Video 전용 RT ---
    private RenderTexture videoRT;

    [Serializable] private class UserData { public string name; public int qrcodeSeq; }
    [Serializable] private class LogQRData { public int seq; public string code; public int boardId; }
    [Serializable] private class VideoData { public string path; public int path_count; public string scanned_cloth_list; public int qrStatus; }
    [Serializable] private class QueryResult { public int affectedRows; public string message; }

    [Serializable]
    private class SessionData
    {
        public int seq;
        public string created_at;
        public int is_scanned;
        public int is_video_ended;
        public int is_activated;
    }

    [Serializable]
    private class ClothesData
    {
        public int seq;
        public int session_seq;
        public string clothes_types_name;
        public string clothes_product_id;
        public string first_image_url;
        public string second_image_url;
        public string video_url;
        public string scanned_at;
        public int scan_source_type;
    }

    [Serializable]
    private class VideoRoomDoorSensorData
    {
        public int seq;
        public string video_start_time;
        public string video_end_time;
        public string event_time;
        public int is_opened;
    }

    void Awake()
    {
        foreach (var rt in GetTargets())
            if (rt) original[rt] = rt.anchoredPosition;
    }

    void Start()
    {

        // ------------------- 보더리스 빌드 Start -------------------
        Screen.SetResolution(3840, 1080, false);
#if UNITY_STANDALONE_WIN
        var hwnd = GetActiveWindow();
        int vx = GetSystemMetrics(SM_XVIRTUALSCREEN);
        int vy = GetSystemMetrics(SM_YVIRTUALSCREEN);
        SetWindowPos(hwnd, IntPtr.Zero, vx, vy, 3840, 1080, SWP_NOZORDER | SWP_NOACTIVATE);
#endif
        // ------------------- 보더리스 빌드 End -------------------
videoScreen.uvRect = new Rect(0f, 0.21875f, 1f, 0.5625f); 

        // 프레임 페이싱 고정 (MadMapper 캡처 안정화)
        Application.runInBackground = true;
        QualitySettings.vSyncCount = 1;         // 모니터 주사율(30Hz/60Hz)에 맞춤
        Application.targetFrameRate = 30;       // 변수로 관리할 수 있도록 처리
#if UNITY_2019_3_OR_NEWER
        UnityEngine.Rendering.OnDemandRendering.renderFrameInterval = 1;
#endif
#if UNITY_2019_2_OR_NEWER
        QualitySettings.maxQueuedFrames = 2;    // 지터 감소
#endif

        // ------------------- 좌표 보정 Start -------------------
        var path = GetExeDirPath("pos.txt");
        if (File.Exists(path))
        {
            if (TryReadPos(path, out float lx, out float ly, out float rx, out float ry))
            {
                Vector2 offL = new(lx - L_DEF_X, ly - L_DEF_Y);
                Vector2 offR = new(rx - R_DEF_X, ry - R_DEF_Y);

                foreach (var rt in original.Keys)
                {
                    if (!rt) continue;
                    var basePos = original[rt];
                    var off = (basePos.x < 0f) ? offL : offR;
                    rt.anchoredPosition = basePos + off;
                }
                Debug.Log($"[UIPosApplier] 적용 완료. offL={offL}, offR={offR}");
            }
            else
            {
                Debug.LogWarning("[UIPosApplier] pos.txt 파싱 실패. 형식: Lx,Ly,Rx,Ry (예: -967,30,955,-10)");
            }
        }
        // ------------------- 좌표 보정 End -------------------

        // 기본 비디오 화면 색상(플래시 방지)
        if (videoScreen != null) videoScreen.color = Color.black;

        // VideoPlayer 기본 옵션 (초반 잘림 방지)
        if (mVideoPlayer)
        {
            mVideoPlayer.playOnAwake = false;
            mVideoPlayer.isLooping = false;          // 필요 시 true
            mVideoPlayer.skipOnDrop = true;          // 시각적 지터 감소
            mVideoPlayer.waitForFirstFrame = true;
            mVideoPlayer.renderMode = VideoRenderMode.RenderTexture;
            mVideoPlayer.Pause();
            mVideoPlayer.time = 0;
        }

        SetAllScreensActive(main: true, fitting: false, video: false, exit: false);

        if (videoScreen && mVideoPlayer)
            StartCoroutine(PrepareVideo());

        StartCoroutine(MonitorSessionWhileNotMain());
    }

    private void OnDestroy()
    {
        if (videoRT != null)
        {
            videoRT.Release();
            videoRT = null;
        }
    }

    private void ResetData()
    {
        sessionData = null;
        clothesData = null;
        fittingTextLeft.text = "";
        isProcessingQR = false;
        lastDoorEventTime = null;

        ClearClothesImages();

        videoTransitionArmed = false;
        if (pendingVideoCo != null)
        {
            StopCoroutine(pendingVideoCo);
            pendingVideoCo = null;
        }

        if (mVideoPlayer != null)
        {
            mVideoPlayer.Stop();
            mVideoPlayer.clip = null;
            mVideoPlayer.url = string.Empty;
            mVideoPlayer.Pause();
            mVideoPlayer.time = 0;
        }
    }

    // ------------------- 위치 보정 Start -------------------
    RectTransform[] GetTargets()
    {
        return new RectTransform[]
        {
            clothesImage ? clothesImage.rectTransform : null,
            clothesImage2 ? clothesImage2.rectTransform : null,
            clothesImage3 ? clothesImage3.rectTransform : null,
            introLogoLeft ? introLogoLeft.rectTransform : null,
            introLogoRight ? introLogoRight.rectTransform : null,
            fittingTextRight ? fittingTextRight.rectTransform : null,
            exitTextLeft ? exitTextLeft.rectTransform : null,
            exitTextRight ? exitTextRight.rectTransform : null,
            exitMDCAImage ? exitMDCAImage.rectTransform : null
        };
    }

    static bool TryReadPos(string path, out float lx, out float ly, out float rx, out float ry)
    {
        lx = ly = rx = ry = 0f;
        var txt = File.ReadAllText(path).Trim();
        var p = txt.Split(',');
        if (p.Length != 4) return false;

        var ci = CultureInfo.InvariantCulture;
        return float.TryParse(p[0], NumberStyles.Float, ci, out lx)
            && float.TryParse(p[1], NumberStyles.Float, ci, out ly)
            && float.TryParse(p[2], NumberStyles.Float, ci, out rx)
            && float.TryParse(p[3], NumberStyles.Float, ci, out ry);
    }

    static string GetExeDirPath(string fileName)
    {
        var parent = Directory.GetParent(Application.dataPath);
        var dir = parent?.FullName ?? Application.dataPath;
        return Path.Combine(dir, fileName);
    }
    // ------------------- 위치 보정 End -------------------

    // ---------- 이미지 초기화 & 선로딩 유틸 ----------
    private void ClearRawImage(RawImage ri)
    {
        if (!ri) return;
        ri.texture = null;
        ri.color = new Color(1, 1, 1, 1);
        ri.gameObject.SetActive(false);
    }

    private void ClearClothesImages()
    {
        ClearRawImage(clothesImage);
        ClearRawImage(clothesImage2);
        ClearRawImage(clothesImage3);
    }

    private IEnumerator DownloadTexture(string url, Action<Texture2D> onDone)
    {
        Texture2D tex = null;
        if (!string.IsNullOrEmpty(url))
        {
            using (UnityWebRequest req = UnityWebRequestTexture.GetTexture(url))
            {
                yield return req.SendWebRequest();
                if (req.result == UnityWebRequest.Result.Success)
                    tex = DownloadHandlerTexture.GetContent(req);
                else
                    Debug.LogError($"이미지 로드 실패: {req.error} ({url})");
            }
        }
        onDone?.Invoke(tex);
    }

    // clothesData의 first/second 이미지를 미리 다운로드 → RawImage 세팅 → 그 다음 Fitting 진입
    private IEnumerator PreloadImagesThenSwitchToFitting()
    {
        ClearClothesImages(); // 이전 실행 잔상 제거

        string first = clothesData?.first_image_url;
        string second = clothesData?.second_image_url;

        Texture2D tex1 = null, tex2 = null;

        bool done1 = false, done2 = false;
        StartCoroutine(DownloadTexture(first,  t => { tex1 = t; done1 = true; }));
        StartCoroutine(DownloadTexture(second, t => { tex2 = t; done2 = true; }));

        yield return new WaitUntil(() => done1 && done2);

        if (tex1 == null && tex2 == null)
        {
            // 아무 것도 없으면 비노출 유지
        }
        else if (tex1 != null && tex2 == null)
        {
            clothesImage.texture = tex1;
            clothesImage.gameObject.SetActive(true);
        }
        else // 둘 다 존재
        {
            clothesImage2.texture = tex1;
            clothesImage3.texture = tex2;
            clothesImage2.gameObject.SetActive(true);
            clothesImage3.gameObject.SetActive(true);
        }

        // 세팅 완료 후에야 Fitting으로 전환
        yield return SwitchToState(ScreenState.Fitting);
    }

    private IEnumerator MonitorSessionWhileNotMain()
    {
        while (true)
        {
            if (currentState != ScreenState.Main)
            {
                // 1) session 상태 확인
                string url = $"{urlUse}/session/last";
                using (UnityWebRequest request = UnityWebRequest.Get(url))
                {
                    yield return request.SendWebRequest();

                    if (request.result == UnityWebRequest.Result.Success)
                    {
                        SessionData tempData = null;
                        try
                        {
                            tempData = JsonConvert.DeserializeObject<SessionData>(request.downloadHandler.text);
                        }
                        catch (Exception e)
                        {
                            Debug.LogError($"SessionData 파싱 실패: {e.Message}");
                        }

                        if (tempData != null && tempData.is_scanned == 1 && tempData.is_activated == 0)
                        {
                            Debug.Log("조건 만족 → Reset 후 Main으로 복귀");
                            ResetData();
                            yield return SwitchToState(ScreenState.Main);
                            RestartScreenCycle();
                        }
                    }
                }

                // 2) door sensor 확인 (doorDataB)
                string doorUrl = $"{urlUse}/video-room-door-sensor/last/record/{sessionData?.seq}";
                using (UnityWebRequest doorRequest = UnityWebRequest.Get(doorUrl))
                {
                    yield return doorRequest.SendWebRequest();

                    if (doorRequest.result == UnityWebRequest.Result.Success)
                    {
                        bool needReset = false;
                        try
                        {
                            var doorDataB = JsonConvert.DeserializeObject<VideoRoomDoorSensorData>(doorRequest.downloadHandler.text);

                            if (doorDataB != null && doorDataB.is_opened == 0)
                            {
                                DateTime doorBTime = DateTime.Parse(doorDataB.event_time);
                                if (lastDoorEventTime.HasValue && doorBTime > lastDoorEventTime.Value)
                                {
                                    Debug.Log("doorDataB 조건 충족 → Reset 후 Main으로 복귀");
                                    needReset = true;
                                    lastDoorEventTime = null;
                                }
                            }
                        }
                        catch (Exception e)
                        {
                            Debug.LogError($"DoorSensor 파싱 실패: {e.Message}");
                        }

                        if (needReset)
                        {
                            if (sessionData != null)
                            {
                                yield return StartCoroutine(PatchLogCompleted(sessionData.seq, 2));
                            }
                            else
                            {
                                Debug.LogWarning("sessionData가 null 상태여서 PatchLogCompleted 실행 생략");
                            }

                            ResetData();
                            yield return SwitchToState(ScreenState.Main);
                            RestartScreenCycle();
                        }
                    }
                    else
                    {
                        Debug.LogError($"DoorSensor API 실패: {doorRequest.error}");
                    }
                }
            }
            yield return new WaitForSeconds(2f);
        }
    }

    private IEnumerator PrepareVideo()
    {
        mVideoPlayer.Prepare();
        while (!mVideoPlayer.isPrepared) yield return new WaitForSeconds(0.5f);
        // 텍스처는 실제 전환 시 연결(또는 PrepareVideoForPlayback에서 설정)
        StartCoroutine(ScreenCycle());
    }

    private IEnumerator GetLogData()
    {
        string url = $"{urlUse}/session/last";
        using (UnityWebRequest request = UnityWebRequest.Get(url))
        {
            yield return request.SendWebRequest();

            if (request.result != UnityWebRequest.Result.Success)
            {
                Debug.LogError($"HTTP GET 실패: {request.error}");
                yield break;
            }

            try
            {
                sessionData = JsonConvert.DeserializeObject<SessionData>(request.downloadHandler.text);
            }
            catch (Exception e)
            {
                Debug.LogError($"SessionData 파싱 실패: {e.Message}");
                yield break;
            }

            if (sessionData == null)
            {
                Debug.Log("세션 데이터가 없음");
                yield break;
            }

            // 조건: 스캔 완료, 영상 미종료, 활성화 상태
            if (sessionData.is_scanned == 1 && sessionData.is_video_ended == 0 && sessionData.is_activated == 1)
            {
                // 의류 데이터 조회
                yield return StartCoroutine(GetClothesData(sessionData.seq));
                if (clothesData == null)
                {
                    Debug.Log("ClothesData 조회 결과가 없음");
                    yield break;
                }

                // 좌측 텍스트 업데이트
                fittingTextLeft.text = clothesData.clothes_types_name;

                // 문 상태/시간 확인
                if (!string.IsNullOrEmpty(clothesData.video_url))
                {
                    string doorUrl = $"{urlUse}/video-room-door-sensor/last/record/{sessionData.seq}";
                    bool shouldSwitchToFitting = false;

                    using (UnityWebRequest doorRequest = UnityWebRequest.Get(doorUrl))
                    {
                        yield return doorRequest.SendWebRequest();

                        if (doorRequest.result != UnityWebRequest.Result.Success)
                        {
                            Debug.LogError($"DoorSensor API 실패: {doorRequest.error}");
                            yield break;
                        }

                        try
                        {
                            var doorData = JsonConvert.DeserializeObject<VideoRoomDoorSensorData>(doorRequest.downloadHandler.text);
                            if (doorData != null && doorData.is_opened == 0)
                            {
                                DateTime doorEventTime = DateTime.Parse(doorData.event_time);
                                DateTime scannedAtTime = DateTime.Parse(clothesData.scanned_at);

                                if (doorEventTime > scannedAtTime)
                                {
                                    lastDoorEventTime = doorEventTime;

                                    // 1) 비디오 선 준비(준비만, 재생 금지)
                                    StartCoroutine(PrepareVideoForPlayback(clothesData.video_url));

                                    // 2) try/catch 밖에서 전환하도록 플래그만 세팅
                                    shouldSwitchToFitting = true;
                                }
                                else
                                {
                                    Debug.Log("Door event_time이 scanned_at보다 이전 → Fitting 전환 안 함");
                                }
                            }
                            else
                            {
                                Debug.Log("비디오룸 문이 열려 있음 또는 데이터 없음 → Fitting 진입 안 함");
                            }
                        }
                        catch (Exception e)
                        {
                            Debug.LogError($"DoorSensor 파싱 또는 시간 비교 오류: {e.Message}");
                        }
                    }

                    // ★ try/catch/using 블록 밖에서 yield 실행 → CS1626 회피
                    if (shouldSwitchToFitting)
                    {
                        yield return StartCoroutine(PreloadImagesThenSwitchToFitting());

                        // Fitting 진입 후, 지연 전환 타이머
                        videoTransitionArmed = true;
                        if (pendingVideoCo != null) StopCoroutine(pendingVideoCo);
                        pendingVideoCo = StartCoroutine(DelayedGoVideo(10f));
                    }
                }
            }
            else
            {
                Debug.Log("조건 불일치: 스캔 상태 아님 또는 비활성화/이미 종료");
            }
        }
    }

    // (남겨둠: 필요 시 다른 경로에서 사용할 수 있음)
    private void UpdateClothesImages()
    {
        string first = clothesData?.first_image_url;
        string second = clothesData?.second_image_url;

        bool hasFirst = !string.IsNullOrEmpty(first);
        bool hasSecond = !string.IsNullOrEmpty(second);

        if (!hasFirst && !hasSecond)
        {
            SetImageActive(clothesImage, false);
            SetImageActive(clothesImage2, false);
            SetImageActive(clothesImage3, false);
        }
        else if (hasFirst && !hasSecond)
        {
            SetImageActive(clothesImage, true);
            SetImageActive(clothesImage2, false);
            SetImageActive(clothesImage3, false);
            StartCoroutine(LoadImageInto(clothesImage, first));
        }
        else if (hasFirst && hasSecond)
        {
            SetImageActive(clothesImage, false);
            SetImageActive(clothesImage2, true);
            SetImageActive(clothesImage3, true);
            StartCoroutine(LoadImageInto(clothesImage2, first));
            StartCoroutine(LoadImageInto(clothesImage3, second));
        }
    }

    private IEnumerator LoadImageInto(RawImage target, string url)
    {
        if (target == null || string.IsNullOrEmpty(url)) yield break;

        using (UnityWebRequest request = UnityWebRequestTexture.GetTexture(url))
        {
            yield return request.SendWebRequest();
            if (request.result == UnityWebRequest.Result.Success)
            {
                target.texture = DownloadHandlerTexture.GetContent(request);
            }
            else
            {
                Debug.LogError($"이미지 로드 실패: {request.error} ({url})");
            }
        }
    }

    private void SetImageActive(RawImage img, bool on)
    {
        if (img != null && img.gameObject.activeSelf != on)
            img.gameObject.SetActive(on);
    }

    private IEnumerator LoadImageFromUrl(string url)
    {
        if (string.IsNullOrEmpty(url))
        {
            Debug.Log("first_image_url이 비어있음");
            yield break;
        }

        using (UnityWebRequest request = UnityWebRequestTexture.GetTexture(url))
        {
            yield return request.SendWebRequest();
            if (request.result == UnityWebRequest.Result.Success)
            {
                clothesImage.texture = DownloadHandlerTexture.GetContent(request);
            }
            else
            {
                Debug.LogError($"이미지 로드 실패: {request.error}");
            }
        }
    }

    private IEnumerator ChangeToSecondImageAfterDelay(string url, float delay)
    {
        yield return new WaitForSeconds(delay);
        StartCoroutine(LoadImageFromUrl(url));
    }

    private IEnumerator GetClothesData(int seq)
    {
        string url = $"{urlUse}/rfid-scan/last/record/all/{seq}";
        using (UnityWebRequest request = UnityWebRequest.Get(url))
        {
            yield return request.SendWebRequest();
            if (request.result == UnityWebRequest.Result.Success)
            {
                try
                {
                    clothesData = JsonConvert.DeserializeObject<ClothesData>(request.downloadHandler.text);
                    Debug.Log($"받은 ClothesData: {clothesData.clothes_types_name}, Video: {clothesData.video_url}");
                }
                catch (Exception e)
                {
                    Debug.LogError($"ClothesData 파싱 실패: {e.Message}");
                }
            }
            else
            {
                Debug.LogError($"HTTP GET (data/latest) 실패: {request.error}");
            }
        }
    }

    private IEnumerator PatchLogCompleted(int seq, int state)
    {
        string url = $"{urlUse}/session/is-video-ended/{seq}";
        string jsonBody = JsonConvert.SerializeObject(new { value = state });
        byte[] bodyRaw = System.Text.Encoding.UTF8.GetBytes(jsonBody);

        using (UnityWebRequest request = new UnityWebRequest(url, "PATCH"))
        {
            request.uploadHandler = new UploadHandlerRaw(bodyRaw);
            request.downloadHandler = new DownloadHandlerBuffer();
            request.SetRequestHeader("Content-Type", "application/json");

            yield return request.SendWebRequest();

            if (request.result == UnityWebRequest.Result.Success)
                Debug.Log("Log 상태 갱신 성공");
            else
                Debug.LogError($"PATCH 실패: {request.error}");
        }
    }

    private IEnumerator ScreenCycle()
    {
        while (true)
        {
            if (!isProcessingQR && currentState == ScreenState.Main)
            {
                if (currentState != ScreenState.Main)
                    yield return SwitchToState(ScreenState.Main);

                ResetVideoPlayerUrl();
                ResetData();

                yield return new WaitForSeconds(1f);
                yield return StartCoroutine(GetLogData());
                yield return new WaitForSeconds(1f);
            }
            yield return null;
        }
    }

    private void RestartScreenCycle()
    {
        Debug.Log("Restarting ScreenCycle");
        isProcessingQR = false;
        mainStartTime = Time.time;

        StopCoroutine("ScreenCycle");
        StopCoroutine("SwitchToFittingScreen");
        StartCoroutine(ScreenCycle());
    }

    private void ResetVideoPlayerUrl() { if (mVideoPlayer != null) mVideoPlayer.url = string.Empty; }

    private void SetAllScreensActive(bool main = false, bool fitting = false, bool video = false, bool exit = false)
    {
        if (mainCanvas) mainCanvas.gameObject.SetActive(main);
        if (fittingCanvas) fittingCanvas.gameObject.SetActive(fitting);
        if (videoScreen) videoScreen.gameObject.SetActive(video);
        if (exitCanvas) exitCanvas.gameObject.SetActive(exit);
    }

    private IEnumerator SwitchToState(ScreenState newState)
    {
        Debug.Log($"화면 전환: {currentState} → {newState}");
        if (currentState == newState) yield break;

        currentState = newState;

        switch (newState)
        {
            case ScreenState.Main: SetAllScreensActive(main: true); break;
            case ScreenState.Fitting: SetAllScreensActive(fitting: true); break;
            case ScreenState.Video: SetAllScreensActive(video: true); break;
            case ScreenState.Exit: SetAllScreensActive(exit: true); break;
        }

        CanvasGroup cg = GetCanvasGroup(newState);
        if (cg != null)
        {
            if (newState == ScreenState.Video)
            {
                cg.alpha = 1f; // 비디오는 별도 페이드 처리
            }
            else
            {
                cg.alpha = 0f;
                float duration = 1f, t = 0f;
                while (t < duration)
                {
                    t += Time.unscaledDeltaTime; // 프레임 변동에도 속도 일정
                    cg.alpha = Mathf.Lerp(0, 1, t / duration);
                    yield return null;
                }
                cg.alpha = 1f;
            }
        }

        if (newState == ScreenState.Main)
            ResetData();
    }

    private CanvasGroup GetCanvasGroup(ScreenState state)
    {
        switch (state)
        {
            case ScreenState.Main: return mainCanvas?.GetComponent<CanvasGroup>();
            case ScreenState.Fitting: return fittingCanvas?.GetComponent<CanvasGroup>();
            case ScreenState.Video: return videoScreen?.GetComponentInParent<CanvasGroup>();
            case ScreenState.Exit: return exitCanvas?.GetComponent<CanvasGroup>();
            default: return null;
        }
    }

    // (기존 API: 필요 시 사용. 현재 흐름은 PreloadImagesThenSwitchToFitting 사용)
    private IEnumerator SwitchToFittingScreen()
    {
        isProcessingQR = true;
        yield return SwitchToState(ScreenState.Fitting);

        if (clothesData != null && !string.IsNullOrEmpty(clothesData.video_url))
            yield return StartCoroutine(PrepareVideoForPlayback(clothesData.video_url));

        videoTransitionArmed = true;

        if (pendingVideoCo != null) StopCoroutine(pendingVideoCo);
        pendingVideoCo = StartCoroutine(DelayedGoVideo(10f));
    }

    private IEnumerator DelayedGoVideo(float delay)
    {
        yield return new WaitForSeconds(delay);
        if (!videoTransitionArmed || currentState != ScreenState.Fitting) yield break;
        yield return StartCoroutine(SwitchToVideoScreen());
    }

    private void EnsureVideoRT(int width, int height)
    {
        if (width <= 0 || height <= 0) return;

        if (videoRT != null && videoRT.width == width && videoRT.height == height) return;

        if (videoRT != null) videoRT.Release();

        videoRT = new RenderTexture(width, height, 0, RenderTextureFormat.ARGB32)
        {
            useMipMap = false,
            autoGenerateMips = false,
            antiAliasing = 1,
            wrapMode = TextureWrapMode.Clamp,
            filterMode = FilterMode.Bilinear,
            enableRandomWrite = false
        };
        videoRT.Create();
    }

    private IEnumerator PrepareVideoForPlayback(string path)
    {
        if (mVideoPlayer == null) yield break;

        mVideoPlayer.playOnAwake = false;
        mVideoPlayer.skipOnDrop  = true;
        mVideoPlayer.waitForFirstFrame = true;
        mVideoPlayer.isLooping = false;

        mVideoPlayer.Pause();
        mVideoPlayer.time = 0;
        mVideoPlayer.Stop(); // URL 교체 전 초기화

        var fileUri = path.StartsWith("file://") ? path : "file:///" + path.Replace("\\", "/");
        mVideoPlayer.url = fileUri;
        Debug.Log($"비디오 경로 설정: {fileUri}");

        // RenderTexture 출력
        mVideoPlayer.renderMode = VideoRenderMode.RenderTexture;

        mVideoPlayer.Prepare();
        float t = 0f;
        const float MaxPrepare = 10f;
        while (!mVideoPlayer.isPrepared && t < MaxPrepare)
        {
            t += Time.deltaTime;
            yield return null;
        }

        if (!mVideoPlayer.isPrepared)
        {
            Debug.LogError($"비디오 준비 시간 초과 (경로: {mVideoPlayer.url})");
            yield break;
        }

        // 준비 후 실제 해상도 확보(일부 플랫폼에서 한 프레임 필요할 수 있음)
        yield return null;

        int w = (int)Mathf.Max(1, mVideoPlayer.texture != null ? mVideoPlayer.texture.width  : 3840);
        int h = (int)Mathf.Max(1, mVideoPlayer.texture != null ? mVideoPlayer.texture.height : 1080);
        EnsureVideoRT(w, h);

        mVideoPlayer.targetTexture = videoRT;

        if (videoScreen && videoScreen.texture != videoRT)
            videoScreen.texture = videoRT;

        mVideoPlayer.Pause();
        mVideoPlayer.time = 0;   // 항상 처음부터
        Debug.Log($"비디오 준비 완료: {w}x{h}, length={mVideoPlayer.length:F2}s");
    }

    // 비디오는 전환 시점에만 재생
    private IEnumerator SwitchToVideoScreen()
    {
        // 1. 비디오 준비 완료 대기
        yield return new WaitUntil(() => mVideoPlayer != null && mVideoPlayer.isPrepared && videoRT != null);

        // 2. 여전히 Fitting 상태인지 확인
        if (currentState != ScreenState.Fitting) yield break;

        // 3. 전환 예약 소모
        videoTransitionArmed = false;
        pendingVideoCo = null;

        // 4. 전환 전 검정
        if (videoScreen)
        {
            videoScreen.color = Color.black;
            if (videoScreen.texture != videoRT) videoScreen.texture = videoRT;
        }

        // 5. Video 상태로 전환
        yield return SwitchToState(ScreenState.Video);

        // 6. 텍스처 연결 + 정확히 0초에서 재생 시작(프레임 경계 맞춤)
        mVideoPlayer.time = 0;
        mVideoPlayer.frame = 0;
        mVideoPlayer.Pause();
        yield return new WaitForEndOfFrame(); // 마지막 그리기 동기화

        mVideoPlayer.Play();
        Debug.Log($"비디오 재생 시작: {mVideoPlayer.length}초");

        // 7. 첫 프레임이 RT로 도착할 때까지 안전마진 대기
        yield return null;
        yield return null;

        // 8. 부드러운 페이드(언스케일드 시간)
        float fadeDuration = 1.0f, timer = 0f;
        while (timer < fadeDuration)
        {
            timer += Time.unscaledDeltaTime;
            if (videoScreen) videoScreen.color = Color.Lerp(Color.black, Color.white, timer / fadeDuration);
            yield return null;
        }
        if (videoScreen) videoScreen.color = Color.white;

        // 9. 종료 대기
        bool finished = false;
        VideoPlayer.EventHandler onEnd = null;
        onEnd = (vp) => { finished = true; mVideoPlayer.loopPointReached -= onEnd; };
        mVideoPlayer.loopPointReached += onEnd;
        while (!finished) yield return null;

        // 10. 사후 처리
        if (sessionData != null)
            yield return StartCoroutine(PatchLogCompleted(sessionData.seq, 1));

        yield return new WaitForSeconds(1f);
        yield return SwitchToState(ScreenState.Exit);
        yield return new WaitForSeconds(10f);
        yield return SwitchToState(ScreenState.Main);
        RestartScreenCycle();
    }
}
