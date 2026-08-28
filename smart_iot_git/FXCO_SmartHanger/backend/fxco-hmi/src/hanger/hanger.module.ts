import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { HangerController } from './hanger.controller';
import { HangerService } from './hanger.service';
import { Hanger } from '../entities/hanger.entity';
import { HangerLog } from '../entities/hanger-log.entity';
import { HangerlegModule } from '../hangerleg/hangerleg.module';
import { ClothesModule } from '../clothes/clothes.module';

@Module({
  imports: [
    TypeOrmModule.forFeature([Hanger, HangerLog]),
    HangerlegModule,
    ClothesModule,
  ],
  controllers: [HangerController],
  providers: [HangerService],
  exports: [HangerService],
})
export class HangerModule {}



