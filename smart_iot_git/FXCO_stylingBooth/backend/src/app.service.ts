import { Injectable } from '@nestjs/common';
import * as crypto from 'crypto';

@Injectable()
export class AppService {
  getHello(): string {
    return 'Hello World!';
  }

  // 대시보드 비밀번호 검증
  // 환경변수 DASHBOARD_PASSWORD가 있으면 사용, 없으면 기본값 사용
  private readonly DASHBOARD_PASSWORD = process.env.DASHBOARD_PASSWORD || '71805802';
  
  /**
   * 비밀번호를 해시화하여 저장된 해시와 비교
   * @param password 입력된 비밀번호
   * @returns 검증 성공 여부
   */
  verifyDashboardPassword(password: string): boolean {
    // 간단한 해시 비교 (SHA-256 사용)
    const inputHash = crypto.createHash('sha256').update(password).digest('hex');
    const correctHash = crypto.createHash('sha256').update(this.DASHBOARD_PASSWORD).digest('hex');
    
    // 타이밍 공격 방지를 위한 상수 시간 비교
    return crypto.timingSafeEqual(
      Buffer.from(inputHash),
      Buffer.from(correctHash)
    );
  }
}
