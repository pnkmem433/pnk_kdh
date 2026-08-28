import { Injectable } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { Session } from 'src/entities/session.entity';
import { FittingRoomDoorSensor } from 'src/entities/fittingRoomDoorSensor.entity';

@Injectable()
export class SessionService {
  constructor(
    @InjectRepository(Session)
    private readonly sessionRepository: Repository<Session>,
  ) { }

  async create(data: Partial<Session>): Promise<Session> {
    const entity = this.sessionRepository.create(data);
    entity.created_at = new Date();
    return this.sessionRepository.save(entity);
  }

  async findAll(): Promise<Session[]> {
    return this.sessionRepository.find();
  }

  async findOneSession(seq: number): Promise<Session | null> {
    return this.sessionRepository.findOne({ where: { seq } });
  }

  async update(seq: number, data: Partial<Session>): Promise<Session | null> {
    await this.sessionRepository.update(seq, data);
    return this.findOneSession(seq);
  }

  async remove(seq: number): Promise<void> {
    await this.sessionRepository.delete(seq);
  }

  // 마지막 세션 조회
  async getLastSession() {
    const result = await this.sessionRepository.createQueryBuilder('session')
      .orderBy('session.seq', 'DESC')
      .limit(1)
      .getOne();
    return result ?? null;
  }

  // 세션이 시작되지 않았는지 체크
  async checkSessionNotStarted() {
    const result = await this.sessionRepository.createQueryBuilder('session')
      .orderBy('session.seq', 'DESC')
      .limit(1)
      .getOne();

    // 피팅룸 문의 최근 기록 중 is_opened가 0인 것 찾기
    const fittingRoomDoorSensor = await this.getLastFittingRoomDoorClosed();

    // 세션 생성 시간과 문 닫힘 시간 비교
    let timeComparison = null;
    let pirSensorIssue = false;

    if (result && result.created_at && fittingRoomDoorSensor && fittingRoomDoorSensor.event_time) {
      const sessionCreatedAt = new Date(result.created_at);
      const doorClosedAt = new Date(fittingRoomDoorSensor.event_time);
      const timeDiff = Math.abs(sessionCreatedAt.getTime() - doorClosedAt.getTime());

      // 세션 생성 시간이 문 닫힘 시간보다 빠른지 확인
      const noSessionFlag = sessionCreatedAt < doorClosedAt;

      timeComparison = {
        sessionCreatedAt: sessionCreatedAt,
        doorClosedAt: doorClosedAt,
        timeDiffMs: timeDiff,
        timeDiffSeconds: Math.round(timeDiff / 1000),
        isWithin5Seconds: timeDiff <= 5000,
      };

      // PIR 센서 인식 문제 판단
      // if (isSessionCreatedBeforeDoorClosed && timeDiff <= 5000) {
      //   pirSensorIssue = true;
      //   console.log('🚨 PIR 센서 인식 문제 감지: 세션이 문이 닫히기 전에 생성됨');
      //   console.log('세션 생성 시간:', sessionCreatedAt.toLocaleString());
      //   console.log('문 닫힘 시간:', doorClosedAt.toLocaleString());
      //   console.log('시간 차이:', `${Math.round(timeDiff / 1000)}초`);
      // }
      if (noSessionFlag) {
        pirSensorIssue = true;
        console.log('시간 비교 결과:', {
          세션생성시간: sessionCreatedAt.toLocaleString(),
          문닫힘시간: doorClosedAt.toLocaleString(),
          시간차이: `${Math.round(timeDiff / 1000)}초`,
          '5초이내': timeDiff <= 5000,
          '세션 생성 안됨 의심 플래그': noSessionFlag,
          'PIR센서인식문제': pirSensorIssue
        }
        );
      }
      return {
        noSessionFlag: noSessionFlag,
        session: result ?? null,
        fittingRoomDoorClosed: fittingRoomDoorSensor,
        timeComparison: timeComparison,
        pirSensorIssue: pirSensorIssue
      };
    }

  }

  // 피팅룸 문의 최근 닫힘 기록 가져오기
  private async getLastFittingRoomDoorClosed() {
    try {
      const fittingRoomDoorSensorRepository = this.sessionRepository.manager.getRepository(FittingRoomDoorSensor);

      return await fittingRoomDoorSensorRepository
        .createQueryBuilder('sensor')
        .where('sensor.is_opened = :isOpened', { isOpened: 0 })
        .orderBy('sensor.seq', 'DESC')
        .limit(1)
        .getOne();
    } catch (error) {
      console.log('피팅룸 문 센서 데이터를 가져올 수 없습니다:', error.message);
      return null;
    }
  }

  // is_scanned 값 변경
  async updateIsScanned(seq: number, value: number) {
    const session = await this.findOneSession(seq);
    if (!session) throw new Error('세션을 찾을 수 없습니다.');
    session.is_scanned = value;
    // 실제 DB 저장 로직 필요 (예: TypeORM 사용 시)
    return await this.sessionRepository.save(session);
    return session;
  }

  async updateIsVideoEnded(seq: number, value: number) {
    const session = await this.findOneSession(seq);
    if (!session) throw new Error('세션을 찾을 수 없습니다.');
    session.is_video_ended = value;
    // 실제 DB 저장 로직 필요
    return await this.sessionRepository.save(session);
    return session;
  }

  async updateIsActivated(seq: number, value: number) {
    const session = await this.findOneSession(seq);
    if (!session) throw new Error('세션을 찾을 수 없습니다.');
    session.is_activated = value;
    // 실제 DB 저장 로직 필요
    return await this.sessionRepository.save(session);
    return session;
  }

  async getLastPirStatus() {
    const result = await this.sessionRepository.createQueryBuilder('session')
      .orderBy('session.seq', 'DESC')
      .limit(1)
      .getOne();
    return result?.pir_status ?? null;
  }

  async updatePirStatus(seq: number, value: number) {
    const session = await this.findOneSession(seq);
    if (!session) throw new Error('세션을 찾을 수 없습니다.');
    session.pir_status = value;
    return await this.sessionRepository.save(session);
    return session;
  }

  async updateLatestSessionPirStatus(pir_status: number) {
    const session = await this.getLastSession();
    if (!session) throw new Error('세션을 찾을 수 없습니다.');
    session.pir_status = pir_status;
    return await this.sessionRepository.save(session);
    return session;
  }

  async checkAndDeactivateSession(seq: number) {
    const session = await this.findOneSession(seq);
    if (!session) throw new Error('세션을 찾을 수 없습니다.');

    // 모든 조건이 1인지 확인
    if (session.is_activated === 1 &&
      session.is_video_ended === 1 &&
      session.is_scanned === 1) {

      // is_activated를 0으로 변경
      session.is_activated = 0;
      return await this.sessionRepository.save(session);
    }
    else {
      let message = '';
      if (session.is_activated !== 1) {
        message = '세션이 이미 비활성화되어 있습니다.';
      } else if (session.is_video_ended !== 1) {
        message = '비디오가 아직 종료되지 않았습니다.';
      } else if (session.is_scanned !== 1) {
        message = 'RFID 스캔이 완료되지 않았습니다.';
      }

      return {
        is_activated: session.is_activated === 1,
        is_video_ended: session.is_video_ended === 1,
        is_scanned: session.is_scanned === 1,
        message: message
      };
    }


    return session;
  }

  //문이 닫힐 때 세션 시작
  async activateDoorClose(pir_status: number) {
    // 최근 세션 가져오기
    const session = await this.sessionRepository
      .createQueryBuilder('session')
      .orderBy('session.seq', 'DESC')
      .getOne();

    // 세션 상태 로그 출력
    console.log('현재 세션 상태:', {
      seq: session?.seq,
      is_activated: session?.is_activated,
      is_video_ended: session?.is_video_ended,
      is_scanned: session?.is_scanned,
      pir_status: session?.pir_status,
      created_at: session?.created_at,
      scanned_pir_status: pir_status
    });

    if (pir_status === 1) {
      // 세션이 없거나 마지막 세션이 비활성화된 상태라면 새로운 세션 생성
      if (!session || session.is_activated === 0) {
        const newSession = this.sessionRepository.create({
          is_scanned: 0,
          is_video_ended: 0,
          is_activated: 1,
          pir_status: 1
        });
        await this.sessionRepository.save(newSession);
        console.log('새로운 세션이 시작되었습니다:', newSession.seq);
        return newSession;
      } else {
        // 이미 활성화된 세션이 있는 경우
        console.log('이미 활성화된 세션이 있습니다:', session.seq);
        return {
          message: '이미 활성화된 세션이 있습니다.',
          session: session
        };
      }
    } else {
      return {
        message: '사람이 감지되지 않았습니다.세션생성실패. 문을 닫고 다시 시도해주세요.',
        session: session,
        pir_status: pir_status
      };
    }
  }

  //문이 열릴 때 세션 종료
  async deactivateDoorOpen() {
    // 최근 세션 가져오기
    const session = await this.sessionRepository
      .createQueryBuilder('session')
      .orderBy('session.seq', 'DESC')
      .getOne();

    // 세션 상태 로그 출력
    console.log('현재 세션 상태:', {
      seq: session?.seq,
      is_activated: session?.is_activated,
      is_video_ended: session?.is_video_ended,
      is_scanned: session?.is_scanned,
      pir_status: session?.pir_status,
      created_at: session?.created_at
    });

    if (!session) {
      console.log('종료할 세션이 없습니다.');
      return {
        message: '종료할 세션이 없습니다.'
      };
    }

    // 활성화된 세션이 있는 경우에만 종료
    if (session.is_activated === 1) {
      // 세션 비활성화
      session.is_activated = 0;
      session.pir_status = 0;
      await this.sessionRepository.save(session);
      console.log('세션이 종료되었습니다:', session.seq);
      return {
        message: '세션이 종료되었습니다.',
        session: session
      };
    } else {
      console.log('이미 비활성화된 세션입니다:', session.seq);
      return {
        message: '이미 비활성화된 세션입니다.',
        session: session
      };
    }
  }

  async emergencySessionReset() {
    // 마지막 세션 가져오기
    const session = await this.sessionRepository
      .createQueryBuilder('session')
      .orderBy('session.seq', 'DESC')
      .getOne();

    if (!session) throw new Error('세션을 찾을 수 없습니다.');

    // 마지막 세션 상태 110으로 변경 (비활성화)
    // session.is_scanned = 1;
    // session.is_video_ended = 1;
    session.is_activated = 0;
    await this.sessionRepository.save(session);

    return {
      message: '긴급 세션 종료가 완료되었습니다.',
      session: session
    };
  }
}
