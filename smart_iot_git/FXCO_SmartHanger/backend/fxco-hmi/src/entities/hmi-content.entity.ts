import { Entity, PrimaryColumn, Column } from 'typeorm';
import { ApiProperty } from '@nestjs/swagger';

@Entity('hmi_content')
export class HmiContent {
  @ApiProperty({ description: 'HMI 콘텐츠 시퀀스 ID', example: 1 })
  @PrimaryColumn({ name: 'seq', type: 'int' })
  seq: number;

  @ApiProperty({ description: '콘텐츠 제목', example: 'GARDEN OF SPRING' })
  @Column({ name: 'title', type: 'varchar', nullable: true })
  title: string;

  @ApiProperty({ description: '콘텐츠 코드', example: 'GARDEN OF SPRING' })
  @Column({ name: 'code', type: 'varchar', nullable: true })
  code: string;
}


