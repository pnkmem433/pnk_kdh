import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { TimeCheck } from '../entities/time-check.entity';
import { Hanger } from '../entities/hanger.entity';
import { HangerLeg } from '../entities/hanger-leg.entity';
import { TimeTrackingController } from './time-tracking.controller';
import { TimeTrackingService } from './time-tracking.service';

@Module({
  imports: [TypeOrmModule.forFeature([TimeCheck, Hanger, HangerLeg])],
  controllers: [TimeTrackingController],
  providers: [TimeTrackingService],
})
export class TimeTrackingModule {}
