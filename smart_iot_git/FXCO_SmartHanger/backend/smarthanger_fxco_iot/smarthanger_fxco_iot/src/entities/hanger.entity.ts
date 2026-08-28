import {
  Entity,
  PrimaryGeneratedColumn,
  Column,
  ManyToOne,
  OneToMany,
  JoinColumn,
} from 'typeorm';
import { Clothes } from './clothes.entity';
import { HangerLeg } from './hanger-leg.entity';
import { HangerLog } from './hanger-log.entity';

@Entity({ name: 'hanger' })
export class Hanger {
  @PrimaryGeneratedColumn({ type: 'int' })
  seq: number;

  @Column({ type: 'varchar', length: 50 })
  uuid: string;

  // FK 컬럼들 (원하면 유지해도 되고, 없어도 관계로 동작함)
  @Column({ type: 'int', nullable: true })
  clothes_seq: number | null;

  @Column({ type: 'int', nullable: true })
  hangerleg_seq: number | null; // ✅ 정답 위치(기대 위치)

  @Column({ type: 'tinyint', default: 0 })
  nfc_active: boolean;

  @Column({ type: 'varchar', length: 255, nullable: true })
  last_pickdown_uuid: string | null; // ✅ 현재 감지된 위치의 leg.uuid

  // 관계: clothes_seq -> clothes.seq
  @ManyToOne(() => Clothes, (clothes) => clothes.hangers, { nullable: true })
  @JoinColumn({ name: 'clothes_seq', referencedColumnName: 'seq' })
  clothes: Clothes | null;

  // 관계: hangerleg_seq -> hangerleg.seq (정답 위치)
  @ManyToOne(() => HangerLeg, (leg) => leg.hangers, { nullable: true })
  @JoinColumn({ name: 'hangerleg_seq', referencedColumnName: 'seq' })
  hangerLeg: HangerLeg | null;

  // 로그 관계
  @OneToMany(() => HangerLog, (log) => log.hanger)
  logs: HangerLog[];
}
