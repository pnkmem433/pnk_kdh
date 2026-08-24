import { Entity, PrimaryGeneratedColumn, ManyToOne, JoinColumn, Column } from 'typeorm';
import { ClothesTypes } from './clothesTypes.entity';
import { VideoContent } from './videoContent.entity';
@Entity('clothes_video_match')
export class ClothesVideoMatch {
  @PrimaryGeneratedColumn()
  seq: number;

  // @ManyToOne(() => ClothesTypes, { eager: true })
  // @JoinColumn({ name: 'clothes_seq' })
  // clothes: ClothesTypes;

  @Column({ name: 'clothes_seq' })
  clothes_seq: number;

  // @ManyToOne(() => VideoContent, { eager: true })
  // @JoinColumn({ name: 'video_seq' })
  // video: VideoContent;

  @Column({ name: 'video_seq' })
  video_seq: number;  
}
