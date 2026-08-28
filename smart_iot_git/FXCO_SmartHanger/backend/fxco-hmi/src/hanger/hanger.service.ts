import { Injectable, NotFoundException } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { Hanger } from '../entities/hanger.entity';
import { HangerLog } from '../entities/hanger-log.entity';
import { HangerResponseDto } from './dto/hanger-response.dto';
import { ReplaceHangerDto } from './dto/replace-hanger.dto';
import { MatchClothesDto } from './dto/match-clothes.dto';
import { HangerlegService } from '../hangerleg/hangerleg.service';
import { ClothesService } from '../clothes/clothes.service';

@Injectable()
export class HangerService {
  constructor(
    @InjectRepository(Hanger)
    private hangerRepository: Repository<Hanger>,
    @InjectRepository(HangerLog)
    private hangerLogRepository: Repository<HangerLog>,
    private hangerlegService: HangerlegService,
    private clothesService: ClothesService,
  ) {}

  async findAll(): Promise<HangerResponseDto[]> {
    const hangers = await this.hangerRepository.find({
      relations: ['clothes'],
    });

    return await Promise.all(
      hangers.map(async (hanger) => {
        const hangerlegSeqCurrent = await this.getCurrentHangerlegSeq(hanger.seq);
        return {
          ...hanger,
          hangerlegSeqCurrent,
        };
      }),
    );
  }

  async findOne(seq: number): Promise<HangerResponseDto | null> {
    const hanger = await this.hangerRepository.findOne({
      where: { seq },
      relations: ['clothes'],
    });

    if (!hanger) {
      return null;
    }

    const hangerlegSeqCurrent = await this.getCurrentHangerlegSeq(seq);
    return {
      ...hanger,
      hangerlegSeqCurrent,
    };
  }

  private async getCurrentHangerlegSeq(hangerSeq: number): Promise<number | null> {
    const latestLog = await this.hangerLogRepository.findOne({
      where: { hangerSeq },
      order: { seq: 'DESC' },
    });

    return latestLog?.hangerlegSeq ?? null;
  }

  /**
   * 스마트 헹거 교체
   * 스마트 헹거의 위치(헹거랙)를 변경합니다.
   * 기존에 해당 헹거랙에 연결되어 있던 헹거의 hangerleg_seq는 null로 설정됩니다.
   * @param dto 교체 요청 DTO (hangerUuid, hangerlegUuid)
   * @returns 업데이트된 헹거 정보
   */
  async replaceHanger(dto: ReplaceHangerDto): Promise<Hanger> {
    // 1. 헹거 UUID로 헹거 조회
    const hanger = await this.hangerRepository.findOne({
      where: { uuid: dto.hangerUuid },
    });

    if (!hanger) {
      throw new NotFoundException(`헹거를 찾을 수 없습니다. UUID: ${dto.hangerUuid}`);
    }

    // 2. 헹거랙 UUID로 헹거랙 조회하여 seq 획득
    const hangerleg = await this.hangerlegService.findByUuid(dto.hangerlegUuid);
    if (!hangerleg) {
      throw new NotFoundException(`헹거랙을 찾을 수 없습니다. UUID: ${dto.hangerlegUuid}`);
    }

    // 3. 기존에 해당 헹거랙에 연결되어 있던 헹거들의 hangerleg_seq를 null로 설정
    await this.hangerRepository.update(
      { hangerlegSeq: hangerleg.seq },
      { hangerlegSeq: null },
    );

    // 4. 새로운 헹거의 hangerleg_seq 업데이트
    hanger.hangerlegSeq = hangerleg.seq;
    await this.hangerRepository.save(hanger);

    return hanger;
  }

  /**
   * 헹거와 옷 매칭
   * 스마트 헹거에 의류를 연결합니다.
   * 기존에 해당 의류에 연결되어 있던 헹거의 clothes_seq는 null로 설정됩니다.
   * @param dto 매칭 요청 DTO (hangerUuid, clothesSeq)
   * @returns 업데이트된 헹거 정보
   */
  async matchClothes(dto: MatchClothesDto): Promise<Hanger> {
    // 1. 헹거 UUID로 헹거 조회
    const hanger = await this.hangerRepository.findOne({
      where: { uuid: dto.hangerUuid },
    });

    if (!hanger) {
      throw new NotFoundException(`헹거를 찾을 수 없습니다. UUID: ${dto.hangerUuid}`);
    }

    // 2. 의류 시퀀스로 의류 조회하여 존재 여부 확인
    const clothes = await this.clothesService.findOne(dto.clothesSeq);
    if (!clothes) {
      throw new NotFoundException(`의류를 찾을 수 없습니다. 시퀀스: ${dto.clothesSeq}`);
    }

    // 3. 기존에 해당 의류에 연결되어 있던 헹거들의 clothes_seq를 null로 설정
    await this.hangerRepository.update(
      { clothesSeq: dto.clothesSeq },
      { clothesSeq: null },
    );

    // 4. 새로운 헹거의 clothes_seq 업데이트
    hanger.clothesSeq = dto.clothesSeq;
    await this.hangerRepository.save(hanger);

    return hanger;
  }
}

