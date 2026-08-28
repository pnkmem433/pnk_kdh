import { Injectable, NotFoundException } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository, DataSource } from 'typeorm';
import { Hangerleg } from '../entities/hangerleg.entity';
import { Hanger } from '../entities/hanger.entity';
import { ReplaceHangerlegDto } from './dto/replace-hangerleg.dto';
import { RackService } from '../rack/rack.service';

@Injectable()
export class HangerlegService {
  constructor(
    @InjectRepository(Hangerleg)
    private hangerlegRepository: Repository<Hangerleg>,
    @InjectRepository(Hanger)
    private hangerRepository: Repository<Hanger>,
    private rackService: RackService,
    private dataSource: DataSource,
  ) {}

  async findAll(): Promise<Hangerleg[]> {
    return await this.hangerlegRepository.find();
  }

  async findOne(seq: number): Promise<Hangerleg | null> {
    return await this.hangerlegRepository.findOne({ where: { seq } });
  }

  async findByUuid(uuid: string): Promise<Hangerleg | null> {
    return await this.hangerlegRepository.findOne({ where: { uuid } });
  }

  /**
   * 스마트 헹거랙 교체
   * 특정 위치(rack_seq, position)에 있는 기존 헹거랙을 해제하고,
   * 새로운 헹거랙을 해당 위치에 배치합니다.
   * 해당 헹거랙을 참조하는 헹거들의 hangerleg_seq도 함께 업데이트합니다.
   * @param dto 교체 요청 DTO (hangerlegUuid, rackSeq, position)
   * @returns 업데이트된 헹거랙 정보
   */
  async replaceHangerleg(dto: ReplaceHangerlegDto): Promise<Hangerleg> {
    // 트랜잭션으로 처리하여 데이터 일관성 보장
    return await this.dataSource.transaction(async (manager) => {
      const hangerlegRepository = manager.getRepository(Hangerleg);
      const hangerRepository = manager.getRepository(Hanger);

      // 1. rackSeq가 제공된 경우 붙을 행거랙 존재 확인
      let existingHangerlegSeq: number | null = null;
      
      if (dto.rackSeq !== undefined) {
        const rack = await this.rackService.findOne(dto.rackSeq);
        if (!rack) {
          throw new NotFoundException(`붙을 행거랙을 찾을 수 없습니다. SEQ: ${dto.rackSeq}`);
        }

        // 2. 해당 위치에 있는 기존 헹거랙 찾기
        const existingHangerleg = await hangerlegRepository.findOne({
          where: {
            rackSeq: dto.rackSeq,
            position: dto.position,
          },
        });

        // 3. 기존 헹거랙이 있고, 새로 배치할 헹거랙과 다르면 해제
        if (existingHangerleg && existingHangerleg.uuid !== dto.hangerlegUuid) {
          // 3-1. 기존 헹거랙의 seq 저장 (나중에 헹거 업데이트에 사용)
          existingHangerlegSeq = existingHangerleg.seq;

          // 3-2. 기존 헹거랙 해제
          existingHangerleg.rackSeq = null;
          existingHangerleg.position = null;
          await hangerlegRepository.save(existingHangerleg);
        }
      }

      // 4. 새로 배치할 헹거랙 UUID로 조회
      const newHangerleg = await hangerlegRepository.findOne({
        where: { uuid: dto.hangerlegUuid },
      });

      if (!newHangerleg) {
        throw new NotFoundException(`헹거랙을 찾을 수 없습니다. UUID: ${dto.hangerlegUuid}`);
      }

      // 5. 새 헹거랙의 rack_seq와 position 설정
      newHangerleg.rackSeq = dto.rackSeq ?? null;
      newHangerleg.position = dto.position;
      await hangerlegRepository.save(newHangerleg);

      // 6. 기존 위치에 있던 헹거랙을 참조하던 헹거들의 hangerleg_seq를 새 헹거랙의 seq로 업데이트
      if (existingHangerlegSeq !== null) {
        await hangerRepository.update(
          { hangerlegSeq: existingHangerlegSeq },
          { hangerlegSeq: newHangerleg.seq },
        );
      }

      return newHangerleg;
    });
  }
}


