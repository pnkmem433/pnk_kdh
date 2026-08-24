import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { ClothesController } from './clothes.controller';
import { ClothesService } from './clothes.service';
import { Hanger } from '../entities/hanger.entity';
import { Clothes } from '../entities/clothes.entity';

@Module({
  imports: [TypeOrmModule.forFeature([Hanger, Clothes])],
  controllers: [ClothesController],
  providers: [ClothesService],
})
export class ClothesModule {}
