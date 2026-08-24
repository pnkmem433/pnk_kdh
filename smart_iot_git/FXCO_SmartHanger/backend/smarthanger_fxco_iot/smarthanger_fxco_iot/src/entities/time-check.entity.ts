import { Entity, PrimaryGeneratedColumn, Column, ManyToOne, JoinColumn } from 'typeorm';
import { Hanger } from './hanger.entity';
import { HangerLeg } from './hanger-leg.entity';

@Entity({ name: 'time_check' })
export class TimeCheck {
  @PrimaryGeneratedColumn({ type: 'int' })
  seq: number;

  @Column({ type: 'int' })
  hanger_seq: number;

  @Column({ type: 'int', nullable: true })
  hangerleg_seq: number | null;

  // milliseconds since device power-on
  @Column({ type: 'int', comment: 'start ms (since device on)' })
  start_at: number;

  // milliseconds since device power-on
  @Column({ type: 'int', comment: 'end ms (since device on)' })
  end_at: number;

  // when the record was requested; if omitted/null, DB uses CURRENT_TIMESTAMP(3)
  @Column({ type: 'datetime', precision: 3, default: () => 'CURRENT_TIMESTAMP(3)' })
  request_at: Date;

  @ManyToOne(() => Hanger, { eager: false })
  @JoinColumn({ name: 'hanger_seq', referencedColumnName: 'seq' })
  hanger: Hanger;

  @ManyToOne(() => HangerLeg, { eager: false, nullable: true })
  @JoinColumn({ name: 'hangerleg_seq', referencedColumnName: 'seq' })
  hangerLeg: HangerLeg | null;
}