import { Entity, PrimaryColumn, Column, ManyToOne, JoinColumn } from 'typeorm';
import { ApiProperty } from '@nestjs/swagger';
import { Rack } from './rack.entity';

@Entity('hangerleg')
export class Hangerleg {
  @ApiProperty({ description: '행거랙 시퀀스 ID', example: 1 })
  @PrimaryColumn({ name: 'seq', type: 'int' })
  seq: number;

  @ApiProperty({ description: '행거랙 UUID', example: '1D947649930000' })
  @Column({ name: 'uuid', type: 'varchar', nullable: true })
  uuid: string;

  @ApiProperty({ description: '붙을 행거랙 시퀀스 번호 (외래키)', example: 1, nullable: true })
  @Column({ name: 'rack_seq', type: 'int', nullable: true })
  rackSeq: number | null;

  @ApiProperty({ description: '순서 (1, 2, 3, 4, 5...)', example: 1, nullable: true })
  @Column({ name: 'position', type: 'int', nullable: true })
  position: number | null;

  @ApiProperty({ description: '붙을 행거랙 정보', type: () => Rack, nullable: true })
  @ManyToOne(() => Rack, (rack) => rack.hangerlegs, { nullable: true })
  @JoinColumn({ name: 'rack_seq', referencedColumnName: 'seq' })
  rack: Rack | null;
}


