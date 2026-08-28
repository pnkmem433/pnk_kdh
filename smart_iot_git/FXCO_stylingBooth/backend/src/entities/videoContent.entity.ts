import { Entity, PrimaryGeneratedColumn, Column, OneToMany } from 'typeorm';
import { ClothesVideoMatch } from './clothesVideoMatch.entity';

@Entity('video_content')
export class VideoContent {
  @PrimaryGeneratedColumn()
  seq: number;

  @Column({ type: 'varchar', length: 255 })
  video_url: string;

  @OneToMany(() => ClothesVideoMatch, match => match.video_seq)
  matches: ClothesVideoMatch[];
}
