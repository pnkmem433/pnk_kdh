import { Entity, PrimaryGeneratedColumn, Column, ManyToOne, JoinColumn } from 'typeorm';
import { Session } from './session.entity';

@Entity('video_room_door_sensor')
export class VideoRoomDoorSensor {
  @PrimaryGeneratedColumn()
  seq: number;

  @ManyToOne(() => Session, session => session.video_room_door_sensors, { onDelete: 'CASCADE' })
  @JoinColumn({ name: 'session_seq' })
  session: Session;

  @Column({ type: 'datetime', nullable: true })
  video_start_time: Date;

  @Column({ type: 'datetime', nullable: true })
  video_end_time: Date; 

  @Column({ type: 'datetime' })
  event_time: Date;

  @Column({ type: 'int', comment: '0: 닫힘, 1: 열림' })
  is_opened: number;
}
