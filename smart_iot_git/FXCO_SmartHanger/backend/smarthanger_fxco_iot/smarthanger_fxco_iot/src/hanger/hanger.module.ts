import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { Hanger } from '../entities/hanger.entity';
import { HangerController } from './hanger.controller';
import { HangerService } from './hanger.service';
import { HangerLog } from 'src/entities/hanger-log.entity';
import { HangerLeg } from 'src/entities/hanger-leg.entity';

@Module({
  imports: [TypeOrmModule.forFeature([Hanger, HangerLog, HangerLeg])],
  controllers: [HangerController],
  providers: [HangerService],
})
export class HangerModule {}
