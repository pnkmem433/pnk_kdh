import { Entity, Column, PrimaryGeneratedColumn, ManyToOne, CreateDateColumn, JoinColumn } from 'typeorm';
import { ApiProperty } from '@nestjs/swagger';
import { Project } from './project.entity';

@Entity()
export class ProjectVersion {
  @PrimaryGeneratedColumn()
  @ApiProperty({ description: '버전의 고유 ID', example: 1 })
  id: number;

  @ManyToOne(() => Project, project => project.id, { eager: true })
  @JoinColumn({ name: 'project' })
  @ApiProperty({ description: '연관된 프로젝트', type: Project })
  project: Project;

  @CreateDateColumn()
  @ApiProperty({ description: '버전 생성 날짜', example: '2024-09-10T12:34:56Z' })
  createdAt: Date;

  @Column()
  @ApiProperty({ description: '버전 번호', example: 1 })
  versionNumber: number;

  @Column()
  @ApiProperty({ description: '버전 이름', example: 'v1.0.0' })
  versionName: string;

  @Column({ default: true })
  @ApiProperty({ description: '버전 활성 여부', example: true })
  isActive: boolean;

  @Column({ type: 'varchar', length: '255', nullable: true })
  @ApiProperty({ description: 'BIN 파일 경로 또는 정보', example: '/path/to/file.bin', nullable: true })
  binFile: string;
}