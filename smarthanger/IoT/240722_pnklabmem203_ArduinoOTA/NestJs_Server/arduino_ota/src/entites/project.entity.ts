import { Entity, Column, PrimaryGeneratedColumn, Unique, ManyToOne, JoinColumn } from 'typeorm';
import { ApiProperty } from '@nestjs/swagger';
import { User } from './user.entity';

@Entity()
@Unique(['name', 'userSeq'])
export class Project {
  @PrimaryGeneratedColumn()
  @ApiProperty({ description: '프로젝트의 고유 ID', example: 1 })
  id: number;

  @Column({ type: 'uuid', unique: true })
  @ApiProperty({ description: '프로젝트의 UUID', example: '550e8400-e29b-41d4-a716-446655440000' })
  uuid: string;

  @Column()
  @ApiProperty({ description: '프로젝트 이름', example: '새 프로젝트' })
  name: string;

  @ManyToOne(() => User, user => user.seq, { eager: true })
  @JoinColumn({ name: 'userSeq' })
  @ApiProperty({ description: '사용자 ID', example: 1 })
  userSeq: User;
}
