import { BadRequestException, Injectable } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { Hanger } from '../entities/hanger.entity';
import { HangerLeg } from '../entities/hanger-leg.entity';
import { TimeCheck } from '../entities/time-check.entity';
import { TimeTrackingDto } from './dto/time-tracking.dto';

function parseLocalDateTime(input: string): Date {
  const isoLike = input.replace(' ', 'T');
  return new Date(isoLike);
}

function parseMsLike(value: unknown, field: string): number {
  if (typeof value === 'number') {
    if (!Number.isFinite(value) || value < 0) {
      throw new BadRequestException(`${field} must be a non-negative integer ms`);
    }
    return Math.floor(value);
  }
  if (typeof value === 'string') {
    const s = value.trim();
    if (!/^\d+$/.test(s)) {
      throw new BadRequestException(`${field} must be integer ms (string of digits)`);
    }
    const n = Number(s);
    if (!Number.isFinite(n)) {
      throw new BadRequestException(`${field} is not a finite number`);
    }
    return Math.floor(n);
  }
  throw new BadRequestException(`${field} is required and must be number or numeric string`);
}

@Injectable()
export class TimeTrackingService {
  constructor(
    @InjectRepository(TimeCheck)
    private readonly timeCheckRepo: Repository<TimeCheck>,
    @InjectRepository(Hanger)
    private readonly hangerRepo: Repository<Hanger>,
    @InjectRepository(HangerLeg)
    private readonly hangerLegRepo: Repository<HangerLeg>,
  ) {}

  async create(dto: TimeTrackingDto) {
    const hanger = await this.hangerRepo.findOne({ where: { uuid: dto.uuid } });
    if (!hanger) {
      return { ignored: true, reason: 'HANGER_NOT_FOUND' };
    }

    const startMs = parseMsLike((dto as any).startMs ?? (dto as any).startTime, 'startMs');
    const endMs = parseMsLike((dto as any).endMs ?? (dto as any).endTime, 'endMs');
    if (endMs < startMs) {
      throw new BadRequestException('endMs must be >= startMs');
    }

    // Optional explicit request time; if null/omitted, DB default is used
    let requestAt: Date | undefined;
    const rt: any = (dto as any).requestTime;
    if (rt !== undefined && rt !== null && String(rt).trim() !== '') {
      const s = String(rt).trim();
      if (/^\d{12,}$/.test(s)) {
        const d = new Date(Number(s));
        if (isNaN(d.getTime())) {
          throw new BadRequestException('Invalid requestTime epoch ms.');
        }
        requestAt = d;
      } else {
        const d = parseLocalDateTime(s);
        if (isNaN(d.getTime())) {
          throw new BadRequestException('Invalid requestTime format. Use YYYY-MM-DD HH:mm:ss');
        }
        requestAt = d;
      }
    }

    let leg: HangerLeg | null = null;
    if (dto.legUuid) {
      leg = await this.hangerLegRepo.findOne({ where: { uuid: dto.legUuid } });
      if (!leg) {
        return { ignored: true, reason: 'HANGERLEG_NOT_FOUND' };
      }
    }

    const data: Partial<TimeCheck> = {
      hanger_seq: hanger.seq,
      hangerleg_seq: leg ? leg.seq : null,
      start_at: startMs,
      end_at: endMs,
      hanger,
      hangerLeg: leg ?? undefined,
    };
    if (requestAt) {
      (data as any).request_at = requestAt;
    }
    const rec = this.timeCheckRepo.create(data);

    await this.timeCheckRepo.save(rec);

    return { success: true };
  }
}