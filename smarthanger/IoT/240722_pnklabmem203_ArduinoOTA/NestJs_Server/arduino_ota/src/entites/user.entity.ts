import { Entity, Column, PrimaryGeneratedColumn } from 'typeorm';
import { ApiProperty } from '@nestjs/swagger';

@Entity()
export class User {
  @PrimaryGeneratedColumn()
  @ApiProperty({ description: '사용자의 고유 ID', example: 1 })
  seq: number;

  @Column({ unique: true })
  @ApiProperty({ description: '사용자 ID', example: 'user123' })
  id: string;

  @Column()
  @ApiProperty({ description: '사용자 이름', example: '홍길동' })
  name: string;

  @Column()
  @ApiProperty({ description: '비밀번호', example: 'password123' })
  password: string;

  // user.entity.ts
  @Column({ nullable: true })
  @ApiProperty({ description: 'jwt refresh rotation token', example: 'jwt.refresh.token' })
  refreshToken?: string;

}