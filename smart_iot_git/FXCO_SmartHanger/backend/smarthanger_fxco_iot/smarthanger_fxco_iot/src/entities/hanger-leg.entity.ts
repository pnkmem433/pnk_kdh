import { Entity, PrimaryGeneratedColumn, Column, OneToMany } from 'typeorm';
import { Hanger } from './hanger.entity';
import { HangerLog } from './hanger-log.entity';

@Entity({ name: 'hangerleg' })
export class HangerLeg {
  @PrimaryGeneratedColumn({ type: 'int' })
  seq: number;

  @Column({ type: 'varchar', length: 50 })
  uuid: string;

  // ✅ 정답 위치로 지정된 행거들 (hanger.hangerleg_seq)
  @OneToMany(() => Hanger, (hanger) => hanger.hangerLeg)
  hangers: Hanger[];

  @OneToMany(() => HangerLog, (log) => log.hangerLeg)
  logs: HangerLog[];
}
