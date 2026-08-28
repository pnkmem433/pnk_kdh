import { Entity, PrimaryGeneratedColumn, Column, ManyToOne, JoinColumn } from 'typeorm';
import { Session } from './session.entity';

@Entity('fitting_room_door_sensor')
export class FittingRoomDoorSensor {
  @PrimaryGeneratedColumn()
  seq: number;

  @ManyToOne(() => Session, session => session.fitting_room_door_sensors, { onDelete: 'CASCADE' })
  @JoinColumn({ name: 'session_seq' })
  session: Session;

  @Column({ type: 'int', comment: '세션 고유번호' })
  session_seq: number;

  @Column({ type: 'datetime' })
  event_time: Date;

  @Column({ type: 'tinyint', comment: '0: 닫힘, 1: 열림' })
  is_opened: number;
}
