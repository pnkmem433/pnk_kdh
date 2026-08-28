import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { HangerlegController } from './hangerleg.controller';
import { HangerlegService } from './hangerleg.service';
import { HangerLeg } from '../entities/hanger-leg.entity';
import { Hanger } from '../entities/hanger.entity';

@Module({
  imports: [TypeOrmModule.forFeature([HangerLeg, Hanger])],
  controllers: [HangerlegController],
  providers: [HangerlegService],
})
export class HangerlegModule {}
