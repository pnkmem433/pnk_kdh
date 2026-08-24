import { Entity, PrimaryGeneratedColumn, Column, OneToMany } from 'typeorm';
import { FittingRoomDoorSensor } from './fittingRoomDoorSensor.entity';
import { VideoRoomDoorSensor } from './videoRoomDoorSensor.entity';
import { RfidScan } from './rfidScan.entity';

@Entity('session')
export class Session {
  @PrimaryGeneratedColumn()
  seq: number;

  @Column({ type: 'datetime', default: () => 'CURRENT_TIMESTAMP' })
  created_at: Date;

  @Column({ type: 'int', default: 0 })
  is_scanned: number;

  @Column({ type: 'int', default: 0 })
  is_video_ended: number;

  @Column({ type: 'int', default: 0 })
  is_activated: number;

  @Column({ type: 'int', default: 0 })
  pir_status: number;

  @OneToMany(() => FittingRoomDoorSensor, door => door.session)
  fitting_room_door_sensors: FittingRoomDoorSensor[];

  @OneToMany(() => VideoRoomDoorSensor, door => door.session)
  video_room_door_sensors: VideoRoomDoorSensor[];

  @OneToMany(() => RfidScan, scan => scan.session_seq)
  rfid_scans: RfidScan[];
}
