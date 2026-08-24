import {
  Entity,
  PrimaryGeneratedColumn,
  Column,
  ManyToOne,
  JoinColumn,
  CreateDateColumn,
} from 'typeorm';
import { Hanger } from './hanger.entity';
import { HangerLeg } from './hanger-leg.entity';

@Entity({ name: 'hanger_log' })
export class HangerLog {
  @PrimaryGeneratedColumn({ type: 'int' })
  seq: number;

  @Column({ type: 'int' })
  hanger_seq: number;

  @Column({ type: 'int', nullable: true })
  hangerleg_seq: number | null;

  /**
   * 이 로그가 정상 흐름(자기 자신)에 의해 생성되었는지 여부
   * 1 = 정상 (픽다운 → 픽업)
   * 0 = 시스템 보정 / 강제 처리
   */
  @Column({
    type: 'tinyint',
    width: 1,
    default: () => '1',
    comment: '1: self written, 0: system generated',
  })
  self_written: number;

  /**
   * 이벤트 발생 시각
   */
  @CreateDateColumn({
    type: 'datetime',
    comment: 'event created time',
  })
  created_at: Date;

  /* ================= relations ================= */

  @ManyToOne(() => Hanger, (hanger) => hanger.logs, { eager: false })
  @JoinColumn({ name: 'hanger_seq' })
  hanger: Hanger;

  @ManyToOne(() => HangerLeg, (leg) => leg.logs, {
    eager: false,
    nullable: true,
  })
  @JoinColumn({ name: 'hangerleg_seq' })
  hangerLeg: HangerLeg;
}
