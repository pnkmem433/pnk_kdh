import { Entity, PrimaryColumn, Column } from 'typeorm';
import { ApiProperty } from '@nestjs/swagger';

@Entity('clothes')
export class Clothes {
  @ApiProperty({ description: '의류 시퀀스 ID', example: 1 })
  @PrimaryColumn({ name: 'seq', type: 'int' })
  seq: number;

  @ApiProperty({ description: '의류 이름', example: '의류 제품명' })
  @Column({ name: 'name', type: 'varchar', nullable: true })
  name: string;

  @ApiProperty({ description: '의류 태그', example: '태그명' })
  @Column({ name: 'tag', type: 'varchar', nullable: true })
  tag: string;

  @ApiProperty({ description: '미디어 ID', example: 123 })
  @Column({ name: 'media', type: 'int', nullable: true })
  media: number;

  @ApiProperty({ 
    description: '사이즈 및 색상 옵션 (JSON)', 
    example: { size: ['S', 'M', 'L'], color: ['red', 'blue'] }
  })
  @Column({ name: 'size_color_options', type: 'json', nullable: true })
  sizeColorOptions: object;

  @ApiProperty({ description: '가격', example: 50000 })
  @Column({ name: 'price', type: 'int', nullable: true })
  price: number;

  @ApiProperty({ description: '웹사이트 URL', example: 'https://example.com' })
  @Column({ name: 'web_site', type: 'text', nullable: true })
  webSite: string;
}




