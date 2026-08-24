import { Injectable, HttpException, HttpStatus } from '@nestjs/common';
import { ConfigService } from '@nestjs/config';
import * as crypto from 'crypto';

export interface PlugDTO {
  appKey: string;
  clientId: string;
  clientSecret: string;
  hejhomeId: string;
  hejhomePw: string;
}

export interface PlugDeviceDTO {
  id: string;
  name: string;
  deviceType: string;
  state: boolean;
}

export interface TokenResponse {
  access_token: string;
  refresh_token: string;
}

interface StoredToken {
  access_token: string;
  refresh_token: string;
  expires_at: Date;
  client_id: string;
}

@Injectable()
export class PlugService {
  private readonly tokenStorage = new Map<string, StoredToken>();
  private readonly deviceCache = new Map<string, PlugDeviceDTO[]>();
  private readonly cacheExpiry = 5 * 60 * 1000; // 5분

  constructor(
    private readonly configService: ConfigService,
  ) {}

  // 환경 변수에서 헤이홈 설정 가져오기
  private getHejhomeConfig(): PlugDTO {
    const appKey = this.configService.get<string>('HEJHOME_APP_KEY');
    const clientId = this.configService.get<string>('HEJHOME_CLIENT_ID');
    const clientSecret = this.configService.get<string>('HEJHOME_CLIENT_SECRET');
    const hejhomeId = this.configService.get<string>('HEJHOME_USERNAME');
    const hejhomePw = this.configService.get<string>('HEJHOME_PASSWORD');

    // console.log('🔧 환경 변수 로딩 상태:', {
    //   appKey: appKey ? `***${appKey.slice(-4)}` : 'NOT_FOUND',
    //   clientId: clientId ? `***${clientId.slice(-4)}` : 'NOT_FOUND',
    //   clientSecret: clientSecret ? `***${clientSecret.slice(-4)}` : 'NOT_FOUND',
    //   hejhomeId: hejhomeId || 'NOT_FOUND',
    //   hejhomePw: hejhomePw ? `***${hejhomePw.slice(-4)}` : 'NOT_FOUND'
    // });

    if (!appKey || !clientId || !clientSecret || !hejhomeId || !hejhomePw) {
      console.log('❌ 환경 변수 누락:', {
        appKey: !!appKey,
        clientId: !!clientId,
        clientSecret: !!clientSecret,
        hejhomeId: !!hejhomeId,
        hejhomePw: !!hejhomePw
      });
      throw new HttpException('헤이홈 설정이 완료되지 않았습니다.', HttpStatus.BAD_REQUEST);
    }

    return {
      appKey,
      clientId,
      clientSecret,
      hejhomeId,
      hejhomePw,
    };
  }

  // 헤이홈 계정 로그인 및 토큰 저장 (Flutter 코드와 동일하게)
  private async loginToHejhome(): Promise<boolean> {
    try {
      const plug = this.getHejhomeConfig();
      const url = 'https://goqual.io/openapi/token';

      // 요청 바디 암호화 (Flutter와 동일)
      const requestBody = {
        client_id: plug.clientId,
        client_secret: plug.clientSecret,
        grant_type: 'password',
        username: plug.hejhomeId,
        password: plug.hejhomePw,
      };

    //   console.log('🔍 로그인 요청 데이터:', {
    //     client_id: plug.clientId,
    //     client_secret: '***' + plug.clientSecret.slice(-4),
    //     grant_type: requestBody.grant_type,
    //     username: plug.hejhomeId,
    //     password: '***' + plug.hejhomePw.slice(-4)
    //   });

      const encryptedData = this.encryptData(
        plug.appKey,
        JSON.stringify(requestBody),
      );

    //   console.log('🔐 암호화된 데이터 길이:', encryptedData.length);
    //   console.log('🔐 암호화된 데이터 샘플:', encryptedData.substring(0, 50) + '...');

      // Flutter와 동일한 요청 형식
      const requestPayload = { data: encryptedData };
    //   console.log('📤 API 요청 페이로드 형식:', Object.keys(requestPayload));

      const response = await fetch(url, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(requestPayload),
      });

    //   console.log('📥 API 응답 상태:', response.status);
    //   console.log('📥 API 응답 헤더:', Object.fromEntries(response.headers.entries()));

      if (response.status === 200) {
        const responseText = await response.text();
        // console.log('📥 응답 본문:', responseText);
        
        const responseData = JSON.parse(responseText) as TokenResponse;
        // console.log('✅ 로그인 성공, 응답 데이터:', {
        //   access_token: responseData.access_token ? '***' + responseData.access_token.slice(-10) : 'NOT_FOUND',
        //   refresh_token: responseData.refresh_token ? '***' + responseData.refresh_token.slice(-10) : 'NOT_FOUND'
        // });

        // 토큰 저장 (만료 시간 설정: 1시간)
        const expiresAt = new Date();
        expiresAt.setHours(expiresAt.getHours() + 1);

        this.tokenStorage.set(plug.clientId, {
          access_token: responseData.access_token,
          refresh_token: responseData.refresh_token,
          expires_at: expiresAt,
          client_id: plug.clientId,
        });

        console.log('💾 토큰 저장 완료, 만료 시간:', expiresAt);
        return true;
      } else {
        const errorText = await response.text();
        console.log(`❌ 로그인 실패: ${response.status}`);
        console.log(`❌ 에러 응답:`, errorText);
        console.log(`❌ 에러 응답 길이:`, errorText.length);
        
        // JSON 파싱 시도
        try {
          const errorJson = JSON.parse(errorText);
          console.log(`❌ 파싱된 에러 JSON:`, errorJson);
        } catch (parseError) {
          console.log(`❌ JSON 파싱 실패:`, parseError.message);
        }
        
        return false;
      }
    } catch (error) {
      console.log('💥 로그인 중 예외 발생:', error.message);
      throw new HttpException('헤이홈 로그인 실패', HttpStatus.INTERNAL_SERVER_ERROR);
    }
  }

  // 토큰 갱신 (Flutter 코드와 동일하게)
  private async refreshHejhomeToken(): Promise<boolean> {
    try {
      const plug = this.getHejhomeConfig();
      const storedToken = this.tokenStorage.get(plug.clientId);

      if (!storedToken) {
        console.log('❌ 저장된 토큰이 없습니다.');
        return false;
      }

      const url = 'https://goqual.io/openapi/token/refresh';

      // 요청 바디 암호화 (Flutter와 동일)
      const requestBody = {
        client_id: plug.clientId,
        client_secret: plug.clientSecret,
        grant_type: 'refresh_token',
        refresh_token: storedToken.refresh_token,
      };

      console.log('🔄 토큰 갱신 요청:', {
        client_id: plug.clientId,
        grant_type: requestBody.grant_type,
        refresh_token: '***' + storedToken.refresh_token.slice(-10)
      });

      const encryptedData = this.encryptData(
        plug.appKey,
        JSON.stringify(requestBody),
      );

      const requestPayload = { data: encryptedData };
      const response = await fetch(url, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(requestPayload),
      });

      if (response.status === 200) {
        const responseText = await response.text();
        const responseData = JSON.parse(responseText) as TokenResponse;

        // console.log('✅ 토큰 갱신 성공');

        // 토큰 갱신
        const expiresAt = new Date();
        expiresAt.setHours(expiresAt.getHours() + 1);

        this.tokenStorage.set(plug.clientId, {
          access_token: responseData.access_token,
          refresh_token: responseData.refresh_token,
          expires_at: expiresAt,
          client_id: plug.clientId,
        });

        return true;
      } else {
        const errorText = await response.text();
        console.log(`❌ 토큰 갱신 실패: ${response.status} - ${errorText}`);
        return false;
      }
    } catch (error) {
      console.log('💥 토큰 갱신 중 예외 발생:', error.message);
      throw new HttpException('토큰 갱신 실패', HttpStatus.INTERNAL_SERVER_ERROR);
    }
  }

  // 유효한 토큰 가져오기 (자동 갱신 포함)
  private async getValidToken(): Promise<string | null> {
    const plug = this.getHejhomeConfig();
    const storedToken = this.tokenStorage.get(plug.clientId);

    if (!storedToken) {
      // 토큰이 없으면 로그인 시도
      const loginSuccess = await this.loginToHejhome();
      if (loginSuccess) {
        return this.tokenStorage.get(plug.clientId)?.access_token || null;
      }
      return null;
    }

    // 토큰 만료 체크
    if (new Date() >= storedToken.expires_at) {
      console.log('토큰이 만료되었습니다. 갱신을 시도합니다.');
      const refreshSuccess = await this.refreshHejhomeToken();
      if (refreshSuccess) {
        return this.tokenStorage.get(plug.clientId)?.access_token || null;
      } else {
        // 갱신 실패 시 재로그인
        const loginSuccess = await this.loginToHejhome();
        if (loginSuccess) {
          return this.tokenStorage.get(plug.clientId)?.access_token || null;
        }
        return null;
      }
    }

    return storedToken.access_token;
  }

  // 데이터 암호화 (Flutter 코드와 동일하게)
  private encryptData(appKey: string, text: string): string {
    try {
      const key = appKey.substring(0, 32);
      const iv = appKey.substring(0, 16);

      // Flutter와 동일한 AES-CBC 암호화 구현
      const cipher = crypto.createCipheriv('aes-256-cbc', Buffer.from(key), Buffer.from(iv));
      cipher.setAutoPadding(true);
      
      let encrypted = cipher.update(text, 'utf8');
      encrypted = Buffer.concat([encrypted, cipher.final()]);

      // Flutter와 동일하게 base64Url 인코딩
      return encrypted.toString('base64').replace(/\+/g, '-').replace(/\//g, '_').replace(/=/g, '');
    } catch (error) {
      console.log('데이터 암호화 중 오류 발생:', error.message);
      throw new HttpException('데이터 암호화 실패', HttpStatus.INTERNAL_SERVER_ERROR);
    }
  }

  // 스마트 플러그 장치 목록 조회
  async getPlugDevices(): Promise<PlugDeviceDTO[]> {
    try {
      const token = await this.getValidToken();
      if (!token) {
        throw new HttpException('유효한 토큰이 없습니다.', HttpStatus.UNAUTHORIZED);
      }

      // 캐시된 장치 목록이 있고 만료되지 않았으면 반환
      const cachedDevices = this.deviceCache.get('devices');
      if (cachedDevices) {
        return cachedDevices;
      }

      const url = 'https://goqual.io/openapi/devices';
      const response = await fetch(url, {
        headers: {
          'Content-Type': 'application/json',
          'Authorization': `Bearer ${token}`,
        },
      });

      if (response.status !== 200) {
        return [];
      }

      // 장치 상태 정보 조회
      const stateUrl = 'https://goqual.io/openapi/devices/state';
      const stateResponse = await fetch(stateUrl, {
        headers: {
          'Content-Type': 'application/json',
          'Authorization': `Bearer ${token}`,
        },
      });

      const status: { [key: string]: boolean } = {};

      if (stateResponse.status === 200) {
        const stateData = await stateResponse.json() as any[];
        for (const device of stateData) {
          status[device.id] = device.deviceState?.power || false;
        }
      }

      // 장치 목록과 상태 정보 결합
      const devices = await response.json() as any[];
      const deviceList = devices.map((device) => ({
        id: device.id,
        name: device.name,
        deviceType: device.deviceType,
        state: status[device.id] || false,
      }));

      // 캐시에 저장
      this.deviceCache.set('devices', deviceList);
      setTimeout(() => {
        this.deviceCache.delete('devices');
      }, this.cacheExpiry);

      return deviceList;
    } catch (error) {
      console.log('스마트 플러그 장치 목록 조회 중 오류 발생:', error.message);
      throw new HttpException('장치 목록 조회 실패', HttpStatus.INTERNAL_SERVER_ERROR);
    }
  }

  // 특정 플러그 상태 조회
  async getPlugStatus(serialId: string): Promise<PlugDeviceDTO | null> {
    try {
      const devices = await this.getPlugDevices();
      const targetDevice = devices.find(device => device.id === serialId);
      
      if (!targetDevice) {
        return null;
      }

      return targetDevice;
    } catch (error) {
      console.log(`플러그 ${serialId} 상태 조회 중 오류 발생:`, error.message);
      throw new HttpException('플러그 상태 조회 실패', HttpStatus.INTERNAL_SERVER_ERROR);
    }
  }

  // 스마트 플러그 제어
  private async controlSmartPlug(deviceId: string, power: boolean): Promise<boolean> {
    try {
      const token = await this.getValidToken();
      if (!token) {
        return false;
      }

      const url = `https://goqual.io/openapi/control/${deviceId}`;
      const response = await fetch(url, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'Authorization': `Bearer ${token}`,
        },
        body: JSON.stringify({ requirments: { power } }),
      });

      return response.status === 200;
    } catch (error) {
      console.log('스마트 플러그 제어 중 오류 발생:', error.message);
      throw new HttpException('플러그 제어 실패', HttpStatus.INTERNAL_SERVER_ERROR);
    }
  }

  // 🚀 메인 기능: 플러그 시리얼 ID로 껐다 켜기 (한번에 모든 과정 실행)
  async togglePlugBySerialId(serialId: string): Promise<{ success: boolean; message: string; finalState: boolean }> {
    try {
      console.log(`🚀 플러그 ${serialId} 껐다 켜기 시작...`);

      // 1단계: 헤이홈 로그인 및 토큰 발급
      console.log('1️⃣ 헤이홈 로그인 중...');
      const token = await this.getValidToken();
      if (!token) {
        throw new HttpException('헤이홈 로그인 실패', HttpStatus.UNAUTHORIZED);
      }
      console.log('✅ 헤이홈 로그인 성공');

      // 2단계: 장치 목록 조회
      console.log('2️⃣ 장치 목록 조회 중...');
      const devices = await this.getPlugDevices();
      const targetDevice = devices.find(device => device.id === serialId);

      if (!targetDevice) {
        throw new HttpException(`시리얼 ID ${serialId}에 해당하는 장치를 찾을 수 없습니다.`, HttpStatus.NOT_FOUND);
      }
      console.log(`✅ 장치 발견: ${targetDevice.name} (현재 상태: ${targetDevice.state ? '켜짐' : '꺼짐'})`);

      // 3단계: 현재 상태 확인 및 반대 상태로 변경
      const currentState = targetDevice.state;
      const newState = !currentState;

      console.log(`3️⃣ 플러그 ${currentState ? '끄기' : '켜기'} 중...`);
      const controlResult = await this.controlSmartPlug(serialId, newState);

      if (!controlResult) {
        throw new HttpException(`플러그 ${serialId} 제어 실패`, HttpStatus.INTERNAL_SERVER_ERROR);
      }

      // 4단계: 상태 변경 확인
      console.log(`4️⃣ 상태 변경 확인 중...`);
      await new Promise(resolve => setTimeout(resolve, 1000)); // 1초 대기
      
      // 캐시 무효화
      this.deviceCache.delete('devices');
      
      // 최종 상태 확인
      const finalDevices = await this.getPlugDevices();
      const finalDevice = finalDevices.find(device => device.id === serialId);
      const finalState = finalDevice?.state || newState;

      console.log(`✅ 플러그 ${serialId} ${currentState ? '끄기' : '켜기'} 완료! 최종 상태: ${finalState ? '켜짐' : '꺼짐'}`);

      return {
        success: true,
        message: `플러그 ${serialId} ${currentState ? '끄기' : '켜기'} 성공! 최종 상태: ${finalState ? '켜짐' : '꺼짐'}`,
        finalState: finalState
      };

    } catch (error) {
      console.log(`❌ 플러그 ${serialId} 껐다 켜기 실패:`, error.message);
      throw error;
    }
  }

  // 🔌 새로운 기능: 플러그를 원클릭 껐다 켜기 (현재 상태와 관계없이)
  async forceTogglePlugBySerialId(serialId: string): Promise<{ success: boolean; message: string; finalState: boolean }> {
    try {
      console.log(`🔌 플러그 ${serialId} 원클릭 껐다 켜기 시작...`);

      // 1단계: 헤이홈 로그인 및 토큰 발급
      console.log('1️⃣ 헤이홈 로그인 중...');
      const token = await this.getValidToken();
      if (!token) {
        throw new HttpException('헤이홈 로그인 실패', HttpStatus.UNAUTHORIZED);
      }
      console.log('✅ 헤이홈 로그인 성공');

      // 2단계: 장치 목록 조회
      console.log('2️⃣ 장치 목록 조회 중...');
      const devices = await this.getPlugDevices();
      const targetDevice = devices.find(device => device.id === serialId);

      if (!targetDevice) {
        throw new HttpException(`시리얼 ID ${serialId}에 해당하는 장치를 찾을 수 없습니다.`, HttpStatus.NOT_FOUND);
      }
      console.log(`✅ 장치 발견: ${targetDevice.name} (현재 상태: ${targetDevice.state ? '켜짐' : '꺼짐'})`);

      // 3단계: 무조건 끄기 (현재 상태와 관계없이)
      console.log('3️⃣ 1차: 플러그 끄기 중...');
      const turnOffResult = await this.controlSmartPlug(serialId, false);
      
      if (!turnOffResult) {
        throw new HttpException(`플러그 ${serialId} 끄기 실패`, HttpStatus.INTERNAL_SERVER_ERROR);
      }
      console.log('✅ 플러그 끄기 완료');

      // 4단계: 잠시 대기 (끄기 완료 대기)
      console.log('4️⃣ 끄기 완료 대기 중... (2초)');
      await new Promise(resolve => setTimeout(resolve, 2000)); // 2초 대기

      // 5단계: 무조건 켜기
      console.log('5️⃣ 2차: 플러그 켜기 중...');
      const turnOnResult = await this.controlSmartPlug(serialId, true);
      
      if (!turnOnResult) {
        throw new HttpException(`플러그 ${serialId} 켜기 실패`, HttpStatus.INTERNAL_SERVER_ERROR);
      }
      console.log('✅ 플러그 켜기 완료');

      // 6단계: 최종 상태 확인
      console.log('6️⃣ 최종 상태 확인 중...');
      await new Promise(resolve => setTimeout(resolve, 1000)); // 1초 대기
      
      // 캐시 무효화
      this.deviceCache.delete('devices');
      
      // 최종 상태 확인
      const finalDevices = await this.getPlugDevices();
      const finalDevice = finalDevices.find(device => device.id === serialId);
      const finalState = finalDevice?.state || true; // 켜기 명령을 보냈으므로 기본값은 true

      console.log(`✅ 플러그 ${serialId} 원클릭 껐다 켜기 완료! 최종 상태: ${finalState ? '켜짐' : '꺼짐'}`);

      return {
        success: true,
        message: `플러그 ${serialId} 원클릭 껐다 켜기 성공! 최종 상태: ${finalState ? '켜짐' : '꺼짐'}`,
        finalState: finalState
      };

    } catch (error) {
      console.log(`❌ 플러그 ${serialId} 원클릭 껐다 켜기 실패:`, error.message);
      throw error;
    }
  }

  // 플러그 무조건 켜기
  async turnOnPlugBySerialId(serialId: string): Promise<{ success: boolean; message: string; finalState: boolean }> {
    try {
      console.log(`🔌 플러그 ${serialId} 켜기 시작...`);

      // 1단계: 헤이홈 로그인 및 토큰 발급
      const token = await this.getValidToken();
      if (!token) {
        throw new HttpException('헤이홈 로그인 실패', HttpStatus.UNAUTHORIZED);
      }

      // 2단계: 장치 목록 조회 (장치 존재 확인)
      const devices = await this.getPlugDevices();
      const targetDevice = devices.find(device => device.id === serialId);

      if (!targetDevice) {
        throw new HttpException(`시리얼 ID ${serialId}에 해당하는 장치를 찾을 수 없습니다.`, HttpStatus.NOT_FOUND);
      }

      // 3단계: 무조건 켜기
      console.log(`플러그 ${serialId} 켜기 중...`);
      const controlResult = await this.controlSmartPlug(serialId, true);

      if (!controlResult) {
        throw new HttpException(`플러그 ${serialId} 켜기 실패`, HttpStatus.INTERNAL_SERVER_ERROR);
      }

      // 4단계: 상태 변경 확인
      await new Promise(resolve => setTimeout(resolve, 1000)); // 1초 대기
      
      // 캐시 무효화
      this.deviceCache.delete('devices');
      
      // 최종 상태 확인
      const finalDevices = await this.getPlugDevices();
      const finalDevice = finalDevices.find(device => device.id === serialId);
      const finalState = finalDevice?.state ?? true;

      console.log(`✅ 플러그 ${serialId} 켜기 완료! 최종 상태: ${finalState ? '켜짐' : '꺼짐'}`);

      return {
        success: true,
        message: `플러그 ${serialId} 켜기 성공! 최종 상태: ${finalState ? '켜짐' : '꺼짐'}`,
        finalState: finalState
      };

    } catch (error) {
      console.log(`❌ 플러그 ${serialId} 켜기 실패:`, error.message);
      throw error;
    }
  }

  // 플러그 무조건 끄기
  async turnOffPlugBySerialId(serialId: string): Promise<{ success: boolean; message: string; finalState: boolean }> {
    try {
      console.log(`🔌 플러그 ${serialId} 끄기 시작...`);

      // 1단계: 헤이홈 로그인 및 토큰 발급
      const token = await this.getValidToken();
      if (!token) {
        throw new HttpException('헤이홈 로그인 실패', HttpStatus.UNAUTHORIZED);
      }

      // 2단계: 장치 목록 조회 (장치 존재 확인)
      const devices = await this.getPlugDevices();
      const targetDevice = devices.find(device => device.id === serialId);

      if (!targetDevice) {
        throw new HttpException(`시리얼 ID ${serialId}에 해당하는 장치를 찾을 수 없습니다.`, HttpStatus.NOT_FOUND);
      }

      // 3단계: 무조건 끄기
      console.log(`플러그 ${serialId} 끄기 중...`);
      const controlResult = await this.controlSmartPlug(serialId, false);

      if (!controlResult) {
        throw new HttpException(`플러그 ${serialId} 끄기 실패`, HttpStatus.INTERNAL_SERVER_ERROR);
      }

      // 4단계: 상태 변경 확인
      await new Promise(resolve => setTimeout(resolve, 1000)); // 1초 대기
      
      // 캐시 무효화
      this.deviceCache.delete('devices');
      
      // 최종 상태 확인
      const finalDevices = await this.getPlugDevices();
      const finalDevice = finalDevices.find(device => device.id === serialId);
      const finalState = finalDevice?.state ?? false;

      console.log(`✅ 플러그 ${serialId} 끄기 완료! 최종 상태: ${finalState ? '켜짐' : '꺼짐'}`);

      return {
        success: true,
        message: `플러그 ${serialId} 끄기 성공! 최종 상태: ${finalState ? '켜짐' : '꺼짐'}`,
        finalState: finalState
      };

    } catch (error) {
      console.log(`❌ 플러그 ${serialId} 끄기 실패:`, error.message);
      throw error;
    }
  }

  // 로그아웃 (토큰 삭제)
  async logout(): Promise<void> {
    const plug = this.getHejhomeConfig();
    this.tokenStorage.delete(plug.clientId);
    this.deviceCache.clear();
    console.log('로그아웃 완료, 토큰 및 캐시 삭제됨');
  }

  // 로그인 상태 확인
  async isLoggedIn(): Promise<boolean> {
    try {
      const token = await this.getValidToken();
      return !!token;
    } catch (error) {
      return false;
    }
  }

  // 저장된 토큰 정보 가져오기
  getStoredTokenInfo(): { clientId: string; expiresAt: Date; isValid: boolean } | null {
    const plug = this.getHejhomeConfig();
    const storedToken = this.tokenStorage.get(plug.clientId);
    
    if (!storedToken) {
      return null;
    }

    return {
      clientId: storedToken.client_id,
      expiresAt: storedToken.expires_at,
      isValid: new Date() < storedToken.expires_at
    };
  }
}
