import { Column, Entity, PrimaryGeneratedColumn } from "typeorm";


@Entity('system_parameter')
export class SystemParameter {
  @PrimaryGeneratedColumn()
  seq: number;

  @Column({ type: 'varchar', length: 100 })
  param_name: string;

  @Column({ type: 'int' })
  parameter: number;
}
