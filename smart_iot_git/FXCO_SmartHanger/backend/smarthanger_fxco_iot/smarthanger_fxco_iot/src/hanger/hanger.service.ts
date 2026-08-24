import { Injectable } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { Hanger } from '../entities/hanger.entity';
import { HangerLog } from 'src/entities/hanger-log.entity';
import { HangerLeg } from 'src/entities/hanger-leg.entity';

@Injectable()
export class HangerService {
    constructor(
        @InjectRepository(Hanger)
        private readonly hangerRepo: Repository<Hanger>,
        @InjectRepository(HangerLog)
        private readonly hangerLogRepo: Repository<HangerLog>,
        @InjectRepository(HangerLeg)
        private readonly hangerLegRepo: Repository<HangerLeg>,
    ) { }

    async connect(uuid: string) {
        let hanger = await this.hangerRepo.findOne({
            where: { uuid },
            relations: ['clothes'],
        });

        if (!hanger) {
            hanger = await this.hangerRepo.save({
                uuid,
                clothes: null,
            } as any);
        }

        // ⭕ 옷 정보 반환
        return hanger.clothes ? {
            size_color_options: hanger.clothes.size_color_options,
            price: hanger.clothes.price,
            web_site: hanger.clothes.web_site,
        } : {};
    }

    async pickdown(uuid: string, pickdownUuid: string) {
        const hanger = await this.hangerRepo.findOneBy({ uuid });
        if (!hanger) return { ignored: true };

        const leg = await this.hangerLegRepo.findOneBy({ uuid: pickdownUuid });
        if (!leg) {
            return { ignored: true };
        }

        /**
         * 1️⃣ 같은 행거에서 PICKDOWN → PICKDOWN
         * → 시스템 PICKUP 추가
         */
        if (hanger.nfc_active) {
            await this.forcePickup(hanger);
        }

        /**
         * 2️⃣ 다른 행거에서 이 NFC가 이미 사용 중인지 체크
         */
        const otherHanger = await this.hangerRepo.findOne({
            where: {
                last_pickdown_uuid: pickdownUuid,
                nfc_active: true,
            },
        });

        if (otherHanger && otherHanger.seq !== hanger.seq) {
            await this.forcePickup(otherHanger);
        }

        /**
         * 3️⃣ 현재 행거 PICKDOWN
         */
        hanger.nfc_active = true;
        hanger.last_pickdown_uuid = pickdownUuid;
        await this.hangerRepo.save(hanger);

        await this.hangerLogRepo.save({
            hanger,
            hangerLeg: leg,
            event_type: 'PICKDOWN',
            self_written: 1,
        });

        return { success: true };
    }


    async pickup(uuid: string) {
        const hanger = await this.hangerRepo.findOneBy({ uuid });
        if (!hanger) return { ignored: true };

        /**
         * PICKUP → PICKUP : 무시
         */
        if (!hanger.nfc_active) {
            return { ignored: true };
        }

        hanger.nfc_active = false;
        hanger.last_pickdown_uuid = null;
        await this.hangerRepo.save(hanger);

        await this.hangerLogRepo.save({
            hanger,
            event_type: 'PICKUP',
            self_written: 1,
        });

        return { success: true };
    }


    // 📝 로그
    private async log(
        hanger: Hanger,
        type: 'PICKDOWN' | 'PICKUP',
        valid: boolean,
        reason?: string,
    ) {
        await this.hangerLogRepo.save({
            hanger,
            event_type: type,
            pickdown_uuid: hanger.last_pickdown_uuid,
            valid,
            reason: reason ?? null,
        });
    }

    private async forcePickup(hanger: Hanger) {
        hanger.nfc_active = false;
        hanger.last_pickdown_uuid = null;
        await this.hangerRepo.save(hanger);

        await this.hangerLogRepo.save({
            hanger,
            event_type: 'PICKUP',
            self_written: 0, // ⭐ 시스템 처리
        });
    }
}
