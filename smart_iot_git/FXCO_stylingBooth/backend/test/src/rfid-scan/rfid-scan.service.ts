import { Injectable } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { RfidScan } from 'src/entities/rfidScan.entity';
import { ClothesVideoMatch } from 'src/entities/clothesVideoMatch.entity';
import { ClothesTypes } from 'src/entities/clothesTypes.entity';
import { VideoContent } from 'src/entities/videoContent.entity';

@Injectable()
export class RfidScanService {
  constructor(
    @InjectRepository(RfidScan)
    private readonly rfidScanRepository: Repository<RfidScan>,
    @InjectRepository(ClothesTypes)
    private readonly clothesTypesRepository: Repository<ClothesTypes>,
    @InjectRepository(ClothesVideoMatch)
    private readonly clothesVideoMatchRepository: Repository<ClothesVideoMatch>,
    @InjectRepository(VideoContent)
    private readonly videoContentRepository: Repository<VideoContent>,
  ) {}

  async create(data: Partial<RfidScan>): Promise<RfidScan> {
    const entity = this.rfidScanRepository.create(data);
    entity.scanned_at = new Date();
    return this.rfidScanRepository.save(entity);
  }

  async findAll(): Promise<RfidScan[]> {
    return this.rfidScanRepository.find();
  }

  async findOne(seq: number): Promise<RfidScan | null> {
    return this.rfidScanRepository.findOne({ where: { seq } });
  }

  async update(seq: number, data: Partial<RfidScan>): Promise<RfidScan | null> {
    await this.rfidScanRepository.update(seq, data);
    return this.findOne(seq);
  }

  async remove(seq: number): Promise<void> {
    await this.rfidScanRepository.delete(seq);
  }

  async getLastRfidScan(sessionSeq?: number): Promise<RfidScan | null> {
    const query = this.rfidScanRepository.createQueryBuilder('rfidScan')
      .orderBy('rfidScan.seq', 'DESC');

    if (sessionSeq) {
      query.where('rfidScan.session_seq = :sessionSeq', { sessionSeq });
    }

    return query.getOne();
  }

  async getLastRfidScanWithAllClothesInfo(sessionSeq?: number) {
  // 1. RFID 스캔 정보 조회
  const scan = await this.rfidScanRepository.findOne({ where: { session_seq: sessionSeq } });
  if (!scan) return null;

  // 2. 옷 정보 조회
  const clothes = await this.clothesTypesRepository.findOne({
    where: { product_id: scan.clothes_product_id }
  });

  // 3. 매칭 정보 조회
  const match = await this.clothesVideoMatchRepository.findOne({
    where: { clothes_seq: clothes.seq }
  });

  // 4. 비디오 정보 조회
  const video = match
    ? await this.videoContentRepository.findOne({ where: { seq: match.video_seq } })
    : null;

  // 5. 결과 조합
  return {
    seq: scan.seq,
    session_seq: scan.session_seq,
    clothes_types_name: clothes?.name ?? null,
    clothes_product_id: scan.clothes_product_id,
    first_image_url: clothes?.first_image_url ?? null,
    second_image_url: clothes?.second_image_url ?? null,
    video_url: video?.video_url ?? null,
    scanned_at: scan.scanned_at,
    scan_source_type: scan.scan_source_type,
  };
  }
}
