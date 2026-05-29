import {
  Entity,
  Column,
  PrimaryGeneratedColumn,
  ManyToOne,
  CreateDateColumn,
  JoinColumn,
} from 'typeorm';
import { ApiProperty } from '@nestjs/swagger';
import { Project } from './project.entity';

@Entity()
export class ProjectVersion {
  @PrimaryGeneratedColumn()
  @ApiProperty({ description: '버전 고유 ID', example: 1 })
  id: number;

  @ManyToOne(() => Project, (project) => project.id, { eager: true })
  @JoinColumn({ name: 'project' })
  @ApiProperty({ description: '연결된 프로젝트', type: Project })
  project: Project;

  @CreateDateColumn()
  @ApiProperty({ description: '버전 생성 일시', example: '2026-04-17T09:00:00Z' })
  createdAt: Date;

  @Column()
  @ApiProperty({ description: '버전 번호', example: 23 })
  versionNumber: number;

  @Column()
  @ApiProperty({ description: '버전 이름', example: 'v23_custom_esp8685' })
  versionName: string;

  @Column({ type: 'varchar', length: 32, default: 'esp8685' })
  @ApiProperty({ description: '칩 종류', example: 'esp8685' })
  chipType: string;

  @Column({ type: 'varchar', length: 32, default: 'custom' })
  @ApiProperty({ description: '업데이트 대상 펌웨어 계열', example: 'custom' })
  firmwareFamily: string;

  @Column({ default: true })
  @ApiProperty({ description: '버전 활성 여부', example: true })
  isActive: boolean;

  @Column({ type: 'varchar', length: '255', nullable: true })
  @ApiProperty({
    description: '업로드된 BIN 파일 경로',
    example: '/path/to/file.bin',
    nullable: true,
  })
  binFile: string;
}
