import { Injectable } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { HangerLeg } from '../entities/hanger-leg.entity';
import { Hanger } from '../entities/hanger.entity';

@Injectable()
export class HangerlegService {
    constructor(
        @InjectRepository(HangerLeg)
        private readonly legRepo: Repository<HangerLeg>,

        @InjectRepository(Hanger)
        private readonly hangerRepo: Repository<Hanger>,
    ) { }

    async getAllStatus() {
  // 1) 전체 위치
  const legs = await this.legRepo.find({ order: { seq: 'ASC' } });

  // 2) 현재 ACTIVE 행거(현재 걸린 것만)
  const activeHangers = await this.hangerRepo.find({
    where: { nfc_active: true },
  });

  // 3) last_pickdown_uuid(= hangerleg.uuid)로 위치 매핑
  const hangerByLegUuid = new Map<string, Hanger>();
  for (const h of activeHangers) {
    if (h.last_pickdown_uuid) hangerByLegUuid.set(h.last_pickdown_uuid, h);
  }

  // 4) 응답 생성
  const items = legs.map((leg) => {
    const h = hangerByLegUuid.get(leg.uuid) ?? null;

    const hangerSeq = h?.seq ?? null;

    // 정답 위치:
    // - 행거가 없으면 null
    // - hanger.hangerleg_seq(기대 위치)가 현재 leg.seq와 같으면 null
    // - 다르면 기대 위치 seq 반환
    const expected = h?.hangerleg_seq ?? null;

    const correctHangerLegSeq =
      expected != null && expected !== leg.seq ? expected : null;

    return {
      hangerLegSeq: leg.seq,
      hangerSeq,
      correctHangerLegSeq,
    };
  });

  return { items };
}

}
