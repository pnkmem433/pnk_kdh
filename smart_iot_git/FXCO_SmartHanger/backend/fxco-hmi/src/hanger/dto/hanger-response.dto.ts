import { ApiProperty } from '@nestjs/swagger';
import { Clothes } from '../../entities/clothes.entity';

export class HangerResponseDto {
  @ApiProperty({ description: '행거 시퀀스 ID', example: 1 })
  seq: number;

  @ApiProperty({ description: '행거 UUID', example: '524F0ACC-4414-41E6-A0CC-C8C97EF17A52' })
  uuid: string;

  @ApiProperty({ description: '의류 시퀀스 번호', example: 1 })
  clothesSeq: number;

  @ApiProperty({ description: '행거랙 시퀀스 번호', example: 1 })
  hangerlegSeq: number;

  @ApiProperty({ description: 'NFC 활성화 상태', example: 0 })
  nfcActive: number;

  @ApiProperty({ description: '마지막 픽다운 UUID', example: null, nullable: true })
  lastPickdownUuid: string | null;

  @ApiProperty({ description: '의류 정보', type: () => Clothes, nullable: true })
  clothes: Clothes | null;

  @ApiProperty({ description: '현재 행거랙 시퀀스 번호 (hanger_log에서 최근 데이터)', example: 1, nullable: true })
  hangerlegSeqCurrent: number | null;
}

