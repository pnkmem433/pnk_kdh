import { Entity, PrimaryColumn, Column } from 'typeorm';
import { ApiProperty } from '@nestjs/swagger';

@Entity('hanger_log')
export class HangerLog {
  @ApiProperty({ description: '로그 시퀀스 ID', example: 1 })
  @PrimaryColumn({ name: 'seq', type: 'int' })
  seq: number;

  @ApiProperty({ description: '행거 시퀀스 번호 (외래키)', example: 1 })
  @Column({ name: 'hanger_seq', type: 'int', nullable: true })
  hangerSeq: number;

  @ApiProperty({ description: '행거랙 시퀀스 번호 (외래키)', example: 1 })
  @Column({ name: 'hangerleg_seq', type: 'int', nullable: true })
  hangerlegSeq: number;

  @ApiProperty({ description: '생성된 시간날짜', example: '2024-12-22T00:00:00Z' })
  @Column({ name: 'created_at', type: 'datetime', nullable: true })
  createdAt: Date;

  @ApiProperty({ description: 'self_written', example: 'pickup' })
  @Column({ name: 'self_written', type: 'varchar', nullable: true })
  selfWritten: string;
}
