import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { HangerlegController } from './hangerleg.controller';
import { HangerlegService } from './hangerleg.service';
import { Hangerleg } from '../entities/hangerleg.entity';
import { Hanger } from '../entities/hanger.entity';
import { RackModule } from '../rack/rack.module';

@Module({
  imports: [
    TypeOrmModule.forFeature([Hangerleg, Hanger]),
    RackModule,
  ],
  controllers: [HangerlegController],
  providers: [HangerlegService],
  exports: [HangerlegService],
})
export class HangerlegModule {}


