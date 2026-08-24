import { Entity, PrimaryGeneratedColumn, Column, ManyToOne, JoinColumn } from 'typeorm';
import { Session } from './session.entity';
import { ClothesTypes } from './clothesTypes.entity';

@Entity('rfid_scan')
export class RfidScan {
  @PrimaryGeneratedColumn()
  seq: number;

  @Column({ name: 'session_seq' })
  session_seq: number;

  @Column({ name: 'clothes_product_id' })
  clothes_product_id: string;

  @Column({ type: 'datetime' })
  scanned_at: Date;

  @Column({ type: 'tinyint', comment: '0: 안테나, 1: 카메라' })
  scan_source_type: number;

  @ManyToOne(() => ClothesTypes, clothes => clothes.scans)
  @JoinColumn({ name: 'clothes_product_id' })
  clothes: ClothesTypes;

  @ManyToOne(() => Session, session => session.rfid_scans)
  @JoinColumn({ name: 'session_seq' })
  session: Session;
}
