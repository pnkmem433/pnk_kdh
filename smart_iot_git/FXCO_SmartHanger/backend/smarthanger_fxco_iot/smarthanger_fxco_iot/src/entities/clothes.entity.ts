import {
  Entity,
  PrimaryGeneratedColumn,
  Column,
  OneToMany,
  JoinColumn,
  ManyToOne,
} from 'typeorm';
import { Hanger } from './hanger.entity';

@Entity({ name: 'clothes' })
export class Clothes {
  @PrimaryGeneratedColumn({ type: 'int' })
  seq: number;

  @Column({ type: 'varchar', length: 255 })
  name: string;

  @Column({ type: 'varchar', length: 255 })
  tag: string;

  @Column({ type: 'int', nullable: true })
  media: number | null;

  @Column({ type: 'json', nullable: true })
  size_color_options: any;

  @Column({ type: 'int', nullable: true })
  price: number | null;

  @Column({ type: 'text', nullable: true })
  web_site: string | null;

  @OneToMany(() => Hanger, (hanger) => hanger.clothes)
  hangers: Hanger[];
}
