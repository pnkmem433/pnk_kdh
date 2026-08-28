import { Entity, PrimaryGeneratedColumn, Column, OneToMany } from 'typeorm';
import { RfidScan } from './rfidScan.entity';
import { ClothesVideoMatch } from './clothesVideoMatch.entity';

@Entity('clothes_types')
export class ClothesTypes {
  @PrimaryGeneratedColumn()
  seq: number;

  @Column({ type: 'varchar', length: 100 })
  name: string;

  @Column({ type: 'varchar', length: 100, unique: true })
  product_id: string;

  @Column({ type: 'varchar', length: 100 })
  first_image_url: string;

  @Column({ type: 'varchar', length: 100 })
  second_image_url: string;

  @OneToMany(() => RfidScan, scan => scan.clothes)
  scans: RfidScan[];

  @OneToMany(() => ClothesVideoMatch, match => match.clothes_seq)
  matches: ClothesVideoMatch[]; 
}
