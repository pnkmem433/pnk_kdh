import { Entity, PrimaryColumn, Column, ManyToOne, JoinColumn } from 'typeorm';
import { ApiProperty } from '@nestjs/swagger';
import { Clothes } from './clothes.entity';

@Entity('hanger')
export class Hanger {
  @ApiProperty({ description: '행거 시퀀스 ID', example: 1 })
  @PrimaryColumn({ name: 'seq', type: 'int' })
  seq: number;

  @ApiProperty({ description: '행거 UUID', example: '550e8400-e29b-41d4-a716-446655440000' })
  @Column({ name: 'uuid', type: 'varchar', length: 50, nullable: true })
  uuid: string;

  @ApiProperty({ description: '의류 시퀀스 번호 (외래키)', example: 123 })
  @Column({ name: 'clothes_seq', type: 'int', nullable: true })
  clothesSeq: number;

  @ApiProperty({ description: '행거랙 시퀀스 번호 (외래키)', example: 456 })
  @Column({ name: 'hangerleg_seq', type: 'int', nullable: true })
  hangerlegSeq: number;

  @ApiProperty({ description: 'NFC 활성화 상태', example: 1 })
  @Column({ name: 'nfc_active', type: 'tinyint', nullable: true })
  nfcActive: number;

  @ApiProperty({ description: '마지막 픽다운 UUID', example: '550e8400-e29b-41d4-a716-446655440000' })
  @Column({ name: 'last_pickdown_uuid', type: 'varchar', length: 255, nullable: true })
  lastPickdownUuid: string;

  @ApiProperty({ description: '의류 정보', type: () => Clothes, nullable: true })
  @ManyToOne(() => Clothes, { nullable: true })
  @JoinColumn({ name: 'clothes_seq', referencedColumnName: 'seq' })
  clothes: Clothes;
}

