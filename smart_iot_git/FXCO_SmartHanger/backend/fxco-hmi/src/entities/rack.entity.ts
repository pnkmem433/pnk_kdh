import { Entity, PrimaryColumn, Column, OneToMany } from 'typeorm';
import { ApiProperty } from '@nestjs/swagger';
import { Hangerleg } from './hangerleg.entity';

@Entity('rack')
export class Rack {
  @ApiProperty({ description: '붙을 행거랙 시퀀스 ID', example: 1 })
  @PrimaryColumn({ name: 'seq', type: 'int' })
  seq: number;

  @ApiProperty({ description: '행거랙 넘버/식별자', example: 'RACK-001' })
  @Column({ name: 'rack_number', type: 'varchar', length: 50 })
  rackNumber: string;

  @ApiProperty({ description: '행거랙 위치 정보', example: '1층 매장 A구역', nullable: true })
  @Column({ name: 'rack_location', type: 'varchar', length: 255, nullable: true })
  rackLocation: string | null;

  @ApiProperty({ description: '생성일시', example: '2026-01-27T00:00:00.000Z', nullable: true })
  @Column({ name: 'created_at', type: 'datetime', nullable: true, default: () => 'CURRENT_TIMESTAMP' })
  createdAt: Date | null;

  @ApiProperty({ description: '스마트 행거랙 목록', type: () => [Hangerleg], nullable: true })
  @OneToMany(() => Hangerleg, (hangerleg) => hangerleg.rack)
  hangerlegs: Hangerleg[];
}
